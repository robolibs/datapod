//! General-purpose sequence Pod — holds any `T: bytemuck::Pod`.
//!
//! Payload is `Vec<u8>` interpreted as `[T; N]` where `N = data.len() /
//! element_size`. The container is type-erased on the wire (just bytes
//! and a width); the user picks `T` at access time and we runtime-check
//! that `size_of::<T>()` matches the stored element_size.

use crate::seq::assert_element_size;
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
        Self {
            element_size: std::mem::size_of::<T>() as u32,
            _pad: 0,
            data: Vec::new(),
        }
    }

    pub fn with_capacity<T: bytemuck::Pod>(element_capacity: usize) -> Self {
        let es = std::mem::size_of::<T>();
        Self {
            element_size: es as u32,
            _pad: 0,
            data: Vec::with_capacity(element_capacity * es),
        }
    }

    /// Construct from already-bytes-encoded payload. The caller asserts
    /// `data.len() % size_of::<T>() == 0`.
    pub fn from_bytes<T: bytemuck::Pod>(data: Vec<u8>) -> Self {
        let es = std::mem::size_of::<T>();
        debug_assert_eq!(
            data.len() % es,
            0,
            "Vector::from_bytes data not aligned to T"
        );
        Self {
            element_size: es as u32,
            _pad: 0,
            data,
        }
    }

    /// Number of logical elements.
    pub fn size(&self) -> usize {
        if self.element_size == 0 {
            0
        } else {
            self.data.len() / self.element_size as usize
        }
    }

    pub fn empty(&self) -> bool {
        self.data.is_empty()
    }

    pub fn clear(&mut self) {
        self.data.clear();
    }

    pub fn as_slice<T: bytemuck::Pod>(&self) -> &[T] {
        assert_element_size::<T>(self.element_size);
        bytemuck::cast_slice(&self.data)
    }

    pub fn as_mut_slice<T: bytemuck::Pod>(&mut self) -> &mut [T] {
        assert_element_size::<T>(self.element_size);
        bytemuck::cast_slice_mut(&mut self.data)
    }

    pub fn get<T: bytemuck::Pod>(&self, i: usize) -> T {
        self.as_slice::<T>()[i]
    }

    pub fn set<T: bytemuck::Pod>(&mut self, i: usize, value: T) {
        self.as_mut_slice::<T>()[i] = value;
    }

    pub fn push<T: bytemuck::Pod>(&mut self, value: T) {
        assert_element_size::<T>(self.element_size);
        self.data.extend_from_slice(bytemuck::bytes_of(&value));
    }

    pub fn pop<T: bytemuck::Pod>(&mut self) -> Option<T> {
        assert_element_size::<T>(self.element_size);
        let es = self.element_size as usize;
        if self.data.len() < es {
            return None;
        }
        let start = self.data.len() - es;
        let value: T = *bytemuck::from_bytes(&self.data[start..]);
        self.data.truncate(start);
        Some(value)
    }
}

/// Borrowed, validation-backed view over a `Vector` wire payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VectorView<'a> {
    pub header: VectorHeader,
    pub data: &'a [u8],
}

impl<'a> VectorView<'a> {
    pub fn element_size(&self) -> u32 {
        self.header.element_size
    }

    pub fn payload_bytes(&self) -> &'a [u8] {
        self.data
    }

    pub fn size(&self) -> usize {
        if self.header.element_size == 0 {
            0
        } else {
            self.data.len() / self.header.element_size as usize
        }
    }

    pub fn get_unaligned<T: bytemuck::Pod + Copy>(&self, index: usize) -> Result<T, WireError> {
        assert_element_size::<T>(self.header.element_size);
        if index >= self.size() {
            return Err(crate::wire::invalid_header::<Vector>(format!(
                "vector index out of bounds: {index} for len {}",
                self.size()
            )));
        }
        let elem_size = core::mem::size_of::<T>();
        let offset = index
            .checked_mul(elem_size)
            .ok_or_else(|| crate::wire::invalid_payload::<Vector>("element offset overflowed"))?;
        let end = offset
            .checked_add(elem_size)
            .ok_or_else(|| crate::wire::invalid_payload::<Vector>("element end overflowed"))?;
        Ok(bytemuck::pod_read_unaligned(&self.data[offset..end]))
    }

    pub fn as_aligned_slice<T: bytemuck::Pod>(&self) -> Result<&'a [T], WireError> {
        assert_element_size::<T>(self.header.element_size);
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
        if payload.len() % header.element_size as usize != 0 {
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
