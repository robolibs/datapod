//! `DpString` — variable-length UTF-8 text that owns its bytes.

use crate::{DataPodAccess, DataPodValidate, WireError};

/// Sentinel for "no string assigned" — use this value when a `*_id: u32`
/// field is unset.
pub const STRING_NONE: u32 = u32::MAX;

/// Heap-owned UTF-8 string. The bytes ride as the wire payload; the
/// generated `DpStringHeader` is empty (length recovered from
/// `payload.len()`).
#[datapod::datapod]
#[dp(manual_access)]
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
        Self::try_from_str(s).unwrap_or_default()
    }

    pub fn try_from_str(s: &str) -> Result<Self, WireError> {
        let source = s.as_bytes();
        let mut bytes = Vec::new();
        bytes.try_reserve_exact(source.len()).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "failed to reserve {} UTF-8 string bytes: {err}",
                source.len()
            ))
        })?;
        bytes.extend_from_slice(source);
        Ok(Self { bytes })
    }

    pub fn byte_len(&self) -> usize {
        self.bytes.len()
    }

    pub fn is_empty(&self) -> bool {
        self.bytes.is_empty()
    }

    /// Interpret the bytes as UTF-8. Returns `None` on invalid UTF-8.
    pub fn as_str(&self) -> Option<&str> {
        self.try_as_str().ok()
    }

    /// Fallible UTF-8 view for callers handling potentially malformed owned
    /// string buffers.
    pub fn try_as_str(&self) -> Result<&str, WireError> {
        <Self as DataPodValidate>::validate_wire_parts(&DpStringHeader::default(), &self.bytes)?;
        std::str::from_utf8(&self.bytes)
            .map_err(|error| crate::wire::invalid_payload::<Self>(error.to_string()))
    }
}

/// Borrowed view over validated UTF-8 `DpString` wire payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct DpStringView<'a> {
    pub header: DpStringHeader,
    pub payload: &'a [u8],
}

impl<'a> DpStringView<'a> {
    pub fn payload_bytes(&self) -> &'a [u8] {
        self.payload
    }

    pub fn try_as_str(&self) -> Result<&'a str, WireError> {
        std::str::from_utf8(self.payload)
            .map_err(|error| crate::wire::invalid_payload::<DpString>(error.to_string()))
    }

    pub fn as_str(&self) -> &'a str {
        self.try_as_str().unwrap_or("")
    }
}

impl DataPodValidate for DpString {
    fn validate_wire_parts(_header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        std::str::from_utf8(payload)
            .map(|_| ())
            .map_err(|error| crate::wire::invalid_payload::<Self>(error.to_string()))
    }
}

impl DataPodAccess for DpString {
    type View<'a> = DpStringView<'a>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Self::validate_wire_parts(&header, payload)?;
        Ok(DpStringView { header, payload })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Self::View<'a> {
        DpStringView { header, payload }
    }
}
