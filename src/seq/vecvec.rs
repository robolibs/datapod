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

impl Vecvec {
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self::try_new::<T>().unwrap_or_default()
    }

    pub fn try_new<T: bytemuck::Pod>() -> Result<Self, WireError> {
        let vecvec = Self {
            element_size: super::checked_pod_element_size::<Self, T>()?,
            ..Self::default()
        };
        <Self as DataPodValidate>::validate_wire_parts(
            &VecvecHeader {
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
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("vecvec header size overflowed"))
    }

    fn offset(&self, i: usize) -> u32 {
        self.try_offset(i).unwrap_or(0)
    }

    fn try_offset(&self, i: usize) -> Result<u32, WireError> {
        let start = i
            .checked_mul(4)
            .and_then(|offset| offset.checked_add(4))
            .ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("vecvec offset index overflowed")
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
            &VecvecHeader {
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

    /// Bytes of bucket `i`, viewed as a slice of `T`. `T`'s size must
    /// match `self.element_size`.
    pub fn bucket<T: bytemuck::Pod>(&self, i: usize) -> &[T] {
        self.try_bucket::<T>(i).unwrap_or(&[])
    }

    /// Fallible variant of [`Self::bucket`] that reports malformed owned
    /// state, out-of-bounds indexes, element-size mismatches, and alignment
    /// failures instead of relying on debug assertions or panics.
    pub fn try_bucket<T: bytemuck::Pod>(&self, i: usize) -> Result<&[T], WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        <Self as DataPodValidate>::validate_wire_parts(
            &VecvecHeader {
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
            &VecvecHeader {
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

    /// Append a new bucket from a slice of `T`.
    pub fn push_bucket<T: bytemuck::Pod>(&mut self, bucket: &[T]) {
        let _ = self.try_push_bucket(bucket);
    }

    /// Fallible variant of [`Self::push_bucket`] that refuses to mutate if
    /// the current owned value is malformed or the append would overflow the
    /// wire offset/count fields.
    pub fn try_push_bucket<T: bytemuck::Pod>(&mut self, bucket: &[T]) -> Result<(), WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.try_push_bucket_bytes(bytemuck::cast_slice(bucket))
    }

    /// Append a new bucket from raw bytes. `bytes.len()` must be a
    /// multiple of `element_size`.
    pub fn push_bucket_bytes(&mut self, bytes: &[u8]) {
        let _ = self.try_push_bucket_bytes(bytes);
    }

    /// Fallible byte-bucket append. This is the production-safe path: it
    /// validates the existing owned state first, checks byte alignment, and
    /// rejects offset/count overflow before rewriting the backing buffer.
    pub fn try_push_bucket_bytes(&mut self, bytes: &[u8]) -> Result<(), WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &VecvecHeader {
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
                    "failed to reserve vecvec offsets: {err}"
                ))
            })?;
        for i in 0..=count_usize {
            new_offsets.push(self.try_offset(i)?);
        }
        new_offsets.push(new_end_off);

        let old_header = self.try_header_bytes()?;
        let Some(payload) = self.data.get(old_header..) else {
            return Err(crate::wire::invalid_payload::<Self>(
                "vecvec payload start is out of bounds",
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
                crate::wire::invalid_payload::<Self>("rewritten vecvec size overflowed")
            })?;

        let mut new_data = Vec::new();
        new_data.try_reserve_exact(new_len).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "failed to reserve rewritten vecvec payload: {err}"
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
        self.try_bucket_count().unwrap_or(0)
    }

    pub fn try_bucket_count(&self) -> Result<u32, WireError> {
        Vecvec::validate_wire_parts(&self.header, self.data)?;
        read_u32_le::<Vecvec>(self.data, 0)
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    pub fn try_size(&self) -> Result<usize, WireError> {
        Vecvec::validate_wire_parts(&self.header, self.data)?;
        self.try_bucket_count_usize()
    }

    pub fn header_bytes(&self) -> usize {
        self.try_header_bytes().unwrap_or(0)
    }

    pub fn try_header_bytes(&self) -> Result<usize, WireError> {
        let size = self.try_size()?;
        size.checked_add(1)
            .and_then(|slots| slots.checked_mul(4))
            .and_then(|offsets| offsets.checked_add(4))
            .ok_or_else(|| crate::wire::invalid_payload::<Vecvec>("header length overflowed"))
    }

    pub fn bucket_bytes(&self, index: usize) -> Result<&'a [u8], WireError> {
        Vecvec::validate_wire_parts(&self.header, self.data)?;
        let size = self.try_size()?;
        if index >= size {
            return Err(crate::wire::invalid_header::<Vecvec>(format!(
                "bucket index out of bounds: {index} for len {size}",
            )));
        }
        let payload_start = self.try_header_bytes()?;
        let start = payload_start
            .checked_add(self.try_offset_usize(index)?)
            .ok_or_else(|| crate::wire::invalid_payload::<Vecvec>("bucket start overflowed"))?;
        let end = payload_start
            .checked_add(self.try_offset_usize(index + 1)?)
            .ok_or_else(|| crate::wire::invalid_payload::<Vecvec>("bucket end overflowed"))?;
        self.data
            .get(start..end)
            .ok_or_else(|| crate::wire::invalid_payload::<Vecvec>("bucket range out of bounds"))
    }

    pub fn bucket_unaligned<T: bytemuck::Pod + Copy>(
        &self,
        index: usize,
    ) -> Result<Vec<T>, WireError> {
        check_element_size::<Vecvec, T>(self.header.element_size)?;
        let bytes = self.bucket_bytes(index)?;
        let elem_size = core::mem::size_of::<T>();
        let count = bytes.len() / elem_size;
        let mut values = Vec::new();
        values.try_reserve_exact(count).map_err(|err| {
            crate::wire::invalid_payload::<Vecvec>(format!(
                "failed to reserve {count} unaligned bucket elements: {err}"
            ))
        })?;
        values.extend(
            bytes
                .chunks_exact(elem_size)
                .map(bytemuck::pod_read_unaligned::<T>),
        );
        Ok(values)
    }

    fn try_offset(&self, index: usize) -> Result<u32, WireError> {
        let offset = index
            .checked_mul(4)
            .and_then(|byte| byte.checked_add(4))
            .ok_or_else(|| crate::wire::invalid_payload::<Vecvec>("offset index overflowed"))?;
        read_u32_le::<Vecvec>(self.data, offset)
    }

    fn try_offset_usize(&self, index: usize) -> Result<usize, WireError> {
        super::checked_u32_to_usize::<Vecvec>(self.try_offset(index)?, "bucket offset")
    }

    fn try_bucket_count_usize(&self) -> Result<usize, WireError> {
        super::checked_u32_to_usize::<Vecvec>(self.try_bucket_count()?, "bucket_count")
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
        let bucket_count =
            super::checked_u32_to_usize::<Self>(read_u32_le::<Self>(payload, 0)?, "bucket_count")?;
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
        let data_len = payload
            .len()
            .checked_sub(header_bytes)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("data length underflowed"))?;
        let mut previous = None;
        for index in 0..=bucket_count {
            let offset_start = index
                .checked_mul(4)
                .and_then(|offset| 4usize.checked_add(offset))
                .ok_or_else(|| crate::wire::invalid_payload::<Self>("offset slot overflowed"))?;
            let offset = super::checked_u32_to_usize::<Self>(
                read_u32_le::<Self>(payload, offset_start)?,
                "offset",
            )?;
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
        let element_size =
            super::checked_u32_to_usize::<Self>(header.element_size, "element_size")?;
        for index in 0..bucket_count {
            let start_offset = index
                .checked_mul(4)
                .and_then(|offset| 4usize.checked_add(offset))
                .ok_or_else(|| {
                    crate::wire::invalid_payload::<Self>("bucket start offset overflowed")
                })?;
            let end_offset = index
                .checked_add(1)
                .and_then(|next| next.checked_mul(4))
                .and_then(|offset| 4usize.checked_add(offset))
                .ok_or_else(|| {
                    crate::wire::invalid_payload::<Self>("bucket end offset overflowed")
                })?;
            let start = super::checked_u32_to_usize::<Self>(
                read_u32_le::<Self>(payload, start_offset)?,
                "start",
            )?;
            let end = super::checked_u32_to_usize::<Self>(
                read_u32_le::<Self>(payload, end_offset)?,
                "end",
            )?;
            let bucket_len = end
                .checked_sub(start)
                .ok_or_else(|| crate::wire::invalid_payload::<Self>("bucket length underflowed"))?;
            if bucket_len % element_size != 0 {
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
