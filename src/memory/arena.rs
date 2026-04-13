use crate::Vector;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Arena<T> {
    pub values: Vector<T>,
}
