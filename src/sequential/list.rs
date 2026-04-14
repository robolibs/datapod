//! Doubly linked list matching `datapod::List<T>`.
//!
//! The C++ type uses index-based nodes (stored in a `Vector<Node>`) rather
//! than pointers so that the whole structure is serialisable via
//! `members()`. We mirror that layout exactly, which also sidesteps the
//! usual borrow-checker pain you get when implementing a pointer-based
//! linked list in Rust.

use crate::Vector;

pub const INVALID_INDEX: usize = usize::MAX;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Node<T> {
    pub value: T,
    pub prev: usize,
    pub next: usize,
}

impl<T> Node<T> {
    pub fn new(value: T, prev: usize, next: usize) -> Self {
        Self { value, prev, next }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct List<T> {
    pub nodes: Vector<Node<T>>,
    pub head: usize,
    pub tail: usize,
    pub size_: usize,
    pub free_list: Vector<usize>,
}

impl<T> Default for List<T> {
    fn default() -> Self {
        Self {
            nodes: Vector::new(),
            head: INVALID_INDEX,
            tail: INVALID_INDEX,
            size_: 0,
            free_list: Vector::new(),
        }
    }
}

impl<T> List<T> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn len(&self) -> usize {
        self.size_
    }

    pub fn size(&self) -> usize {
        self.size_
    }

    pub fn is_empty(&self) -> bool {
        self.size_ == 0
    }

    pub fn empty(&self) -> bool {
        self.is_empty()
    }

    pub fn front(&self) -> &T {
        if self.is_empty() {
            panic!("List::front: empty");
        }
        &self.nodes[self.head].value
    }

    pub fn front_mut(&mut self) -> &mut T {
        if self.is_empty() {
            panic!("List::front: empty");
        }
        let h = self.head;
        &mut self.nodes[h].value
    }

    pub fn back(&self) -> &T {
        if self.is_empty() {
            panic!("List::back: empty");
        }
        &self.nodes[self.tail].value
    }

    pub fn back_mut(&mut self) -> &mut T {
        if self.is_empty() {
            panic!("List::back: empty");
        }
        let t = self.tail;
        &mut self.nodes[t].value
    }

    pub fn push_front(&mut self, value: T) -> usize {
        let new_index = self.allocate_node(value, INVALID_INDEX, self.head);
        if self.head != INVALID_INDEX {
            self.nodes[self.head].prev = new_index;
        }
        self.head = new_index;
        if self.tail == INVALID_INDEX {
            self.tail = new_index;
        }
        self.size_ += 1;
        new_index
    }

    pub fn push_back(&mut self, value: T) -> usize {
        let new_index = self.allocate_node(value, self.tail, INVALID_INDEX);
        if self.tail != INVALID_INDEX {
            self.nodes[self.tail].next = new_index;
        }
        self.tail = new_index;
        if self.head == INVALID_INDEX {
            self.head = new_index;
        }
        self.size_ += 1;
        new_index
    }

    pub fn pop_front(&mut self) -> Option<()> {
        if self.is_empty() {
            return None;
        }
        let old = self.head;
        self.head = self.nodes[old].next;
        if self.head != INVALID_INDEX {
            self.nodes[self.head].prev = INVALID_INDEX;
        } else {
            self.tail = INVALID_INDEX;
        }
        self.free_list.push_back(old);
        self.size_ -= 1;
        Some(())
    }

    pub fn pop_back(&mut self) -> Option<()> {
        if self.is_empty() {
            return None;
        }
        let old = self.tail;
        self.tail = self.nodes[old].prev;
        if self.tail != INVALID_INDEX {
            self.nodes[self.tail].next = INVALID_INDEX;
        } else {
            self.head = INVALID_INDEX;
        }
        self.free_list.push_back(old);
        self.size_ -= 1;
        Some(())
    }

    pub fn insert_before(&mut self, pos_index: usize, value: T) -> usize {
        if pos_index == INVALID_INDEX {
            return self.push_back(value);
        }
        if pos_index == self.head {
            return self.push_front(value);
        }
        let prev_index = self.nodes[pos_index].prev;
        let new_index = self.allocate_node(value, prev_index, pos_index);
        self.nodes[prev_index].next = new_index;
        self.nodes[pos_index].prev = new_index;
        self.size_ += 1;
        new_index
    }

    pub fn erase(&mut self, pos_index: usize) -> usize {
        if pos_index == INVALID_INDEX {
            panic!("List::erase: invalid index");
        }
        let prev_index = self.nodes[pos_index].prev;
        let next_index = self.nodes[pos_index].next;
        if prev_index != INVALID_INDEX {
            self.nodes[prev_index].next = next_index;
        } else {
            self.head = next_index;
        }
        if next_index != INVALID_INDEX {
            self.nodes[next_index].prev = prev_index;
        } else {
            self.tail = prev_index;
        }
        self.free_list.push_back(pos_index);
        self.size_ -= 1;
        next_index
    }

    pub fn clear(&mut self) {
        self.nodes.clear();
        self.free_list.clear();
        self.head = INVALID_INDEX;
        self.tail = INVALID_INDEX;
        self.size_ = 0;
    }

    pub fn reverse(&mut self) {
        let mut current = self.head;
        while current != INVALID_INDEX {
            let next = self.nodes[current].next;
            let prev = self.nodes[current].prev;
            self.nodes[current].next = prev;
            self.nodes[current].prev = next;
            current = next;
        }
        std::mem::swap(&mut self.head, &mut self.tail);
    }

    pub fn move_to_front(&mut self, pos_index: usize) {
        if pos_index == INVALID_INDEX || pos_index == self.head {
            return;
        }
        let prev_index = self.nodes[pos_index].prev;
        let next_index = self.nodes[pos_index].next;
        if prev_index != INVALID_INDEX {
            self.nodes[prev_index].next = next_index;
        }
        if next_index != INVALID_INDEX {
            self.nodes[next_index].prev = prev_index;
        } else {
            self.tail = prev_index;
        }
        self.nodes[pos_index].prev = INVALID_INDEX;
        self.nodes[pos_index].next = self.head;
        if self.head != INVALID_INDEX {
            self.nodes[self.head].prev = pos_index;
        }
        self.head = pos_index;
    }

    pub fn iter(&self) -> ListIter<'_, T> {
        ListIter {
            list: self,
            current: self.head,
        }
    }

    fn allocate_node(&mut self, value: T, prev: usize, next: usize) -> usize {
        if let Some(index) = self.free_list.pop_back() {
            self.nodes[index] = Node::new(value, prev, next);
            index
        } else {
            self.nodes.push_back(Node::new(value, prev, next));
            self.nodes.len() - 1
        }
    }
}

pub struct ListIter<'a, T> {
    list: &'a List<T>,
    current: usize,
}

impl<'a, T> Iterator for ListIter<'a, T> {
    type Item = &'a T;
    fn next(&mut self) -> Option<&'a T> {
        if self.current == INVALID_INDEX {
            return None;
        }
        let node = &self.list.nodes[self.current];
        self.current = node.next;
        Some(&node.value)
    }
}
