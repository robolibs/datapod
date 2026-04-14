//! Paged vector-of-vectors matching `datapod::PagedVecvec<Index, PagedAlloc>`.
//!
//! The C++ variant layers on top of a `Paged` allocator (pages carved out of
//! a single backing buffer) and an index vector of page descriptors. We
//! preserve the existing Rust alias `PagedVecvec<T> = Vec<Vec<T>>` so tests
//! initialising with `vec![vec![..]]` keep compiling, and expose the C++
//! API via `PagedVecvecExt` (plus a proper `PagedVecvecOwned<T>` that
//! mirrors the C++ layout with a flat backing store and page offsets).

use crate::Vector;

pub type PagedVecvec<T> = Vec<Vec<T>>;

/// Extension API mirroring the paged vecvec surface on top of `Vec<Vec<T>>`.
pub trait PagedVecvecExt<T> {
    fn size(&self) -> usize;
    fn empty(&self) -> bool;
    fn clear_all(&mut self);
    fn bucket(&self, i: usize) -> &[T];
    fn bucket_mut(&mut self, i: usize) -> &mut [T];
    fn bucket_size(&self, i: usize) -> usize;
    fn front_bucket(&self) -> &[T];
    fn back_bucket(&self) -> &[T];
    fn emplace_back_bucket<I: IntoIterator<Item = T>>(&mut self, bucket: I);
    fn emplace_back_empty(&mut self);
    fn insert_bucket<I: IntoIterator<Item = T>>(&mut self, index: usize, bucket: I);
    fn resize_buckets(&mut self, new_size: usize);
}

impl<T> PagedVecvecExt<T> for Vec<Vec<T>> {
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
        self.first().expect("PagedVecvec::front: empty").as_slice()
    }

    fn back_bucket(&self) -> &[T] {
        self.last().expect("PagedVecvec::back: empty").as_slice()
    }

    fn emplace_back_bucket<I: IntoIterator<Item = T>>(&mut self, bucket: I) {
        self.push(bucket.into_iter().collect());
    }

    fn emplace_back_empty(&mut self) {
        self.push(Vec::new());
    }

    fn insert_bucket<I: IntoIterator<Item = T>>(&mut self, index: usize, bucket: I) {
        self.insert(index, bucket.into_iter().collect());
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

/// Owning paged-vecvec mirroring the C++ layout of a flat data buffer plus a
/// vector of `Page` descriptors. Useful when you want the C++ semantics
/// without going through `Vec<Vec<T>>`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Page {
    pub offset: usize,
    pub size: usize,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PagedVecvecOwned<T> {
    pub data: Vector<T>,
    pub pages: Vector<Page>,
}

impl<T> Default for PagedVecvecOwned<T> {
    fn default() -> Self {
        Self {
            data: Vector::new(),
            pages: Vector::new(),
        }
    }
}

impl<T> PagedVecvecOwned<T> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn size(&self) -> usize {
        self.pages.len()
    }

    pub fn is_empty(&self) -> bool {
        self.pages.is_empty()
    }

    pub fn empty(&self) -> bool {
        self.is_empty()
    }

    pub fn clear(&mut self) {
        self.pages.clear();
        self.data.clear();
    }

    pub fn emplace_back<I: IntoIterator<Item = T>>(&mut self, bucket: I) {
        let offset = self.data.len();
        let mut count = 0usize;
        for item in bucket {
            self.data.push_back(item);
            count += 1;
        }
        self.pages.push_back(Page {
            offset,
            size: count,
        });
    }

    pub fn emplace_back_empty(&mut self) {
        let offset = self.data.len();
        self.pages.push_back(Page { offset, size: 0 });
    }

    pub fn bucket(&self, i: usize) -> &[T] {
        let p = self.pages[i];
        &self.data.as_slice()[p.offset..p.offset + p.size]
    }

    pub fn bucket_mut(&mut self, i: usize) -> &mut [T] {
        let p = self.pages[i];
        &mut self.data.as_mut_slice()[p.offset..p.offset + p.size]
    }

    pub fn bucket_size(&self, i: usize) -> usize {
        self.pages[i].size
    }

    pub fn resize(&mut self, new_size: usize)
    where
        T: Default + Clone,
    {
        while self.pages.len() > new_size {
            let p = self.pages.pop_back().unwrap();
            self.data.truncate(p.offset);
        }
        while self.pages.len() < new_size {
            self.emplace_back_empty();
        }
    }
}
