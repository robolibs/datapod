use crate::Vector;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FwsMultimap<K, V> {
    pub data: Vector<V>,
    pub index: Vector<usize>,
    pub current_start: usize,
    pub complete: bool,
    _marker: std::marker::PhantomData<K>,
}

impl<K, V> Default for FwsMultimap<K, V> {
    fn default() -> Self {
        Self {
            data: Vector::default(),
            index: Vector::default(),
            current_start: 0,
            complete: false,
            _marker: std::marker::PhantomData,
        }
    }
}

#[derive(Debug)]
pub struct FwsMultimapEntry<'a, V> {
    data: &'a Vector<V>,
    start: usize,
    end: usize,
}

impl<'a, V> FwsMultimapEntry<'a, V> {
    pub fn size(&self) -> usize {
        self.end - self.start
    }

    pub fn len(&self) -> usize {
        self.size()
    }

    pub fn is_empty(&self) -> bool {
        self.size() == 0
    }

    pub fn iter(&self) -> std::slice::Iter<'a, V> {
        self.data.as_slice()[self.start..self.end].iter()
    }

    pub fn as_slice(&self) -> &'a [V] {
        &self.data.as_slice()[self.start..self.end]
    }
}

impl<'a, V> std::ops::Index<usize> for FwsMultimapEntry<'a, V> {
    type Output = V;
    fn index(&self, i: usize) -> &V {
        &self.data[self.start + i]
    }
}

impl<K, V> FwsMultimap<K, V> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn push_back(&mut self, val: V) {
        debug_assert!(!self.complete);
        self.data.push(val);
    }

    pub fn emplace_back(&mut self, val: V) {
        self.push_back(val);
    }

    pub fn current_key(&self) -> usize {
        self.index.len()
    }

    pub fn finish_key(&mut self) {
        debug_assert!(!self.complete);
        self.index.push(self.current_start);
        self.current_start = self.data.len();
    }

    pub fn finish_map(&mut self) {
        debug_assert!(!self.complete);
        self.index.push(self.data.len());
        self.complete = true;
    }

    pub fn reserve_index(&mut self, size: usize) {
        self.index.reserve(size + 1);
    }

    pub fn index_size(&self) -> usize {
        self.index.len()
    }

    pub fn data_size(&self) -> usize {
        self.data.len()
    }

    pub fn finished(&self) -> bool {
        self.complete
    }

    pub fn get_entry(&self, key: usize) -> FwsMultimapEntry<'_, V> {
        FwsMultimapEntry {
            data: &self.data,
            start: self.index[key],
            end: self.index[key + 1],
        }
    }

    pub fn insert(&mut self, _key: K, value: V) {
        self.push_back(value);
        self.finish_key();
    }
}
