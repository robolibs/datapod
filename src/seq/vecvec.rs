//! Ragged 2-D bucketed Pod — holds any `T: bytemuck::Pod`.
//!
//! Wire layout (single `#[dp(bytes)]` payload):
//!
//! ```text
//! [bucket_count : u32 LE]
//! [offset_0 : u32 LE] [offset_1 : u32 LE] ... [offset_N : u32 LE]
//! [bucket_0 bytes] [bucket_1 bytes] ... [bucket_{N-1} bytes]
//! ```
//!
//! Bucket `i` occupies bytes `[offset_i, offset_{i+1})` of the data
//! region. Element width is captured in the struct header as
//! `element_size: u32`.

use crate::seq::assert_element_size;

#[datapod::datapod]
pub struct Vecvec {
    pub element_size: u32,
    pub _pad: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Default for Vecvec {
    fn default() -> Self {
        // Bootstrap with a 0-bucket header: [count=0 : u32][off_0=0 : u32].
        let mut data = Vec::with_capacity(8);
        data.extend_from_slice(&0u32.to_le_bytes());
        data.extend_from_slice(&0u32.to_le_bytes());
        Self { element_size: 0, _pad: 0, data }
    }
}

impl Vecvec {
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self {
            element_size: std::mem::size_of::<T>() as u32,
            ..Self::default()
        }
    }

    fn bucket_count(&self) -> u32 {
        u32::from_le_bytes(self.data[0..4].try_into().unwrap())
    }

    fn header_bytes(&self) -> usize {
        4 + (self.bucket_count() as usize + 1) * 4
    }

    fn offset(&self, i: usize) -> u32 {
        let start = 4 + i * 4;
        u32::from_le_bytes(self.data[start..start + 4].try_into().unwrap())
    }

    pub fn size(&self) -> usize {
        self.bucket_count() as usize
    }

    pub fn empty(&self) -> bool {
        self.size() == 0
    }

    /// Bytes of bucket `i`, viewed as a slice of `T`. `T`'s size must
    /// match `self.element_size`.
    pub fn bucket<T: bytemuck::Pod>(&self, i: usize) -> &[T] {
        assert_element_size::<T>(self.element_size);
        debug_assert!(i < self.size());
        let header = self.header_bytes();
        let start = header + self.offset(i) as usize;
        let end = header + self.offset(i + 1) as usize;
        bytemuck::cast_slice(&self.data[start..end])
    }

    pub fn bucket_size(&self, i: usize) -> usize {
        if self.element_size == 0 {
            return 0;
        }
        let span = self.offset(i + 1) - self.offset(i);
        span as usize / self.element_size as usize
    }

    /// Append a new bucket from a slice of `T`.
    pub fn push_bucket<T: bytemuck::Pod>(&mut self, bucket: &[T]) {
        assert_element_size::<T>(self.element_size);
        self.push_bucket_bytes(bytemuck::cast_slice(bucket));
    }

    /// Append a new bucket from raw bytes. `bytes.len()` must be a
    /// multiple of `element_size`.
    pub fn push_bucket_bytes(&mut self, bytes: &[u8]) {
        debug_assert!(
            self.element_size == 0 || bytes.len() % self.element_size as usize == 0,
            "Vecvec::push_bucket_bytes length not aligned to element_size"
        );
        let count = self.bucket_count();
        let new_count = count + 1;
        let end_off = self.offset(count as usize);
        let old_header = self.header_bytes();
        let payload = self.data[old_header..].to_vec();

        let mut new_offsets: Vec<u32> =
            (0..=count as usize).map(|i| self.offset(i)).collect();
        new_offsets.push(end_off + bytes.len() as u32);

        self.data.clear();
        self.data.extend_from_slice(&new_count.to_le_bytes());
        for o in &new_offsets {
            self.data.extend_from_slice(&o.to_le_bytes());
        }
        self.data.extend_from_slice(&payload);
        self.data.extend_from_slice(bytes);
    }
}
