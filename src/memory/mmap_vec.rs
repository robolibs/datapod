use crate::Vector;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct MmapVec<T> {
    pub values: Vector<T>,
}
