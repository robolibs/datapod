use std::collections::VecDeque;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct FixedQueue<T> {
    pub values: VecDeque<T>,
    pub capacity: usize,
}
