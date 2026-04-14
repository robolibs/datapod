//! Fixed-size array alias matching `datapod::Array<T, N>` from C++.
//!
//! The C++ header exposes `Array` as a typed wrapper around `std::array<T, N>`
//! with a handful of convenience helpers. On the Rust side we keep the alias
//! pointing at the native `[T; N]` so that spatial/geometry code using
//! `Array<T, N>` as a plain array continues to work, while still providing an
//! extension trait for C++ style accessors.

use std::ops::{Index, IndexMut};

pub type Array<T, const N: usize> = [T; N];

/// Extension methods mirroring the subset of `datapod::Array` helpers that
/// make sense on a Rust `[T; N]`.
pub trait ArrayExt<T> {
    fn size(&self) -> usize;
    fn empty(&self) -> bool;
    fn front(&self) -> &T;
    fn back(&self) -> &T;
    fn front_mut(&mut self) -> &mut T;
    fn back_mut(&mut self) -> &mut T;
    fn at(&self, i: usize) -> &T;
    fn at_mut(&mut self, i: usize) -> &mut T;
    fn data(&self) -> *const T;
    fn data_mut(&mut self) -> *mut T;
    fn fill(&mut self, value: T)
    where
        T: Clone;
}

impl<T, const N: usize> ArrayExt<T> for [T; N] {
    #[inline]
    fn size(&self) -> usize {
        N
    }

    #[inline]
    fn empty(&self) -> bool {
        N == 0
    }

    #[inline]
    fn front(&self) -> &T {
        &self[0]
    }

    #[inline]
    fn back(&self) -> &T {
        &self[N - 1]
    }

    #[inline]
    fn front_mut(&mut self) -> &mut T {
        &mut self[0]
    }

    #[inline]
    fn back_mut(&mut self) -> &mut T {
        &mut self[N - 1]
    }

    #[inline]
    fn at(&self, i: usize) -> &T {
        <[T; N] as Index<usize>>::index(self, i)
    }

    #[inline]
    fn at_mut(&mut self, i: usize) -> &mut T {
        <[T; N] as IndexMut<usize>>::index_mut(self, i)
    }

    #[inline]
    fn data(&self) -> *const T {
        self.as_ptr()
    }

    #[inline]
    fn data_mut(&mut self) -> *mut T {
        self.as_mut_ptr()
    }

    #[inline]
    fn fill(&mut self, value: T)
    where
        T: Clone,
    {
        for slot in self.iter_mut() {
            *slot = value.clone();
        }
    }
}
