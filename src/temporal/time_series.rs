use super::Stamped;
use crate::Vector;

#[derive(Debug, Clone, PartialEq)]
pub struct TimeSeries<T> {
    pub timestamps: Vector<i64>,
    pub values: Vector<T>,
}

impl<T> Default for TimeSeries<T> {
    fn default() -> Self {
        Self {
            timestamps: Vector::new(),
            values: Vector::new(),
        }
    }
}

impl<T: Clone> TimeSeries<T> {
    pub fn new() -> Self {
        Self {
            timestamps: Vector::new(),
            values: Vector::new(),
        }
    }

    pub fn with_capacity(capacity: usize) -> Self {
        Self {
            timestamps: Vector::with_capacity(capacity),
            values: Vector::with_capacity(capacity),
        }
    }

    pub fn from_stamps(stamps: &[Stamped<T>]) -> Self {
        let mut out = Self::with_capacity(stamps.len());
        for s in stamps {
            out.timestamps.push(s.timestamp);
            out.values.push(s.value.clone());
        }
        out
    }

    pub fn size(&self) -> usize {
        self.timestamps.len()
    }

    pub fn len(&self) -> usize {
        self.timestamps.len()
    }

    pub fn is_empty(&self) -> bool {
        self.timestamps.is_empty()
    }

    pub fn reserve(&mut self, n: usize) {
        self.timestamps.reserve(n);
        self.values.reserve(n);
    }

    pub fn capacity(&self) -> usize {
        self.timestamps.capacity().min(self.values.capacity())
    }

    pub fn clear(&mut self) {
        self.timestamps.clear();
        self.values.clear();
    }

    pub fn append(&mut self, ts: i64, value: T) {
        self.timestamps.push(ts);
        self.values.push(value);
    }

    pub fn append_stamped(&mut self, stamp: Stamped<T>) {
        self.timestamps.push(stamp.timestamp);
        self.values.push(stamp.value);
    }

    pub fn append_many(&mut self, stamps: &[Stamped<T>]) {
        self.reserve(self.len() + stamps.len());
        for s in stamps {
            self.timestamps.push(s.timestamp);
            self.values.push(s.value.clone());
        }
    }

    pub fn at(&self, i: usize) -> Option<Stamped<T>> {
        if i >= self.len() {
            return None;
        }
        Some(Stamped::new(self.timestamps[i], self.values[i].clone()))
    }

    pub fn front(&self) -> Option<Stamped<T>> {
        if self.is_empty() {
            return None;
        }
        Some(Stamped::new(self.timestamps[0], self.values[0].clone()))
    }

    pub fn back(&self) -> Option<Stamped<T>> {
        if self.is_empty() {
            return None;
        }
        let i = self.len() - 1;
        Some(Stamped::new(self.timestamps[i], self.values[i].clone()))
    }

    pub fn query(&self, start: i64, end: i64) -> (usize, usize) {
        if self.is_empty() {
            return (0, 0);
        }
        let ts = self.timestamps.as_slice();
        let start_idx = ts.partition_point(|&t| t < start);
        let end_idx = ts.partition_point(|&t| t < end);
        (start_idx, end_idx)
    }

    pub fn query_slice(&self, start: i64, end: i64) -> (&[i64], &[T]) {
        let (s, e) = self.query(start, end);
        (&self.timestamps.as_slice()[s..e], &self.values.as_slice()[s..e])
    }

    pub fn is_sorted(&self) -> bool {
        let ts = self.timestamps.as_slice();
        ts.windows(2).all(|w| w[0] <= w[1])
    }

    pub fn sort_by_time(&mut self) {
        if self.len() <= 1 {
            return;
        }
        let mut indices: Vec<usize> = (0..self.len()).collect();
        indices.sort_by_key(|&i| self.timestamps[i]);

        let mut sorted_ts: Vec<i64> = Vec::with_capacity(self.len());
        let mut sorted_vals: Vec<T> = Vec::with_capacity(self.len());
        for &i in &indices {
            sorted_ts.push(self.timestamps[i]);
            sorted_vals.push(self.values[i].clone());
        }
        self.timestamps = Vector::from(sorted_ts);
        self.values = Vector::from(sorted_vals);
    }

    pub fn duration(&self) -> i64 {
        if self.len() < 2 {
            return 0;
        }
        self.timestamps[self.len() - 1] - self.timestamps[0]
    }

    pub fn start_time(&self) -> i64 {
        if self.is_empty() { 0 } else { self.timestamps[0] }
    }

    pub fn end_time(&self) -> i64 {
        if self.is_empty() {
            0
        } else {
            self.timestamps[self.len() - 1]
        }
    }

    pub fn downsample(&self, n: usize) -> Self {
        if n <= 1 {
            return self.clone();
        }
        let mut out = Self::with_capacity((self.len() + n - 1) / n);
        let mut i = 0;
        while i < self.len() {
            out.timestamps.push(self.timestamps[i]);
            out.values.push(self.values[i].clone());
            i += n;
        }
        out
    }

    pub fn to_stamps(&self) -> Vector<Stamped<T>> {
        let mut out = Vector::with_capacity(self.len());
        for i in 0..self.len() {
            out.push(Stamped::new(self.timestamps[i], self.values[i].clone()));
        }
        out
    }
}

impl TimeSeries<f64> {
    pub fn mean(&self) -> f64 {
        if self.is_empty() {
            return 0.0;
        }
        self.values.iter().copied().sum::<f64>() / self.len() as f64
    }

    pub fn sum(&self) -> f64 {
        self.values.iter().copied().sum()
    }

    pub fn min(&self) -> f64 {
        self.values
            .iter()
            .copied()
            .fold(f64::INFINITY, f64::min)
    }

    pub fn max(&self) -> f64 {
        self.values
            .iter()
            .copied()
            .fold(f64::NEG_INFINITY, f64::max)
    }

    pub fn time_at_min(&self) -> i64 {
        if self.is_empty() {
            return 0;
        }
        let mut best_i = 0;
        let mut best = self.values[0];
        for i in 1..self.len() {
            if self.values[i] < best {
                best = self.values[i];
                best_i = i;
            }
        }
        self.timestamps[best_i]
    }

    pub fn time_at_max(&self) -> i64 {
        if self.is_empty() {
            return 0;
        }
        let mut best_i = 0;
        let mut best = self.values[0];
        for i in 1..self.len() {
            if self.values[i] > best {
                best = self.values[i];
                best_i = i;
            }
        }
        self.timestamps[best_i]
    }
}
