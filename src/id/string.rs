//! `DpString` — variable-length UTF-8 text that owns its bytes.

/// Sentinel for "no string assigned" — use this value when a `*_id: u32`
/// field is unset.
pub const STRING_NONE: u32 = u32::MAX;

/// Heap-owned UTF-8 string. The bytes ride as the wire payload; the
/// generated `DpStringHeader` is empty (length recovered from
/// `payload.len()`).
#[datapod::datapod]
#[derive(Default)]
pub struct DpString {
    #[dp(bytes)]
    pub bytes: Vec<u8>,
}

impl DpString {
    pub fn new(bytes: Vec<u8>) -> Self {
        Self { bytes }
    }

    pub fn from_str(s: &str) -> Self {
        Self {
            bytes: s.as_bytes().to_vec(),
        }
    }

    pub fn byte_len(&self) -> usize {
        self.bytes.len()
    }

    pub fn is_empty(&self) -> bool {
        self.bytes.is_empty()
    }

    /// Interpret the bytes as UTF-8. Returns `None` on invalid UTF-8.
    pub fn as_str(&self) -> Option<&str> {
        std::str::from_utf8(&self.bytes).ok()
    }
}
