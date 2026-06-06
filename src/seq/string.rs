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
        Self::try_from_str(s).unwrap_or_default()
    }

    pub fn try_from_str(s: &str) -> Result<Self, WireError> {
        let bytes = s.as_bytes();
        let mut data = Vec::new();
        data.try_reserve_exact(bytes.len()).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "failed to reserve {} UTF-8 bytes: {err}",
                bytes.len()
            ))
        })?;
        data.extend_from_slice(bytes);
        Ok(Self { data })
    }

    /// Try to view the payload as UTF-8.
    pub fn as_str(&self) -> Option<&str> {
        self.try_as_str().ok()
    }

    /// Fallible UTF-8 view for callers handling potentially malformed owned
    /// string buffers.
    pub fn try_as_str(&self) -> Result<&str, WireError> {
        <Self as DataPodValidate>::validate_wire_parts(&DpStrHeader {}, &self.data)?;
        std::str::from_utf8(&self.data)
            .map_err(|error| crate::wire::invalid_payload::<Self>(error.to_string()))
    }

    /// View the payload as UTF-8, replacing invalid sequences with U+FFFD.
    pub fn to_string_lossy(&self) -> String {
        self.try_to_string_lossy().unwrap_or_default()
    }

    /// Fallible lossy UTF-8 conversion. Unlike [`Self::to_string_lossy`], this
    /// reports allocation failure instead of panicking/aborting through an
    /// infallible `String` growth path.
    pub fn try_to_string_lossy(&self) -> Result<String, WireError> {
        let output_len = lossy_utf8_len::<Self>(&self.data)?;
        let mut output = String::new();
        output.try_reserve_exact(output_len).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "failed to reserve {output_len} lossy UTF-8 bytes: {err}"
            ))
        })?;
        push_lossy_utf8::<Self>(&mut output, &self.data)?;
        Ok(output)
    }

    pub fn size_bytes(&self) -> usize {
        self.data.len()
    }

    pub fn empty(&self) -> bool {
        self.data.is_empty()
    }

    pub fn append(&mut self, s: &str) {
        let _ = self.try_append(s);
    }

    pub fn try_append(&mut self, s: &str) -> Result<(), WireError> {
        let bytes = s.as_bytes();
        self.data.try_reserve_exact(bytes.len()).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "failed to reserve {} appended UTF-8 bytes: {err}",
                bytes.len()
            ))
        })?;
        self.data.extend_from_slice(bytes);
        Ok(())
    }

    pub fn push(&mut self, ch: char) {
        let _ = self.try_push(ch);
    }

    pub fn try_push(&mut self, ch: char) -> Result<(), WireError> {
        let mut buf = [0u8; 4];
        let s = ch.encode_utf8(&mut buf);
        self.data.try_reserve_exact(s.len()).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "failed to reserve {} pushed UTF-8 bytes: {err}",
                s.len()
            ))
        })?;
        self.data.extend_from_slice(s.as_bytes());
        Ok(())
    }

    pub fn clear(&mut self) {
        self.data.clear();
    }

    pub fn find(&self, needle: &str, from: usize) -> usize {
        if from > self.data.len() {
            return STR_NPOS;
        }
        if needle.is_empty() {
            return from;
        }
        if from == self.data.len() {
            return STR_NPOS;
        }
        let Some(hay) = self.data.get(from..) else {
            return STR_NPOS;
        };
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

    pub fn try_as_str(&self) -> Result<&'a str, WireError> {
        std::str::from_utf8(self.data)
            .map_err(|error| crate::wire::invalid_payload::<DpStr>(error.to_string()))
    }

    pub fn as_str(&self) -> &'a str {
        self.try_as_str().unwrap_or("")
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

const UTF8_REPLACEMENT: &str = "\u{FFFD}";

fn lossy_utf8_len<T: 'static>(mut bytes: &[u8]) -> Result<usize, WireError> {
    let mut len = 0usize;
    loop {
        match std::str::from_utf8(bytes) {
            Ok(valid) => {
                return len.checked_add(valid.len()).ok_or_else(|| {
                    crate::wire::invalid_payload::<T>("lossy UTF-8 length overflow")
                });
            }
            Err(error) => {
                let valid_up_to = error.valid_up_to();
                len = len
                    .checked_add(valid_up_to)
                    .and_then(|len| len.checked_add(UTF8_REPLACEMENT.len()))
                    .ok_or_else(|| {
                        crate::wire::invalid_payload::<T>("lossy UTF-8 length overflow")
                    })?;
                let Some(invalid_len) = error.error_len() else {
                    return Ok(len);
                };
                let advance = valid_up_to.checked_add(invalid_len).ok_or_else(|| {
                    crate::wire::invalid_payload::<T>("lossy UTF-8 cursor overflow")
                })?;
                bytes = bytes.get(advance..).ok_or_else(|| {
                    crate::wire::invalid_payload::<T>("lossy UTF-8 cursor is out of bounds")
                })?;
            }
        }
    }
}

fn push_lossy_utf8<T: 'static>(out: &mut String, mut bytes: &[u8]) -> Result<(), WireError> {
    loop {
        match std::str::from_utf8(bytes) {
            Ok(valid) => {
                out.push_str(valid);
                return Ok(());
            }
            Err(error) => {
                let valid_up_to = error.valid_up_to();
                let valid = bytes
                    .get(..valid_up_to)
                    .and_then(|part| std::str::from_utf8(part).ok())
                    .ok_or_else(|| {
                        crate::wire::invalid_payload::<T>(
                            "lossy UTF-8 valid prefix is out of bounds",
                        )
                    })?;
                out.push_str(valid);
                out.push_str(UTF8_REPLACEMENT);
                let Some(invalid_len) = error.error_len() else {
                    return Ok(());
                };
                let advance = valid_up_to.checked_add(invalid_len).ok_or_else(|| {
                    crate::wire::invalid_payload::<T>("lossy UTF-8 cursor overflow")
                })?;
                bytes = bytes.get(advance..).ok_or_else(|| {
                    crate::wire::invalid_payload::<T>("lossy UTF-8 cursor is out of bounds")
                })?;
            }
        }
    }
}
