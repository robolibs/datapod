//! Wire contract for the whole stack.
//!
//! Every type that travels between processes or hosts implements [`DataPod`].
//! The trait exposes a Pod **header** (rides in iceoryx2 `user_header` slot
//! or iroh's frame prefix) and an optional **byte payload** (rides in
//! iceoryx2's slice payload or iroh's body).
//!
//! Two shapes:
//!
//! - **Fixed-Pod types** (e.g. `Point`, `Pose`, `Quaternion`): the type IS
//!   its own header. `Header = Self`, `Payload = ()`, no heap.
//!
//! - **Heap-bearing types** (e.g. `Polygon`, `Grid`, `DpString`): the user-
//!   facing struct OWNS a `Vec<T>` inside it, marked `#[dp(bytes)]`. The
//!   macro generates a sibling `<T>Header` Pod struct containing every
//!   non-bytes field. `header()` builds that on demand from `&self`, and
//!   `payload()` returns `bytemuck::cast_slice(&self.<bytes_field>)`.

use core::fmt::{self, Debug};

use bytemuck::{Pod, Zeroable};

/// Marker for types that are safe to ship by zero-copy: self-contained PODs
/// with no pointers, references, or heap-allocated interior. Owned by datapod
/// so the crate has **no** transport dependency — geometry/robotics consumers
/// never pull in a messaging runtime.
///
/// A messaging layer that needs iceoryx2's own `ZeroCopySend` does not impl it
/// on datapod types directly (the orphan rule forbids that). Instead it wraps a
/// datapod header in a newtype it owns and derives iceoryx2's marker there; this
/// trait is the datapod-side contract guaranteeing such a wrap is sound.
///
/// # Safety
/// Implementors must be `#[repr(C)]`/`#[repr(transparent)]` PODs with a fixed
/// layout and no indirection — identical to the contract a shared-memory
/// transport assumes.
pub unsafe trait ZeroCopySend {}

/// The wire contract. Implemented by both fixed-Pod types (header = Self)
/// and heap-bearing types (header is a generated companion struct).
pub trait DataPod: Debug + 'static {
    /// Optional language-neutral canonical schema name for custom Rust
    /// datapods, e.g. `"robolibs.camera_frame.v1"`.
    ///
    /// Built-in datapods are still resolved through the global registry.
    /// `#[datapod(name = "...")]` overrides this for application structs so
    /// Rust, C, and Python can agree on the same schema hash.
    const CANONICAL_NAME: Option<&'static str> = None;

    /// The Pod header that rides on the wire's metadata slot.
    /// For fixed types this is `Self`. For heap types it's a generated
    /// `<T>Header` containing the non-bytes fields.
    type Header: Pod + Zeroable + ZeroCopySend + Debug + Copy;

    /// `()` for fixed-size messages; `[u8]` for variable-payload messages.
    type Payload: ?Sized;

    /// Build the Pod header value from this instance.
    fn header(&self) -> Self::Header;

    /// Byte view of the variable-length payload. Empty slice for fixed-Pod
    /// types (where `Payload = ()`).
    ///
    /// Most datapods have one contiguous payload and can expose it directly.
    /// Sectioned datapods with multiple owned payload vectors should override
    /// [`DataPod::payload_len`] and [`DataPod::write_payload_bytes`] instead.
    fn payload_bytes(&self) -> &[u8];

    /// Number of payload bytes that will be written to the wire.
    fn payload_len(&self) -> usize {
        self.payload_bytes().len()
    }

    /// Append this datapod's payload bytes to `out`.
    fn write_payload_bytes(&self, out: &mut Vec<u8>) {
        out.extend_from_slice(self.payload_bytes());
    }

    /// Borrow payload as one or more contiguous byte segments.
    ///
    /// Most datapods have exactly one payload segment. Sectioned datapods with
    /// multiple owned payload vectors override this so scatter/gather
    /// transports can publish all segments without first joining them.
    fn with_payload_segments<R>(&self, f: impl FnOnce(&[&[u8]]) -> R) -> R
    where
        Self: Sized,
    {
        let payload = self.payload_bytes();
        let segments = [payload];
        f(&segments)
    }
}

/// Owned canonical datapod wire message.
///
/// `bytes` is always `header_bytes || payload_bytes`. This is the convenient
/// contiguous/copying form for files, simple transports, and tests. The
/// zero-copy fast path is [`WireFrame`], which borrows header and payload
/// slices instead of joining them into an owned `Vec`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct WireMessage {
    pub type_hash: u64,
    pub bytes: Vec<u8>,
}

/// Errors returned by generic datapod wire decoding.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum WireError {
    WrongTypeHash {
        expected: u64,
        got: u64,
    },
    UnknownTypeHash {
        type_hash: u64,
    },
    ShortHeader {
        type_name: &'static str,
        needed: usize,
        got: usize,
    },
    MalformedHeader {
        type_name: &'static str,
    },
    InvalidHeader {
        type_name: &'static str,
        message: String,
    },
    InvalidPayloadSize {
        type_name: &'static str,
        message: String,
    },
    MissingCanonicalType {
        type_name: &'static str,
    },
}

impl fmt::Display for WireError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::WrongTypeHash { expected, got } => {
                write!(f, "wrong datapod type hash: got {got}, expected {expected}")
            }
            Self::UnknownTypeHash { type_hash } => {
                write!(f, "unknown datapod type hash: {type_hash}")
            }
            Self::ShortHeader {
                type_name,
                needed,
                got,
            } => write!(
                f,
                "{type_name} wire message is too short: got {got} bytes, need at least {needed}"
            ),
            Self::MalformedHeader { type_name } => {
                write!(f, "{type_name} wire message has malformed header bytes")
            }
            Self::InvalidHeader { type_name, message } => {
                write!(f, "{type_name} wire message has invalid header: {message}")
            }
            Self::InvalidPayloadSize { type_name, message } => {
                write!(
                    f,
                    "{type_name} wire message has invalid payload size: {message}"
                )
            }
            Self::MissingCanonicalType { type_name } => {
                write!(f, "{type_name} has no registered canonical datapod name")
            }
        }
    }
}

impl std::error::Error for WireError {}

/// Types that can reconstruct an owned value from datapod wire parts.
pub trait DataPodDecode: DataPod + Sized {
    fn from_wire_parts(header: Self::Header, payload: Vec<u8>) -> Result<Self, WireError>;
}

/// Borrowed zero-copy datapod frame.
///
/// A frame is the fast-path ABI shape:
///
/// ```text
/// type_hash + borrowed_header + borrowed_payload
/// ```
///
/// It does not allocate and it does not require `header || payload` to be
/// joined into one buffer. The header and payload slices must outlive the frame
/// and any borrowed view derived from it.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct WireFrame<'a> {
    pub type_hash: u64,
    pub header: &'a [u8],
    pub payload: &'a [u8],
}

impl<'a> WireFrame<'a> {
    pub fn joined_len(&self) -> usize {
        self.header.len() + self.payload.len()
    }

    pub fn is_empty_payload(&self) -> bool {
        self.payload.is_empty()
    }
}

/// Borrowed scatter/gather slices for a single-payload frame.
///
/// The first slice is the little-endian `type_hash`, followed by the borrowed
/// header and borrowed payload. This is the transport-facing shape for
/// vectored writes or shared-memory slot writers that can consume a small slice
/// list without requiring an intermediate joined `Vec`.
pub fn with_wire_frame_slices<R>(frame: WireFrame<'_>, f: impl FnOnce(&[&[u8]; 3]) -> R) -> R {
    let type_hash = frame.type_hash.to_le_bytes();
    let slices: [&[u8]; 3] = [&type_hash, frame.header, frame.payload];
    f(&slices)
}

/// Borrowed scatter/gather datapod frame.
///
/// This extends [`WireFrame`] to multiple payload slices for sectioned datapods
/// with more than one owned `Vec`. It is still zero-copy: callers get borrowed
/// header bytes plus borrowed payload segments.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct WireSegmentedFrame<'a> {
    pub type_hash: u64,
    pub header: &'a [u8],
    pub payloads: &'a [&'a [u8]],
}

impl<'a> WireSegmentedFrame<'a> {
    pub fn payload_len(&self) -> usize {
        self.payloads.iter().map(|payload| payload.len()).sum()
    }

    pub fn joined_len(&self) -> usize {
        self.header.len() + self.payload_len()
    }

    pub fn payload_segment_count(&self) -> usize {
        self.payloads.len()
    }
}

/// Borrowed scatter/gather slices for a segmented frame.
///
/// `prefix[0]` is the little-endian `type_hash`, `prefix[1]` is the borrowed
/// header, and `payloads` are the original borrowed payload segments. Keeping
/// payloads separate lets transports publish multi-`Vec` datapods without
/// first joining their payloads.
pub fn with_wire_segmented_frame_slices<R>(
    frame: WireSegmentedFrame<'_>,
    f: impl FnOnce(&[&[u8]; 2], &[&[u8]]) -> R,
) -> R {
    let type_hash = frame.type_hash.to_le_bytes();
    let prefix: [&[u8]; 2] = [&type_hash, frame.header];
    f(&prefix, frame.payloads)
}

/// Borrowed split of canonical datapod wire bytes.
///
/// This is the registry-backed, type-erased form of `header || payload`.
/// It borrows the original wire buffer and does not allocate.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct WireParts<'a> {
    pub type_hash: u64,
    pub header: &'a [u8],
    pub payload: &'a [u8],
}

impl<'a> WireParts<'a> {
    pub fn joined_len(&self) -> usize {
        self.header.len() + self.payload.len()
    }
}

/// Header codec for the stable little-endian wire mode.
///
/// This codec writes fields one by one in little-endian order and does not copy
/// native struct padding.
/// Implementations are generated by `#[datapod]` for macro-defined headers and
/// fixed structs.
pub trait LeWireHeader: Copy + 'static {
    /// Number of bytes in the stable little-endian wire header.
    const LE_WIRE_SIZE: usize;

    /// Append this header to `out` in stable little-endian wire form.
    fn write_le(&self, out: &mut Vec<u8>);

    /// Read this header from its exact stable little-endian wire form.
    fn read_le(bytes: &[u8]) -> Result<Self, WireError>;
}

macro_rules! impl_le_wire_for_int {
    ($($ty:ty),* $(,)?) => {
        $(
            impl LeWireHeader for $ty {
                const LE_WIRE_SIZE: usize = core::mem::size_of::<Self>();

                fn write_le(&self, out: &mut Vec<u8>) {
                    out.extend_from_slice(&self.to_le_bytes());
                }

                fn read_le(bytes: &[u8]) -> Result<Self, WireError> {
                    if bytes.len() != Self::LE_WIRE_SIZE {
                        return Err(invalid_header::<Self>(format!(
                            "little-endian header field has {} bytes, expected {}",
                            bytes.len(),
                            Self::LE_WIRE_SIZE
                        )));
                    }
                    let mut array = [0u8; core::mem::size_of::<Self>()];
                    array.copy_from_slice(bytes);
                    Ok(Self::from_le_bytes(array))
                }
            }
        )*
    };
}

impl_le_wire_for_int!(u8, u16, u32, u64, u128, i8, i16, i32, i64, i128);

impl LeWireHeader for f32 {
    const LE_WIRE_SIZE: usize = 4;

    fn write_le(&self, out: &mut Vec<u8>) {
        out.extend_from_slice(&self.to_bits().to_le_bytes());
    }

    fn read_le(bytes: &[u8]) -> Result<Self, WireError> {
        Ok(Self::from_bits(u32::read_le(bytes)?))
    }
}

impl LeWireHeader for f64 {
    const LE_WIRE_SIZE: usize = 8;

    fn write_le(&self, out: &mut Vec<u8>) {
        out.extend_from_slice(&self.to_bits().to_le_bytes());
    }

    fn read_le(bytes: &[u8]) -> Result<Self, WireError> {
        Ok(Self::from_bits(u64::read_le(bytes)?))
    }
}

impl LeWireHeader for bool {
    const LE_WIRE_SIZE: usize = 1;

    fn write_le(&self, out: &mut Vec<u8>) {
        out.push(u8::from(*self));
    }

    fn read_le(bytes: &[u8]) -> Result<Self, WireError> {
        let value = u8::read_le(bytes)?;
        match value {
            0 => Ok(false),
            1 => Ok(true),
            other => Err(invalid_header::<Self>(format!(
                "invalid bool wire value {other}"
            ))),
        }
    }
}

impl<T, const N: usize> LeWireHeader for [T; N]
where
    T: LeWireHeader + 'static,
{
    const LE_WIRE_SIZE: usize = T::LE_WIRE_SIZE * N;

    fn write_le(&self, out: &mut Vec<u8>) {
        for item in self {
            item.write_le(out);
        }
    }

    fn read_le(bytes: &[u8]) -> Result<Self, WireError> {
        if bytes.len() != Self::LE_WIRE_SIZE {
            return Err(invalid_header::<Self>(format!(
                "little-endian array field has {} bytes, expected {}",
                bytes.len(),
                Self::LE_WIRE_SIZE
            )));
        }
        let mut items = Vec::with_capacity(N);
        for chunk in bytes.chunks_exact(T::LE_WIRE_SIZE) {
            items.push(T::read_le(chunk)?);
        }
        items.try_into().map_err(|_| {
            invalid_header::<Self>("little-endian array field produced the wrong item count")
        })
    }
}

/// Read a single `LeWireHeader` field from `bytes`, advancing `offset`.
///
/// This helper is public for the `#[datapod]` macro expansion.
pub fn read_le_field<T>(
    bytes: &[u8],
    offset: &mut usize,
    type_name: &'static str,
) -> Result<T, WireError>
where
    T: LeWireHeader + 'static,
{
    let end = offset
        .checked_add(T::LE_WIRE_SIZE)
        .ok_or_else(|| invalid_header::<T>("little-endian header offset overflowed"))?;
    if end > bytes.len() {
        return Err(WireError::ShortHeader {
            type_name,
            needed: end,
            got: bytes.len(),
        });
    }
    let value = T::read_le(&bytes[*offset..end])?;
    *offset = end;
    Ok(value)
}

/// Borrowed view for fixed-size datapods.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct FixedView<T> {
    pub value: T,
}

/// Borrowed view for the common datapod shape: copied header plus borrowed
/// byte payload.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct BytePayloadView<'a, H> {
    pub header: H,
    pub payload: &'a [u8],
}

/// Types that can validate canonical datapod wire parts before borrowed access.
pub trait DataPodValidate: DataPod {
    fn validate_wire_parts(_header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        if payload.is_empty() {
            Ok(())
        } else {
            Err(WireError::InvalidPayloadSize {
                type_name: core::any::type_name::<Self>(),
                message: format!("fixed datapod payload must be empty, got {}", payload.len()),
            })
        }
    }
}

/// Types that can expose a borrowed view over canonical datapod wire bytes.
pub trait DataPodAccess: DataPodValidate {
    type View<'a>
    where
        Self: 'a;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError>;

    /// Build a borrowed view without validation.
    ///
    /// # Safety
    ///
    /// The caller must ensure that `header` and `payload` satisfy all
    /// invariants required by `Self`, including payload size, semantic header
    /// validity, and the borrowed payload lifetime.
    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Self::View<'a>;
}

/// Borrow a zero-copy frame for a datapod with an explicit type hash.
///
/// This is callback-based because heap datapods build their compact header on
/// the stack. The callback receives a frame whose header points at that stack
/// header and whose payload points at the original datapod payload.
///
/// This function never falls back to copying payload bytes. If a datapod cannot
/// expose one contiguous payload slice, it returns an error instead of hiding a
/// copy behind a zero-copy API.
pub fn with_wire_frame_named<T, R>(
    type_hash: u64,
    value: &T,
    f: impl FnOnce(WireFrame<'_>) -> R,
) -> Result<R, WireError>
where
    T: DataPod,
{
    let payload = value.payload_bytes();
    if payload.len() != value.payload_len() {
        return Err(WireError::InvalidPayloadSize {
            type_name: core::any::type_name::<T>(),
            message: format!(
                "datapod exposes {} contiguous payload bytes but reports {} payload bytes; \
                 use a segmented frame API for multi-payload datapods",
                payload.len(),
                value.payload_len()
            ),
        });
    }

    #[cfg(not(target_endian = "little"))]
    {
        return Err(WireError::InvalidHeader {
            type_name: core::any::type_name::<T>(),
            message: "zero-copy wire frames require native little-endian header storage"
                .to_string(),
        });
    }

    #[cfg(target_endian = "little")]
    {
        let header = value.header();
        let header_bytes = bytemuck::bytes_of(&header);
        Ok(f(WireFrame {
            type_hash,
            header: header_bytes,
            payload,
        }))
    }
}

/// Borrow a zero-copy frame for a datapod using the current canonical type hash.
pub fn with_wire_frame<T, R>(value: &T, f: impl FnOnce(WireFrame<'_>) -> R) -> Result<R, WireError>
where
    T: DataPod,
{
    with_wire_frame_named(crate::bind::type_hash::<T>(), value, f)
}

/// Borrow a scatter/gather zero-copy frame for a datapod with an explicit type
/// hash.
pub fn with_segmented_wire_frame_named<T, R>(
    type_hash: u64,
    value: &T,
    f: impl FnOnce(WireSegmentedFrame<'_>) -> R,
) -> Result<R, WireError>
where
    T: DataPod,
{
    #[cfg(not(target_endian = "little"))]
    {
        return Err(WireError::InvalidHeader {
            type_name: core::any::type_name::<T>(),
            message: "zero-copy wire frames require native little-endian header storage"
                .to_string(),
        });
    }

    #[cfg(target_endian = "little")]
    {
        let header = value.header();
        let header_bytes = bytemuck::bytes_of(&header);
        Ok(value.with_payload_segments(|payloads| {
            f(WireSegmentedFrame {
                type_hash,
                header: header_bytes,
                payloads,
            })
        }))
    }
}

/// Borrow a scatter/gather zero-copy frame for a datapod using the current
/// canonical type hash.
pub fn with_segmented_wire_frame<T, R>(
    value: &T,
    f: impl FnOnce(WireSegmentedFrame<'_>) -> R,
) -> Result<R, WireError>
where
    T: DataPod,
{
    with_segmented_wire_frame_named(crate::bind::type_hash::<T>(), value, f)
}

/// Collect a borrowed frame into the owned/copying [`WireMessage`] shape.
pub fn wire_frame_to_message(frame: WireFrame<'_>) -> WireMessage {
    let mut bytes = Vec::with_capacity(frame.joined_len());
    bytes.extend_from_slice(frame.header);
    bytes.extend_from_slice(frame.payload);
    WireMessage {
        type_hash: frame.type_hash,
        bytes,
    }
}

/// Collect a borrowed scatter/gather frame into the owned/copying
/// [`WireMessage`] shape.
pub fn wire_segmented_frame_to_message(frame: WireSegmentedFrame<'_>) -> WireMessage {
    let mut bytes = Vec::with_capacity(frame.joined_len());
    bytes.extend_from_slice(frame.header);
    for payload in frame.payloads {
        bytes.extend_from_slice(payload);
    }
    WireMessage {
        type_hash: frame.type_hash,
        bytes,
    }
}

/// Encode any datapod value as the current canonical owned wire message.
///
/// The current default is `datapod-wire-v1/le`: headers are written
/// field-by-field in little-endian order without native struct padding. Built-in
/// registered types emit their canonical-name hash.
pub fn to_wire_message<T>(value: &T) -> WireMessage
where
    T: DataPod,
    T::Header: LeWireHeader,
{
    to_wire_message_v1_named(crate::bind::type_hash::<T>(), value)
}

/// Encode a datapod in the stable `datapod-wire-v1/le` form.
///
/// This helper is retained as an explicit v1 alias for callers that want the
/// format in the name.
pub fn to_wire_message_v1<T>(value: &T) -> Result<WireMessage, WireError>
where
    T: DataPod,
    T::Header: LeWireHeader,
{
    Ok(to_wire_message(value))
}

/// Encode a datapod in `datapod-wire-v1/le` form with an explicit canonical
/// type hash.
///
/// This is useful for custom Rust datapods that are not in datapod's built-in
/// registry but still have an application-level canonical schema name.
pub fn to_wire_message_v1_named<T>(type_hash: u64, value: &T) -> WireMessage
where
    T: DataPod,
    T::Header: LeWireHeader,
{
    let header = value.header();
    let mut bytes = Vec::with_capacity(T::Header::LE_WIRE_SIZE + value.payload_len());
    header.write_le(&mut bytes);
    value.write_payload_bytes(&mut bytes);
    WireMessage { type_hash, bytes }
}

/// Validate typed `datapod-wire-v1/le` bytes without constructing an owned
/// value.
pub fn validate_wire_bytes_v1<T>(type_hash: u64, bytes: &[u8]) -> Result<(), WireError>
where
    T: DataPodValidate,
    T::Header: LeWireHeader,
{
    let (header, payload) = typed_wire_parts_v1::<T>(type_hash, bytes)?;
    T::validate_wire_parts(&header, payload)
}

/// Validate typed borrowed `datapod-wire-v1/le` frame bytes without
/// constructing an owned value.
pub fn validate_wire_frame_v1<T>(frame: WireFrame<'_>) -> Result<(), WireError>
where
    T: DataPodValidate,
    T::Header: LeWireHeader,
{
    let (header, payload) = typed_wire_frame_parts_v1::<T>(frame)?;
    T::validate_wire_parts(&header, payload)
}

/// Access a typed borrowed view from `datapod-wire-v1/le` bytes.
pub fn access_wire_bytes_v1<T>(type_hash: u64, bytes: &[u8]) -> Result<T::View<'_>, WireError>
where
    T: DataPodAccess,
    T::Header: LeWireHeader,
{
    let (header, payload) = typed_wire_parts_v1::<T>(type_hash, bytes)?;
    T::access_wire_parts(header, payload)
}

/// Access a typed borrowed view from a borrowed `datapod-wire-v1/le` frame.
pub fn access_wire_frame_v1<T>(frame: WireFrame<'_>) -> Result<T::View<'_>, WireError>
where
    T: DataPodAccess,
    T::Header: LeWireHeader,
{
    let (header, payload) = typed_wire_frame_parts_v1::<T>(frame)?;
    T::access_wire_parts(header, payload)
}

/// Decode a concrete datapod type from an owned `datapod-wire-v1/le` message.
pub fn from_wire_message_v1<T>(msg: &WireMessage) -> Result<T, WireError>
where
    T: DataPodDecode + DataPodValidate,
    T::Header: LeWireHeader,
{
    let (header, payload) = typed_wire_parts_v1::<T>(msg.type_hash, &msg.bytes)?;
    T::validate_wire_parts(&header, payload)?;
    T::from_wire_parts(header, payload.to_vec())
}

/// Decode a concrete datapod type from a borrowed `datapod-wire-v1/le` frame.
///
/// This is intentionally the owned/copying path. Prefer
/// [`access_wire_frame_v1`] when a borrowed view is sufficient.
pub fn from_wire_frame_v1<T>(frame: WireFrame<'_>) -> Result<T, WireError>
where
    T: DataPodDecode + DataPodValidate,
    T::Header: LeWireHeader,
{
    let (header, payload) = typed_wire_frame_parts_v1::<T>(frame)?;
    T::validate_wire_parts(&header, payload)?;
    T::from_wire_parts(header, payload.to_vec())
}

/// Split a registered datapod wire body into borrowed header and payload bytes.
pub fn split_wire_parts(type_hash: u64, bytes: &[u8]) -> Result<WireParts<'_>, WireError> {
    let Some(info) = crate::registry::find_type_info(type_hash) else {
        return Err(WireError::UnknownTypeHash { type_hash });
    };
    if bytes.len() < info.header_size {
        return Err(WireError::ShortHeader {
            type_name: "registered datapod type",
            needed: info.header_size,
            got: bytes.len(),
        });
    }
    Ok(WireParts {
        type_hash,
        header: &bytes[..info.header_size],
        payload: &bytes[info.header_size..],
    })
}

/// Split a registered contiguous `header || payload` buffer into a borrowed
/// zero-copy frame.
pub fn split_wire_frame(type_hash: u64, bytes: &[u8]) -> Result<WireFrame<'_>, WireError> {
    let parts = split_wire_parts(type_hash, bytes)?;
    Ok(WireFrame {
        type_hash: parts.type_hash,
        header: parts.header,
        payload: parts.payload,
    })
}

/// Validate a typed datapod message without constructing an owned value.
pub fn validate_wire<T>(msg: &WireMessage) -> Result<(), WireError>
where
    T: DataPodValidate,
    T::Header: LeWireHeader,
{
    validate_wire_bytes::<T>(msg.type_hash, &msg.bytes)
}

/// Validate typed current-format datapod wire bytes without constructing an
/// owned value.
///
pub fn validate_wire_bytes<T>(type_hash: u64, bytes: &[u8]) -> Result<(), WireError>
where
    T: DataPodValidate,
    T::Header: LeWireHeader,
{
    let (header, payload) = typed_wire_parts_v1::<T>(type_hash, bytes)?;
    T::validate_wire_parts(&header, payload)
}

/// Validate a typed borrowed frame without constructing an owned value.
pub fn validate_wire_frame<T>(frame: WireFrame<'_>) -> Result<(), WireError>
where
    T: DataPodValidate,
    T::Header: LeWireHeader,
{
    validate_wire_frame_v1::<T>(frame)
}

/// Validate a registered datapod wire message using the strongest validator
/// known to this crate.
///
/// Built-in types with semantic validators are dispatched to their typed
/// [`DataPodValidate`] implementations. Runtime schemas registered through the
/// C/Python ABI only have registry metadata, so validation falls back to
/// proving that the type hash is known and the message contains a full header.
pub fn validate_registered_wire(type_hash: u64, bytes: &[u8]) -> Result<(), WireError> {
    macro_rules! validate_builtin {
        ($($ty:ty),* $(,)?) => {
            $(
                if crate::bind::is_type_hash_for::<$ty>(type_hash) {
                    return validate_wire_bytes::<$ty>(type_hash, bytes);
                }
            )*
        };
    }

    validate_builtin!(
        crate::Bytes,
        crate::BitVec,
        crate::Encoding,
        crate::DpStr,
        crate::Deque,
        crate::ForwardList,
        crate::Geometry,
        crate::GeometryKind,
        crate::Grid,
        crate::Heap,
        crate::IndexedHeap,
        crate::Joint,
        crate::JointType,
        crate::Layer,
        crate::List,
        crate::Map,
        crate::Matrix,
        crate::PagedVecvec,
        crate::Queue,
        crate::Set,
        crate::Stack,
        crate::Tensor,
        crate::Vector,
        crate::Vecvec,
    );

    split_wire_parts(type_hash, bytes).map(|_| ())
}

/// Validate a registered `datapod-wire-v1/le` message using the strongest
/// validator known to this crate.
pub fn validate_registered_wire_v1(type_hash: u64, bytes: &[u8]) -> Result<(), WireError> {
    crate::registry::validate_registered_wire_v1(type_hash, bytes)
}

/// Validate a registered borrowed frame using the strongest validator known to
/// this crate.
///
/// Built-in types dispatch to typed semantic validators. Runtime schemas
/// registered through the C/Python ABI only have registry metadata, so
/// validation proves that the type hash is known and the frame has the
/// registered header width.
pub fn validate_registered_wire_frame(frame: WireFrame<'_>) -> Result<(), WireError> {
    validate_registered_wire_frame_v1(frame)
}

/// Validate a registered borrowed `datapod-wire-v1/le` frame using the
/// strongest validator known to this crate.
pub fn validate_registered_wire_frame_v1(frame: WireFrame<'_>) -> Result<(), WireError> {
    macro_rules! validate_builtin {
        ($($ty:ty),* $(,)?) => {
            $(
                if crate::bind::is_type_hash_for::<$ty>(frame.type_hash) {
                    return validate_wire_frame_v1::<$ty>(frame);
                }
            )*
        };
    }

    validate_builtin!(
        crate::Bytes,
        crate::BitVec,
        crate::Encoding,
        crate::DpStr,
        crate::Deque,
        crate::ForwardList,
        crate::Geometry,
        crate::GeometryKind,
        crate::Grid,
        crate::Heap,
        crate::IndexedHeap,
        crate::Joint,
        crate::JointType,
        crate::Layer,
        crate::List,
        crate::Map,
        crate::Matrix,
        crate::PagedVecvec,
        crate::Queue,
        crate::Set,
        crate::Stack,
        crate::Tensor,
        crate::Vector,
        crate::Vecvec,
    );

    let Some(info) = crate::registry::find_type_info(frame.type_hash) else {
        return Err(WireError::UnknownTypeHash {
            type_hash: frame.type_hash,
        });
    };
    if frame.header.len() != info.header_size {
        return Err(WireError::ShortHeader {
            type_name: "registered datapod type",
            needed: info.header_size,
            got: frame.header.len(),
        });
    }
    Ok(())
}

/// Access a typed borrowed view from an owned wire message.
pub fn access_wire<T>(msg: &WireMessage) -> Result<T::View<'_>, WireError>
where
    T: DataPodAccess,
    T::Header: LeWireHeader,
{
    access_wire_bytes::<T>(msg.type_hash, &msg.bytes)
}

/// Access a typed borrowed view from a current-format borrowed wire byte slice.
pub fn access_wire_bytes<T>(type_hash: u64, bytes: &[u8]) -> Result<T::View<'_>, WireError>
where
    T: DataPodAccess,
    T::Header: LeWireHeader,
{
    let (header, payload) = typed_wire_parts_v1::<T>(type_hash, bytes)?;
    T::access_wire_parts(header, payload)
}

/// Access a typed borrowed view from a borrowed current-format frame.
pub fn access_wire_frame<T>(frame: WireFrame<'_>) -> Result<T::View<'_>, WireError>
where
    T: DataPodAccess,
    T::Header: LeWireHeader,
{
    access_wire_frame_v1::<T>(frame)
}

/// Access a typed borrowed view without validating the bytes.
///
/// # Safety
///
/// `msg` must have the correct type hash for `T`, contain at least
/// `size_of::<T::Header>()` header bytes, and satisfy all invariants required
/// by `T`'s view type.
pub unsafe fn access_wire_unchecked<T>(msg: &WireMessage) -> T::View<'_>
where
    T: DataPodAccess,
    T::Header: LeWireHeader,
{
    unsafe { access_wire_bytes_unchecked::<T>(msg.type_hash, &msg.bytes) }
}

/// Access a typed borrowed view from borrowed bytes without validation.
///
/// # Safety
///
/// `type_hash` and `bytes` must represent a valid wire message for `T`.
pub unsafe fn access_wire_bytes_unchecked<T>(type_hash: u64, bytes: &[u8]) -> T::View<'_>
where
    T: DataPodAccess,
    T::Header: LeWireHeader,
{
    assert!(
        crate::bind::is_type_hash_for::<T>(type_hash),
        "wrong datapod type hash"
    );
    assert!(
        bytes.len() >= T::Header::LE_WIRE_SIZE,
        "short datapod header"
    );
    let header = T::Header::read_le(&bytes[..T::Header::LE_WIRE_SIZE])
        .expect("invalid little-endian datapod header");
    let payload = &bytes[T::Header::LE_WIRE_SIZE..];
    unsafe { T::access_wire_parts_unchecked(header, payload) }
}

/// Access a typed borrowed view from a borrowed frame without validation.
///
/// # Safety
///
/// `frame` must have the correct type hash for `T`, contain an exact valid
/// current-format header, and satisfy all invariants required by `T`'s view
/// type.
pub unsafe fn access_wire_frame_unchecked<T>(frame: WireFrame<'_>) -> T::View<'_>
where
    T: DataPodAccess,
    T::Header: LeWireHeader,
{
    assert!(
        crate::bind::is_type_hash_for::<T>(frame.type_hash),
        "wrong datapod type hash"
    );
    assert!(
        frame.header.len() == T::Header::LE_WIRE_SIZE,
        "invalid datapod header length"
    );
    let header = T::Header::read_le(frame.header).expect("invalid little-endian datapod header");
    unsafe { T::access_wire_parts_unchecked(header, frame.payload) }
}

/// Decode a concrete datapod type from a current-format owned wire message.
///
pub fn from_wire_message<T>(msg: &WireMessage) -> Result<T, WireError>
where
    T: DataPodDecode,
    T::Header: LeWireHeader,
{
    let (header, payload) = typed_wire_parts_v1::<T>(msg.type_hash, &msg.bytes)?;
    T::from_wire_parts(header, payload.to_vec())
}

/// Decode a concrete datapod type from a borrowed current-format frame.
///
/// This allocates/copies payload bytes to reconstruct an owned value. Prefer
/// [`access_wire_frame`] for the zero-copy receive path.
pub fn from_wire_frame<T>(frame: WireFrame<'_>) -> Result<T, WireError>
where
    T: DataPodDecode,
    T::Header: LeWireHeader,
{
    let (header, payload) = typed_wire_frame_parts_v1::<T>(frame)?;
    T::from_wire_parts(header, payload.to_vec())
}

fn typed_wire_parts_v1<T>(type_hash: u64, bytes: &[u8]) -> Result<(T::Header, &[u8]), WireError>
where
    T: DataPod,
    T::Header: LeWireHeader,
{
    let expected = crate::bind::type_hash::<T>();
    if type_hash != expected {
        return Err(WireError::WrongTypeHash {
            expected,
            got: type_hash,
        });
    }

    let header_size = T::Header::LE_WIRE_SIZE;
    if bytes.len() < header_size {
        return Err(WireError::ShortHeader {
            type_name: core::any::type_name::<T>(),
            needed: header_size,
            got: bytes.len(),
        });
    }

    let header = T::Header::read_le(&bytes[..header_size])?;
    Ok((header, &bytes[header_size..]))
}

fn typed_wire_frame_parts_v1<T>(frame: WireFrame<'_>) -> Result<(T::Header, &[u8]), WireError>
where
    T: DataPod,
    T::Header: LeWireHeader,
{
    let expected = crate::bind::type_hash::<T>();
    if frame.type_hash != expected {
        return Err(WireError::WrongTypeHash {
            expected,
            got: frame.type_hash,
        });
    }

    let header_size = T::Header::LE_WIRE_SIZE;
    if frame.header.len() != header_size {
        return Err(WireError::ShortHeader {
            type_name: core::any::type_name::<T>(),
            needed: header_size,
            got: frame.header.len(),
        });
    }

    let header = T::Header::read_le(frame.header)?;
    Ok((header, frame.payload))
}

pub(crate) fn invalid_header<T: 'static>(message: impl Into<String>) -> WireError {
    WireError::InvalidHeader {
        type_name: core::any::type_name::<T>(),
        message: message.into(),
    }
}

pub(crate) fn invalid_payload<T: 'static>(message: impl Into<String>) -> WireError {
    WireError::InvalidPayloadSize {
        type_name: core::any::type_name::<T>(),
        message: message.into(),
    }
}

pub(crate) fn checked_product<T: 'static>(factors: &[usize]) -> Result<usize, WireError> {
    factors.iter().try_fold(1usize, |acc, factor| {
        acc.checked_mul(*factor)
            .ok_or_else(|| invalid_payload::<T>("payload length calculation overflowed"))
    })
}

/// Decode a heap-backed `#[dp(bytes)] Vec<T>` payload.
///
/// Payload bytes are densely packed and may be unaligned, so reconstruction
/// uses `pod_read_unaligned` instead of casting the input slice.
pub fn decode_payload_vec<T>(payload: &[u8]) -> Result<Vec<T>, WireError>
where
    T: Pod + Copy + 'static,
{
    let elem_size = core::mem::size_of::<T>();
    if elem_size == 0 {
        return Err(WireError::InvalidPayloadSize {
            type_name: core::any::type_name::<T>(),
            message: "zero-sized payload elements are not supported".to_string(),
        });
    }
    if payload.len() % elem_size != 0 {
        return Err(WireError::InvalidPayloadSize {
            type_name: core::any::type_name::<T>(),
            message: format!(
                "{} bytes is not a multiple of element size {elem_size}",
                payload.len()
            ),
        });
    }
    Ok(payload
        .chunks_exact(elem_size)
        .map(bytemuck::pod_read_unaligned::<T>)
        .collect())
}

/// Universal transport envelope. Set by the messaging layer on every
/// published message and made available to the receiver.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, Pod, Zeroable)]
pub struct Envelope {
    pub seq: u64,
    pub stamp_ns: u64,
    pub source_id: [u8; 32],
    pub topic_hash: u64,
    pub kind: u32,
    pub flags: u32,
}

unsafe impl ZeroCopySend for Envelope {}

impl LeWireHeader for Envelope {
    const LE_WIRE_SIZE: usize = <u64 as LeWireHeader>::LE_WIRE_SIZE
        + <u64 as LeWireHeader>::LE_WIRE_SIZE
        + <[u8; 32] as LeWireHeader>::LE_WIRE_SIZE
        + <u64 as LeWireHeader>::LE_WIRE_SIZE
        + <u32 as LeWireHeader>::LE_WIRE_SIZE
        + <u32 as LeWireHeader>::LE_WIRE_SIZE;

    fn write_le(&self, out: &mut Vec<u8>) {
        self.seq.write_le(out);
        self.stamp_ns.write_le(out);
        self.source_id.write_le(out);
        self.topic_hash.write_le(out);
        self.kind.write_le(out);
        self.flags.write_le(out);
    }

    fn read_le(bytes: &[u8]) -> Result<Self, WireError> {
        if bytes.len() != Self::LE_WIRE_SIZE {
            return Err(invalid_header::<Self>(format!(
                "little-endian header has {} bytes, expected {}",
                bytes.len(),
                Self::LE_WIRE_SIZE
            )));
        }
        let mut offset = 0usize;
        Ok(Self {
            seq: read_le_field(bytes, &mut offset, core::any::type_name::<Self>())?,
            stamp_ns: read_le_field(bytes, &mut offset, core::any::type_name::<Self>())?,
            source_id: read_le_field(bytes, &mut offset, core::any::type_name::<Self>())?,
            topic_hash: read_le_field(bytes, &mut offset, core::any::type_name::<Self>())?,
            kind: read_le_field(bytes, &mut offset, core::any::type_name::<Self>())?,
            flags: read_le_field(bytes, &mut offset, core::any::type_name::<Self>())?,
        })
    }
}

impl DataPod for Envelope {
    type Header = Envelope;
    type Payload = ();
    fn header(&self) -> Envelope {
        *self
    }
    fn payload_bytes(&self) -> &[u8] {
        &[]
    }
}

impl DataPodDecode for Envelope {
    fn from_wire_parts(header: Self::Header, payload: Vec<u8>) -> Result<Self, WireError> {
        if !payload.is_empty() {
            return Err(WireError::InvalidPayloadSize {
                type_name: core::any::type_name::<Self>(),
                message: format!("fixed datapod payload must be empty, got {}", payload.len()),
            });
        }
        Ok(header)
    }
}

impl DataPodValidate for Envelope {}

impl DataPodAccess for Envelope {
    type View<'a> = FixedView<Self>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Self::validate_wire_parts(&header, payload)?;
        Ok(FixedView { value: header })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        _payload: &'a [u8],
    ) -> Self::View<'a> {
        FixedView { value: header }
    }
}

/// Encoding tag for variable-payload messages whose bytes are typed.
#[repr(transparent)]
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct Encoding(pub u32);

#[allow(non_upper_case_globals)]
impl Encoding {
    pub const U8: Self = Self(0);
    pub const U16: Self = Self(1);
    pub const U32: Self = Self(2);
    pub const U64: Self = Self(3);
    pub const I8: Self = Self(4);
    pub const I16: Self = Self(5);
    pub const I32: Self = Self(6);
    pub const I64: Self = Self(7);
    pub const F32: Self = Self(8);
    pub const F64: Self = Self(9);
    pub const Rgb8: Self = Self(10);
    pub const Rgba8: Self = Self(11);
    pub const Mono16: Self = Self(12);
    pub const Mono8: Self = Self(13);

    pub fn is_valid(self) -> bool {
        self.0 <= Self::Mono8.0
    }

    pub fn byte_width(self) -> usize {
        match self {
            Self::U8 | Self::I8 | Self::Mono8 => 1,
            Self::U16 | Self::I16 | Self::Mono16 => 2,
            Self::Rgb8 => 3,
            Self::U32 | Self::I32 | Self::F32 | Self::Rgba8 => 4,
            Self::U64 | Self::I64 | Self::F64 => 8,
            _ => 0,
        }
    }
}

unsafe impl Zeroable for Encoding {}
unsafe impl Pod for Encoding {}
unsafe impl ZeroCopySend for Encoding {}

impl LeWireHeader for Encoding {
    const LE_WIRE_SIZE: usize = u32::LE_WIRE_SIZE;

    fn write_le(&self, out: &mut Vec<u8>) {
        self.0.write_le(out);
    }

    fn read_le(bytes: &[u8]) -> Result<Self, WireError> {
        Ok(Self(u32::read_le(bytes)?))
    }
}

impl DataPod for Encoding {
    type Header = Encoding;
    type Payload = ();
    fn header(&self) -> Encoding {
        *self
    }
    fn payload_bytes(&self) -> &[u8] {
        &[]
    }
}

impl DataPodDecode for Encoding {
    fn from_wire_parts(header: Self::Header, payload: Vec<u8>) -> Result<Self, WireError> {
        if !payload.is_empty() {
            return Err(WireError::InvalidPayloadSize {
                type_name: core::any::type_name::<Self>(),
                message: format!("fixed datapod payload must be empty, got {}", payload.len()),
            });
        }
        Self::validate_wire_parts(&header, &payload)?;
        Ok(header)
    }
}

impl DataPodValidate for Encoding {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        if !payload.is_empty() {
            return Err(WireError::InvalidPayloadSize {
                type_name: core::any::type_name::<Self>(),
                message: format!("fixed datapod payload must be empty, got {}", payload.len()),
            });
        }
        if !header.is_valid() {
            return Err(invalid_header::<Self>(format!(
                "unknown encoding tag {}",
                header.0
            )));
        }
        Ok(())
    }
}

impl DataPodAccess for Encoding {
    type View<'a> = FixedView<Self>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Self::validate_wire_parts(&header, payload)?;
        Ok(FixedView { value: header })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        _payload: &'a [u8],
    ) -> Self::View<'a> {
        FixedView { value: header }
    }
}
