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
use crate::{DataPodAccess, DataPodValidate, WireError};

#[datapod::datapod]
#[dp(manual_access)]
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
        Self {
            element_size: 0,
            _pad: 0,
            data,
        }
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

/// Borrowed, validation-backed view over a ragged `Vecvec` payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VecvecView<'a> {
    pub header: VecvecHeader,
    pub data: &'a [u8],
}

impl<'a> VecvecView<'a> {
    pub fn element_size(&self) -> u32 {
        self.header.element_size
    }

    pub fn payload_bytes(&self) -> &'a [u8] {
        self.data
    }

    pub fn bucket_count(&self) -> u32 {
        read_u32(self.data, 0)
    }

    pub fn size(&self) -> usize {
        self.bucket_count() as usize
    }

    pub fn header_bytes(&self) -> usize {
        4 + (self.size() + 1) * 4
    }

    pub fn bucket_bytes(&self, index: usize) -> Result<&'a [u8], WireError> {
        if index >= self.size() {
            return Err(crate::wire::invalid_header::<Vecvec>(format!(
                "bucket index out of bounds: {index} for len {}",
                self.size()
            )));
        }
        let payload_start = self.header_bytes();
        let start = payload_start + self.offset(index) as usize;
        let end = payload_start + self.offset(index + 1) as usize;
        Ok(&self.data[start..end])
    }

    pub fn bucket_unaligned<T: bytemuck::Pod + Copy>(
        &self,
        index: usize,
    ) -> Result<Vec<T>, WireError> {
        assert_element_size::<T>(self.header.element_size);
        let bytes = self.bucket_bytes(index)?;
        Ok(bytes
            .chunks_exact(core::mem::size_of::<T>())
            .map(bytemuck::pod_read_unaligned::<T>)
            .collect())
    }

    fn offset(&self, index: usize) -> u32 {
        read_u32(self.data, 4 + index * 4)
    }
}

impl DataPodValidate for Vecvec {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        if header._pad != 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "reserved _pad field must be zero",
            ));
        }
        if payload.len() < 8 {
            return Err(crate::wire::invalid_payload::<Self>(
                "payload must contain bucket count and at least one offset",
            ));
        }
        let bucket_count = read_u32(payload, 0) as usize;
        let header_bytes = 4usize
            .checked_add(
                bucket_count
                    .checked_add(1)
                    .and_then(|count| count.checked_mul(4))
                    .ok_or_else(|| {
                        crate::wire::invalid_payload::<Self>("offset table length overflowed")
                    })?,
            )
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("header length overflowed"))?;
        if payload.len() < header_bytes {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "payload too short for offset table: got {}, need at least {header_bytes}",
                payload.len()
            )));
        }
        let data_len = payload.len() - header_bytes;
        let mut previous = None;
        for index in 0..=bucket_count {
            let offset = read_u32(payload, 4 + index * 4) as usize;
            if offset > data_len {
                return Err(crate::wire::invalid_payload::<Self>(format!(
                    "bucket offset {offset} exceeds data length {data_len}"
                )));
            }
            if let Some(previous) = previous
                && offset < previous
            {
                return Err(crate::wire::invalid_payload::<Self>(
                    "bucket offsets must be non-decreasing",
                ));
            }
            if index == 0 && offset != 0 {
                return Err(crate::wire::invalid_payload::<Self>(
                    "first bucket offset must be zero",
                ));
            }
            previous = Some(offset);
        }
        let last = previous.unwrap_or(0);
        if last != data_len {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "last bucket offset {last} must equal data length {data_len}"
            )));
        }
        if header.element_size == 0 {
            return if data_len == 0 {
                Ok(())
            } else {
                Err(crate::wire::invalid_payload::<Self>(
                    "zero element_size requires empty bucket data",
                ))
            };
        }
        for index in 0..bucket_count {
            let start = read_u32(payload, 4 + index * 4) as usize;
            let end = read_u32(payload, 4 + (index + 1) * 4) as usize;
            if (end - start) % header.element_size as usize != 0 {
                return Err(crate::wire::invalid_payload::<Self>(format!(
                    "bucket {index} byte length is not a multiple of element_size {}",
                    header.element_size
                )));
            }
        }
        Ok(())
    }
}

impl DataPodAccess for Vecvec {
    type View<'a> = VecvecView<'a>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Self::validate_wire_parts(&header, payload)?;
        Ok(VecvecView {
            header,
            data: payload,
        })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Self::View<'a> {
        VecvecView {
            header,
            data: payload,
        }
    }
}

fn read_u32(bytes: &[u8], offset: usize) -> u32 {
    u32::from_le_bytes(bytes[offset..offset + 4].try_into().unwrap())
}
