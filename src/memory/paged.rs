use crate::Vector;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Paged<T> {
    pub pages: Vector<Vector<T>>,
}
