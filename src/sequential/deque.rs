//! Double-ended queue matching `datapod::Deque<T>` from the C++ library.
//!
//! The C++ implementation pairs two vectors (front + back) so that both ends
//! amortise to O(1) while remaining serialisable via `members()`. We keep the
//! same layout on the Rust side with `Vector<T>` so that code inspecting the
//! internal representation behaves identically.

use crate::Vector;
use std::fmt;

#[derive(Clone, Default, PartialEq, Eq)]
pub struct Deque<T> {
    pub front_buf: Vector<T>, // reversed: front of deque = back of vector
    pub back_buf: Vector<T>,  // normal order
}

impl<T> Deque<T> {
    pub fn new() -> Self {
        Self {
            front_buf: Vector::new(),
            back_buf: Vector::new(),
        }
    }

    pub fn with_capacity_hint(cap: usize) -> Self {
        Self {
            front_buf: Vector::new(),
            back_buf: Vector::with_capacity(cap),
        }
    }

    pub fn len(&self) -> usize {
        self.front_buf.len() + self.back_buf.len()
    }

    pub fn size(&self) -> usize {
        self.len()
    }

    pub fn is_empty(&self) -> bool {
        self.front_buf.is_empty() && self.back_buf.is_empty()
    }

    pub fn empty(&self) -> bool {
        self.is_empty()
    }

    pub fn reserve(&mut self, additional: usize) {
        self.back_buf.reserve(additional);
    }

    pub fn shrink_to_fit(&mut self) {
        self.front_buf.shrink_to_fit();
        self.back_buf.shrink_to_fit();
    }

    pub fn clear(&mut self) {
        self.front_buf.clear();
        self.back_buf.clear();
    }

    pub fn push_front(&mut self, value: T) {
        self.front_buf.push_back(value);
    }

    pub fn push_back(&mut self, value: T) {
        self.back_buf.push_back(value);
    }

    pub fn get(&self, pos: usize) -> Option<&T> {
        if pos >= self.len() {
            return None;
        }
        if pos < self.front_buf.len() {
            Some(&self.front_buf[self.front_buf.len() - 1 - pos])
        } else {
            Some(&self.back_buf[pos - self.front_buf.len()])
        }
    }

    pub fn get_mut(&mut self, pos: usize) -> Option<&mut T> {
        if pos >= self.len() {
            return None;
        }
        if pos < self.front_buf.len() {
            let idx = self.front_buf.len() - 1 - pos;
            Some(&mut self.front_buf[idx])
        } else {
            let idx = pos - self.front_buf.len();
            Some(&mut self.back_buf[idx])
        }
    }

    pub fn at(&self, pos: usize) -> &T {
        self.get(pos).expect("Deque::at: index out of range")
    }

    pub fn at_mut(&mut self, pos: usize) -> &mut T {
        self.get_mut(pos).expect("Deque::at: index out of range")
    }

    pub fn front(&self) -> &T {
        if self.is_empty() {
            panic!("Deque::front: empty");
        }
        if let Some(last) = self.front_buf.last() {
            last
        } else {
            self.back_buf.first().unwrap()
        }
    }

    pub fn back(&self) -> &T {
        if self.is_empty() {
            panic!("Deque::back: empty");
        }
        if let Some(last) = self.back_buf.last() {
            last
        } else {
            self.front_buf.first().unwrap()
        }
    }

    pub fn front_mut(&mut self) -> &mut T {
        if self.is_empty() {
            panic!("Deque::front: empty");
        }
        if !self.front_buf.is_empty() {
            self.front_buf.last_mut().unwrap()
        } else {
            self.back_buf.first_mut().unwrap()
        }
    }

    pub fn back_mut(&mut self) -> &mut T {
        if self.is_empty() {
            panic!("Deque::back: empty");
        }
        if !self.back_buf.is_empty() {
            self.back_buf.last_mut().unwrap()
        } else {
            self.front_buf.first_mut().unwrap()
        }
    }

    pub fn pop_front(&mut self) -> Option<T> {
        if self.is_empty() {
            return None;
        }
        if self.front_buf.is_empty() {
            self.rebalance_to_front();
        }
        self.front_buf.pop_back()
    }

    pub fn pop_back(&mut self) -> Option<T> {
        if self.is_empty() {
            return None;
        }
        if self.back_buf.is_empty() {
            self.rebalance_to_back();
        }
        self.back_buf.pop_back()
    }

    pub fn iter(&self) -> DequeIter<'_, T> {
        DequeIter { dq: self, idx: 0 }
    }

    fn rebalance_to_front(&mut self)
    where
        T: Sized,
    {
        // Move first half of back_ to front_ (reversed).
        let n = self.back_buf.len();
        if n == 0 {
            return;
        }
        let half = (n / 2).max(1);
        let mut moved = Vec::with_capacity(n - half);
        for _ in 0..(n - half) {
            moved.push(self.back_buf.pop_back().unwrap());
        }
        moved.reverse();
        // Now self.back_ holds the first `half` items; transfer them in reverse
        // into front_.
        while let Some(v) = self.back_buf.pop_back() {
            self.front_buf.push_back(v);
        }
        for v in moved {
            self.back_buf.push_back(v);
        }
    }

    fn rebalance_to_back(&mut self)
    where
        T: Sized,
    {
        let n = self.front_buf.len();
        if n == 0 {
            return;
        }
        let half = (n / 2).max(1);
        // front_ order: index 0 .. n-1, logical order reversed. We want to
        // move the "newest" front_ items (near index 0) into back_ while
        // keeping the oldest items (near end) still in front_.
        let mut moved: Vec<T> = Vec::with_capacity(n - half);
        for _ in 0..(n - half) {
            moved.push(self.front_buf.pop_back().unwrap());
        }
        // The remaining `half` items at the front of front_ represent the
        // logical back. Drain them into back_ in logical order (reverse of
        // front_ storage order).
        let mut remaining: Vec<T> = Vec::with_capacity(self.front_buf.len());
        while let Some(v) = self.front_buf.pop_back() {
            remaining.push(v);
        }
        // remaining is now oldest-first (logical order of tail half).
        for v in remaining.into_iter().rev() {
            self.back_buf.push_back(v);
        }
        // Put the logical-front items back into front_ in their original
        // reversed order (most-recent push_front at the end of front_).
        for v in moved.into_iter().rev() {
            self.front_buf.push_back(v);
        }
    }
}

impl<T: Clone> Deque<T> {
    pub fn resize(&mut self, count: usize, value: T) {
        while self.len() > count {
            self.pop_back();
        }
        while self.len() < count {
            self.push_back(value.clone());
        }
    }
}

impl<T> std::ops::Index<usize> for Deque<T> {
    type Output = T;
    fn index(&self, pos: usize) -> &T {
        self.at(pos)
    }
}

impl<T> std::ops::IndexMut<usize> for Deque<T> {
    fn index_mut(&mut self, pos: usize) -> &mut T {
        self.at_mut(pos)
    }
}

impl<T: fmt::Debug> fmt::Debug for Deque<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_list().entries(self.iter()).finish()
    }
}

pub struct DequeIter<'a, T> {
    dq: &'a Deque<T>,
    idx: usize,
}

impl<'a, T> Iterator for DequeIter<'a, T> {
    type Item = &'a T;
    fn next(&mut self) -> Option<&'a T> {
        let out = self.dq.get(self.idx);
        if out.is_some() {
            self.idx += 1;
        }
        out
    }
}
