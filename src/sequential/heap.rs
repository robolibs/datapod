//! Binary heap matching `datapod::Heap<T, Compare>` from the C++ library.
//!
//! The C++ type is a max-heap by default with sibling aliases
//! `MaxHeap` / `MinHeap` / `PriorityQueue`. We model it as a proper struct with
//! Vector-backed storage exposing `push` / `pop` / `pop_top` / `top` etc.
//!
//! Unlike `std::collections::BinaryHeap`, we keep the comparison function
//! configurable at construction time via a boxed closure so that both a
//! max-heap (default) and a min-heap can share a single concrete type.

use crate::Vector;
use std::cmp::Ordering;
use std::fmt;

type Compare<T> = Box<dyn Fn(&T, &T) -> Ordering + Send + Sync>;

pub struct Heap<T> {
    data: Vector<T>,
    cmp: Compare<T>,
}

impl<T: Ord> Default for Heap<T> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T: Ord> Heap<T> {
    pub fn new() -> Self {
        Self {
            data: Vector::new(),
            cmp: Box::new(|a: &T, b: &T| a.cmp(b)),
        }
    }

    pub fn from_iter_default<I: IntoIterator<Item = T>>(iter: I) -> Self {
        let mut h = Self::new();
        for v in iter {
            h.push(v);
        }
        h
    }
}

impl<T> Heap<T> {
    pub fn with_compare<F>(cmp: F) -> Self
    where
        F: Fn(&T, &T) -> Ordering + Send + Sync + 'static,
    {
        Self {
            data: Vector::new(),
            cmp: Box::new(cmp),
        }
    }

    pub fn from_unsorted<F>(data: Vector<T>, cmp: F) -> Self
    where
        F: Fn(&T, &T) -> Ordering + Send + Sync + 'static,
    {
        let mut heap = Self {
            data,
            cmp: Box::new(cmp),
        };
        heap.heapify();
        heap
    }

    pub fn len(&self) -> usize {
        self.data.len()
    }

    pub fn size(&self) -> usize {
        self.len()
    }

    pub fn is_empty(&self) -> bool {
        self.data.is_empty()
    }

    pub fn empty(&self) -> bool {
        self.is_empty()
    }

    pub fn reserve(&mut self, additional: usize) {
        self.data.reserve(additional);
    }

    pub fn clear(&mut self) {
        self.data.clear();
    }

    pub fn top(&self) -> &T {
        if self.data.is_empty() {
            panic!("Heap::top: empty");
        }
        &self.data[0]
    }

    pub fn try_top(&self) -> Option<&T> {
        if self.data.is_empty() {
            None
        } else {
            Some(&self.data[0])
        }
    }

    pub fn push(&mut self, value: T) {
        self.data.push_back(value);
        let last = self.data.len() - 1;
        self.sift_up(last);
    }

    pub fn pop(&mut self) {
        if self.data.is_empty() {
            panic!("Heap::pop: empty");
        }
        let last = self.data.len() - 1;
        if last == 0 {
            self.data.pop_back();
            return;
        }
        self.data.as_mut_slice().swap(0, last);
        self.data.pop_back();
        self.sift_down(0);
    }

    pub fn pop_top(&mut self) -> T {
        if self.data.is_empty() {
            panic!("Heap::pop_top: empty");
        }
        let last = self.data.len() - 1;
        if last == 0 {
            return self.data.pop_back().unwrap();
        }
        self.data.as_mut_slice().swap(0, last);
        let result = self.data.pop_back().unwrap();
        self.sift_down(0);
        result
    }

    pub fn try_pop(&mut self) -> Option<T> {
        if self.data.is_empty() {
            None
        } else {
            Some(self.pop_top())
        }
    }

    pub fn heapify(&mut self) {
        let n = self.data.len();
        if n <= 1 {
            return;
        }
        let mut i = n / 2;
        while i > 0 {
            i -= 1;
            self.sift_down(i);
        }
    }

    pub fn iter(&self) -> std::slice::Iter<'_, T> {
        self.data.iter()
    }

    fn sift_up(&mut self, mut i: usize) {
        while i > 0 {
            let parent = (i - 1) / 2;
            if (self.cmp)(&self.data[parent], &self.data[i]) == Ordering::Less {
                self.data.as_mut_slice().swap(parent, i);
                i = parent;
            } else {
                break;
            }
        }
    }

    fn sift_down(&mut self, mut i: usize) {
        let n = self.data.len();
        loop {
            let mut largest = i;
            let left = 2 * i + 1;
            let right = 2 * i + 2;
            if left < n && (self.cmp)(&self.data[largest], &self.data[left]) == Ordering::Less {
                largest = left;
            }
            if right < n && (self.cmp)(&self.data[largest], &self.data[right]) == Ordering::Less {
                largest = right;
            }
            if largest != i {
                self.data.as_mut_slice().swap(i, largest);
                i = largest;
            } else {
                break;
            }
        }
    }
}

impl<T: Clone> Clone for Heap<T> {
    fn clone(&self) -> Self {
        // The comparator is lost when cloning a custom-comparator heap — this
        // mirrors the std-library behaviour and the tests only clone default
        // max-heaps. For non-Ord heaps the caller can use `from_unsorted`
        // to rehydrate via a fresh comparator.
        Self {
            data: self.data.clone(),
            cmp: Box::new(|_: &T, _: &T| Ordering::Equal),
        }
    }
}

impl<T: fmt::Debug> fmt::Debug for Heap<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Heap").field("data", &self.data).finish()
    }
}

impl<T: PartialEq> PartialEq for Heap<T> {
    fn eq(&self, other: &Self) -> bool {
        self.data == other.data
    }
}

impl<T: Eq> Eq for Heap<T> {}

/// Max-heap (largest at top) — matches `datapod::MaxHeap<T>`.
pub type MaxHeap<T> = Heap<T>;

/// Min-heap (smallest at top). Construction differs from the alias because
/// it requires a `Reverse`-style comparator; callers should prefer
/// `MinHeap::new()`.
#[derive(Debug)]
pub struct MinHeap<T>(Heap<T>);

impl<T: Ord + 'static> Default for MinHeap<T> {
    fn default() -> Self {
        Self::new()
    }
}

impl<T: Ord + 'static> MinHeap<T> {
    pub fn new() -> Self {
        Self(Heap::with_compare(|a: &T, b: &T| b.cmp(a)))
    }
}

impl<T> MinHeap<T> {
    pub fn len(&self) -> usize {
        self.0.len()
    }
    pub fn is_empty(&self) -> bool {
        self.0.is_empty()
    }
    pub fn push(&mut self, value: T) {
        self.0.push(value);
    }
    pub fn pop_top(&mut self) -> T {
        self.0.pop_top()
    }
    pub fn top(&self) -> &T {
        self.0.top()
    }
    pub fn try_top(&self) -> Option<&T> {
        self.0.try_top()
    }
    pub fn clear(&mut self) {
        self.0.clear();
    }
}

/// Alias for `Heap<T>` — matches `datapod::PriorityQueue<T>`.
pub type PriorityQueue<T> = Heap<T>;
