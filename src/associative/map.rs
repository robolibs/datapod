use std::collections::{BTreeMap, HashMap};
use std::hash::Hash;

/// Unordered hash map. Fast O(1) lookup; iteration order is random and
/// changes between runs. Mirrors C++ `std::unordered_map`.
pub type Map<K, V> = HashMap<K, V>;

/// **Ordered** map. Keys are kept sorted; iteration is deterministic and
/// stable across runs and machines — handy for reproducible serialisation,
/// golden-file tests, and range queries. Mirrors C++ `std::map`.
pub type OMap<K, V> = BTreeMap<K, V>;

pub trait MapExt<K, V> {
    fn contains(&self, key: &K) -> bool;
    fn count(&self, key: &K) -> usize;
    fn at(&self, key: &K) -> &V;
    fn at_mut(&mut self, key: &K) -> &mut V;
    fn emplace(&mut self, key: K, value: V) -> (bool, &mut V);
    fn try_emplace(&mut self, key: K, value: V) -> (bool, &mut V);
    fn insert_or_assign(&mut self, key: K, value: V) -> bool;
    fn erase(&mut self, key: &K) -> usize;
    fn bucket_count(&self) -> usize;
    fn load_factor(&self) -> f32;
    fn max_load_factor(&self) -> f32;
}

impl<K: Eq + Hash, V> MapExt<K, V> for HashMap<K, V> {
    fn contains(&self, key: &K) -> bool {
        self.contains_key(key)
    }

    fn count(&self, key: &K) -> usize {
        if self.contains_key(key) { 1 } else { 0 }
    }

    fn at(&self, key: &K) -> &V {
        self.get(key).expect("Map::at() key not found")
    }

    fn at_mut(&mut self, key: &K) -> &mut V {
        self.get_mut(key).expect("Map::at() key not found")
    }

    fn emplace(&mut self, key: K, value: V) -> (bool, &mut V) {
        use std::collections::hash_map::Entry;
        match self.entry(key) {
            Entry::Occupied(e) => (false, e.into_mut()),
            Entry::Vacant(e) => (true, e.insert(value)),
        }
    }

    fn try_emplace(&mut self, key: K, value: V) -> (bool, &mut V) {
        self.emplace(key, value)
    }

    fn insert_or_assign(&mut self, key: K, value: V) -> bool {
        self.insert(key, value).is_none()
    }

    fn erase(&mut self, key: &K) -> usize {
        if self.remove(key).is_some() { 1 } else { 0 }
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

/// Same C++-flavoured operations, but for the ordered `OMap` /
/// `BTreeMap`. `bucket_count` / `load_factor` / `max_load_factor` return
/// placeholder values because tree maps have no buckets — they exist
/// only for surface parity with `MapExt`.
pub trait OMapExt<K, V> {
    fn contains(&self, key: &K) -> bool;
    fn count(&self, key: &K) -> usize;
    fn at(&self, key: &K) -> &V;
    fn at_mut(&mut self, key: &K) -> &mut V;
    fn emplace(&mut self, key: K, value: V) -> (bool, &mut V);
    fn try_emplace(&mut self, key: K, value: V) -> (bool, &mut V);
    fn insert_or_assign(&mut self, key: K, value: V) -> bool;
    fn erase(&mut self, key: &K) -> usize;
    fn bucket_count(&self) -> usize;
    fn load_factor(&self) -> f32;
    fn max_load_factor(&self) -> f32;
}

impl<K: Ord, V> OMapExt<K, V> for BTreeMap<K, V> {
    fn contains(&self, key: &K) -> bool {
        self.contains_key(key)
    }

    fn count(&self, key: &K) -> usize {
        if self.contains_key(key) { 1 } else { 0 }
    }

    fn at(&self, key: &K) -> &V {
        self.get(key).expect("OMap::at() key not found")
    }

    fn at_mut(&mut self, key: &K) -> &mut V {
        self.get_mut(key).expect("OMap::at() key not found")
    }

    fn emplace(&mut self, key: K, value: V) -> (bool, &mut V) {
        use std::collections::btree_map::Entry;
        match self.entry(key) {
            Entry::Occupied(e) => (false, e.into_mut()),
            Entry::Vacant(e) => (true, e.insert(value)),
        }
    }

    fn try_emplace(&mut self, key: K, value: V) -> (bool, &mut V) {
        self.emplace(key, value)
    }

    fn insert_or_assign(&mut self, key: K, value: V) -> bool {
        self.insert(key, value).is_none()
    }

    fn erase(&mut self, key: &K) -> usize {
        if self.remove(key).is_some() { 1 } else { 0 }
    }

    fn bucket_count(&self) -> usize { 0 }
    fn load_factor(&self) -> f32 { 0.0 }
    fn max_load_factor(&self) -> f32 { 1.0 }
}
