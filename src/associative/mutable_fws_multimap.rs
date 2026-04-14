use crate::Vector;

#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct IndexEntry {
    pub begin: usize,
    pub size: usize,
    pub capacity: usize,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MutableFwsMultimap<K, V>
where
    V: Default + Clone,
{
    pub index: Vector<IndexEntry>,
    pub data: Vector<V>,
    pub free_buckets: Vec<Vector<IndexEntry>>,
    pub element_count: usize,
    _marker: std::marker::PhantomData<K>,
}

const LOG2_MAX_ENTRIES_PER_BUCKET: usize = 20;

impl<K, V: Default + Clone> Default for MutableFwsMultimap<K, V> {
    fn default() -> Self {
        Self {
            index: Vector::default(),
            data: Vector::default(),
            free_buckets: (0..=LOG2_MAX_ENTRIES_PER_BUCKET).map(|_| Vector::default()).collect(),
            element_count: 0,
            _marker: std::marker::PhantomData,
        }
    }
}

fn next_power_of_two(n: usize) -> usize {
    if n <= 1 {
        1
    } else {
        let mut p = 1usize;
        while p < n {
            p <<= 1;
        }
        p
    }
}

fn trailing_zeros_usize(n: usize) -> usize {
    n.trailing_zeros() as usize
}

impl<K, V: Default + Clone> MutableFwsMultimap<K, V> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn size(&self) -> usize {
        self.index.len()
    }

    pub fn len(&self) -> usize {
        self.index.len()
    }

    pub fn is_empty(&self) -> bool {
        self.index.is_empty()
    }

    pub fn empty(&self) -> bool {
        self.is_empty()
    }

    pub fn data_size(&self) -> usize {
        self.data.len()
    }

    pub fn element_count(&self) -> usize {
        self.element_count
    }

    pub fn max_entries_per_bucket(&self) -> usize {
        1 << LOG2_MAX_ENTRIES_PER_BUCKET
    }

    pub fn clear(&mut self) {
        self.index.clear();
        self.data.clear();
        for fb in self.free_buckets.iter_mut() {
            fb.clear();
        }
        self.element_count = 0;
    }

    pub fn reserve(&mut self, index_cap: usize, data_cap: usize) {
        self.index.reserve(index_cap);
        self.data.reserve(data_cap);
    }

    pub fn ensure_index(&mut self, map_index: usize) {
        if map_index >= self.index.len() {
            self.index.resize(map_index + 1, IndexEntry::default());
        }
    }

    pub fn push_back_entry(&mut self, map_index: usize, val: V) -> usize {
        self.ensure_index(map_index);
        let data_index = self.insert_new_entry(map_index);
        self.data[data_index] = val;
        self.element_count += 1;
        data_index
    }

    pub fn bucket_size(&self, map_index: usize) -> usize {
        if map_index >= self.index.len() {
            return 0;
        }
        self.index[map_index].size
    }

    pub fn bucket_capacity(&self, map_index: usize) -> usize {
        if map_index >= self.index.len() {
            return 0;
        }
        self.index[map_index].capacity
    }

    pub fn bucket_slice(&self, map_index: usize) -> &[V] {
        let idx = &self.index[map_index];
        &self.data.as_slice()[idx.begin..idx.begin + idx.size]
    }

    pub fn bucket_slice_mut(&mut self, map_index: usize) -> &mut [V] {
        let begin = self.index[map_index].begin;
        let size = self.index[map_index].size;
        &mut self.data.as_mut_slice()[begin..begin + size]
    }

    pub fn erase_bucket(&mut self, map_index: usize) {
        if map_index < self.index.len() {
            let mut idx = self.index[map_index];
            self.element_count -= idx.size;
            self.release_bucket(&mut idx);
            self.index[map_index] = idx;
        }
    }

    fn insert_new_entry(&mut self, map_index: usize) -> usize {
        let idx = self.index[map_index];
        if idx.size == idx.capacity {
            self.grow_bucket(map_index, idx.capacity + 1);
        }
        let idx = &mut self.index[map_index];
        let data_index = idx.begin + idx.size;
        idx.size += 1;
        data_index
    }

    fn grow_bucket(&mut self, map_index: usize, requested_capacity: usize) {
        assert!(requested_capacity > 0);
        let new_capacity = next_power_of_two(requested_capacity);
        let new_order = if new_capacity == 0 { 0 } else { trailing_zeros_usize(new_capacity) };
        assert!(new_order <= LOG2_MAX_ENTRIES_PER_BUCKET, "too many entries in a bucket");

        let old_bucket = self.index[map_index];

        if let Some(free) = self.pop_free_bucket(new_order) {
            if old_bucket.capacity != 0 {
                self.move_entries(old_bucket.begin, free.begin, old_bucket.size);
                let mut released = old_bucket;
                self.release_bucket(&mut released);
            }
            let idx = &mut self.index[map_index];
            idx.begin = free.begin;
            idx.capacity = free.capacity;
        } else {
            let at_end = old_bucket.begin + old_bucket.capacity == self.data.len();
            if at_end {
                let additional = new_capacity - old_bucket.capacity;
                let new_len = self.data.len() + additional;
                self.data.resize(new_len, V::default());
                self.index[map_index].capacity = new_capacity;
            } else {
                let new_begin = self.data.len();
                let new_len = new_begin + new_capacity;
                self.data.resize(new_len, V::default());
                self.move_entries(old_bucket.begin, new_begin, old_bucket.size);
                let idx = &mut self.index[map_index];
                idx.begin = new_begin;
                idx.capacity = new_capacity;
                let mut released = old_bucket;
                self.release_bucket(&mut released);
            }
        }
    }

    fn pop_free_bucket(&mut self, requested_order: usize) -> Option<IndexEntry> {
        let vec = &mut self.free_buckets[requested_order];
        if vec.is_empty() {
            None
        } else {
            let last_idx = vec.len() - 1;
            let entry = vec[last_idx];
            vec.remove(last_idx);
            Some(entry)
        }
    }

    fn release_bucket(&mut self, bucket: &mut IndexEntry) {
        if bucket.capacity != 0 {
            let order = trailing_zeros_usize(bucket.capacity);
            bucket.size = 0;
            self.free_buckets[order].push(*bucket);
            bucket.capacity = 0;
        }
    }

    fn move_entries(&mut self, old_data_index: usize, new_data_index: usize, count: usize) {
        if count == 0 {
            return;
        }
        for i in 0..count {
            let v = std::mem::take(&mut self.data[old_data_index + i]);
            self.data[new_data_index + i] = v;
        }
    }

    pub fn insert(&mut self, _key: K, value: V) {
        let map_index = self.index.len();
        self.ensure_index(map_index);
        self.push_back_entry(map_index, value);
    }
}
