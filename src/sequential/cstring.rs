//! C-string container matching `datapod::Cstring` / `CstringView`.
//!
//! The C++ `GenericCstring` is an owning small-string-optimised string that
//! always keeps a trailing `\0` terminator so that `data()` can be passed as
//! a C string. We model the owning variant as `CString` (always
//! null-terminated under the hood, storing the payload in a `Vec<u8>`) and
//! the non-owning variant as `CStringView` (pointing at an externally owned
//! slice).
//!
//! The underlying buffer is kept as raw bytes rather than a UTF-8 `String`
//! because the C++ API explicitly permits embedded `\0`s within the valid
//! data range and only promises null-termination at the end.

use std::borrow::Cow;
use std::fmt;

#[derive(Clone, PartialEq, Eq, Hash)]
pub struct CString {
    // Invariant: `bytes` always ends with a single `0` terminator. `len` is
    // the logical payload length (excluding that terminator).
    bytes: Vec<u8>,
    len: usize,
}

impl Default for CString {
    fn default() -> Self {
        Self {
            bytes: vec![0],
            len: 0,
        }
    }
}

impl CString {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn from_str_lossy<S: AsRef<str>>(s: S) -> Self {
        let slice = s.as_ref().as_bytes();
        let mut bytes = Vec::with_capacity(slice.len() + 1);
        bytes.extend_from_slice(slice);
        bytes.push(0);
        Self {
            bytes,
            len: slice.len(),
        }
    }

    pub fn from_bytes(bytes: &[u8]) -> Self {
        let mut buf = Vec::with_capacity(bytes.len() + 1);
        buf.extend_from_slice(bytes);
        buf.push(0);
        Self {
            bytes: buf,
            len: bytes.len(),
        }
    }

    pub fn len(&self) -> usize {
        self.len
    }

    pub fn size(&self) -> usize {
        self.len
    }

    pub fn is_empty(&self) -> bool {
        self.len == 0
    }

    pub fn empty(&self) -> bool {
        self.is_empty()
    }

    pub fn capacity(&self) -> usize {
        self.bytes.capacity().saturating_sub(1)
    }

    pub fn clear(&mut self) {
        self.bytes.clear();
        self.bytes.push(0);
        self.len = 0;
    }

    pub fn reset(&mut self) {
        self.clear();
    }

    pub fn as_bytes(&self) -> &[u8] {
        &self.bytes[..self.len]
    }

    pub fn data(&self) -> *const u8 {
        self.bytes.as_ptr()
    }

    pub fn c_str(&self) -> *const u8 {
        self.bytes.as_ptr()
    }

    pub fn view(&self) -> Cow<'_, str> {
        String::from_utf8_lossy(self.as_bytes())
    }

    pub fn str(&self) -> String {
        String::from_utf8_lossy(self.as_bytes()).into_owned()
    }

    pub fn reserve(&mut self, new_cap: usize) {
        if new_cap + 1 > self.bytes.capacity() {
            self.bytes.reserve(new_cap + 1 - self.bytes.len());
        }
    }

    pub fn resize(&mut self, new_size: usize) {
        if new_size < self.len {
            self.bytes.truncate(new_size);
            self.bytes.push(0);
        } else {
            self.bytes.pop(); // drop old terminator
            self.bytes.resize(new_size, 0);
            self.bytes.push(0);
        }
        self.len = new_size;
    }

    pub fn push_back(&mut self, byte: u8) {
        // Replace terminator with byte, then re-append terminator.
        let term_pos = self.bytes.len() - 1;
        self.bytes[term_pos] = byte;
        self.bytes.push(0);
        self.len += 1;
    }

    pub fn append_str<S: AsRef<str>>(&mut self, s: S) {
        self.append_bytes(s.as_ref().as_bytes());
    }

    pub fn append_bytes(&mut self, bytes: &[u8]) {
        if bytes.is_empty() {
            return;
        }
        let old_term = self.bytes.len() - 1;
        self.bytes.truncate(old_term);
        self.bytes.extend_from_slice(bytes);
        self.bytes.push(0);
        self.len += bytes.len();
    }

    pub fn set_owning<S: AsRef<str>>(&mut self, s: S) {
        *self = Self::from_str_lossy(s);
    }

    pub fn get(&self, i: usize) -> Option<u8> {
        self.as_bytes().get(i).copied()
    }
}

impl fmt::Debug for CString {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("CString")
            .field("view", &self.view())
            .field("len", &self.len)
            .finish()
    }
}

impl fmt::Display for CString {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.view())
    }
}

impl From<&str> for CString {
    fn from(value: &str) -> Self {
        Self::from_str_lossy(value)
    }
}

impl From<String> for CString {
    fn from(value: String) -> Self {
        Self::from_str_lossy(value)
    }
}

impl From<Vec<u8>> for CString {
    fn from(mut value: Vec<u8>) -> Self {
        // Strip any trailing null, recompute length, then append one.
        while value.last() == Some(&0) {
            value.pop();
        }
        let len = value.len();
        value.push(0);
        Self { bytes: value, len }
    }
}

impl PartialOrd for CString {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for CString {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.as_bytes().cmp(other.as_bytes())
    }
}

/// Non-owning view counterpart to `CString`, mirroring the C++ `CstringView`.
/// Holds a borrowed slice and does not guarantee null-termination without
/// manual construction from a `&CString`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct CStringView<'a> {
    data: &'a [u8],
}

impl<'a> CStringView<'a> {
    pub fn new(data: &'a [u8]) -> Self {
        Self { data }
    }

    pub fn from_str(s: &'a str) -> Self {
        Self {
            data: s.as_bytes(),
        }
    }

    pub fn from_cstring(c: &'a CString) -> Self {
        Self {
            data: c.as_bytes(),
        }
    }

    pub fn as_bytes(&self) -> &[u8] {
        self.data
    }

    pub fn len(&self) -> usize {
        self.data.len()
    }

    pub fn size(&self) -> usize {
        self.data.len()
    }

    pub fn is_empty(&self) -> bool {
        self.data.is_empty()
    }

    pub fn empty(&self) -> bool {
        self.is_empty()
    }

    pub fn view(&self) -> Cow<'_, str> {
        String::from_utf8_lossy(self.data)
    }
}

impl<'a> Default for CStringView<'a> {
    fn default() -> Self {
        Self { data: &[] }
    }
}
