use std::collections::HashMap;
use std::hash::Hash;

#[derive(Debug, Clone)]
pub struct HashStorage<K: Eq + Hash, V> {
    entries: HashMap<K, V>,
}

impl<K: Eq + Hash, V> Default for HashStorage<K, V> {
    fn default() -> Self {
        Self { entries: HashMap::new() }
    }
}

impl<K: Eq + Hash, V: PartialEq> PartialEq for HashStorage<K, V> {
    fn eq(&self, other: &Self) -> bool {
        self.entries == other.entries
    }
}

impl<K: Eq + Hash, V: Eq> Eq for HashStorage<K, V> {}

impl<K: Eq + Hash, V> HashStorage<K, V> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_capacity(cap: usize) -> Self {
        Self { entries: HashMap::with_capacity(cap) }
    }

    pub fn size(&self) -> usize {
        self.entries.len()
    }

    pub fn len(&self) -> usize {
        self.entries.len()
    }

    pub fn capacity(&self) -> usize {
        self.entries.capacity()
    }

    pub fn bucket_count(&self) -> usize {
        self.entries.capacity()
    }

    pub fn empty(&self) -> bool {
        self.entries.is_empty()
    }

    pub fn is_empty(&self) -> bool {
        self.entries.is_empty()
    }

    pub fn clear(&mut self) {
        self.entries.clear();
    }

    pub fn reserve(&mut self, additional: usize) {
        let needed = additional.saturating_sub(self.entries.capacity().saturating_sub(self.entries.len()));
        self.entries.reserve(needed);
    }

    pub fn rehash(&mut self, count: usize) {
        self.entries.reserve(count.saturating_sub(self.entries.len()));
    }

    pub fn contains(&self, key: &K) -> bool {
        self.entries.contains_key(key)
    }

    pub fn count(&self, key: &K) -> usize {
        if self.contains(key) { 1 } else { 0 }
    }

    pub fn find(&self, key: &K) -> Option<&V> {
        self.entries.get(key)
    }

    pub fn find_mut(&mut self, key: &K) -> Option<&mut V> {
        self.entries.get_mut(key)
    }

    pub fn get(&self, key: &K) -> Option<&V> {
        self.entries.get(key)
    }

    pub fn at(&self, key: &K) -> &V {
        self.entries.get(key).expect("HashStorage::at() key not found")
    }

    pub fn at_mut(&mut self, key: &K) -> &mut V {
        self.entries.get_mut(key).expect("HashStorage::at() key not found")
    }

    pub fn insert(&mut self, key: K, value: V) -> (bool, &mut V) {
        use std::collections::hash_map::Entry;
        match self.entries.entry(key) {
            Entry::Occupied(e) => (false, e.into_mut()),
            Entry::Vacant(e) => (true, e.insert(value)),
        }
    }

    pub fn emplace(&mut self, key: K, value: V) -> (bool, &mut V) {
        self.insert(key, value)
    }

    pub fn try_emplace(&mut self, key: K, value: V) -> (bool, &mut V) {
        self.insert(key, value)
    }

    pub fn insert_or_assign(&mut self, key: K, value: V) -> bool {
        self.entries.insert(key, value).is_none()
    }

    pub fn erase(&mut self, key: &K) -> usize {
        if self.entries.remove(key).is_some() { 1 } else { 0 }
    }

    pub fn swap(&mut self, other: &mut Self) {
        std::mem::swap(&mut self.entries, &mut other.entries);
    }

    pub fn load_factor(&self) -> f32 {
        let cap = self.entries.capacity();
        if cap == 0 { 0.0 } else { self.entries.len() as f32 / cap as f32 }
    }

    pub fn max_load_factor(&self) -> f32 {
        0.875
    }

    pub fn iter(&self) -> std::collections::hash_map::Iter<'_, K, V> {
        self.entries.iter()
    }

    pub fn iter_mut(&mut self) -> std::collections::hash_map::IterMut<'_, K, V> {
        self.entries.iter_mut()
    }

    pub fn keys(&self) -> std::collections::hash_map::Keys<'_, K, V> {
        self.entries.keys()
    }

    pub fn values(&self) -> std::collections::hash_map::Values<'_, K, V> {
        self.entries.values()
    }
}

impl<K: Eq + Hash, V> std::ops::Index<&K> for HashStorage<K, V> {
    type Output = V;
    fn index(&self, key: &K) -> &V {
        self.at(key)
    }
}

impl<K: Eq + Hash, V> FromIterator<(K, V)> for HashStorage<K, V> {
    fn from_iter<I: IntoIterator<Item = (K, V)>>(iter: I) -> Self {
        Self { entries: HashMap::from_iter(iter) }
    }
}

impl<'a, K: Eq + Hash, V> IntoIterator for &'a HashStorage<K, V> {
    type Item = (&'a K, &'a V);
    type IntoIter = std::collections::hash_map::Iter<'a, K, V>;
    fn into_iter(self) -> Self::IntoIter {
        self.entries.iter()
    }
}
