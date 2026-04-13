use crate::Vector;

use super::Event;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct TimeSeries<T> {
    pub values: Vector<Event<T>>,
}
