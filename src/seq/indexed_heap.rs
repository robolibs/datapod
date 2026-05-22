//! Indexed binary heap Pod — `u64` key + any `T: bytemuck::Pod + PartialOrd`
//! priority.
//!
//! The wire entry is `[key: u64][priority: T]` packed; total entry size is
//! `8 + size_of::<T>()` bytes, captured in the header as `priority_size`.
//! The runtime `key -> position` map from the old `IndexedHeap` is not on
//! the wire; build it with [`Self::build_index`] when you need O(1)
//! `position_of` queries.

use crate::seq::assert_element_size;
use crate::seq::heap::HeapOrder;
use std::cmp::Ordering;
use std::collections::HashMap;

#[datapod::datapod]
#[derive(Default)]
pub struct IndexedHeap {
    pub priority_size: u32,
    pub order: HeapOrder,
    pub _pad: [u8; 3],
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl IndexedHeap {
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self {
            priority_size: std::mem::size_of::<T>() as u32,
            order: HeapOrder::Max,
            _pad: [0; 3],
            data: Vec::new(),
        }
    }

    pub fn new_min<T: bytemuck::Pod>() -> Self {
        Self {
            priority_size: std::mem::size_of::<T>() as u32,
            order: HeapOrder::Min,
            _pad: [0; 3],
            data: Vec::new(),
        }
    }

    /// Total bytes per entry: `u64 key + T priority`.
    #[inline]
    fn entry_size(&self) -> usize {
        8 + self.priority_size as usize
    }

    pub fn size(&self) -> usize {
        let es = self.entry_size();
        if es == 0 { 0 } else { self.data.len() / es }
    }

    pub fn empty(&self) -> bool {
        self.data.is_empty()
    }

    pub fn clear(&mut self) {
        self.data.clear();
    }

    fn entry_offset(&self, i: usize) -> usize {
        i * self.entry_size()
    }

    fn key_at(&self, i: usize) -> u64 {
        let off = self.entry_offset(i);
        u64::from_le_bytes(self.data[off..off + 8].try_into().unwrap())
    }

    fn set_key(&mut self, i: usize, key: u64) {
        let off = self.entry_offset(i);
        self.data[off..off + 8].copy_from_slice(&key.to_le_bytes());
    }

    fn priority_at<T: bytemuck::Pod>(&self, i: usize) -> T {
        let off = self.entry_offset(i) + 8;
        *bytemuck::from_bytes(&self.data[off..off + self.priority_size as usize])
    }

    fn set_priority<T: bytemuck::Pod>(&mut self, i: usize, value: T) {
        let off = self.entry_offset(i) + 8;
        let ps = self.priority_size as usize;
        self.data[off..off + ps].copy_from_slice(bytemuck::bytes_of(&value));
    }

    fn swap_entries(&mut self, a: usize, b: usize) {
        let es = self.entry_size();
        let oa = a * es;
        let ob = b * es;
        // Pairwise byte swap (no slice::swap because the two ranges
        // overlap if a == b; but we'd never call this with a == b).
        for k in 0..es {
            self.data.swap(oa + k, ob + k);
        }
    }

    fn better<T: PartialOrd>(&self, a: &T, b: &T) -> bool {
        let raw = a.partial_cmp(b).unwrap_or(Ordering::Equal);
        match self.order {
            HeapOrder::Max => raw == Ordering::Greater,
            HeapOrder::Min => raw == Ordering::Less,
        }
    }

    /// Insert `(key, priority)` or update an existing key's priority.
    pub fn push<T: bytemuck::Pod + PartialOrd>(&mut self, key: u64, priority: T) {
        assert_element_size::<T>(self.priority_size);
        if let Some(pos) = self.position_of(key) {
            self.update_at::<T>(pos, priority);
            return;
        }
        // Append a fresh entry.
        self.data.extend_from_slice(&key.to_le_bytes());
        self.data.extend_from_slice(bytemuck::bytes_of(&priority));
        let last = self.size() - 1;
        self.sift_up::<T>(last);
    }

    /// Pop the top entry (max or min depending on `order`).
    pub fn pop<T: bytemuck::Pod + PartialOrd>(&mut self) -> Option<(u64, T)> {
        assert_element_size::<T>(self.priority_size);
        let n = self.size();
        if n == 0 {
            return None;
        }
        let top_key = self.key_at(0);
        let top_priority = self.priority_at::<T>(0);
        if n == 1 {
            self.data.clear();
            return Some((top_key, top_priority));
        }
        // Move last entry to slot 0, truncate, sift down.
        let last_key = self.key_at(n - 1);
        let last_priority = self.priority_at::<T>(n - 1);
        self.set_key(0, last_key);
        self.set_priority::<T>(0, last_priority);
        let es = self.entry_size();
        self.data.truncate(self.data.len() - es);
        self.sift_down::<T>(0);
        Some((top_key, top_priority))
    }

    pub fn top<T: bytemuck::Pod>(&self) -> Option<(u64, T)> {
        assert_element_size::<T>(self.priority_size);
        if self.empty() {
            None
        } else {
            Some((self.key_at(0), self.priority_at::<T>(0)))
        }
    }

    /// O(n) scan for a key.
    pub fn position_of(&self, key: u64) -> Option<usize> {
        (0..self.size()).find(|&i| self.key_at(i) == key)
    }

    /// Build a fresh `key -> position` map (not on the wire).
    pub fn build_index(&self) -> HashMap<u64, usize> {
        (0..self.size()).map(|i| (self.key_at(i), i)).collect()
    }

    fn update_at<T: bytemuck::Pod + PartialOrd>(&mut self, idx: usize, new_priority: T) {
        let old = self.priority_at::<T>(idx);
        self.set_priority::<T>(idx, new_priority);
        if self.better(&new_priority, &old) {
            self.sift_up::<T>(idx);
        } else {
            self.sift_down::<T>(idx);
        }
    }

    fn sift_up<T: bytemuck::Pod + PartialOrd>(&mut self, mut idx: usize) {
        while idx > 0 {
            let parent = (idx - 1) / 2;
            let child = self.priority_at::<T>(idx);
            let par = self.priority_at::<T>(parent);
            if self.better(&child, &par) {
                self.swap_entries(idx, parent);
                idx = parent;
            } else {
                break;
            }
        }
    }

    fn sift_down<T: bytemuck::Pod + PartialOrd>(&mut self, mut idx: usize) {
        let n = self.size();
        loop {
            let l = 2 * idx + 1;
            let r = 2 * idx + 2;
            let mut best = idx;
            let mut best_p = self.priority_at::<T>(idx);
            if l < n {
                let lp = self.priority_at::<T>(l);
                if self.better(&lp, &best_p) {
                    best = l;
                    best_p = lp;
                }
            }
            if r < n {
                let rp = self.priority_at::<T>(r);
                if self.better(&rp, &best_p) {
                    best = r;
                }
            }
            if best == idx {
                break;
            }
            self.swap_entries(idx, best);
            idx = best;
        }
    }
}
