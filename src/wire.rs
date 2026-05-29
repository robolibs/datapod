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

use core::fmt::Debug;

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
