use std::collections::VecDeque;

use crate::Vector;

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Default)]
pub struct Stamp {
    pub nanos: i64,
}

impl Stamp {
    pub fn new(nanos: i64) -> Self {
        Self { nanos }
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Event<T> {
    pub stamp: Stamp,
    pub value: T,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Window<T> {
    pub start: Stamp,
    pub end: Stamp,
    pub values: Vector<T>,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct CircularBuffer<T> {
    pub values: VecDeque<T>,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct TimeSeries<T> {
    pub values: Vector<Event<T>>,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct MultiSeries<T> {
    pub series: Vector<TimeSeries<T>>,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Financial {
    pub open: f64,
    pub high: f64,
    pub low: f64,
    pub close: f64,
    pub volume: f64,
}
