//! Raw byte buffer matching `datapod::Bytes` (template `BasicBytes`).
//!
//! The C++ type is a thin wrapper around `Vector<u8>` exposing string-like
//! search/compare helpers alongside the usual vector API. We keep the Rust
//! alias `Bytes = Vec<u8>` so callers can initialise with `vec![..]`, and
//! provide the extended C++ API through an extension trait.

pub type Bytes = Vec<u8>;

pub const BYTES_NPOS: usize = usize::MAX;

/// C++-style byte-buffer API for `Vec<u8>`.
pub trait BytesExt {
    fn size(&self) -> usize;
    fn empty(&self) -> bool;
    fn append_slice(&mut self, slice: &[u8]);
    fn append_bytes(&mut self, other: &Vec<u8>);
    fn zero(&mut self);
    fn fill_byte(&mut self, value: u8);
    fn find_byte(&self, needle: u8, pos: usize) -> usize;
    fn find_bytes(&self, needle: &[u8], pos: usize) -> usize;
    fn rfind_byte(&self, needle: u8, pos: usize) -> usize;
    fn contains_byte(&self, needle: u8) -> bool;
    fn contains_bytes(&self, needle: &[u8]) -> bool;
    fn starts_with_byte(&self, byte: u8) -> bool;
    fn starts_with_bytes(&self, prefix: &[u8]) -> bool;
    fn ends_with_byte(&self, byte: u8) -> bool;
    fn ends_with_bytes(&self, suffix: &[u8]) -> bool;
    fn substr(&self, pos: usize, count: usize) -> Vec<u8>;
    fn front_byte(&self) -> u8;
    fn back_byte(&self) -> u8;
}

impl BytesExt for Vec<u8> {
    #[inline]
    fn size(&self) -> usize {
        self.len()
    }

    #[inline]
    fn empty(&self) -> bool {
        self.is_empty()
    }

    fn append_slice(&mut self, slice: &[u8]) {
        self.extend_from_slice(slice);
    }

    fn append_bytes(&mut self, other: &Vec<u8>) {
        self.extend_from_slice(other.as_slice());
    }

    fn zero(&mut self) {
        for b in self.iter_mut() {
            *b = 0;
        }
    }

    fn fill_byte(&mut self, value: u8) {
        for b in self.iter_mut() {
            *b = value;
        }
    }

    fn find_byte(&self, needle: u8, pos: usize) -> usize {
        if pos >= self.len() {
            return BYTES_NPOS;
        }
        for (i, byte) in self[pos..].iter().enumerate() {
            if *byte == needle {
                return pos + i;
            }
        }
        BYTES_NPOS
    }

    fn find_bytes(&self, needle: &[u8], pos: usize) -> usize {
        if needle.is_empty() {
            return if pos <= self.len() { pos } else { BYTES_NPOS };
        }
        if pos + needle.len() > self.len() {
            return BYTES_NPOS;
        }
        for i in pos..=self.len() - needle.len() {
            if &self[i..i + needle.len()] == needle {
                return i;
            }
        }
        BYTES_NPOS
    }

    fn rfind_byte(&self, needle: u8, pos: usize) -> usize {
        if self.is_empty() {
            return BYTES_NPOS;
        }
        let start = pos.min(self.len() - 1);
        let mut i = start + 1;
        while i > 0 {
            i -= 1;
            if self[i] == needle {
                return i;
            }
        }
        BYTES_NPOS
    }

    fn contains_byte(&self, needle: u8) -> bool {
        self.iter().any(|b| *b == needle)
    }

    fn contains_bytes(&self, needle: &[u8]) -> bool {
        self.find_bytes(needle, 0) != BYTES_NPOS
    }

    fn starts_with_byte(&self, byte: u8) -> bool {
        self.first() == Some(&byte)
    }

    fn starts_with_bytes(&self, prefix: &[u8]) -> bool {
        self.as_slice().starts_with(prefix)
    }

    fn ends_with_byte(&self, byte: u8) -> bool {
        self.last() == Some(&byte)
    }

    fn ends_with_bytes(&self, suffix: &[u8]) -> bool {
        self.as_slice().ends_with(suffix)
    }

    fn substr(&self, pos: usize, count: usize) -> Vec<u8> {
        if pos >= self.len() {
            return Vec::new();
        }
        let end = pos.saturating_add(count).min(self.len());
        self[pos..end].to_vec()
    }

    fn front_byte(&self) -> u8 {
        *self.first().expect("Bytes::front_byte: empty")
    }

    fn back_byte(&self) -> u8 {
        *self.last().expect("Bytes::back_byte: empty")
    }
}
