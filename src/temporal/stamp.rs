use std::cmp::Ordering;
use std::time::{SystemTime, UNIX_EPOCH};

#[repr(C)]
#[derive(
    Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default,
    bytemuck::Pod, bytemuck::Zeroable,
)]
pub struct Stamp {
    pub nanos: i64,
}

impl Stamp {
    pub const fn new(nanos: i64) -> Self {
        Self { nanos }
    }

    pub fn now() -> Self {
        Self { nanos: now_nanos() }
    }

    pub const fn timestamp(&self) -> i64 {
        self.nanos
    }

    pub fn age(&self) -> i64 {
        now_nanos() - self.nanos
    }

    pub fn seconds(&self) -> f64 {
        self.nanos as f64 / 1e9
    }

    pub const fn milliseconds(&self) -> i64 {
        self.nanos / 1_000_000
    }

    pub const fn microseconds(&self) -> i64 {
        self.nanos / 1_000
    }

    pub fn from_seconds_f64(seconds: f64) -> Self {
        Self {
            nanos: (seconds * 1e9) as i64,
        }
    }

    pub const fn from_milliseconds(ms: i64) -> Self {
        Self {
            nanos: ms * 1_000_000,
        }
    }

    pub const fn from_microseconds(us: i64) -> Self {
        Self { nanos: us * 1_000 }
    }
}

#[inline]
pub fn now_nanos() -> i64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_nanos() as i64)
        .unwrap_or(0)
}

#[derive(Debug, Clone, Copy, Default)]
pub struct Stamped<T> {
    pub timestamp: i64,
    pub value: T,
}

impl<T> Stamped<T> {
    pub const fn new(timestamp: i64, value: T) -> Self {
        Self { timestamp, value }
    }

    pub fn with_now(value: T) -> Self {
        Self {
            timestamp: now_nanos(),
            value,
        }
    }

    pub fn age(&self) -> i64 {
        now_nanos() - self.timestamp
    }

    pub fn seconds(&self) -> f64 {
        self.timestamp as f64 / 1e9
    }

    pub const fn milliseconds(&self) -> i64 {
        self.timestamp / 1_000_000
    }

    pub const fn microseconds(&self) -> i64 {
        self.timestamp / 1_000
    }

    pub fn from_seconds(seconds: f64, value: T) -> Self {
        Self {
            timestamp: (seconds * 1e9) as i64,
            value,
        }
    }

    pub const fn from_milliseconds(ms: i64, value: T) -> Self {
        Self {
            timestamp: ms * 1_000_000,
            value,
        }
    }

    pub const fn from_microseconds(us: i64, value: T) -> Self {
        Self {
            timestamp: us * 1_000,
            value,
        }
    }
}

impl<T: PartialEq> PartialEq for Stamped<T> {
    fn eq(&self, other: &Self) -> bool {
        self.timestamp == other.timestamp
    }
}

impl<T: Eq> Eq for Stamped<T> {}

impl<T> PartialOrd for Stamped<T>
where
    T: PartialEq,
{
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.timestamp.cmp(&other.timestamp))
    }
}

impl<T> Ord for Stamped<T>
where
    T: Eq,
{
    fn cmp(&self, other: &Self) -> Ordering {
        self.timestamp.cmp(&other.timestamp)
    }
}

pub type StampedDouble = Stamped<f64>;
pub type StampedFloat = Stamped<f32>;
pub type StampedInt = Stamped<i32>;
pub type StampedLong = Stamped<i64>;
