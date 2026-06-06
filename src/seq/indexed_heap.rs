//! Indexed binary heap Pod — `u64` key + any `T: bytemuck::Pod + PartialOrd`
//! priority.
//!
//! The wire entry is `[key: u64][priority: T]` packed; total entry size is
//! `8 + size_of::<T>()` bytes, captured in the header as `priority_size`.
//! The runtime `key -> position` map from the old `IndexedHeap` is not on
//! the wire; build it with [`Self::build_index`] when you need O(1)
//! `position_of` queries.

use crate::seq::heap::HeapOrder;
use crate::{DataPodValidate, WireError};
use std::cmp::Ordering;
use std::collections::HashMap;

#[datapod::datapod]
#[dp(manual_access)]
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
        Self::try_new::<T>().unwrap_or_default()
    }

    pub fn try_new<T: bytemuck::Pod>() -> Result<Self, WireError> {
        let heap = Self {
            priority_size: super::checked_pod_element_size::<Self, T>()?,
            order: HeapOrder::Max,
            _pad: [0; 3],
            data: Vec::new(),
        };
        heap.validate_owned()?;
        Ok(heap)
    }

    pub fn new_min<T: bytemuck::Pod>() -> Self {
        Self::try_new_min::<T>().unwrap_or_default()
    }

    pub fn try_new_min<T: bytemuck::Pod>() -> Result<Self, WireError> {
        let heap = Self {
            priority_size: super::checked_pod_element_size::<Self, T>()?,
            order: HeapOrder::Min,
            _pad: [0; 3],
            data: Vec::new(),
        };
        heap.validate_owned()?;
        Ok(heap)
    }

    /// Total bytes per entry: `u64 key + T priority`.
    #[inline]
    fn entry_size(&self) -> usize {
        self.try_entry_size().unwrap_or(0)
    }

    fn try_entry_size(&self) -> Result<usize, WireError> {
        8usize
            .checked_add(self.try_priority_size()?)
            .ok_or_else(|| {
                crate::wire::invalid_header::<Self>("indexed heap entry size overflowed usize")
            })
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    pub fn try_size(&self) -> Result<usize, WireError> {
        self.validate_owned()?;
        let es = self.try_entry_size()?;
        if es == 0 {
            Ok(0)
        } else {
            Ok(self.data.len() / es)
        }
    }

    pub fn empty(&self) -> bool {
        self.try_empty().unwrap_or(true)
    }

    pub fn try_empty(&self) -> Result<bool, WireError> {
        Ok(self.try_size()? == 0)
    }

    pub fn clear(&mut self) {
        self.data.clear();
    }

    fn entry_offset(&self, i: usize) -> Option<usize> {
        i.checked_mul(self.entry_size())
    }

    fn try_key_at(&self, i: usize) -> Result<u64, WireError> {
        let Some(off) = self.entry_offset(i) else {
            return Err(crate::wire::invalid_payload::<Self>(
                "indexed heap key offset overflowed",
            ));
        };
        let Some(raw) = off.checked_add(8).and_then(|end| self.data.get(off..end)) else {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "indexed heap key range for entry {i} is out of bounds"
            )));
        };
        let mut bytes = [0u8; 8];
        bytes.copy_from_slice(raw);
        Ok(u64::from_le_bytes(bytes))
    }

    fn set_key(&mut self, i: usize, key: u64) {
        let Some(off) = self.entry_offset(i) else {
            return;
        };
        let Some(slot) = off
            .checked_add(8)
            .and_then(|end| self.data.get_mut(off..end))
        else {
            return;
        };
        slot.copy_from_slice(&key.to_le_bytes());
    }

    fn priority_at<T: bytemuck::Pod>(&self, i: usize) -> T {
        let Some(off) = self.entry_offset(i).and_then(|off| off.checked_add(8)) else {
            return bytemuck::Zeroable::zeroed();
        };
        let Ok(priority_size) = self.try_priority_size() else {
            return bytemuck::Zeroable::zeroed();
        };
        let Some(bytes) = priority_size
            .checked_add(off)
            .and_then(|end| self.data.get(off..end))
        else {
            return bytemuck::Zeroable::zeroed();
        };
        bytemuck::pod_read_unaligned(bytes)
    }

    fn set_priority<T: bytemuck::Pod>(&mut self, i: usize, value: T) {
        let Some(off) = self.entry_offset(i).and_then(|off| off.checked_add(8)) else {
            return;
        };
        let Ok(ps) = self.try_priority_size() else {
            return;
        };
        let Some(slot) = off
            .checked_add(ps)
            .and_then(|end| self.data.get_mut(off..end))
        else {
            return;
        };
        slot.copy_from_slice(bytemuck::bytes_of(&value));
    }

    fn try_priority_size(&self) -> Result<usize, WireError> {
        super::checked_u32_to_usize::<Self>(self.priority_size, "priority_size")
    }

    fn swap_entries(&mut self, a: usize, b: usize) {
        let es = self.entry_size();
        let Some(oa) = a.checked_mul(es) else {
            return;
        };
        let Some(ob) = b.checked_mul(es) else {
            return;
        };
        // Pairwise byte swap (no slice::swap because the two ranges
        // overlap if a == b; but we'd never call this with a == b).
        for k in 0..es {
            let Some(ia) = oa.checked_add(k) else {
                return;
            };
            let Some(ib) = ob.checked_add(k) else {
                return;
            };
            if ia >= self.data.len() || ib >= self.data.len() {
                return;
            }
            self.data.swap(ia, ib);
        }
    }

    fn better<T: PartialOrd>(&self, a: &T, b: &T) -> bool {
        let raw = a.partial_cmp(b).unwrap_or(Ordering::Equal);
        if self.order == HeapOrder::Min {
            raw == Ordering::Less
        } else {
            raw == Ordering::Greater
        }
    }

    /// Insert `(key, priority)` or update an existing key's priority.
    pub fn push<T: bytemuck::Pod + PartialOrd>(&mut self, key: u64, priority: T) {
        let _ = self.try_push(key, priority);
    }

    pub fn try_push<T: bytemuck::Pod + PartialOrd>(
        &mut self,
        key: u64,
        priority: T,
    ) -> Result<(), WireError> {
        check_element_size::<Self, T>(self.priority_size)?;
        self.validate_owned()?;
        self.validate_typed_heap_order::<T>()?;
        if let Some(pos) = self.position_of_validated(key)? {
            self.update_at::<T>(pos, priority)?;
            return Ok(());
        }
        // Append a fresh entry.
        let entry_size = self.try_entry_size()?;
        self.data.try_reserve_exact(entry_size).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "indexed heap payload allocation failed: {err}"
            ))
        })?;
        self.data.extend_from_slice(&key.to_le_bytes());
        self.data.extend_from_slice(bytemuck::bytes_of(&priority));
        let last = self.try_size()?.checked_sub(1).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("indexed heap length underflowed after push")
        })?;
        self.sift_up::<T>(last);
        Ok(())
    }

    /// Pop the top entry (max or min depending on `order`).
    pub fn pop<T: bytemuck::Pod + PartialOrd>(&mut self) -> Option<(u64, T)> {
        self.try_pop().unwrap_or(None)
    }

    pub fn try_pop<T: bytemuck::Pod + PartialOrd>(
        &mut self,
    ) -> Result<Option<(u64, T)>, WireError> {
        check_element_size::<Self, T>(self.priority_size)?;
        self.validate_owned()?;
        self.validate_typed_heap_order::<T>()?;
        let n = self.try_size()?;
        if n == 0 {
            return Ok(None);
        }
        let top_key = self.try_key_at(0)?;
        let top_priority = self.priority_at::<T>(0);
        if n == 1 {
            self.data.clear();
            return Ok(Some((top_key, top_priority)));
        }
        // Move last entry to slot 0, truncate, sift down.
        let last_key = self.try_key_at(n - 1)?;
        let last_priority = self.priority_at::<T>(n - 1);
        self.set_key(0, last_key);
        self.set_priority::<T>(0, last_priority);
        let es = self.try_entry_size()?;
        let truncate_len = self.data.len().checked_sub(es).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("indexed heap truncate underflowed")
        })?;
        self.data.truncate(truncate_len);
        self.try_sift_down::<T>(0)?;
        Ok(Some((top_key, top_priority)))
    }

    pub fn top<T: bytemuck::Pod>(&self) -> Option<(u64, T)> {
        self.try_top().unwrap_or(None)
    }

    pub fn try_top<T: bytemuck::Pod>(&self) -> Result<Option<(u64, T)>, WireError> {
        check_element_size::<Self, T>(self.priority_size)?;
        self.validate_owned()?;
        if self.try_empty()? {
            Ok(None)
        } else {
            Ok(Some((self.try_key_at(0)?, self.priority_at::<T>(0))))
        }
    }

    /// O(n) scan for a key.
    pub fn position_of(&self, key: u64) -> Option<usize> {
        self.try_position_of(key).unwrap_or(None)
    }

    pub fn try_position_of(&self, key: u64) -> Result<Option<usize>, WireError> {
        self.validate_owned()?;
        self.position_of_validated(key)
    }

    fn position_of_validated(&self, key: u64) -> Result<Option<usize>, WireError> {
        for i in 0..self.try_size()? {
            if self.try_key_at(i)? == key {
                return Ok(Some(i));
            }
        }
        Ok(None)
    }

    /// Build a fresh `key -> position` map (not on the wire).
    pub fn build_index(&self) -> HashMap<u64, usize> {
        self.try_build_index().unwrap_or_default()
    }

    pub fn try_build_index(&self) -> Result<HashMap<u64, usize>, WireError> {
        self.validate_owned()?;
        self.build_index_validated()
    }

    fn build_index_validated(&self) -> Result<HashMap<u64, usize>, WireError> {
        let size = self.try_size()?;
        let mut index = HashMap::new();
        index.try_reserve(size).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "failed to reserve {size} indexed heap index entries: {err}"
            ))
        })?;
        for i in 0..size {
            index.insert(self.try_key_at(i)?, i);
        }
        Ok(index)
    }

    fn update_at<T: bytemuck::Pod + PartialOrd>(
        &mut self,
        idx: usize,
        new_priority: T,
    ) -> Result<(), WireError> {
        let old = self.priority_at::<T>(idx);
        self.set_priority::<T>(idx, new_priority);
        if self.better(&new_priority, &old) {
            self.sift_up::<T>(idx);
        } else {
            self.try_sift_down::<T>(idx)?;
        }
        Ok(())
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

    fn try_sift_down<T: bytemuck::Pod + PartialOrd>(
        &mut self,
        mut idx: usize,
    ) -> Result<(), WireError> {
        let n = self.try_size()?;
        loop {
            let Some(l) = idx.checked_mul(2).and_then(|base| base.checked_add(1)) else {
                break;
            };
            let Some(r) = l.checked_add(1) else {
                break;
            };
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
        Ok(())
    }

    fn validate_owned(&self) -> Result<(), WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &IndexedHeapHeader {
                priority_size: self.priority_size,
                order: self.order,
                _pad: self._pad,
            },
            &self.data,
        )
    }

    fn validate_typed_heap_order<T: bytemuck::Pod + PartialOrd>(&self) -> Result<(), WireError> {
        let n = self.try_size()?;
        for child in 1..n {
            let parent = (child - 1) / 2;
            let child_p = self.priority_at::<T>(child);
            let parent_p = self.priority_at::<T>(parent);
            if self.better(&child_p, &parent_p) {
                return Err(crate::wire::invalid_payload::<Self>(format!(
                    "indexed heap invariant violated at child {child} parent {parent}"
                )));
            }
        }
        Ok(())
    }
}

fn check_element_size<P: 'static, T>(stored: u32) -> Result<(), WireError> {
    let actual = std::mem::size_of::<T>();
    if actual == 0 {
        return Err(crate::wire::invalid_header::<P>(
            "zero-sized Pod elements cannot be represented in byte-counted datapod containers",
        ));
    }
    let stored = super::checked_u32_to_usize::<P>(stored, "priority_size")?;
    if actual != stored {
        return Err(crate::wire::invalid_header::<P>(format!(
            "element size mismatch: T is {actual}, container expects {stored}"
        )));
    }
    Ok(())
}
