//! Binary heap Pod — holds any `T: bytemuck::Pod + PartialOrd`.
//!
//! Order tag (max / min) rides in the header. Closure-based comparators
//! aren't wire-shippable, so this is a structural simplification of the
//! old `Heap<T, Compare>`.
//!
//! All comparison-based methods (`push`, `pop`, `sift_*`) take `T` as a
//! type parameter. NaN comparisons collapse to `Equal` for stability.

use crate::seq::assert_element_size;
use std::cmp::Ordering;

/// 0 = max-heap (largest on top), 1 = min-heap (smallest on top).
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HeapOrder {
    Max = 0,
    Min = 1,
}

impl Default for HeapOrder {
    fn default() -> Self {
        Self::Max
    }
}

unsafe impl bytemuck::Zeroable for HeapOrder {}
unsafe impl bytemuck::Pod for HeapOrder {}

#[datapod::datapod]
#[derive(Default)]
pub struct Heap {
    pub element_size: u32,
    pub order: HeapOrder,
    pub _pad: [u8; 3],
    #[dp(bytes)]
    pub data: Vec<u8>,
}

/// Max-heap (default). Largest on top.
pub type MaxHeap = Heap;
/// Priority queue alias.
pub type PriorityQueue = Heap;

impl Heap {
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self {
            element_size: std::mem::size_of::<T>() as u32,
            order: HeapOrder::Max,
            _pad: [0; 3],
            data: Vec::new(),
        }
    }

    pub fn new_min<T: bytemuck::Pod>() -> Self {
        Self {
            element_size: std::mem::size_of::<T>() as u32,
            order: HeapOrder::Min,
            _pad: [0; 3],
            data: Vec::new(),
        }
    }

    pub fn size(&self) -> usize {
        if self.element_size == 0 { 0 } else { self.data.len() / self.element_size as usize }
    }

    pub fn empty(&self) -> bool {
        self.data.is_empty()
    }

    pub fn clear(&mut self) {
        self.data.clear();
    }

    fn typed<T: bytemuck::Pod>(&self) -> &[T] {
        bytemuck::cast_slice(&self.data)
    }

    fn typed_mut<T: bytemuck::Pod>(&mut self) -> &mut [T] {
        bytemuck::cast_slice_mut(&mut self.data)
    }

    fn cmp<T: bytemuck::Pod + PartialOrd>(&self, a: usize, b: usize) -> Ordering {
        let s = self.typed::<T>();
        let raw = s[a].partial_cmp(&s[b]).unwrap_or(Ordering::Equal);
        match self.order {
            HeapOrder::Max => raw,
            HeapOrder::Min => raw.reverse(),
        }
    }

    pub fn push<T: bytemuck::Pod + PartialOrd>(&mut self, value: T) {
        assert_element_size::<T>(self.element_size);
        self.data.extend_from_slice(bytemuck::bytes_of(&value));
        let last = self.typed::<T>().len() - 1;
        self.sift_up::<T>(last);
    }

    pub fn pop<T: bytemuck::Pod + PartialOrd>(&mut self) -> Option<T> {
        assert_element_size::<T>(self.element_size);
        let n = self.typed::<T>().len();
        if n == 0 {
            return None;
        }
        let top = self.typed::<T>()[0];
        if n == 1 {
            self.data.clear();
            return Some(top);
        }
        let last = self.typed::<T>()[n - 1];
        self.typed_mut::<T>()[0] = last;
        let es = std::mem::size_of::<T>();
        self.data.truncate(self.data.len() - es);
        self.sift_down::<T>(0);
        Some(top)
    }

    pub fn top<T: bytemuck::Pod>(&self) -> Option<T> {
        assert_element_size::<T>(self.element_size);
        self.typed::<T>().first().copied()
    }

    fn sift_up<T: bytemuck::Pod + PartialOrd>(&mut self, mut idx: usize) {
        while idx > 0 {
            let parent = (idx - 1) / 2;
            if self.cmp::<T>(idx, parent) == Ordering::Greater {
                self.typed_mut::<T>().swap(idx, parent);
                idx = parent;
            } else {
                break;
            }
        }
    }

    fn sift_down<T: bytemuck::Pod + PartialOrd>(&mut self, mut idx: usize) {
        let n = self.typed::<T>().len();
        loop {
            let l = 2 * idx + 1;
            let r = 2 * idx + 2;
            let mut best = idx;
            if l < n && self.cmp::<T>(l, best) == Ordering::Greater {
                best = l;
            }
            if r < n && self.cmp::<T>(r, best) == Ordering::Greater {
                best = r;
            }
            if best == idx {
                break;
            }
            self.typed_mut::<T>().swap(idx, best);
            idx = best;
        }
    }
}

/// Min-heap constructor — wire shape identical to [`Heap`] with `order = Min`.
pub struct MinHeap;

impl MinHeap {
    pub fn new<T: bytemuck::Pod>() -> Heap {
        Heap::new_min::<T>()
    }
}
