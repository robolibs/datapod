//! Singly-linked list Pod — holds any `T: bytemuck::Pod`.
//!
//! Node layout: `[value: element_size bytes][next: u32][_pad: u32]`. The
//! padding keeps the node 8-byte aligned regardless of `element_size`.
//! Free-list folded into `next`.

use crate::{DataPodValidate, WireError};

pub const FORWARD_LIST_NIL: u32 = u32::MAX;

#[datapod::datapod]
#[dp(manual_access)]
pub struct ForwardList {
    pub head: u32,
    pub free_head: u32,
    pub size_: u32,
    pub element_size: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Default for ForwardList {
    fn default() -> Self {
        Self {
            head: FORWARD_LIST_NIL,
            free_head: FORWARD_LIST_NIL,
            size_: 0,
            element_size: 0,
            data: Vec::new(),
        }
    }
}

impl ForwardList {
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self::try_new::<T>().unwrap_or_default()
    }

    pub fn try_new<T: bytemuck::Pod>() -> Result<Self, WireError> {
        let list = Self {
            element_size: super::checked_pod_element_size::<Self, T>()?,
            ..Self::default()
        };
        list.validate_owned()?;
        Ok(list)
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    /// Fallible logical element count for callers handling potentially
    /// malformed owned buffers.
    pub fn try_size(&self) -> Result<usize, WireError> {
        self.validate_owned()?;
        super::checked_u32_to_usize::<Self>(self.size_, "size_")
    }

    pub fn empty(&self) -> bool {
        self.try_empty().unwrap_or(true)
    }

    /// Fallible emptiness check for callers handling potentially malformed
    /// owned buffers.
    pub fn try_empty(&self) -> Result<bool, WireError> {
        Ok(self.try_size()? == 0)
    }

    #[inline]
    fn node_size(&self) -> usize {
        self.try_node_size().unwrap_or(0)
    }

    fn try_node_size(&self) -> Result<usize, WireError> {
        self.try_element_size()?.checked_add(8).ok_or_else(|| {
            crate::wire::invalid_header::<Self>("forward-list node size overflowed usize")
        })
    }

    fn slot_count(&self) -> usize {
        self.try_slot_count().unwrap_or(0)
    }

    fn try_slot_count(&self) -> Result<usize, WireError> {
        let ns = self.try_node_size()?;
        Ok(if ns == 0 { 0 } else { self.data.len() / ns })
    }

    /// Slot count tracked separately from logical size — exposed for
    /// diagnostics. Always `>= size()` (the difference is the free-list).
    pub fn capacity(&self) -> usize {
        self.try_capacity().unwrap_or(0)
    }

    /// Fallible slot capacity for callers handling potentially malformed owned
    /// buffers.
    pub fn try_capacity(&self) -> Result<usize, WireError> {
        self.validate_owned()?;
        self.try_slot_count()
    }

    fn value_offset(&self, slot: u32) -> Option<usize> {
        let node_size = self.try_node_size().ok()?;
        let slot = super::checked_u32_to_usize::<Self>(slot, "slot").ok()?;
        slot.checked_mul(node_size)
    }

    fn next_offset(&self, slot: u32) -> Option<usize> {
        let element_size = self.try_element_size().ok()?;
        self.value_offset(slot)
            .and_then(|offset| offset.checked_add(element_size))
    }

    fn read_next(&self, slot: u32) -> u32 {
        let Some(o) = self.next_offset(slot) else {
            return FORWARD_LIST_NIL;
        };
        let Some(raw) = o.checked_add(4).and_then(|end| self.data.get(o..end)) else {
            return FORWARD_LIST_NIL;
        };
        let mut bytes = [0u8; 4];
        bytes.copy_from_slice(raw);
        u32::from_le_bytes(bytes)
    }

    fn write_next(&mut self, slot: u32, value: u32) {
        let Some(o) = self.next_offset(slot) else {
            return;
        };
        let Some(slot) = o.checked_add(4).and_then(|end| self.data.get_mut(o..end)) else {
            return;
        };
        slot.copy_from_slice(&value.to_le_bytes());
    }

    fn read_value<T: bytemuck::Pod>(&self, slot: u32) -> T {
        let Some(o) = self.value_offset(slot) else {
            return bytemuck::Zeroable::zeroed();
        };
        let Ok(es) = self.try_element_size() else {
            return bytemuck::Zeroable::zeroed();
        };
        let Some(bytes) = es.checked_add(o).and_then(|end| self.data.get(o..end)) else {
            return bytemuck::Zeroable::zeroed();
        };
        bytemuck::pod_read_unaligned(bytes)
    }

    fn write_value<T: bytemuck::Pod>(&mut self, slot: u32, value: T) {
        let Some(o) = self.value_offset(slot) else {
            return;
        };
        let Ok(es) = self.try_element_size() else {
            return;
        };
        let Some(slot) = o.checked_add(es).and_then(|end| self.data.get_mut(o..end)) else {
            return;
        };
        slot.copy_from_slice(bytemuck::bytes_of(&value));
    }

    fn try_element_size(&self) -> Result<usize, WireError> {
        super::checked_u32_to_usize::<Self>(self.element_size, "element_size")
    }

    fn try_alloc_slot<T: bytemuck::Pod>(&mut self, value: T, next: u32) -> Result<u32, WireError> {
        if self.free_head != FORWARD_LIST_NIL {
            let idx = self.free_head;
            let next_free = self.read_next(idx);
            self.write_value::<T>(idx, value);
            self.write_next(idx, next);
            self.free_head = next_free;
            Ok(idx)
        } else {
            let ns = self.try_node_size()?;
            let new_off = self.data.len();
            let new_len = new_off.checked_add(ns).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("forward-list payload length overflowed")
            })?;
            let idx = u32::try_from(new_off / ns).map_err(|_| {
                crate::wire::invalid_payload::<Self>("forward-list slot index exceeds u32")
            })?;
            self.data.try_reserve_exact(ns).map_err(|err| {
                crate::wire::invalid_payload::<Self>(format!(
                    "forward-list payload allocation failed: {err}"
                ))
            })?;
            self.data.resize(new_len, 0);
            self.write_value::<T>(idx, value);
            self.write_next(idx, next);
            Ok(idx)
        }
    }

    fn free_slot(&mut self, idx: u32) {
        let prev_free = self.free_head;
        if let Some(o) = self.value_offset(idx)
            && let Ok(es) = self.try_element_size()
            && let Some(value_bytes) = o.checked_add(es).and_then(|end| self.data.get_mut(o..end))
        {
            value_bytes.fill(0);
        }
        self.write_next(idx, prev_free);
        self.free_head = idx;
    }

    pub fn push_front<T: bytemuck::Pod>(&mut self, value: T) {
        let _ = self.try_push_front(value);
    }

    pub fn try_push_front<T: bytemuck::Pod>(&mut self, value: T) -> Result<(), WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        let new_size = self.size_.checked_add(1).ok_or_else(|| {
            crate::wire::invalid_header::<Self>("forward-list size overflowed u32")
        })?;
        let old_head = self.head;
        let new_idx = self.try_alloc_slot::<T>(value, old_head)?;
        self.head = new_idx;
        self.size_ = new_size;
        Ok(())
    }

    pub fn pop_front<T: bytemuck::Pod>(&mut self) -> Option<T> {
        self.try_pop_front().unwrap_or(None)
    }

    pub fn try_pop_front<T: bytemuck::Pod>(&mut self) -> Result<Option<T>, WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        if self.head == FORWARD_LIST_NIL {
            return Ok(None);
        }
        let idx = self.head;
        let value = self.read_value::<T>(idx);
        let new_head = self.read_next(idx);
        self.head = new_head;
        self.free_slot(idx);
        self.size_ = self
            .size_
            .checked_sub(1)
            .ok_or_else(|| crate::wire::invalid_header::<Self>("forward-list size underflowed"))?;
        Ok(Some(value))
    }

    pub fn front<T: bytemuck::Pod>(&self) -> Option<T> {
        self.try_front().unwrap_or(None)
    }

    pub fn try_front<T: bytemuck::Pod>(&self) -> Result<Option<T>, WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        if self.head == FORWARD_LIST_NIL {
            Ok(None)
        } else {
            Ok(Some(self.read_value::<T>(self.head)))
        }
    }

    pub fn iter<T: bytemuck::Pod>(&self) -> ForwardListIter<'_, T> {
        self.try_iter().unwrap_or(ForwardListIter {
            list: self,
            cursor: FORWARD_LIST_NIL,
            _marker: std::marker::PhantomData,
        })
    }

    /// Fallible iterator constructor for callers handling potentially
    /// malformed owned forward-list buffers or runtime element-type mismatches.
    pub fn try_iter<T: bytemuck::Pod>(&self) -> Result<ForwardListIter<'_, T>, WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        Ok(ForwardListIter {
            list: self,
            cursor: self.head,
            _marker: std::marker::PhantomData,
        })
    }

    fn validate_owned(&self) -> Result<(), WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &ForwardListHeader {
                head: self.head,
                free_head: self.free_head,
                size_: self.size_,
                element_size: self.element_size,
            },
            &self.data,
        )
    }
}

pub struct ForwardListIter<'a, T> {
    list: &'a ForwardList,
    cursor: u32,
    _marker: std::marker::PhantomData<T>,
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

impl<'a, T: bytemuck::Pod> Iterator for ForwardListIter<'a, T> {
    type Item = T;
    fn next(&mut self) -> Option<T> {
        if self.cursor == FORWARD_LIST_NIL {
            return None;
        }
        let value = self.list.read_value::<T>(self.cursor);
        self.cursor = self.list.read_next(self.cursor);
        Some(value)
    }
}
