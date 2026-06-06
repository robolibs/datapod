//! General-purpose sequence Pod — holds any `T: bytemuck::Pod`.
//!
//! Payload is `Vec<u8>` interpreted as `[T; N]` where `N = data.len() /
//! element_size`. The container is type-erased on the wire (just bytes
//! and a width); the user picks `T` at access time and we runtime-check
//! that `size_of::<T>()` matches the stored element_size.

use crate::{DataPodAccess, DataPodValidate, WireError};

#[datapod::datapod]
#[dp(manual_access)]
#[derive(Default)]
pub struct Vector {
    pub element_size: u32,
    pub _pad: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Vector {
    /// Construct an empty vector sized for elements of type `T`.
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self::try_new::<T>().unwrap_or_default()
    }

    /// Fallible constructor that rejects element types too wide for the
    /// stable u32 wire header before truncating their size.
    pub fn try_new<T: bytemuck::Pod>() -> Result<Self, WireError> {
        let vector = Self {
            element_size: super::checked_pod_element_size::<Self, T>()?,
            _pad: 0,
            data: Vec::new(),
        };
        vector.validate_owned()?;
        Ok(vector)
    }

    pub fn with_capacity<T: bytemuck::Pod>(element_capacity: usize) -> Self {
        Self::try_with_capacity::<T>(element_capacity).unwrap_or_else(|_| Self::new::<T>())
    }

    pub fn try_with_capacity<T: bytemuck::Pod>(element_capacity: usize) -> Result<Self, WireError> {
        let es = std::mem::size_of::<T>();
        let element_size = super::checked_pod_element_size::<Self, T>()?;
        let byte_capacity = element_capacity.checked_mul(es).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("vector byte capacity overflowed")
        })?;
        let mut data = Vec::new();
        data.try_reserve_exact(byte_capacity).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!("vector payload allocation failed: {err}"))
        })?;
        let vector = Self {
            element_size,
            _pad: 0,
            data,
        };
        vector.validate_owned()?;
        Ok(vector)
    }

    /// Construct from already-bytes-encoded payload. Invalid byte lengths
    /// fall back to an empty vector; use [`Self::try_from_bytes`] to get the
    /// validation error.
    pub fn from_bytes<T: bytemuck::Pod>(data: Vec<u8>) -> Self {
        Self::try_from_bytes::<T>(data).unwrap_or_else(|_| Self::new::<T>())
    }

    pub fn try_from_bytes<T: bytemuck::Pod>(data: Vec<u8>) -> Result<Self, WireError> {
        let es = std::mem::size_of::<T>();
        let element_size = super::checked_pod_element_size::<Self, T>()?;
        if es == 0 && !data.is_empty() {
            return Err(crate::wire::invalid_payload::<Self>(
                "zero element_size requires empty payload",
            ));
        }
        if es != 0 && data.len() % es != 0 {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "{} bytes is not a multiple of element_size {es}",
                data.len()
            )));
        }
        let vector = Self {
            element_size,
            _pad: 0,
            data,
        };
        vector.validate_owned()?;
        Ok(vector)
    }

    /// Number of logical elements.
    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    /// Fallible logical element count for callers handling potentially
    /// malformed owned buffers.
    pub fn try_size(&self) -> Result<usize, WireError> {
        self.validate_owned()?;
        let element_size = self.try_element_size()?;
        if element_size == 0 {
            Ok(0)
        } else {
            Ok(self.data.len() / element_size)
        }
    }

    pub fn empty(&self) -> bool {
        self.try_empty().unwrap_or(true)
    }

    /// Fallible emptiness check for callers handling potentially malformed
    /// owned buffers.
    pub fn try_empty(&self) -> Result<bool, WireError> {
        Ok(self.try_size()? == 0)
    }

    pub fn clear(&mut self) {
        self.data.clear();
    }

    pub fn as_slice<T: bytemuck::Pod>(&self) -> &[T] {
        self.try_as_slice::<T>().unwrap_or(&[])
    }

    pub fn try_as_slice<T: bytemuck::Pod>(&self) -> Result<&[T], WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        bytemuck::try_cast_slice(&self.data)
            .map_err(|error| crate::wire::invalid_payload::<Self>(error.to_string()))
    }

    pub fn as_mut_slice<T: bytemuck::Pod>(&mut self) -> &mut [T] {
        self.try_as_mut_slice::<T>().unwrap_or(&mut [])
    }

    pub fn try_as_mut_slice<T: bytemuck::Pod>(&mut self) -> Result<&mut [T], WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        bytemuck::try_cast_slice_mut(&mut self.data)
            .map_err(|error| crate::wire::invalid_payload::<Self>(error.to_string()))
    }

    pub fn get<T: bytemuck::Pod>(&self, i: usize) -> T {
        self.try_get::<T>(i)
            .unwrap_or_else(|_| bytemuck::Zeroable::zeroed())
    }

    pub fn try_get<T: bytemuck::Pod>(&self, i: usize) -> Result<T, WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        let size = self.try_size()?;
        if i >= size {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "vector index out of bounds: {i} for len {size}",
            )));
        }
        let es = core::mem::size_of::<T>();
        let start = i
            .checked_mul(es)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("element offset overflowed"))?;
        let end = start
            .checked_add(es)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("element end overflowed"))?;
        let bytes = self.data.get(start..end).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("element range is out of bounds")
        })?;
        Ok(bytemuck::pod_read_unaligned(bytes))
    }

    pub fn set<T: bytemuck::Pod>(&mut self, i: usize, value: T) {
        let _ = self.try_set(i, value);
    }

    pub fn try_set<T: bytemuck::Pod>(&mut self, i: usize, value: T) -> Result<(), WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        let size = self.try_size()?;
        if i >= size {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "vector index out of bounds: {i} for len {size}",
            )));
        }
        let es = core::mem::size_of::<T>();
        let start = i
            .checked_mul(es)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("element offset overflowed"))?;
        let end = start
            .checked_add(es)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("element end overflowed"))?;
        let slot = self.data.get_mut(start..end).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("element range is out of bounds")
        })?;
        slot.copy_from_slice(bytemuck::bytes_of(&value));
        Ok(())
    }

    pub fn push<T: bytemuck::Pod>(&mut self, value: T) {
        let _ = self.try_push(value);
    }

    pub fn try_push<T: bytemuck::Pod>(&mut self, value: T) -> Result<(), WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        let bytes = bytemuck::bytes_of(&value);
        self.data.try_reserve_exact(bytes.len()).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!("vector payload allocation failed: {err}"))
        })?;
        self.data.extend_from_slice(bytemuck::bytes_of(&value));
        Ok(())
    }

    pub fn pop<T: bytemuck::Pod>(&mut self) -> Option<T> {
        self.try_pop().unwrap_or(None)
    }

    pub fn try_pop<T: bytemuck::Pod>(&mut self) -> Result<Option<T>, WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        let es = self.try_element_size()?;
        if self.data.len() < es {
            return Ok(None);
        }
        let start = self.data.len().checked_sub(es).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("last element offset underflowed")
        })?;
        let bytes = self.data.get(start..).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("last element range is out of bounds")
        })?;
        let value: T = bytemuck::pod_read_unaligned(bytes);
        self.data.truncate(start);
        Ok(Some(value))
    }

    fn validate_owned(&self) -> Result<(), WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &VectorHeader {
                element_size: self.element_size,
                _pad: self._pad,
            },
            &self.data,
        )
    }

    fn try_element_size(&self) -> Result<usize, WireError> {
        super::checked_u32_to_usize::<Self>(self.element_size, "element_size")
    }
}

/// Borrowed, validation-backed view over a `Vector` wire payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VectorView<'a> {
    pub header: VectorHeader,
    pub data: &'a [u8],
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

impl<'a> VectorView<'a> {
    pub fn element_size(&self) -> u32 {
        self.header.element_size
    }

    pub fn payload_bytes(&self) -> &'a [u8] {
        self.data
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    pub fn try_size(&self) -> Result<usize, WireError> {
        Vector::validate_wire_parts(&self.header, self.data)?;
        let element_size =
            super::checked_u32_to_usize::<Vector>(self.header.element_size, "element_size")?;
        if element_size == 0 {
            Ok(0)
        } else {
            Ok(self.data.len() / element_size)
        }
    }

    pub fn get_unaligned<T: bytemuck::Pod + Copy>(&self, index: usize) -> Result<T, WireError> {
        check_element_size::<Vector, T>(self.header.element_size)?;
        let size = self.try_size()?;
        if index >= size {
            return Err(crate::wire::invalid_header::<Vector>(format!(
                "vector index out of bounds: {index} for len {size}",
            )));
        }
        let elem_size = core::mem::size_of::<T>();
        let offset = index
            .checked_mul(elem_size)
            .ok_or_else(|| crate::wire::invalid_payload::<Vector>("element offset overflowed"))?;
        let end = offset
            .checked_add(elem_size)
            .ok_or_else(|| crate::wire::invalid_payload::<Vector>("element end overflowed"))?;
        let bytes = self.data.get(offset..end).ok_or_else(|| {
            crate::wire::invalid_payload::<Vector>("element range is out of bounds")
        })?;
        Ok(bytemuck::pod_read_unaligned(bytes))
    }

    pub fn as_aligned_slice<T: bytemuck::Pod>(&self) -> Result<&'a [T], WireError> {
        check_element_size::<Vector, T>(self.header.element_size)?;
        Vector::validate_wire_parts(&self.header, self.data)?;
        bytemuck::try_cast_slice(self.data)
            .map_err(|error| crate::wire::invalid_payload::<Vector>(error.to_string()))
    }
}

impl DataPodValidate for Vector {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        if header._pad != 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "reserved _pad field must be zero",
            ));
        }
        if header.element_size == 0 {
            return if payload.is_empty() {
                Ok(())
            } else {
                Err(crate::wire::invalid_payload::<Self>(
                    "zero element_size requires empty payload",
                ))
            };
        }
        let element_size =
            super::checked_u32_to_usize::<Self>(header.element_size, "element_size")?;
        if payload.len() % element_size != 0 {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "{} bytes is not a multiple of element_size {}",
                payload.len(),
                header.element_size
            )));
        }
        Ok(())
    }
}

impl DataPodAccess for Vector {
    type View<'a> = VectorView<'a>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Self::validate_wire_parts(&header, payload)?;
        Ok(VectorView {
            header,
            data: payload,
        })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Self::View<'a> {
        VectorView {
            header,
            data: payload,
        }
    }
}
