//! Double-ended queue Pod — holds any `T: bytemuck::Pod`.
//!
//! Layout: one byte buffer split at `split_byte` into a front half
//! (stored in reverse element order so `push_front` is an O(1) prepend
//! before that point) and a back half (natural order).

use crate::seq::assert_element_size;

#[datapod::datapod]
#[derive(Default)]
pub struct Deque {
    pub element_size: u32,
    /// Byte offset where the back half starts. Bytes `[..split_byte]` are
    /// the front half in reverse element order; `[split_byte..]` are the
    /// back half in natural order.
    pub split_byte: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Deque {
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self {
            element_size: std::mem::size_of::<T>() as u32,
            split_byte: 0,
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
        self.split_byte = 0;
    }

    pub fn push_front<T: bytemuck::Pod>(&mut self, value: T) {
        assert_element_size::<T>(self.element_size);
        let es = self.element_size as usize;
        let bytes = bytemuck::bytes_of(&value).to_vec();
        self.data.splice(0..0, bytes);
        self.split_byte += es as u32;
    }

    pub fn push_back<T: bytemuck::Pod>(&mut self, value: T) {
        assert_element_size::<T>(self.element_size);
        self.data.extend_from_slice(bytemuck::bytes_of(&value));
    }

    pub fn pop_front<T: bytemuck::Pod>(&mut self) -> Option<T> {
        if self.empty() {
            return None;
        }
        assert_element_size::<T>(self.element_size);
        let es = self.element_size as usize;
        if self.split_byte as usize >= es {
            let end = self.split_byte as usize;
            let start = end - es;
            let value: T = *bytemuck::from_bytes(&self.data[start..end]);
            self.data.drain(start..end);
            self.split_byte -= es as u32;
            Some(value)
        } else {
            let start = self.split_byte as usize;
            let end = start + es;
            let value: T = *bytemuck::from_bytes(&self.data[start..end]);
            self.data.drain(start..end);
            Some(value)
        }
    }

    pub fn pop_back<T: bytemuck::Pod>(&mut self) -> Option<T> {
        if self.empty() {
            return None;
        }
        assert_element_size::<T>(self.element_size);
        let es = self.element_size as usize;
        if self.data.len() > self.split_byte as usize {
            let end = self.data.len();
            let start = end - es;
            let value: T = *bytemuck::from_bytes(&self.data[start..end]);
            self.data.truncate(start);
            Some(value)
        } else {
            let value: T = *bytemuck::from_bytes(&self.data[..es]);
            self.data.drain(..es);
            self.split_byte -= es as u32;
            Some(value)
        }
    }
}
