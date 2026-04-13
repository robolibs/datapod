use std::collections::BTreeMap;

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
