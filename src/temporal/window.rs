use crate::Vector;

use super::Stamp;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Window<T> {
    pub start: Stamp,
    pub end: Stamp,
    pub values: Vector<T>,
}
