use super::Stamp;
use super::Stamped;
use super::stamp::now_nanos;
use crate::Vector;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Window<T> {
    pub start: Stamp,
    pub end: Stamp,
    pub values: Vector<T>,
}

impl<T> Window<T> {
    pub fn new(start: Stamp, end: Stamp) -> Self {
        Self {
            start,
            end,
            values: Vector::new(),
        }
    }

    pub fn duration(&self) -> i64 {
        self.end.nanos - self.start.nanos
    }

    pub fn contains(&self, ts: i64) -> bool {
        ts >= self.start.nanos && ts < self.end.nanos
    }

    pub fn is_valid(&self) -> bool {
        self.start.nanos <= self.end.nanos
    }

    pub fn len(&self) -> usize {
        self.values.len()
    }

    pub fn is_empty(&self) -> bool {
        self.values.is_empty()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct TimeWindow {
    pub start: i64,
    pub end: i64,
}

impl TimeWindow {
    pub const fn new(start: i64, end: i64) -> Self {
        Self { start, end }
    }

    pub const fn contains(&self, ts: i64) -> bool {
        ts >= self.start && ts < self.end
    }

    pub const fn overlaps(&self, other: &TimeWindow) -> bool {
        self.start < other.end && other.start < self.end
    }

    pub const fn duration(&self) -> i64 {
        self.end - self.start
    }

    pub const fn is_valid(&self) -> bool {
        self.start <= self.end
    }

    pub fn last_n_seconds(n: i64) -> Self {
        let now = now_nanos();
        Self {
            start: now - n * 1_000_000_000,
            end: now,
        }
    }

    pub fn last_n_minutes(n: i64) -> Self {
        let now = now_nanos();
        Self {
            start: now - n * 60_000_000_000,
            end: now,
        }
    }

    pub fn last_n_hours(n: i64) -> Self {
        let now = now_nanos();
        Self {
            start: now - n * 3_600_000_000_000,
            end: now,
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct SlidingWindow<T> {
    pub window_size_ns: i64,
    pub slide_interval_ns: i64,
    pub buffer: Vector<Stamped<T>>,
}

impl<T> Default for SlidingWindow<T> {
    fn default() -> Self {
        Self {
            window_size_ns: 0,
            slide_interval_ns: 0,
            buffer: Vector::new(),
        }
    }
}

impl<T: Clone> SlidingWindow<T> {
    pub fn new(window_size_ns: i64, slide_interval_ns: i64) -> Self {
        let slide = if slide_interval_ns > 0 {
            slide_interval_ns
        } else {
            window_size_ns
        };
        Self {
            window_size_ns,
            slide_interval_ns: slide,
            buffer: Vector::new(),
        }
    }

    pub fn insert(&mut self, ts: i64, value: T) {
        self.buffer.push(Stamped::new(ts, value));
        self.expire_old(ts);
    }

    pub fn insert_stamped(&mut self, stamped: Stamped<T>) {
        let ts = stamped.timestamp;
        self.buffer.push(stamped);
        self.expire_old(ts);
    }

    pub fn expire_old(&mut self, current_time: i64) {
        let cutoff = current_time - self.window_size_ns;
        let mut i = 0;
        while i < self.buffer.len() && self.buffer[i].timestamp < cutoff {
            i += 1;
        }
        if i > 0 {
            let new_len = self.buffer.len() - i;
            for j in 0..new_len {
                let moved = self.buffer[j + i].clone();
                self.buffer[j] = moved;
            }
            self.buffer.truncate(new_len);
        }
    }

    pub fn clear(&mut self) {
        self.buffer.clear();
    }

    pub fn size(&self) -> usize {
        self.buffer.len()
    }

    pub fn len(&self) -> usize {
        self.buffer.len()
    }

    pub fn is_empty(&self) -> bool {
        self.buffer.is_empty()
    }

    pub fn data(&self) -> &Vector<Stamped<T>> {
        &self.buffer
    }

    pub fn current_window(&self, current_time: i64) -> TimeWindow {
        TimeWindow::new(current_time - self.window_size_ns, current_time)
    }
}

impl SlidingWindow<f64> {
    pub fn sum(&self) -> f64 {
        self.buffer.iter().map(|s| s.value).sum()
    }

    pub fn mean(&self) -> f64 {
        if self.buffer.is_empty() {
            return 0.0;
        }
        self.sum() / self.buffer.len() as f64
    }

    pub fn min(&self) -> f64 {
        self.buffer
            .iter()
            .map(|s| s.value)
            .fold(f64::INFINITY, f64::min)
    }

    pub fn max(&self) -> f64 {
        self.buffer
            .iter()
            .map(|s| s.value)
            .fold(f64::NEG_INFINITY, f64::max)
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct TumblingWindow<T> {
    pub window_size_ns: i64,
    pub current_window_start: i64,
    pub current_batch: Vector<Stamped<T>>,
}

impl<T> Default for TumblingWindow<T> {
    fn default() -> Self {
        Self {
            window_size_ns: 0,
            current_window_start: 0,
            current_batch: Vector::new(),
        }
    }
}

impl<T: Clone> TumblingWindow<T> {
    pub fn new(window_size_ns: i64) -> Self {
        Self {
            window_size_ns,
            current_window_start: 0,
            current_batch: Vector::new(),
        }
    }

    pub fn insert(&mut self, ts: i64, value: T) {
        if self.current_window_start == 0 {
            self.current_window_start = (ts / self.window_size_ns) * self.window_size_ns;
        }
        while ts >= self.current_window_start + self.window_size_ns {
            self.current_window_start += self.window_size_ns;
            self.current_batch.clear();
        }
        self.current_batch.push(Stamped::new(ts, value));
    }

    pub fn insert_stamped(&mut self, stamped: Stamped<T>) {
        self.insert(stamped.timestamp, stamped.value);
    }

    pub fn is_window_complete(&self, current_time: i64) -> bool {
        if self.current_window_start == 0 {
            return false;
        }
        current_time >= self.current_window_start + self.window_size_ns
    }

    pub fn flush(&mut self) -> Vector<Stamped<T>> {
        let result = self.current_batch.clone();
        self.current_batch.clear();
        self.current_window_start += self.window_size_ns;
        result
    }

    pub fn clear(&mut self) {
        self.current_batch.clear();
        self.current_window_start = 0;
    }

    pub fn size(&self) -> usize {
        self.current_batch.len()
    }

    pub fn len(&self) -> usize {
        self.current_batch.len()
    }

    pub fn is_empty(&self) -> bool {
        self.current_batch.is_empty()
    }

    pub fn data(&self) -> &Vector<Stamped<T>> {
        &self.current_batch
    }

    pub fn current_window(&self) -> TimeWindow {
        TimeWindow::new(
            self.current_window_start,
            self.current_window_start + self.window_size_ns,
        )
    }
}
