use super::Stamp;
use crate::Vector;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CircularBuffer<T> {
    pub values: Vector<T>,
    pub capacity: usize,
    pub head: usize,
    pub size: usize,
}

impl<T> Default for CircularBuffer<T> {
    fn default() -> Self {
        Self {
            values: Vector::new(),
            capacity: 0,
            head: 0,
            size: 0,
        }
    }
}

impl<T: Clone + Default> CircularBuffer<T> {
    pub fn with_capacity(capacity: usize) -> Self {
        let mut v = Vector::with_capacity(capacity);
        v.resize(capacity, T::default());
        Self {
            values: v,
            capacity,
            head: 0,
            size: 0,
        }
    }

    pub fn push(&mut self, value: T) {
        if self.capacity == 0 {
            return;
        }
        self.values[self.head] = value;
        self.head = (self.head + 1) % self.capacity;
        if self.size < self.capacity {
            self.size += 1;
        }
    }

    pub fn pop(&mut self) -> Option<T> {
        if self.size == 0 {
            return None;
        }
        let physical = self.physical_index(0);
        let out = self.values[physical].clone();
        if self.size == self.capacity {
            self.head = physical;
        }
        self.size -= 1;
        Some(out)
    }

    pub fn newest(&self) -> Option<T> {
        if self.size == 0 {
            return None;
        }
        let idx = (self.head + self.capacity - 1) % self.capacity;
        Some(self.values[idx].clone())
    }

    pub fn oldest(&self) -> Option<T> {
        if self.size == 0 {
            return None;
        }
        Some(self.values[self.physical_index(0)].clone())
    }
}

impl<T> CircularBuffer<T> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn len(&self) -> usize {
        self.size
    }

    pub fn size(&self) -> usize {
        self.size
    }

    pub fn is_empty(&self) -> bool {
        self.size == 0
    }

    pub fn full(&self) -> bool {
        self.size == self.capacity && self.capacity > 0
    }

    pub fn capacity(&self) -> usize {
        self.capacity
    }

    pub fn clear(&mut self) {
        self.head = 0;
        self.size = 0;
    }

    fn physical_index(&self, logical: usize) -> usize {
        if self.size < self.capacity {
            logical
        } else {
            (self.head + logical) % self.capacity
        }
    }

    pub fn get(&self, logical: usize) -> Option<&T> {
        if logical >= self.size {
            return None;
        }
        Some(&self.values[self.physical_index(logical)])
    }

    pub fn iter(&self) -> CircularBufferIter<'_, T> {
        CircularBufferIter {
            buffer: self,
            logical: 0,
        }
    }
}

pub struct CircularBufferIter<'a, T> {
    buffer: &'a CircularBuffer<T>,
    logical: usize,
}

impl<'a, T> Iterator for CircularBufferIter<'a, T> {
    type Item = &'a T;

    fn next(&mut self) -> Option<&'a T> {
        if self.logical >= self.buffer.size {
            return None;
        }
        let idx = self.buffer.physical_index(self.logical);
        self.logical += 1;
        Some(&self.buffer.values[idx])
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct CircularTimeBuffer<T> {
    pub timestamps: Vector<i64>,
    pub values: Vector<T>,
    pub head: usize,
    pub size: usize,
    pub capacity: usize,
}

impl<T> Default for CircularTimeBuffer<T> {
    fn default() -> Self {
        Self {
            timestamps: Vector::new(),
            values: Vector::new(),
            head: 0,
            size: 0,
            capacity: 0,
        }
    }
}

impl<T: Clone + Default> CircularTimeBuffer<T> {
    pub fn with_capacity(capacity: usize) -> Self {
        let mut timestamps = Vector::with_capacity(capacity);
        timestamps.resize(capacity, 0);
        let mut values = Vector::with_capacity(capacity);
        values.resize(capacity, T::default());
        Self {
            timestamps,
            values,
            head: 0,
            size: 0,
            capacity,
        }
    }

    pub fn push(&mut self, ts: i64, value: T) {
        if self.capacity == 0 {
            return;
        }
        self.timestamps[self.head] = ts;
        self.values[self.head] = value;
        self.head = (self.head + 1) % self.capacity;
        if self.size < self.capacity {
            self.size += 1;
        }
    }

    pub fn push_stamped(&mut self, stamp: Stamp, value: T) {
        self.push(stamp.nanos, value);
    }

    pub fn newest(&self) -> Option<(i64, T)> {
        if self.size == 0 {
            return None;
        }
        let idx = (self.head + self.capacity - 1) % self.capacity;
        Some((self.timestamps[idx], self.values[idx].clone()))
    }

    pub fn oldest(&self) -> Option<(i64, T)> {
        if self.size == 0 {
            return None;
        }
        let idx = self.physical_index(0);
        Some((self.timestamps[idx], self.values[idx].clone()))
    }

    pub fn at(&self, logical: usize) -> Option<(i64, T)> {
        if logical >= self.size {
            return None;
        }
        let idx = self.physical_index(logical);
        Some((self.timestamps[idx], self.values[idx].clone()))
    }
}

impl<T> CircularTimeBuffer<T> {
    pub fn len(&self) -> usize {
        self.size
    }

    pub fn is_empty(&self) -> bool {
        self.size == 0
    }

    pub fn full(&self) -> bool {
        self.size == self.capacity && self.capacity > 0
    }

    pub fn capacity(&self) -> usize {
        self.capacity
    }

    pub fn clear(&mut self) {
        self.head = 0;
        self.size = 0;
    }

    fn physical_index(&self, logical: usize) -> usize {
        if self.size < self.capacity {
            logical
        } else {
            (self.head + logical) % self.capacity
        }
    }

    pub fn duration(&self) -> i64 {
        if self.size < 2 {
            return 0;
        }
        let newest = self.timestamps[(self.head + self.capacity - 1) % self.capacity];
        let oldest = self.timestamps[self.physical_index(0)];
        newest - oldest
    }

    pub fn start_time(&self) -> i64 {
        if self.size == 0 {
            return 0;
        }
        self.timestamps[self.physical_index(0)]
    }

    pub fn end_time(&self) -> i64 {
        if self.size == 0 {
            return 0;
        }
        self.timestamps[(self.head + self.capacity - 1) % self.capacity]
    }
}
