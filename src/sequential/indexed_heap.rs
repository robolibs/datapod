//! Indexed binary heap matching `datapod::IndexedHeap<Key, Priority, Compare>`.
//!
//! Unlike the plain `Heap`, this variant keeps a hash map from keys to
//! internal positions, enabling O(log n) decrease-key / update-priority /
//! erase operations — critical for graph search algorithms like Dijkstra and
//! A*. Default is a min-heap (smallest priority on top).
//!
//! The Rust surface exposes `IndexedHeap<Key, Priority = Key>` so existing
//! tests can spell it as `IndexedHeap<i32>` while still covering the common
//! case where keys double as priorities.

use crate::Vector;
use std::cmp::Ordering;
use std::collections::HashMap;
use std::fmt;
use std::hash::Hash;
use std::marker::PhantomData;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Entry<Key, Priority> {
    pub key: Key,
    pub priority: Priority,
}

pub struct IndexedHeap<Key, Priority = Key, Cmp = MinCompare>
where
    Key: Eq + Hash + Clone,
    Priority: Clone,
    Cmp: CompareOrdering,
{
    data: Vector<Entry<Key, Priority>>,
    index: HashMap<Key, usize>,
    _cmp: PhantomData<Cmp>,
}

/// Trait bound abstracting the compile-time comparator. Defaults to
/// `MinCompare` (smallest wins).
pub trait CompareOrdering {
    fn compare<T: PartialOrd>(a: &T, b: &T) -> Ordering;
}

#[derive(Debug, Clone, Copy, Default)]
pub struct MinCompare;

impl CompareOrdering for MinCompare {
    #[inline]
    fn compare<T: PartialOrd>(a: &T, b: &T) -> Ordering {
        a.partial_cmp(b).unwrap_or(Ordering::Equal)
    }
}

#[derive(Debug, Clone, Copy, Default)]
pub struct MaxCompare;

impl CompareOrdering for MaxCompare {
    #[inline]
    fn compare<T: PartialOrd>(a: &T, b: &T) -> Ordering {
        b.partial_cmp(a).unwrap_or(Ordering::Equal)
    }
}

impl<Key, Priority, Cmp> Default for IndexedHeap<Key, Priority, Cmp>
where
    Key: Eq + Hash + Clone,
    Priority: Clone,
    Cmp: CompareOrdering,
{
    fn default() -> Self {
        Self {
            data: Vector::new(),
            index: HashMap::new(),
            _cmp: PhantomData,
        }
    }
}

impl<Key, Priority, Cmp> IndexedHeap<Key, Priority, Cmp>
where
    Key: Eq + Hash + Clone,
    Priority: Clone + PartialOrd,
    Cmp: CompareOrdering,
{
    pub fn new() -> Self {
        Self::default()
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
        self.index.reserve(additional);
    }

    pub fn clear(&mut self) {
        self.data.clear();
        self.index.clear();
    }

    pub fn contains(&self, key: &Key) -> bool {
        self.index.contains_key(key)
    }

    pub fn priority(&self, key: &Key) -> Option<&Priority> {
        self.index.get(key).map(|&pos| &self.data[pos].priority)
    }

    pub fn top(&self) -> &Entry<Key, Priority> {
        if self.data.is_empty() {
            panic!("IndexedHeap::top: empty");
        }
        &self.data[0]
    }

    pub fn try_top(&self) -> Option<&Entry<Key, Priority>> {
        if self.data.is_empty() {
            None
        } else {
            Some(&self.data[0])
        }
    }

    pub fn push(&mut self, key: Key, priority: Priority) {
        if let Some(&pos) = self.index.get(&key) {
            let old = self.data[pos].priority.clone();
            self.data[pos].priority = priority.clone();
            if Cmp::compare(&priority, &old) == Ordering::Less {
                self.sift_up(pos);
            } else {
                self.sift_down(pos);
            }
        } else {
            let pos = self.data.len();
            self.index.insert(key.clone(), pos);
            self.data.push_back(Entry { key, priority });
            self.sift_up(pos);
        }
    }

    pub fn pop(&mut self) -> Entry<Key, Priority> {
        if self.data.is_empty() {
            panic!("IndexedHeap::pop: empty");
        }
        let last = self.data.len() - 1;
        self.swap_entries(0, last);
        let result = self.data.pop_back().unwrap();
        self.index.remove(&result.key);
        if !self.data.is_empty() {
            self.sift_down(0);
        }
        result
    }

    pub fn try_pop(&mut self) -> Option<Entry<Key, Priority>> {
        if self.is_empty() {
            None
        } else {
            Some(self.pop())
        }
    }

    pub fn update_priority(&mut self, key: &Key, new_priority: Priority) {
        let pos = match self.index.get(key) {
            Some(&pos) => pos,
            None => panic!("IndexedHeap::update_priority: key not found"),
        };
        let old = self.data[pos].priority.clone();
        self.data[pos].priority = new_priority.clone();
        if Cmp::compare(&new_priority, &old) == Ordering::Less {
            self.sift_up(pos);
        } else {
            self.sift_down(pos);
        }
    }

    pub fn decrease_key(&mut self, key: &Key, new_priority: Priority) {
        let pos = match self.index.get(key) {
            Some(&pos) => pos,
            None => panic!("IndexedHeap::decrease_key: key not found"),
        };
        if Cmp::compare(&new_priority, &self.data[pos].priority) != Ordering::Less {
            panic!("IndexedHeap::decrease_key: new priority is not less than current");
        }
        self.data[pos].priority = new_priority;
        self.sift_up(pos);
    }

    pub fn erase(&mut self, key: &Key) -> bool {
        let pos = match self.index.remove(key) {
            Some(pos) => pos,
            None => return false,
        };
        let last = self.data.len() - 1;
        if pos == last {
            self.data.pop_back();
            return true;
        }
        self.data.as_mut_slice().swap(pos, last);
        self.data.pop_back();
        // Update index for the moved entry.
        let moved_key = self.data[pos].key.clone();
        self.index.insert(moved_key, pos);
        // Restore heap property.
        if pos > 0 {
            let parent = (pos - 1) / 2;
            if Cmp::compare(&self.data[pos].priority, &self.data[parent].priority) == Ordering::Less
            {
                self.sift_up(pos);
                return true;
            }
        }
        self.sift_down(pos);
        true
    }

    pub fn iter(&self) -> std::slice::Iter<'_, Entry<Key, Priority>> {
        self.data.iter()
    }

    fn swap_entries(&mut self, i: usize, j: usize) {
        if i == j {
            return;
        }
        self.data.as_mut_slice().swap(i, j);
        let key_i = self.data[i].key.clone();
        let key_j = self.data[j].key.clone();
        self.index.insert(key_i, i);
        self.index.insert(key_j, j);
    }

    fn sift_up(&mut self, mut pos: usize) {
        while pos > 0 {
            let parent = (pos - 1) / 2;
            if Cmp::compare(&self.data[pos].priority, &self.data[parent].priority)
                == Ordering::Less
            {
                self.swap_entries(pos, parent);
                pos = parent;
            } else {
                break;
            }
        }
    }

    fn sift_down(&mut self, mut pos: usize) {
        let n = self.data.len();
        loop {
            let mut smallest = pos;
            let left = 2 * pos + 1;
            let right = 2 * pos + 2;
            if left < n
                && Cmp::compare(&self.data[left].priority, &self.data[smallest].priority)
                    == Ordering::Less
            {
                smallest = left;
            }
            if right < n
                && Cmp::compare(&self.data[right].priority, &self.data[smallest].priority)
                    == Ordering::Less
            {
                smallest = right;
            }
            if smallest != pos {
                self.swap_entries(pos, smallest);
                pos = smallest;
            } else {
                break;
            }
        }
    }
}

impl<Key, Priority, Cmp> Clone for IndexedHeap<Key, Priority, Cmp>
where
    Key: Eq + Hash + Clone,
    Priority: Clone,
    Cmp: CompareOrdering,
{
    fn clone(&self) -> Self {
        Self {
            data: self.data.clone(),
            index: self.index.clone(),
            _cmp: PhantomData,
        }
    }
}

impl<Key, Priority, Cmp> fmt::Debug for IndexedHeap<Key, Priority, Cmp>
where
    Key: Eq + Hash + Clone + fmt::Debug,
    Priority: Clone + fmt::Debug,
    Cmp: CompareOrdering,
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("IndexedHeap")
            .field("data", &self.data)
            .finish()
    }
}

impl<Key, Priority, Cmp> PartialEq for IndexedHeap<Key, Priority, Cmp>
where
    Key: Eq + Hash + Clone,
    Priority: Clone + PartialEq,
    Cmp: CompareOrdering,
{
    fn eq(&self, other: &Self) -> bool {
        self.data.len() == other.data.len()
            && self.data.iter().zip(other.data.iter()).all(|(a, b)| {
                a.key == b.key && a.priority == b.priority
            })
    }
}

pub type MinIndexedHeap<Key, Priority = Key> = IndexedHeap<Key, Priority, MinCompare>;
pub type MaxIndexedHeap<Key, Priority = Key> = IndexedHeap<Key, Priority, MaxCompare>;
