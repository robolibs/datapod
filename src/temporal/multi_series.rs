use crate::Vector;

use super::TimeSeries;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct MultiSeries<T> {
    pub series: Vector<TimeSeries<T>>,
}
