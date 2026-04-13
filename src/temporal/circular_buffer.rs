use std::collections::VecDeque;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct CircularBuffer<T> {
    pub values: VecDeque<T>,
}
