use std::marker::PhantomData;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct OffsetPtr<T> {
    pub offset: usize,
    marker: PhantomData<T>,
}

impl<T> OffsetPtr<T> {
    pub fn new(offset: usize) -> Self {
        Self {
            offset,
            marker: PhantomData,
        }
    }
}
