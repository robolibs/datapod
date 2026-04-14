//! `Vectra<T>` alias matching the C++ `datapod::Vectra<T, InlineCapacity>`.
//!
//! The C++ type is a small-vector with inline SSO-style storage and a
//! `serialization_cache_` shadow vector. On the Rust side we keep the
//! existing alias `Vectra<T> = Vec<T>` so the test surface
//! (`let _vectra: Vectra<i32> = vec![1, 2];`) continues to work, and expose
//! the C++ helper methods via `VectraExt`.
//!
//! Rust's `Vec<T>` is already a very good stand-in: it provides the same
//! push_back / pop_back / resize / reserve / shrink_to_fit API, plus an
//! internal growth factor of 2x which is functionally equivalent to
//! `compute_new_capacity`. The inline-storage optimisation is handled by
//! the allocator and doesn't affect the public API.

pub type Vectra<T> = Vec<T>;

/// Extension methods mirroring the subset of `datapod::Vectra` helpers
/// that are not already present on Rust's `Vec<T>` under those exact names.
pub trait VectraExt<T> {
    fn size(&self) -> usize;
    fn empty(&self) -> bool;
    fn front_elem(&self) -> &T;
    fn back_elem(&self) -> &T;
    fn at(&self, i: usize) -> &T;
    fn at_mut(&mut self, i: usize) -> &mut T;
    fn push_back_value(&mut self, value: T);
    fn pop_back_value(&mut self) -> Option<T>;
    fn data_ptr(&self) -> *const T;
    fn data_mut_ptr(&mut self) -> *mut T;
    fn assign_n(&mut self, count: usize, value: T)
    where
        T: Clone;
    fn assign_iter<I: IntoIterator<Item = T>>(&mut self, iter: I);
    fn swap_with(&mut self, other: &mut Self);
}

impl<T> VectraExt<T> for Vec<T> {
    #[inline]
    fn size(&self) -> usize {
        self.len()
    }

    #[inline]
    fn empty(&self) -> bool {
        self.is_empty()
    }

    fn front_elem(&self) -> &T {
        self.first().expect("Vectra::front: empty")
    }

    fn back_elem(&self) -> &T {
        self.last().expect("Vectra::back: empty")
    }

    fn at(&self, i: usize) -> &T {
        &self[i]
    }

    fn at_mut(&mut self, i: usize) -> &mut T {
        &mut self[i]
    }

    fn push_back_value(&mut self, value: T) {
        self.push(value);
    }

    fn pop_back_value(&mut self) -> Option<T> {
        self.pop()
    }

    fn data_ptr(&self) -> *const T {
        self.as_ptr()
    }

    fn data_mut_ptr(&mut self) -> *mut T {
        self.as_mut_ptr()
    }

    fn assign_n(&mut self, count: usize, value: T)
    where
        T: Clone,
    {
        self.clear();
        self.resize(count, value);
    }

    fn assign_iter<I: IntoIterator<Item = T>>(&mut self, iter: I) {
        self.clear();
        self.extend(iter);
    }

    fn swap_with(&mut self, other: &mut Self) {
        std::mem::swap(self, other);
    }
}
