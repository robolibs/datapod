//! Paged ragged 2-D bucketed Pod — wire-identical to [`crate::seq::Vecvec`].
//!
//! The C++ "paged" distinction was an in-memory allocator strategy; on the
//! wire there is no difference. Kept as a separate type for API symmetry
//! with the old port — if you don't need the symbol, use `Vecvec`.

use crate::seq::assert_element_size;

#[datapod::datapod]
pub struct PagedVecvec {
    pub element_size: u32,
    pub _pad: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Default for PagedVecvec {
    fn default() -> Self {
        let mut data = Vec::with_capacity(8);
        data.extend_from_slice(&0u32.to_le_bytes());
        data.extend_from_slice(&0u32.to_le_bytes());
        Self { element_size: 0, _pad: 0, data }
    }
}

impl PagedVecvec {
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
        (self.offset(i + 1) - self.offset(i)) as usize / self.element_size as usize
    }

    pub fn push_bucket<T: bytemuck::Pod>(&mut self, bucket: &[T]) {
        assert_element_size::<T>(self.element_size);
        self.push_bucket_bytes(bytemuck::cast_slice(bucket));
    }

    pub fn push_bucket_bytes(&mut self, bytes: &[u8]) {
        debug_assert!(
            self.element_size == 0 || bytes.len() % self.element_size as usize == 0,
            "PagedVecvec::push_bucket_bytes length not aligned to element_size"
        );
        let count = self.bucket_count();
        let new_count = count + 1;
        let end_off = self.offset(count as usize);
        let old_header = self.header_bytes();
        let payload = self.data[old_header..].to_vec();

        let mut new_offsets: Vec<u32> = (0..=count as usize).map(|i| self.offset(i)).collect();
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
