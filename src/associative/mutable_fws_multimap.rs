use std::collections::HashMap;
use std::hash::Hash;

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
