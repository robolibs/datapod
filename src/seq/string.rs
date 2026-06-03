//! UTF-8 string Pod.
//!
//! Named `DpStr` to avoid collision with `std::string::String` and with the
//! `DpString` identifier type in [`crate::id`]. The wire form is just the
//! UTF-8 bytes — length is recovered from the payload slice length.

use crate::{DataPodAccess, DataPodValidate, WireError};

/// Sentinel returned by `find` / `rfind` when no match is present.
pub const STR_NPOS: usize = usize::MAX;

#[datapod::datapod]
#[dp(manual_access)]
#[derive(Default)]
pub struct DpStr {
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl DpStr {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn from_str(s: &str) -> Self {
        Self {
            data: s.as_bytes().to_vec(),
        }
    }

    /// Try to view the payload as UTF-8.
    pub fn as_str(&self) -> Option<&str> {
        std::str::from_utf8(&self.data).ok()
    }

    /// View the payload as UTF-8, replacing invalid sequences with U+FFFD.
    pub fn to_string_lossy(&self) -> String {
        String::from_utf8_lossy(&self.data).into_owned()
    }

    pub fn size_bytes(&self) -> usize {
        self.data.len()
    }

    pub fn empty(&self) -> bool {
        self.data.is_empty()
    }

    pub fn append(&mut self, s: &str) {
        self.data.extend_from_slice(s.as_bytes());
    }

    pub fn push(&mut self, ch: char) {
        let mut buf = [0u8; 4];
        let s = ch.encode_utf8(&mut buf);
        self.data.extend_from_slice(s.as_bytes());
    }

    pub fn clear(&mut self) {
        self.data.clear();
    }

    pub fn find(&self, needle: &str, from: usize) -> usize {
        if from >= self.data.len() {
            return STR_NPOS;
        }
        let hay = &self.data[from..];
        hay.windows(needle.len())
            .position(|w| w == needle.as_bytes())
            .map(|i| i + from)
            .unwrap_or(STR_NPOS)
    }

    pub fn starts_with(&self, prefix: &str) -> bool {
        self.data.starts_with(prefix.as_bytes())
    }

    pub fn ends_with(&self, suffix: &str) -> bool {
        self.data.ends_with(suffix.as_bytes())
    }
}

impl From<&str> for DpStr {
    fn from(s: &str) -> Self {
        Self::from_str(s)
    }
}

impl From<String> for DpStr {
    fn from(s: String) -> Self {
        Self {
            data: s.into_bytes(),
        }
    }
}

/// Borrowed view over UTF-8 `DpStr` wire payload.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct DpStrView<'a> {
    pub data: &'a [u8],
}

impl<'a> DpStrView<'a> {
    pub fn as_bytes(&self) -> &'a [u8] {
        self.data
    }

    pub fn as_str(&self) -> &'a str {
        // Safe because `DataPodAccess` validates UTF-8 before constructing the
        // checked view. Unchecked access inherits the caller's safety contract.
        unsafe { std::str::from_utf8_unchecked(self.data) }
    }
}

impl DataPodValidate for DpStr {
    fn validate_wire_parts(_header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        std::str::from_utf8(payload)
            .map(|_| ())
            .map_err(|error| crate::wire::invalid_payload::<Self>(error.to_string()))
    }
}

impl DataPodAccess for DpStr {
    type View<'a> = DpStrView<'a>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Self::validate_wire_parts(&header, payload)?;
        Ok(DpStrView { data: payload })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        _header: Self::Header,
        payload: &'a [u8],
    ) -> Self::View<'a> {
        DpStrView { data: payload }
    }
}
