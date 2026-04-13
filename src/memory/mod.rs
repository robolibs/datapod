use std::marker::PhantomData;

use crate::Vector;

#[derive(Debug, Clone, Copy, Default)]
pub struct Allocator<T>(PhantomData<T>);

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Arena<T> {
    pub values: Vector<T>,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Pool<T> {
    pub values: Vector<T>,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct MmapVec<T> {
    pub values: Vector<T>,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Paged<T> {
    pub pages: Vector<Vector<T>>,
}

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

pub type Ptr<T> = Box<T>;
