//! Singly-linked list Pod — holds any `T: bytemuck::Pod`.
//!
//! Node layout: `[value: element_size bytes][next: u32][_pad: u32]`. The
//! padding keeps the node 8-byte aligned regardless of `element_size`.
//! Free-list folded into `next`.

use crate::seq::assert_element_size;

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

    #[inline]
    fn node_size(&self) -> usize {
        self.element_size as usize + 8
    }

    fn value_offset(&self, slot: u32) -> usize {
        slot as usize * self.node_size()
    }

    fn next_offset(&self, slot: u32) -> usize {
        self.value_offset(slot) + self.element_size as usize
    }

    fn read_next(&self, slot: u32) -> u32 {
        let o = self.next_offset(slot);
        u32::from_le_bytes(self.data[o..o + 4].try_into().unwrap())
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

    fn alloc_slot<T: bytemuck::Pod>(&mut self, value: T, next: u32) -> u32 {
        if self.free_head != FORWARD_LIST_NIL {
            let idx = self.free_head;
            let next_free = self.read_next(idx);
            self.write_value::<T>(idx, value);
            self.write_next(idx, next);
            self.free_head = next_free;
            idx
        } else {
            let ns = self.node_size();
            let new_off = self.data.len();
            self.data.resize(new_off + ns, 0);
            let idx = (new_off / ns) as u32;
            self.write_value::<T>(idx, value);
            self.write_next(idx, next);
            idx
        }
    }

    fn free_slot(&mut self, idx: u32) {
        let prev_free = self.free_head;
        let o = self.value_offset(idx);
        let es = self.element_size as usize;
        for b in &mut self.data[o..o + es] {
            *b = 0;
        }
        self.write_next(idx, prev_free);
        self.free_head = idx;
    }

    pub fn push_front<T: bytemuck::Pod>(&mut self, value: T) {
        assert_element_size::<T>(self.element_size);
        let old_head = self.head;
        let new_idx = self.alloc_slot::<T>(value, old_head);
        self.head = new_idx;
        self.size_ += 1;
    }

    pub fn pop_front<T: bytemuck::Pod>(&mut self) -> Option<T> {
        assert_element_size::<T>(self.element_size);
        if self.head == FORWARD_LIST_NIL {
            return None;
        }
        let idx = self.head;
        let value = self.read_value::<T>(idx);
        let new_head = self.read_next(idx);
        self.head = new_head;
        self.free_slot(idx);
        self.size_ -= 1;
        Some(value)
    }

    pub fn front<T: bytemuck::Pod>(&self) -> Option<T> {
        assert_element_size::<T>(self.element_size);
        if self.head == FORWARD_LIST_NIL {
            None
        } else {
            Some(self.read_value::<T>(self.head))
        }
    }

    pub fn iter<T: bytemuck::Pod>(&self) -> ForwardListIter<'_, T> {
        assert_element_size::<T>(self.element_size);
        ForwardListIter {
            list: self,
            cursor: self.head,
            _marker: std::marker::PhantomData,
        }
    }
}

pub struct ForwardListIter<'a, T> {
    list: &'a ForwardList,
    cursor: u32,
    _marker: std::marker::PhantomData<T>,
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
