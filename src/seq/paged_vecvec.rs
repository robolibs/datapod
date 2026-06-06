//! Paged ragged 2-D bucketed Pod — wire-identical to [`crate::seq::Vecvec`].
//!
//! The C++ "paged" distinction was an in-memory allocator strategy; on the
//! wire there is no difference. Kept as a separate type for API symmetry
//! with the old port — if you don't need the symbol, use `Vecvec`.

use crate::{DataPodValidate, WireError};

#[datapod::datapod]
#[dp(manual_access)]
pub struct PagedVecvec {
    pub element_size: u32,
    pub _pad: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Default for PagedVecvec {
    fn default() -> Self {
        let mut data = Vec::new();
        data.extend_from_slice(&0u32.to_le_bytes());
        data.extend_from_slice(&0u32.to_le_bytes());
        Self {
            element_size: 0,
            _pad: 0,
            data,
        }
    }
}

impl PagedVecvec {
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self::try_new::<T>().unwrap_or_default()
    }

    pub fn try_new<T: bytemuck::Pod>() -> Result<Self, WireError> {
        let vecvec = Self {
            element_size: super::checked_pod_element_size::<Self, T>()?,
            ..Self::default()
        };
        <Self as DataPodValidate>::validate_wire_parts(
            &PagedVecvecHeader {
                element_size: vecvec.element_size,
                _pad: vecvec._pad,
            },
            &vecvec.data,
        )?;
        Ok(vecvec)
    }

    fn bucket_count(&self) -> u32 {
        self.try_bucket_count().unwrap_or(0)
    }

    fn try_bucket_count(&self) -> Result<u32, WireError> {
        read_u32_le::<Self>(&self.data, 0)
    }

    fn header_bytes(&self) -> usize {
        self.try_header_bytes().unwrap_or(0)
    }

    fn try_header_bytes(&self) -> Result<usize, WireError> {
        let count = self.try_bucket_count_usize()?;
        count
            .checked_add(1)
            .and_then(|slots| slots.checked_mul(4))
            .and_then(|offsets| offsets.checked_add(4))
            .ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("paged_vecvec header size overflowed")
            })
    }

    fn offset(&self, i: usize) -> u32 {
        self.try_offset(i).unwrap_or(0)
    }

    fn try_offset(&self, i: usize) -> Result<u32, WireError> {
        let start = i
            .checked_mul(4)
            .and_then(|offset| offset.checked_add(4))
            .ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("paged_vecvec offset index overflowed")
            })?;
        read_u32_le::<Self>(&self.data, start)
    }

    fn try_offset_usize(&self, i: usize) -> Result<usize, WireError> {
        super::checked_u32_to_usize::<Self>(self.try_offset(i)?, "bucket offset")
    }

    fn try_bucket_count_usize(&self) -> Result<usize, WireError> {
        super::checked_u32_to_usize::<Self>(self.try_bucket_count()?, "bucket_count")
    }

    fn try_element_size(&self) -> Result<usize, WireError> {
        super::checked_u32_to_usize::<Self>(self.element_size, "element_size")
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    pub fn try_size(&self) -> Result<usize, WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &PagedVecvecHeader {
                element_size: self.element_size,
                _pad: self._pad,
            },
            &self.data,
        )?;
        self.try_bucket_count_usize()
    }

    pub fn empty(&self) -> bool {
        self.try_empty().unwrap_or(true)
    }

    pub fn try_empty(&self) -> Result<bool, WireError> {
        Ok(self.try_size()? == 0)
    }

    pub fn bucket<T: bytemuck::Pod>(&self, i: usize) -> &[T] {
        self.try_bucket::<T>(i).unwrap_or(&[])
    }

    pub fn try_bucket<T: bytemuck::Pod>(&self, i: usize) -> Result<&[T], WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        <Self as DataPodValidate>::validate_wire_parts(
            &PagedVecvecHeader {
                element_size: self.element_size,
                _pad: self._pad,
            },
            &self.data,
        )?;
        let size = self.try_size()?;
        if i >= size {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "bucket index out of bounds: {i} for len {size}",
            )));
        }
        let header = self.try_header_bytes()?;
        let start = header
            .checked_add(self.try_offset_usize(i)?)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("bucket start overflowed"))?;
        let end = header
            .checked_add(self.try_offset_usize(i + 1)?)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("bucket end overflowed"))?;
        let bytes = self
            .data
            .get(start..end)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("bucket range is out of bounds"))?;
        bytemuck::try_cast_slice(bytes).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "bucket is not aligned for typed view: {err}"
            ))
        })
    }

    pub fn bucket_size(&self, i: usize) -> usize {
        self.try_bucket_size(i).unwrap_or(0)
    }

    pub fn try_bucket_size(&self, i: usize) -> Result<usize, WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &PagedVecvecHeader {
                element_size: self.element_size,
                _pad: self._pad,
            },
            &self.data,
        )?;
        let size = self.try_size()?;
        if i >= size {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "bucket index out of bounds: {i} for len {size}",
            )));
        }
        if self.element_size == 0 {
            return Ok(0);
        }
        let element_size = self.try_element_size()?;
        let next = i
            .checked_add(1)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("bucket next index overflowed"))?;
        let start = self.try_offset(i)?;
        let end = self.try_offset(next)?;
        let span = end
            .checked_sub(start)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("bucket span underflowed"))?;
        let span = super::checked_u32_to_usize::<Self>(span, "bucket span")?;
        Ok(span / element_size)
    }

    pub fn push_bucket<T: bytemuck::Pod>(&mut self, bucket: &[T]) {
        let _ = self.try_push_bucket(bucket);
    }

    pub fn try_push_bucket<T: bytemuck::Pod>(&mut self, bucket: &[T]) -> Result<(), WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.try_push_bucket_bytes(bytemuck::cast_slice(bucket))
    }

    pub fn push_bucket_bytes(&mut self, bytes: &[u8]) {
        let _ = self.try_push_bucket_bytes(bytes);
    }

    pub fn try_push_bucket_bytes(&mut self, bytes: &[u8]) -> Result<(), WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &PagedVecvecHeader {
                element_size: self.element_size,
                _pad: self._pad,
            },
            &self.data,
        )?;
        let element_size = self.try_element_size()?;
        if element_size != 0 && bytes.len() % element_size != 0 {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "bucket byte length {} is not a multiple of element_size {}",
                bytes.len(),
                self.element_size
            )));
        }
        let count = self.try_bucket_count()?;
        let new_count = count
            .checked_add(1)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("bucket count overflowed u32"))?;
        let count_usize = super::checked_u32_to_usize::<Self>(count, "bucket_count")?;
        let end_off = self.try_offset(count_usize)?;
        let bytes_len = u32::try_from(bytes.len()).map_err(|_| {
            crate::wire::invalid_payload::<Self>("bucket byte length exceeds u32 wire offset range")
        })?;
        let new_end_off = end_off.checked_add(bytes_len).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("bucket byte offsets overflowed u32")
        })?;
        let mut new_offsets = Vec::new();
        let offset_capacity = count_usize
            .checked_add(2)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("offset count overflowed"))?;
        new_offsets
            .try_reserve_exact(offset_capacity)
            .map_err(|err| {
                crate::wire::invalid_payload::<Self>(format!(
                    "failed to reserve paged_vecvec offsets: {err}"
                ))
            })?;
        for i in 0..=count_usize {
            new_offsets.push(self.try_offset(i)?);
        }
        new_offsets.push(new_end_off);

        let old_header = self.try_header_bytes()?;
        let Some(payload) = self.data.get(old_header..) else {
            return Err(crate::wire::invalid_payload::<Self>(
                "paged_vecvec payload start is out of bounds",
            ));
        };
        let offset_bytes = new_offsets
            .len()
            .checked_mul(4)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("offset table size overflowed"))?;
        let new_len = 4usize
            .checked_add(offset_bytes)
            .and_then(|len| len.checked_add(payload.len()))
            .and_then(|len| len.checked_add(bytes.len()))
            .ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("rewritten paged_vecvec size overflowed")
            })?;

        let mut new_data = Vec::new();
        new_data.try_reserve_exact(new_len).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "failed to reserve rewritten paged_vecvec payload: {err}"
            ))
        })?;
        new_data.extend_from_slice(&new_count.to_le_bytes());
        for o in &new_offsets {
            new_data.extend_from_slice(&o.to_le_bytes());
        }
        new_data.extend_from_slice(payload);
        new_data.extend_from_slice(bytes);
        self.data = new_data;
        Ok(())
    }
}

fn read_u32_le<P: 'static>(bytes: &[u8], offset: usize) -> Result<u32, WireError> {
    let end = offset
        .checked_add(4)
        .ok_or_else(|| crate::wire::invalid_payload::<P>("u32 offset overflowed"))?;
    let Some(raw) = bytes.get(offset..end) else {
        return Err(crate::wire::invalid_payload::<P>(format!(
            "need 4 bytes at offset {offset}, payload has {} bytes",
            bytes.len()
        )));
    };
    let mut array = [0u8; 4];
    array.copy_from_slice(raw);
    Ok(u32::from_le_bytes(array))
}

fn check_element_size<P: 'static, T>(stored: u32) -> Result<(), WireError> {
    let actual = std::mem::size_of::<T>();
    if actual == 0 {
        return Err(crate::wire::invalid_header::<P>(
            "zero-sized Pod elements cannot be represented in byte-counted datapod containers",
        ));
    }
    let stored = super::checked_u32_to_usize::<P>(stored, "element_size")?;
    if actual != stored {
        return Err(crate::wire::invalid_header::<P>(format!(
            "element size mismatch: T is {actual}, container expects {stored}"
        )));
    }
    Ok(())
}
