use crate::Vector;
use std::collections::BTreeMap;
use std::collections::btree_map;
use std::ops::Bound;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct OrderedMap<K, V>
where
    K: Ord,
{
    data: BTreeMap<K, V>,
}

impl<K: Ord, V> Default for OrderedMap<K, V> {
    fn default() -> Self {
        Self {
            data: BTreeMap::new(),
        }
    }
}

impl<K: Ord, V> OrderedMap<K, V> {
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

    pub fn insert(&mut self, key: K, value: V) -> bool {
        if self.data.contains_key(&key) {
            return false;
        }
        self.data.insert(key, value);
        true
    }

    pub fn insert_or_assign(&mut self, key: K, value: V) -> Option<V> {
        self.data.insert(key, value)
    }

    pub fn erase(&mut self, key: &K) -> usize {
        if self.data.remove(key).is_some() { 1 } else { 0 }
    }

    pub fn remove(&mut self, key: &K) -> Option<V> {
        self.data.remove(key)
    }

    pub fn find(&self, key: &K) -> Option<&V> {
        self.data.get(key)
    }

    pub fn find_mut(&mut self, key: &K) -> Option<&mut V> {
        self.data.get_mut(key)
    }

    pub fn contains(&self, key: &K) -> bool {
        self.data.contains_key(key)
    }

    pub fn contains_key(&self, key: &K) -> bool {
        self.data.contains_key(key)
    }

    pub fn count(&self, key: &K) -> usize {
        if self.data.contains_key(key) { 1 } else { 0 }
    }

    pub fn get(&self, key: &K) -> Option<&V> {
        self.data.get(key)
    }

    pub fn get_mut(&mut self, key: &K) -> Option<&mut V> {
        self.data.get_mut(key)
    }

    pub fn at(&self, key: &K) -> &V {
        self.data
            .get(key)
            .expect("OrderedMap::at: key not found")
    }

    pub fn at_mut(&mut self, key: &K) -> &mut V {
        self.data
            .get_mut(key)
            .expect("OrderedMap::at: key not found")
    }

    pub fn lower_bound(&self, key: &K) -> Option<(&K, &V)> {
        self.data.range((Bound::Included(key), Bound::Unbounded)).next()
    }

    pub fn upper_bound(&self, key: &K) -> Option<(&K, &V)> {
        self.data.range((Bound::Excluded(key), Bound::Unbounded)).next()
    }

    pub fn min_key(&self) -> &K {
        self.data
            .keys()
            .next()
            .expect("OrderedMap::min_key: map is empty")
    }

    pub fn max_key(&self) -> &K {
        self.data
            .keys()
            .next_back()
            .expect("OrderedMap::max_key: map is empty")
    }

    pub fn first(&self) -> Option<(&K, &V)> {
        self.data.iter().next()
    }

    pub fn last(&self) -> Option<(&K, &V)> {
        self.data.iter().next_back()
    }

    pub fn iter(&self) -> btree_map::Iter<'_, K, V> {
        self.data.iter()
    }

    pub fn iter_mut(&mut self) -> btree_map::IterMut<'_, K, V> {
        self.data.iter_mut()
    }

    pub fn keys(&self) -> btree_map::Keys<'_, K, V> {
        self.data.keys()
    }

    pub fn values(&self) -> btree_map::Values<'_, K, V> {
        self.data.values()
    }

    pub fn values_mut(&mut self) -> btree_map::ValuesMut<'_, K, V> {
        self.data.values_mut()
    }

    pub fn begin(&self) -> btree_map::Iter<'_, K, V> {
        self.data.iter()
    }

    pub fn as_btree(&self) -> &BTreeMap<K, V> {
        &self.data
    }

    pub fn as_btree_mut(&mut self) -> &mut BTreeMap<K, V> {
        &mut self.data
    }

    pub fn to_vector(&self) -> Vector<(K, V)>
    where
        K: Clone,
        V: Clone,
    {
        let mut v = Vector::new();
        for (k, val) in self.data.iter() {
            v.push_back((k.clone(), val.clone()));
        }
        v
    }
}

impl<K: Ord, V> From<BTreeMap<K, V>> for OrderedMap<K, V> {
    fn from(data: BTreeMap<K, V>) -> Self {
        Self { data }
    }
}

impl<K: Ord, V> FromIterator<(K, V)> for OrderedMap<K, V> {
    fn from_iter<I: IntoIterator<Item = (K, V)>>(iter: I) -> Self {
        Self {
            data: BTreeMap::from_iter(iter),
        }
    }
}

impl<K: Ord, V> IntoIterator for OrderedMap<K, V> {
    type Item = (K, V);
    type IntoIter = btree_map::IntoIter<K, V>;

    fn into_iter(self) -> Self::IntoIter {
        self.data.into_iter()
    }
}

impl<'a, K: Ord, V> IntoIterator for &'a OrderedMap<K, V> {
    type Item = (&'a K, &'a V);
    type IntoIter = btree_map::Iter<'a, K, V>;

    fn into_iter(self) -> Self::IntoIter {
        self.data.iter()
    }
}
