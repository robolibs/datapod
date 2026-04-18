use std::collections::{BTreeSet, HashSet};
use std::hash::Hash;

/// Unordered hash set. Fast O(1) membership test; iteration order is
/// random and changes between runs. Mirrors C++ `std::unordered_set`.
pub type Set<T> = HashSet<T>;

/// **Ordered** set. Values are kept sorted; iteration is deterministic
/// and stable across runs — useful for reproducible serialisation and
/// range queries. Mirrors C++ `std::set`.
pub type OSet<T> = BTreeSet<T>;

pub trait SetExt<T> {
    fn contains_value(&self, value: &T) -> bool;
    fn count(&self, value: &T) -> usize;
    fn emplace(&mut self, value: T) -> bool;
    fn erase(&mut self, value: &T) -> usize;
    fn bucket_count(&self) -> usize;
    fn load_factor(&self) -> f32;
    fn max_load_factor(&self) -> f32;
}

impl<T: Eq + Hash> SetExt<T> for HashSet<T> {
    fn contains_value(&self, value: &T) -> bool {
        self.contains(value)
    }

    fn count(&self, value: &T) -> usize {
        if self.contains(value) { 1 } else { 0 }
    }

    fn emplace(&mut self, value: T) -> bool {
        self.insert(value)
    }

    fn erase(&mut self, value: &T) -> usize {
        if self.remove(value) { 1 } else { 0 }
    }

    fn bucket_count(&self) -> usize {
        self.capacity()
    }

    fn load_factor(&self) -> f32 {
        let cap = self.capacity();
        if cap == 0 { 0.0 } else { self.len() as f32 / cap as f32 }
    }

    fn max_load_factor(&self) -> f32 {
        0.875
    }
}

/// C++-flavoured aliases for the ordered `OSet`. Bucket-related
/// methods return placeholders — tree sets have no buckets.
pub trait OSetExt<T> {
    fn contains_value(&self, value: &T) -> bool;
    fn count(&self, value: &T) -> usize;
    fn emplace(&mut self, value: T) -> bool;
    fn erase(&mut self, value: &T) -> usize;
    fn bucket_count(&self) -> usize;
    fn load_factor(&self) -> f32;
    fn max_load_factor(&self) -> f32;
}

impl<T: Ord> OSetExt<T> for BTreeSet<T> {
    fn contains_value(&self, value: &T) -> bool { self.contains(value) }
    fn count(&self, value: &T) -> usize {
        if self.contains(value) { 1 } else { 0 }
    }
    fn emplace(&mut self, value: T) -> bool { self.insert(value) }
    fn erase(&mut self, value: &T) -> usize {
        if self.remove(value) { 1 } else { 0 }
    }
    fn bucket_count(&self) -> usize { 0 }
    fn load_factor(&self) -> f32 { 0.0 }
    fn max_load_factor(&self) -> f32 { 1.0 }
}
