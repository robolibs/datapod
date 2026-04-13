use crate::Vector;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Pool<T> {
    pub values: Vector<T>,
}
