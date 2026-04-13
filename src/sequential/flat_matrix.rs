#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct FlatMatrix<T> {
    pub rows: usize,
    pub cols: usize,
    pub values: Vec<T>,
}
