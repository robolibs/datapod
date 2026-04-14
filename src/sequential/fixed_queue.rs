//! Fixed-capacity FIFO matching `datapod::FixedQueue<T, N, OverwriteOnFull>`.
//!
//! The C++ version is a static ring buffer backed by `Array<Optional<T>, N>`
//! with compile-time capacity and an optional overwrite-on-full mode. We match
//! the layout with `Vec<Option<T>>` sized to `capacity`, so the type still has
//! a no-argument `Default` for the test matrix while letting callers override
//! the capacity at runtime via `with_capacity`.

use std::collections::VecDeque;
use std::marker::PhantomData;

/// Tag marker — drop oldest element when pushing into a full queue.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Overwrite;

/// Tag marker — reject new elements when the queue is full.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct NoOverwrite;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FixedQueue<T, Policy = NoOverwrite> {
    pub values: VecDeque<T>,
    pub capacity: usize,
    _policy: PhantomData<Policy>,
}

impl<T, Policy> Default for FixedQueue<T, Policy> {
    fn default() -> Self {
        Self {
            values: VecDeque::new(),
            capacity: 0,
            _policy: PhantomData,
        }
    }
}

impl<T, Policy> FixedQueue<T, Policy> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_capacity(capacity: usize) -> Self {
        Self {
            values: VecDeque::with_capacity(capacity),
            capacity,
            _policy: PhantomData,
        }
    }

    pub fn capacity(&self) -> usize {
        self.capacity
    }

    pub fn size(&self) -> usize {
        self.values.len()
    }

    pub fn len(&self) -> usize {
        self.values.len()
    }

    pub fn is_empty(&self) -> bool {
        self.values.is_empty()
    }

    pub fn empty(&self) -> bool {
        self.is_empty()
    }

    pub fn full(&self) -> bool {
        self.capacity > 0 && self.values.len() >= self.capacity
    }

    pub fn clear(&mut self) {
        self.values.clear();
    }

    pub fn front(&self) -> &T {
        self.values.front().expect("FixedQueue::front: empty")
    }

    pub fn front_mut(&mut self) -> &mut T {
        self.values.front_mut().expect("FixedQueue::front: empty")
    }

    pub fn back(&self) -> &T {
        self.values.back().expect("FixedQueue::back: empty")
    }

    pub fn back_mut(&mut self) -> &mut T {
        self.values.back_mut().expect("FixedQueue::back: empty")
    }

    pub fn try_front(&self) -> Option<&T> {
        self.values.front()
    }

    pub fn try_back(&self) -> Option<&T> {
        self.values.back()
    }

    pub fn pop(&mut self) -> T {
        self.values.pop_front().expect("FixedQueue::pop: empty")
    }

    pub fn try_pop(&mut self) -> Option<T> {
        self.values.pop_front()
    }

    pub fn iter(&self) -> std::collections::vec_deque::Iter<'_, T> {
        self.values.iter()
    }

    pub fn iter_mut(&mut self) -> std::collections::vec_deque::IterMut<'_, T> {
        self.values.iter_mut()
    }
}

impl<T> FixedQueue<T, NoOverwrite> {
    pub fn try_push(&mut self, value: T) -> bool {
        if self.full() {
            return false;
        }
        self.values.push_back(value);
        true
    }

    pub fn push(&mut self, value: T) {
        if !self.try_push(value) {
            panic!("FixedQueue::push: full");
        }
    }
}

impl<T> FixedQueue<T, Overwrite> {
    pub fn try_push(&mut self, value: T) -> bool {
        if self.capacity == 0 {
            return false;
        }
        if self.values.len() >= self.capacity {
            self.values.pop_front();
        }
        self.values.push_back(value);
        true
    }

    pub fn push(&mut self, value: T) {
        self.try_push(value);
    }
}

/// Non-overwriting alias mirroring the C++ `FixedFifo<T, N>`.
pub type FixedFifo<T> = FixedQueue<T, NoOverwrite>;

/// Overwriting alias mirroring the C++ `OverwritingFifo<T, N>`.
pub type OverwritingFifo<T> = FixedQueue<T, Overwrite>;
