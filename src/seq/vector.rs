//! General-purpose sequence Pod — holds any `T: bytemuck::Pod`.
//!
//! Payload is `Vec<u8>` interpreted as `[T; N]` where `N = data.len() /
//! element_size`. The container is type-erased on the wire (just bytes
//! and a width); the user picks `T` at access time and we runtime-check
//! that `size_of::<T>()` matches the stored element_size.

use crate::seq::assert_element_size;

#[datapod::datapod]
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
