use std::marker::PhantomData;

#[derive(Debug, Clone, Copy, Default)]
pub struct Allocator<T>(pub PhantomData<T>);
