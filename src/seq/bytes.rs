//! Raw byte buffer Pod.
//!
//! Mirrors the wire shape of `datapod::Bytes` from the C++ library, but in
//! POD form: the bytes themselves ride on the slice payload, no metadata.

/// Sentinel returned by `find` / `rfind` when no match is present.
pub const BYTES_NPOS: usize = usize::MAX;

/// Heap-bearing byte buffer. `data` is the wire payload, no header fields.
#[datapod::datapod]
#[derive(Default)]
pub struct Bytes {
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Bytes {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn from_slice(slice: &[u8]) -> Self {
        Self { data: slice.to_vec() }
    }

    pub fn size(&self) -> usize {
        self.data.len()
    }

    pub fn empty(&self) -> bool {
        self.data.is_empty()
    }

    pub fn as_slice(&self) -> &[u8] {
        &self.data
    }

    pub fn as_mut_slice(&mut self) -> &mut [u8] {
        &mut self.data
    }

    pub fn append(&mut self, slice: &[u8]) {
        self.data.extend_from_slice(slice);
    }

    pub fn fill_byte(&mut self, value: u8) {
        for b in self.data.iter_mut() {
            *b = value;
        }
    }

    pub fn zero(&mut self) {
        self.fill_byte(0);
    }

    pub fn find(&self, needle: u8, from: usize) -> usize {
        if from >= self.data.len() {
            return BYTES_NPOS;
        }
        self.data[from..]
            .iter()
            .position(|b| *b == needle)
            .map(|i| i + from)
            .unwrap_or(BYTES_NPOS)
    }

    pub fn rfind(&self, needle: u8, from: usize) -> usize {
        let upper = from.min(self.data.len().saturating_sub(1));
        for i in (0..=upper).rev() {
            if self.data[i] == needle {
                return i;
            }
        }
        BYTES_NPOS
    }

    pub fn contains(&self, needle: u8) -> bool {
        self.data.contains(&needle)
    }

    pub fn starts_with(&self, prefix: &[u8]) -> bool {
        self.data.starts_with(prefix)
    }

    pub fn ends_with(&self, suffix: &[u8]) -> bool {
        self.data.ends_with(suffix)
    }

    pub fn substr(&self, pos: usize, count: usize) -> Vec<u8> {
        if pos >= self.data.len() {
            return Vec::new();
        }
        let end = (pos + count).min(self.data.len());
        self.data[pos..end].to_vec()
    }
}
