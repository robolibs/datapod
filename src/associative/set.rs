use std::collections::HashSet;
use std::hash::Hash;

pub type Set<T> = HashSet<T>;

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
