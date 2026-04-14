// Pure-Rust Vec-backed fallback: memmap2 crate not present in Cargo.toml.
use crate::Vector;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MmapVec<T> {
    values: Vector<T>,
}

impl<T> Default for MmapVec<T> {
    fn default() -> Self {
        Self {
            values: Vector::new(),
        }
    }
}

impl<T> MmapVec<T> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_capacity(capacity: usize) -> Self {
        Self {
            values: Vector::with_capacity(capacity),
        }
    }

    pub fn len(&self) -> usize {
        self.values.len()
    }

    pub fn size(&self) -> usize {
        self.values.len()
    }

    pub fn is_empty(&self) -> bool {
        self.values.is_empty()
    }

    pub fn empty(&self) -> bool {
        self.is_empty()
    }

    pub fn capacity(&self) -> usize {
        self.values.capacity()
    }

    pub fn reserve(&mut self, additional: usize) {
        self.values.reserve(additional);
    }

    pub fn clear(&mut self) {
        self.values.clear();
    }

    pub fn push_back(&mut self, value: T) {
        self.values.push_back(value);
    }

    pub fn emplace_back(&mut self, value: T) -> &mut T {
        self.values.push_back(value);
        let idx = self.values.len() - 1;
        &mut self.values[idx]
    }

    pub fn pop_back(&mut self) -> Option<T> {
        self.values.pop_back()
    }

    pub fn get(&self, index: usize) -> Option<&T> {
        (*self.values).get(index)
    }

    pub fn get_mut(&mut self, index: usize) -> Option<&mut T> {
        (*self.values).get_mut(index)
    }

    pub fn as_slice(&self) -> &[T] {
        &self.values
    }

    pub fn as_mut_slice(&mut self) -> &mut [T] {
        &mut self.values
    }

    pub fn iter(&self) -> std::slice::Iter<'_, T> {
        self.values.iter()
    }

    pub fn iter_mut(&mut self) -> std::slice::IterMut<'_, T> {
        self.values.iter_mut()
    }

    pub fn data(&self) -> *const T {
        let slice: &[T] = &self.values;
        slice.as_ptr()
    }

    pub fn data_mut(&mut self) -> *mut T {
        let slice: &mut [T] = &mut self.values;
        slice.as_mut_ptr()
    }
}

impl<T: Clone + Default> MmapVec<T> {
    pub fn resize(&mut self, new_size: usize) {
        self.values.resize(new_size, T::default());
    }
}

impl<T: Clone> MmapVec<T> {
    pub fn set_from_slice(&mut self, src: &[T]) {
        self.values.clear();
        self.values.reserve(src.len());
        for v in src {
            self.values.push_back(v.clone());
        }
    }

    pub fn insert_at(&mut self, index: usize, value: T) {
        self.values.insert(index, value);
    }
}

impl<T> core::ops::Index<usize> for MmapVec<T> {
    type Output = T;
    fn index(&self, i: usize) -> &T {
        &self.values[i]
    }
}

impl<T> core::ops::IndexMut<usize> for MmapVec<T> {
    fn index_mut(&mut self, i: usize) -> &mut T {
        &mut self.values[i]
    }
}
