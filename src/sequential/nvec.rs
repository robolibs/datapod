//! `NVec<T, N>` alias and nested-bucket helpers for `datapod::Nvec<...>`.
//!
//! The C++ `Nvec<K, V, Depth>` is an N-dimensional bucket structure (see
//! `nvec.hpp`). The existing Rust surface however already hands callers a
//! plain `[T; N]` for `NVec<T, N>`, which is the shape used throughout the
//! crate and tests. To honour both worlds we:
//!
//! * Keep `NVec<T, N>` as the fixed-size array alias, with an `NVecExt`
//!   trait that provides the C++-flavoured accessors.
//! * Expose `NestedNvec<T>`, a nested-vector backed equivalent of the C++
//!   multi-bucket container. It uses `Vector<Vector<T>>` storage so callers
//!   that need the full C++ semantics can opt in explicitly.

use crate::Vector;

pub type NVec<T, const N: usize> = [T; N];

/// Helper methods for the fixed-size `NVec<T, N>` alias.
pub trait NVecExt<T> {
    fn size(&self) -> usize;
    fn empty(&self) -> bool;
    fn at(&self, i: usize) -> &T;
    fn at_mut(&mut self, i: usize) -> &mut T;
    fn front(&self) -> &T;
    fn back(&self) -> &T;
    fn data(&self) -> *const T;
    fn data_mut(&mut self) -> *mut T;
    fn fill(&mut self, value: T)
    where
        T: Clone;
}

impl<T, const N: usize> NVecExt<T> for [T; N] {
    #[inline]
    fn size(&self) -> usize {
        N
    }

    #[inline]
    fn empty(&self) -> bool {
        N == 0
    }

    #[inline]
    fn at(&self, i: usize) -> &T {
        &self[i]
    }

    #[inline]
    fn at_mut(&mut self, i: usize) -> &mut T {
        &mut self[i]
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

/// Nested-vector implementation of the C++ `Nvec<Key, Value, Depth>`.
///
/// This is a heap-backed structure exposing bucket / sub-bucket access
/// through a flat data store plus index vectors, matching the C++
/// `BasicNvec` data layout for depth 2 (outer buckets of contiguous items).
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct NestedNvec<T> {
    pub data: Vector<T>,
    pub bucket_starts: Vector<usize>,
}

impl<T> NestedNvec<T> {
    pub fn new() -> Self {
        Self {
            data: Vector::new(),
            bucket_starts: Vector::new(),
        }
    }

    pub fn size(&self) -> usize {
        if self.bucket_starts.is_empty() {
            0
        } else {
            self.bucket_starts.len() - 1
        }
    }

    pub fn is_empty(&self) -> bool {
        self.size() == 0
    }

    pub fn empty(&self) -> bool {
        self.is_empty()
    }

    pub fn clear(&mut self) {
        self.data.clear();
        self.bucket_starts.clear();
    }

    pub fn push_bucket<I: IntoIterator<Item = T>>(&mut self, bucket: I) {
        if self.bucket_starts.is_empty() {
            self.bucket_starts.push_back(0);
        }
        for item in bucket {
            self.data.push_back(item);
        }
        let end = self.data.len();
        self.bucket_starts.push_back(end);
    }

    pub fn bucket(&self, i: usize) -> &[T] {
        let start = self.bucket_starts[i];
        let end = self.bucket_starts[i + 1];
        &self.data.as_slice()[start..end]
    }

    pub fn bucket_mut(&mut self, i: usize) -> &mut [T] {
        let start = self.bucket_starts[i];
        let end = self.bucket_starts[i + 1];
        &mut self.data.as_mut_slice()[start..end]
    }

    pub fn bucket_size(&self, i: usize) -> usize {
        self.bucket_starts[i + 1] - self.bucket_starts[i]
    }
}
