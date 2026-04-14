//! Singly linked list matching `datapod::ForwardList<T>`.
//!
//! Like `List<T>` the C++ version uses index-based nodes so it can be
//! serialised without dealing with raw pointers. We keep the same layout on
//! the Rust side.

use crate::Vector;

pub const INVALID_INDEX: usize = usize::MAX;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Node<T> {
    pub value: T,
    pub next: usize,
}

impl<T> Node<T> {
    pub fn new(value: T, next: usize) -> Self {
        Self { value, next }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ForwardList<T> {
    pub nodes: Vector<Node<T>>,
    pub head: usize,
    pub size_: usize,
    pub free_list: Vector<usize>,
}

impl<T> Default for ForwardList<T> {
    fn default() -> Self {
        Self {
            nodes: Vector::new(),
            head: INVALID_INDEX,
            size_: 0,
            free_list: Vector::new(),
        }
    }
}

impl<T> ForwardList<T> {
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
            panic!("ForwardList::front: empty");
        }
        &self.nodes[self.head].value
    }

    pub fn front_mut(&mut self) -> &mut T {
        if self.is_empty() {
            panic!("ForwardList::front: empty");
        }
        let h = self.head;
        &mut self.nodes[h].value
    }

    pub fn push_front(&mut self, value: T) -> usize {
        let new_index = self.allocate_node(value, self.head);
        self.head = new_index;
        self.size_ += 1;
        new_index
    }

    pub fn pop_front(&mut self) -> Option<()> {
        if self.is_empty() {
            return None;
        }
        let old = self.head;
        self.head = self.nodes[old].next;
        self.free_list.push_back(old);
        self.size_ -= 1;
        Some(())
    }

    pub fn insert_after(&mut self, pos_index: usize, value: T) -> usize {
        if pos_index == INVALID_INDEX {
            panic!("ForwardList::insert_after: invalid iterator");
        }
        let next = self.nodes[pos_index].next;
        let new_index = self.allocate_node(value, next);
        self.nodes[pos_index].next = new_index;
        self.size_ += 1;
        new_index
    }

    pub fn erase_after(&mut self, pos_index: usize) -> usize {
        if pos_index == INVALID_INDEX {
            panic!("ForwardList::erase_after: invalid iterator");
        }
        let to_erase = self.nodes[pos_index].next;
        if to_erase == INVALID_INDEX {
            panic!("ForwardList::erase_after: nothing to erase");
        }
        self.nodes[pos_index].next = self.nodes[to_erase].next;
        self.free_list.push_back(to_erase);
        self.size_ -= 1;
        self.nodes[pos_index].next
    }

    pub fn clear(&mut self) {
        self.nodes.clear();
        self.free_list.clear();
        self.head = INVALID_INDEX;
        self.size_ = 0;
    }

    pub fn reverse(&mut self) {
        let mut prev = INVALID_INDEX;
        let mut current = self.head;
        while current != INVALID_INDEX {
            let next = self.nodes[current].next;
            self.nodes[current].next = prev;
            prev = current;
            current = next;
        }
        self.head = prev;
    }

    pub fn iter(&self) -> ForwardListIter<'_, T> {
        ForwardListIter {
            list: self,
            current: self.head,
        }
    }

    fn allocate_node(&mut self, value: T, next: usize) -> usize {
        if let Some(index) = self.free_list.pop_back() {
            self.nodes[index] = Node::new(value, next);
            index
        } else {
            self.nodes.push_back(Node::new(value, next));
            self.nodes.len() - 1
        }
    }
}

pub struct ForwardListIter<'a, T> {
    list: &'a ForwardList<T>,
    current: usize,
}

impl<'a, T> Iterator for ForwardListIter<'a, T> {
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
