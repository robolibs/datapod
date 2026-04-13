use std::iter::FromIterator;
use std::ops::{Deref, DerefMut, Index, IndexMut, RangeBounds};

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Vector<T>(Vec<T>);

impl<T> Default for Vector<T> {
    fn default() -> Self {
        Self(Vec::new())
    }
}

impl<T> Vector<T> {
    pub fn new() -> Self {
        Self(Vec::new())
    }

    pub fn with_capacity(capacity: usize) -> Self {
        Self(Vec::with_capacity(capacity))
    }

    pub fn from_elem(count: usize, value: T) -> Self
    where
        T: Clone,
    {
        Self(vec![value; count])
    }

    pub fn len(&self) -> usize {
        self.0.len()
    }

    pub fn size(&self) -> usize {
        self.len()
    }

    pub fn is_empty(&self) -> bool {
        self.0.is_empty()
    }

    pub fn empty(&self) -> bool {
        self.is_empty()
    }

    pub fn capacity(&self) -> usize {
        self.0.capacity()
    }

    pub fn reserve(&mut self, additional: usize) {
        self.0.reserve(additional);
    }

    pub fn shrink_to_fit(&mut self) {
        self.0.shrink_to_fit();
    }

    pub fn clear(&mut self) {
        self.0.clear();
    }

    pub fn push_back(&mut self, value: T) {
        self.0.push(value);
    }

    pub fn push(&mut self, value: T) {
        self.0.push(value);
    }

    pub fn pop_back(&mut self) -> Option<T> {
        self.0.pop()
    }

    pub fn insert(&mut self, index: usize, value: T) {
        self.0.insert(index, value);
    }

    pub fn remove(&mut self, index: usize) -> T {
        self.0.remove(index)
    }

    pub fn erase(&mut self, index: usize) -> T {
        self.0.remove(index)
    }

    pub fn truncate(&mut self, len: usize) {
        self.0.truncate(len);
    }

    pub fn resize(&mut self, new_len: usize, value: T)
    where
        T: Clone,
    {
        self.0.resize(new_len, value);
    }

    pub fn assign<I>(&mut self, iter: I)
    where
        I: IntoIterator<Item = T>,
    {
        self.0.clear();
        self.0.extend(iter);
    }

    pub fn extend_from_slice(&mut self, other: &[T])
    where
        T: Clone,
    {
        self.0.extend_from_slice(other);
    }

    pub fn as_slice(&self) -> &[T] {
        self.0.as_slice()
    }

    pub fn as_mut_slice(&mut self) -> &mut [T] {
        self.0.as_mut_slice()
    }

    pub fn first(&self) -> Option<&T> {
        self.0.first()
    }

    pub fn first_mut(&mut self) -> Option<&mut T> {
        self.0.first_mut()
    }

    pub fn front(&self) -> &T {
        &self.0[0]
    }

    pub fn front_mut(&mut self) -> &mut T {
        &mut self.0[0]
    }

    pub fn last(&self) -> Option<&T> {
        self.0.last()
    }

    pub fn last_mut(&mut self) -> Option<&mut T> {
        self.0.last_mut()
    }

    pub fn back(&self) -> &T {
        self.0.last().expect("Vector::back on empty vector")
    }

    pub fn back_mut(&mut self) -> &mut T {
        self.0.last_mut().expect("Vector::back_mut on empty vector")
    }

    pub fn data(&self) -> *const T {
        self.0.as_ptr()
    }

    pub fn data_mut(&mut self) -> *mut T {
        self.0.as_mut_ptr()
    }

    pub fn iter(&self) -> std::slice::Iter<'_, T> {
        self.0.iter()
    }

    pub fn iter_mut(&mut self) -> std::slice::IterMut<'_, T> {
        self.0.iter_mut()
    }

    pub fn drain<R>(&mut self, range: R) -> std::vec::Drain<'_, T>
    where
        R: RangeBounds<usize>,
    {
        self.0.drain(range)
    }
}

impl<T> Deref for Vector<T> {
    type Target = [T];

    fn deref(&self) -> &Self::Target {
        self.0.as_slice()
    }
}

impl<T> DerefMut for Vector<T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.0.as_mut_slice()
    }
}

impl<T> Index<usize> for Vector<T> {
    type Output = T;

    fn index(&self, index: usize) -> &Self::Output {
        &self.0[index]
    }
}

impl<T> IndexMut<usize> for Vector<T> {
    fn index_mut(&mut self, index: usize) -> &mut Self::Output {
        &mut self.0[index]
    }
}

impl<T> From<Vec<T>> for Vector<T> {
    fn from(value: Vec<T>) -> Self {
        Self(value)
    }
}

impl<T, const N: usize> From<[T; N]> for Vector<T> {
    fn from(value: [T; N]) -> Self {
        Self(Vec::from(value))
    }
}

impl<T> FromIterator<T> for Vector<T> {
    fn from_iter<I: IntoIterator<Item = T>>(iter: I) -> Self {
        Self(iter.into_iter().collect())
    }
}

impl<T> IntoIterator for Vector<T> {
    type Item = T;
    type IntoIter = std::vec::IntoIter<T>;

    fn into_iter(self) -> Self::IntoIter {
        self.0.into_iter()
    }
}

impl<'a, T> IntoIterator for &'a Vector<T> {
    type Item = &'a T;
    type IntoIter = std::slice::Iter<'a, T>;

    fn into_iter(self) -> Self::IntoIter {
        self.0.iter()
    }
}

impl<'a, T> IntoIterator for &'a mut Vector<T> {
    type Item = &'a mut T;
    type IntoIter = std::slice::IterMut<'a, T>;

    fn into_iter(self) -> Self::IntoIter {
        self.0.iter_mut()
    }
}
