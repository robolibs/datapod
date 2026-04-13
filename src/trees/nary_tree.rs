use crate::Vector;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct NaryTree<T> {
    pub value: Option<T>,
    pub children: Vector<NaryTree<T>>,
}
