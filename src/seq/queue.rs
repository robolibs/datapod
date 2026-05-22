//! FIFO queue Pod — holds any `T: bytemuck::Pod`.
//!
//! Stored as a contiguous element buffer + a logical `front` index that
//! tracks how many leading slots have been popped. The buffer compacts
//! lazily when the front pointer crosses the half-way mark.

use crate::seq::assert_element_size;

/// FIFO ordering alias.
pub type Fifo = Queue;

#[datapod::datapod]
#[derive(Default)]
pub struct Queue {
    pub element_size: u32,
    /// Number of leading elements that have been logically popped.
    pub front: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Queue {
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self {
            element_size: std::mem::size_of::<T>() as u32,
            front: 0,
            data: Vec::new(),
        }
    }

    fn raw_len(&self) -> usize {
        if self.element_size == 0 { 0 } else { self.data.len() / self.element_size as usize }
    }

    pub fn size(&self) -> usize {
        self.raw_len().saturating_sub(self.front as usize)
    }

    pub fn empty(&self) -> bool {
        self.size() == 0
    }

    pub fn clear(&mut self) {
        self.data.clear();
        self.front = 0;
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
        let byte_start = self.front as usize * es;
        let value: T = *bytemuck::from_bytes(&self.data[byte_start..byte_start + es]);
        self.front += 1;
        self.compact_if_needed();
        Some(value)
    }

    pub fn front_elem<T: bytemuck::Pod>(&self) -> Option<T> {
        if self.empty() {
            return None;
        }
        assert_element_size::<T>(self.element_size);
        let es = self.element_size as usize;
        let byte_start = self.front as usize * es;
        Some(*bytemuck::from_bytes(&self.data[byte_start..byte_start + es]))
    }

    pub fn back_elem<T: bytemuck::Pod>(&self) -> Option<T> {
        if self.empty() {
            return None;
        }
        assert_element_size::<T>(self.element_size);
        let es = self.element_size as usize;
        let byte_end = self.data.len();
        Some(*bytemuck::from_bytes(&self.data[byte_end - es..byte_end]))
    }

    fn compact_if_needed(&mut self) {
        let raw = self.raw_len();
        if raw > 0 && (self.front as usize) * 2 >= raw {
            let es = self.element_size as usize;
            let drop_bytes = self.front as usize * es;
            self.data.drain(..drop_bytes);
            self.front = 0;
        }
    }
}
