//! Doubly-linked list Pod — holds any `T: bytemuck::Pod`.
//!
//! Each node lays out as `[value: element_size bytes][prev: u32, next: u32]`
//! for a total of `element_size + 8` bytes. Nodes live in a single
//! `#[dp(bytes)]` byte buffer. The free-list of vacated slots is woven
//! into the `next` field of the freed nodes (`free_head` chains to the
//! most recent free, whose `next` chains to the previous, terminating in
//! [`LIST_NIL`]).
//!
//! `prev` / `next` are u32 **slot indices**, not byte offsets — divide by
//! the node stride to convert if you need bytes.

use crate::seq::assert_element_size;

/// Sentinel for "no node" — used for `head`/`tail`/`free_head`/`prev`/`next`.
pub const LIST_NIL: u32 = u32::MAX;

#[datapod::datapod]
pub struct List {
    pub head: u32,
    pub tail: u32,
    pub free_head: u32,
    pub size_: u32,
    pub element_size: u32,
    pub _pad: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Default for List {
    fn default() -> Self {
        Self {
            head: LIST_NIL,
            tail: LIST_NIL,
            free_head: LIST_NIL,
            size_: 0,
            element_size: 0,
            _pad: 0,
            data: Vec::new(),
        }
    }
}

impl List {
    pub fn new<T: bytemuck::Pod>() -> Self {
        Self {
            element_size: std::mem::size_of::<T>() as u32,
            ..Self::default()
        }
    }

    pub fn size(&self) -> usize {
        self.size_ as usize
    }

    pub fn empty(&self) -> bool {
        self.size_ == 0
    }

    /// Bytes per node = value width + 2×u32 (prev, next).
    #[inline]
    fn node_size(&self) -> usize {
        self.element_size as usize + 8
    }

    fn slot_count(&self) -> usize {
        let ns = self.node_size();
        if ns == 0 { 0 } else { self.data.len() / ns }
    }

    fn value_offset(&self, slot: u32) -> usize {
        slot as usize * self.node_size()
    }

    fn prev_offset(&self, slot: u32) -> usize {
        self.value_offset(slot) + self.element_size as usize
    }

    fn next_offset(&self, slot: u32) -> usize {
        self.prev_offset(slot) + 4
    }

    fn read_prev(&self, slot: u32) -> u32 {
        let o = self.prev_offset(slot);
        u32::from_le_bytes(self.data[o..o + 4].try_into().unwrap())
    }

    fn read_next(&self, slot: u32) -> u32 {
        let o = self.next_offset(slot);
        u32::from_le_bytes(self.data[o..o + 4].try_into().unwrap())
    }

    fn write_prev(&mut self, slot: u32, value: u32) {
        let o = self.prev_offset(slot);
        self.data[o..o + 4].copy_from_slice(&value.to_le_bytes());
    }

    fn write_next(&mut self, slot: u32, value: u32) {
        let o = self.next_offset(slot);
        self.data[o..o + 4].copy_from_slice(&value.to_le_bytes());
    }

    fn read_value<T: bytemuck::Pod>(&self, slot: u32) -> T {
        let o = self.value_offset(slot);
        *bytemuck::from_bytes(&self.data[o..o + self.element_size as usize])
    }

    fn write_value<T: bytemuck::Pod>(&mut self, slot: u32, value: T) {
        let o = self.value_offset(slot);
        let es = self.element_size as usize;
        self.data[o..o + es].copy_from_slice(bytemuck::bytes_of(&value));
    }

    /// Allocate a slot for a new node. Reuses a freed slot if one exists.
    fn alloc_slot<T: bytemuck::Pod>(&mut self, value: T, prev: u32, next: u32) -> u32 {
        if self.free_head != LIST_NIL {
            let idx = self.free_head;
            // The freed slot's `next` is the chain pointer to the previous
            // freed slot. Pop it.
            let next_free = self.read_next(idx);
            self.write_value::<T>(idx, value);
            self.write_prev(idx, prev);
            self.write_next(idx, next);
            self.free_head = next_free;
            idx
        } else {
            // Grow buffer by one node.
            let ns = self.node_size();
            let new_off = self.data.len();
            self.data.resize(new_off + ns, 0);
            let idx = (new_off / ns) as u32;
            self.write_value::<T>(idx, value);
            self.write_prev(idx, prev);
            self.write_next(idx, next);
            idx
        }
    }

    /// Mark a slot as free. `next` is hijacked as the free-chain pointer.
    fn free_slot(&mut self, idx: u32) {
        let prev_free = self.free_head;
        // Zero out value bytes for cleanliness.
        let o = self.value_offset(idx);
        let es = self.element_size as usize;
        for b in &mut self.data[o..o + es] {
            *b = 0;
        }
        self.write_prev(idx, 0);
        self.write_next(idx, prev_free);
        self.free_head = idx;
    }

    pub fn push_back<T: bytemuck::Pod>(&mut self, value: T) {
        assert_element_size::<T>(self.element_size);
        let old_tail = self.tail;
        let new_idx = self.alloc_slot::<T>(value, old_tail, LIST_NIL);
        if old_tail != LIST_NIL {
            self.write_next(old_tail, new_idx);
        } else {
            self.head = new_idx;
        }
        self.tail = new_idx;
        self.size_ += 1;
    }

    pub fn push_front<T: bytemuck::Pod>(&mut self, value: T) {
        assert_element_size::<T>(self.element_size);
        let old_head = self.head;
        let new_idx = self.alloc_slot::<T>(value, LIST_NIL, old_head);
        if old_head != LIST_NIL {
            self.write_prev(old_head, new_idx);
        } else {
            self.tail = new_idx;
        }
        self.head = new_idx;
        self.size_ += 1;
    }

    pub fn pop_back<T: bytemuck::Pod>(&mut self) -> Option<T> {
        assert_element_size::<T>(self.element_size);
        if self.tail == LIST_NIL {
            return None;
        }
        let idx = self.tail;
        let value = self.read_value::<T>(idx);
        let new_tail = self.read_prev(idx);
        self.tail = new_tail;
        if new_tail != LIST_NIL {
            self.write_next(new_tail, LIST_NIL);
        } else {
            self.head = LIST_NIL;
        }
        self.free_slot(idx);
        self.size_ -= 1;
        Some(value)
    }

    pub fn pop_front<T: bytemuck::Pod>(&mut self) -> Option<T> {
        assert_element_size::<T>(self.element_size);
        if self.head == LIST_NIL {
            return None;
        }
        let idx = self.head;
        let value = self.read_value::<T>(idx);
        let new_head = self.read_next(idx);
        self.head = new_head;
        if new_head != LIST_NIL {
            self.write_prev(new_head, LIST_NIL);
        } else {
            self.tail = LIST_NIL;
        }
        self.free_slot(idx);
        self.size_ -= 1;
        Some(value)
    }

    pub fn front<T: bytemuck::Pod>(&self) -> Option<T> {
        assert_element_size::<T>(self.element_size);
        if self.head == LIST_NIL {
            None
        } else {
            Some(self.read_value::<T>(self.head))
        }
    }

    pub fn back<T: bytemuck::Pod>(&self) -> Option<T> {
        assert_element_size::<T>(self.element_size);
        if self.tail == LIST_NIL {
            None
        } else {
            Some(self.read_value::<T>(self.tail))
        }
    }

    /// Iterate values from head to tail.
    pub fn iter<T: bytemuck::Pod>(&self) -> ListIter<'_, T> {
        assert_element_size::<T>(self.element_size);
        ListIter {
            list: self,
            cursor: self.head,
            _marker: std::marker::PhantomData,
        }
    }

    /// Slot count tracked separately from logical size — exposed for
    /// diagnostics. Always `>= size()` (the difference is the free-list).
    pub fn capacity(&self) -> usize {
        self.slot_count()
    }
}

pub struct ListIter<'a, T> {
    list: &'a List,
    cursor: u32,
    _marker: std::marker::PhantomData<T>,
}

impl<'a, T: bytemuck::Pod> Iterator for ListIter<'a, T> {
    type Item = T;
    fn next(&mut self) -> Option<T> {
        if self.cursor == LIST_NIL {
            return None;
        }
        let value = self.list.read_value::<T>(self.cursor);
        self.cursor = self.list.read_next(self.cursor);
        Some(value)
    }
}
