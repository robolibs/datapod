//! LIFO stack Pod — holds any `T: bytemuck::Pod`.
//!
//! Same wire shape as [`crate::seq::Vector`]; API restricted to LIFO ops.

use crate::seq::assert_element_size;

#[datapod::datapod]
#[derive(Default)]
pub struct Stack {
    pub element_size: u32,
    pub _pad: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Stack {
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self {
            element_size: std::mem::size_of::<T>() as u32,
            _pad: 0,
            data: Vec::new(),
        }
    }

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

    pub fn top<T: bytemuck::Pod>(&self) -> Option<T> {
        assert_element_size::<T>(self.element_size);
        let es = self.element_size as usize;
        if self.data.len() < es {
            return None;
        }
        let start = self.data.len() - es;
        Some(*bytemuck::from_bytes(&self.data[start..]))
    }
}
