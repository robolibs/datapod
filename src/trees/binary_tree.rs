#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct BinaryTree<T> {
    pub value: Option<T>,
    pub left: Option<Box<BinaryTree<T>>>,
    pub right: Option<Box<BinaryTree<T>>>,
}
