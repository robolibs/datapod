//! Bucketed vector-of-vectors matching `datapod::Vecvec<K, V>`.
//!
//! The C++ `BasicVecvec` stores a flat data array plus an index vector of
//! bucket-start offsets, effectively acting as a ragged 2-D array. The
//! existing Rust surface aliases `Vecvec<T> = Vec<Vec<T>>` so tests can
//! initialise with `vec![vec![..]]` — we preserve that alias and add a
//! `VecvecExt` trait providing the C++-flavoured bucket API over it. For
//! callers that want the flat-index layout we also expose `FlatVecvec<T>`.

use crate::Vector;

pub type Vecvec<T> = Vec<Vec<T>>;

/// Extension API mirroring `datapod::Vecvec` methods on top of
/// `Vec<Vec<T>>`.
pub trait VecvecExt<T> {
    fn size(&self) -> usize;
    fn empty(&self) -> bool;
    fn clear_all(&mut self);
    fn bucket(&self, i: usize) -> &[T];
    fn bucket_mut(&mut self, i: usize) -> &mut [T];
    fn bucket_size(&self, i: usize) -> usize;
    fn front_bucket(&self) -> &[T];
    fn back_bucket(&self) -> &[T];
    fn at_bucket(&self, i: usize) -> &[T];
    fn emplace_back_bucket<I: IntoIterator<Item = T>>(&mut self, bucket: I);
    fn resize_buckets(&mut self, new_size: usize);
}

impl<T> VecvecExt<T> for Vec<Vec<T>> {
    #[inline]
    fn size(&self) -> usize {
        self.len()
    }

    #[inline]
    fn empty(&self) -> bool {
        self.is_empty()
    }

    fn clear_all(&mut self) {
        self.clear();
    }

    fn bucket(&self, i: usize) -> &[T] {
        self[i].as_slice()
    }

    fn bucket_mut(&mut self, i: usize) -> &mut [T] {
        self[i].as_mut_slice()
    }

    fn bucket_size(&self, i: usize) -> usize {
        self[i].len()
    }

    fn front_bucket(&self) -> &[T] {
        self.first().expect("Vecvec::front: empty").as_slice()
    }

    fn back_bucket(&self) -> &[T] {
        self.last().expect("Vecvec::back: empty").as_slice()
    }

    fn at_bucket(&self, i: usize) -> &[T] {
        assert!(i < self.len(), "Vecvec::at: index out of range");
        self[i].as_slice()
    }

    fn emplace_back_bucket<I: IntoIterator<Item = T>>(&mut self, bucket: I) {
        self.push(bucket.into_iter().collect());
    }

    fn resize_buckets(&mut self, new_size: usize) {
        if new_size < self.len() {
            self.truncate(new_size);
        } else {
            while self.len() < new_size {
                self.push(Vec::new());
            }
        }
    }
}

/// Flat-index storage variant mirroring the exact C++ layout of
/// `BasicVecvec` (contiguous `data` + `bucket_starts` offsets).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FlatVecvec<T> {
    pub data: Vector<T>,
    pub bucket_starts: Vector<usize>,
}

impl<T> Default for FlatVecvec<T> {
    fn default() -> Self {
        Self {
            data: Vector::new(),
            bucket_starts: Vector::new(),
        }
    }
}

impl<T> FlatVecvec<T> {
    pub fn new() -> Self {
        Self::default()
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

    pub fn emplace_back<I: IntoIterator<Item = T>>(&mut self, bucket: I) {
        if self.bucket_starts.is_empty() {
            self.bucket_starts.push_back(0);
        }
        for item in bucket {
            self.data.push_back(item);
        }
        let end = self.data.len();
        self.bucket_starts.push_back(end);
    }

    pub fn add_back_sized(&mut self, bucket_size: usize) -> usize
    where
        T: Default + Clone,
    {
        if self.bucket_starts.is_empty() {
            self.bucket_starts.push_back(0);
        }
        self.data.resize(self.data.len() + bucket_size, T::default());
        let end = self.data.len();
        self.bucket_starts.push_back(end);
        self.size() - 1
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

    pub fn front(&self) -> &[T] {
        self.bucket(0)
    }

    pub fn back(&self) -> &[T] {
        self.bucket(self.size() - 1)
    }

    pub fn resize(&mut self, new_size: usize) {
        let old_size = self.size();
        if new_size < old_size {
            self.bucket_starts.truncate(new_size + 1);
            let new_data_len = *self.bucket_starts.last().unwrap_or(&0);
            self.data.truncate(new_data_len);
        } else if new_size > old_size {
            if self.bucket_starts.is_empty() {
                self.bucket_starts.push_back(0);
            }
            let end = self.data.len();
            for _ in old_size..new_size {
                self.bucket_starts.push_back(end);
            }
        }
    }
}
