use std::collections::{BTreeMap, HashMap, HashSet};
use std::hash::Hash;

pub type Map<K, V> = HashMap<K, V>;
pub type Set<T> = HashSet<T>;
pub type HashStorage<K, V> = HashMap<K, V>;
pub type OrderedMap<K, V> = BTreeMap<K, V>;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct FwsMultimap<K, V> {
    pub entries: BTreeMap<K, Vec<V>>,
}

impl<K: Ord, V> FwsMultimap<K, V> {
    pub fn insert(&mut self, key: K, value: V) {
        self.entries.entry(key).or_default().push(value);
    }

    pub fn get(&self, key: &K) -> Option<&[V]> {
        self.entries.get(key).map(Vec::as_slice)
    }
}

#[derive(Debug, Clone, Default)]
pub struct MutableFwsMultimap<K, V> {
    pub entries: HashMap<K, Vec<V>>,
}

impl<K: Eq + Hash, V> MutableFwsMultimap<K, V> {
    pub fn insert(&mut self, key: K, value: V) {
        self.entries.entry(key).or_default().push(value);
    }

    pub fn get(&self, key: &K) -> Option<&[V]> {
        self.entries.get(key).map(Vec::as_slice)
    }
}
