use crate::Vector;
use std::collections::BTreeSet;
use std::collections::btree_set;
use std::ops::Bound;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct OrderedSet<T>
where
    T: Ord,
{
    data: BTreeSet<T>,
}

impl<T: Ord> Default for OrderedSet<T> {
    fn default() -> Self {
        Self {
            data: BTreeSet::new(),
        }
    }
}

impl<T: Ord> OrderedSet<T> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn empty(&self) -> bool {
        self.data.is_empty()
    }

    pub fn is_empty(&self) -> bool {
        self.data.is_empty()
    }

    pub fn size(&self) -> usize {
        self.data.len()
    }

    pub fn len(&self) -> usize {
        self.data.len()
    }

    pub fn clear(&mut self) {
        self.data.clear();
    }

    pub fn insert(&mut self, value: T) -> bool {
        self.data.insert(value)
    }

    pub fn erase(&mut self, value: &T) -> usize {
        if self.data.remove(value) { 1 } else { 0 }
    }

    pub fn remove(&mut self, value: &T) -> bool {
        self.data.remove(value)
    }

    pub fn find(&self, value: &T) -> Option<&T> {
        self.data.get(value)
    }

    pub fn contains(&self, value: &T) -> bool {
        self.data.contains(value)
    }

    pub fn count(&self, value: &T) -> usize {
        if self.data.contains(value) { 1 } else { 0 }
    }

    pub fn lower_bound(&self, value: &T) -> Option<&T> {
        self.data.range((Bound::Included(value), Bound::Unbounded)).next()
    }

    pub fn upper_bound(&self, value: &T) -> Option<&T> {
        self.data.range((Bound::Excluded(value), Bound::Unbounded)).next()
    }

    pub fn min(&self) -> &T {
        self.data
            .iter()
            .next()
            .expect("OrderedSet::min: set is empty")
    }

    pub fn max(&self) -> &T {
        self.data
            .iter()
            .next_back()
            .expect("OrderedSet::max: set is empty")
    }

    pub fn first(&self) -> Option<&T> {
        self.data.iter().next()
    }

    pub fn last(&self) -> Option<&T> {
        self.data.iter().next_back()
    }

    pub fn iter(&self) -> btree_set::Iter<'_, T> {
        self.data.iter()
    }

    pub fn begin(&self) -> btree_set::Iter<'_, T> {
        self.data.iter()
    }

    pub fn as_btree(&self) -> &BTreeSet<T> {
        &self.data
    }

    pub fn as_btree_mut(&mut self) -> &mut BTreeSet<T> {
        &mut self.data
    }

    pub fn to_vector(&self) -> Vector<T>
    where
        T: Clone,
    {
        let mut v = Vector::new();
        for item in self.data.iter() {
            v.push_back(item.clone());
        }
        v
    }
}

impl<T: Ord> From<BTreeSet<T>> for OrderedSet<T> {
    fn from(data: BTreeSet<T>) -> Self {
        Self { data }
    }
}

impl<T: Ord> FromIterator<T> for OrderedSet<T> {
    fn from_iter<I: IntoIterator<Item = T>>(iter: I) -> Self {
        Self {
            data: BTreeSet::from_iter(iter),
        }
    }
}

impl<T: Ord> IntoIterator for OrderedSet<T> {
    type Item = T;
    type IntoIter = btree_set::IntoIter<T>;

    fn into_iter(self) -> Self::IntoIter {
        self.data.into_iter()
    }
}

impl<'a, T: Ord> IntoIterator for &'a OrderedSet<T> {
    type Item = &'a T;
    type IntoIter = btree_set::Iter<'a, T>;

    fn into_iter(self) -> Self::IntoIter {
        self.data.iter()
    }
}
