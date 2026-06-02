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
    fn payload_bytes(&self) -> &[u8];
}

/// Owned canonical datapod wire message.
///
/// `bytes` is always `header_bytes || payload_bytes`. Transports can move this
/// value without knowing the concrete datapod type; decoders split it using the
/// registered/static header size for the requested type.
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
    ShortHeader {
        type_name: &'static str,
        needed: usize,
        got: usize,
    },
    MalformedHeader {
        type_name: &'static str,
    },
    InvalidPayloadSize {
        type_name: &'static str,
        message: String,
    },
    UnknownTypeHash {
        type_hash: u64,
    },
}

impl fmt::Display for WireError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::WrongTypeHash { expected, got } => {
                write!(f, "wrong datapod type hash: got {got}, expected {expected}")
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
            Self::InvalidPayloadSize { type_name, message } => {
                write!(
                    f,
                    "{type_name} wire message has invalid payload size: {message}"
                )
            }
            Self::UnknownTypeHash { type_hash } => {
                write!(f, "unknown datapod type hash: {type_hash}")
            }
        }
    }
}

impl std::error::Error for WireError {}

/// Types that can reconstruct an owned value from datapod wire parts.
pub trait DataPodDecode: DataPod + Sized {
    fn from_wire_parts(header: Self::Header, payload: Vec<u8>) -> Result<Self, WireError>;
}

/// Encode any datapod value as the canonical owned wire message.
pub fn to_wire_message<T: DataPod>(value: &T) -> WireMessage {
    let header = value.header();
    let header_bytes = bytemuck::bytes_of(&header);
    let payload = value.payload_bytes();

    let mut bytes = Vec::with_capacity(header_bytes.len() + payload.len());
    bytes.extend_from_slice(header_bytes);
    bytes.extend_from_slice(payload);

    WireMessage {
        type_hash: crate::bind::type_hash::<T>(),
        bytes,
    }
}

/// Decode a concrete datapod type from a canonical owned wire message.
pub fn from_wire_message<T: DataPodDecode>(msg: &WireMessage) -> Result<T, WireError> {
    let expected = crate::bind::type_hash::<T>();
    if msg.type_hash != expected {
        return Err(WireError::WrongTypeHash {
            expected,
            got: msg.type_hash,
        });
    }

    let header_size = core::mem::size_of::<T::Header>();
    if msg.bytes.len() < header_size {
        return Err(WireError::ShortHeader {
            type_name: core::any::type_name::<T>(),
            needed: header_size,
            got: msg.bytes.len(),
        });
    }

    let header =
        bytemuck::try_pod_read_unaligned::<T::Header>(&msg.bytes[..header_size]).map_err(|_| {
            WireError::MalformedHeader {
                type_name: core::any::type_name::<T>(),
            }
        })?;
    let payload = msg.bytes[header_size..].to_vec();
    T::from_wire_parts(header, payload)
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

/// Encoding tag for variable-payload messages whose bytes are typed.
#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Encoding {
    U8 = 0,
    U16 = 1,
    U32 = 2,
    U64 = 3,
    I8 = 4,
    I16 = 5,
    I32 = 6,
    I64 = 7,
    F32 = 8,
    F64 = 9,
    Rgb8 = 10,
    Rgba8 = 11,
    Mono16 = 12,
    Mono8 = 13,
}

impl Default for Encoding {
    fn default() -> Self {
        Self::U8
    }
}

unsafe impl Zeroable for Encoding {}
unsafe impl Pod for Encoding {}
unsafe impl ZeroCopySend for Encoding {}

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
        Ok(header)
    }
}
