//! Sorted-key Pod set. Keys are arbitrary byte runs.
//!
//! Wire layout mirrors [`crate::assoc::Map`] minus the value:
//!
//! ```text
//! [count : u32]
//! [Entry_0] [Entry_1] ... [Entry_{N-1}]    ← fixed 8-byte entries (sorted)
//! [key_0 bytes][key_1 bytes]...            ← blob region
//! ```
//!
//! `SetEntry` is `(key_off: u32, key_len: u32)`. `OSet` is an alias.

use std::cmp::Ordering;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct SetEntry {
    pub key_off: u32,
    pub key_len: u32,
}

const ENTRY_SIZE: usize = std::mem::size_of::<SetEntry>();

#[datapod::datapod]
pub struct Set {
    #[dp(bytes)]
    pub data: Vec<u8>,
}

pub type OSet = Set;

impl Default for Set {
    fn default() -> Self {
        Self { data: 0u32.to_le_bytes().to_vec() }
    }
}

impl Set {
    pub fn new() -> Self {
        Self::default()
    }

    fn count(&self) -> u32 {
        u32::from_le_bytes(self.data[0..4].try_into().unwrap())
    }

    fn set_count(&mut self, count: u32) {
        self.data[0..4].copy_from_slice(&count.to_le_bytes());
    }

    fn entry_table_bytes(&self) -> usize {
        self.count() as usize * ENTRY_SIZE
    }

    fn blob_offset(&self) -> usize {
        4 + self.entry_table_bytes()
    }

    fn entries(&self) -> &[SetEntry] {
        let start = 4;
        let end = self.blob_offset();
        bytemuck::cast_slice(&self.data[start..end])
    }

    pub fn size(&self) -> usize {
        self.count() as usize
    }

    pub fn empty(&self) -> bool {
        self.count() == 0
    }

    pub fn key_at(&self, i: usize) -> &[u8] {
        let e = self.entries()[i];
        let base = self.blob_offset();
        &self.data[base + e.key_off as usize..base + (e.key_off + e.key_len) as usize]
    }

    fn binary_search(&self, key: &[u8]) -> Result<usize, usize> {
        let n = self.size();
        let mut lo = 0usize;
        let mut hi = n;
        while lo < hi {
            let mid = (lo + hi) / 2;
            match self.key_at(mid).cmp(key) {
                Ordering::Less => lo = mid + 1,
                Ordering::Greater => hi = mid,
                Ordering::Equal => return Ok(mid),
            }
        }
        Err(lo)
    }

    pub fn contains(&self, key: &[u8]) -> bool {
        self.binary_search(key).is_ok()
    }

    pub fn contains_str(&self, key: &str) -> bool {
        self.contains(key.as_bytes())
    }

    pub fn contains_pod<K: bytemuck::Pod>(&self, key: &K) -> bool {
        self.contains(bytemuck::bytes_of(key))
    }

    /// Insert a key. Returns `true` if the key was newly added, `false` if
    /// it already existed.
    pub fn insert(&mut self, key: &[u8]) -> bool {
        match self.binary_search(key) {
            Ok(_) => false,
            Err(idx) => {
                self.insert_at(idx, key);
                true
            }
        }
    }

    pub fn insert_str(&mut self, key: &str) -> bool {
        self.insert(key.as_bytes())
    }

    pub fn insert_pod<K: bytemuck::Pod>(&mut self, key: &K) -> bool {
        self.insert(bytemuck::bytes_of(key))
    }

    /// Remove a key. Returns `true` if it was present.
    pub fn remove(&mut self, key: &[u8]) -> bool {
        let idx = match self.binary_search(key) {
            Ok(idx) => idx,
            Err(_) => return false,
        };
        self.remove_at(idx);
        true
    }

    pub fn iter(&self) -> SetIter<'_> {
        SetIter { set: self, cursor: 0 }
    }

    pub fn clear(&mut self) {
        self.data.clear();
        self.data.extend_from_slice(&0u32.to_le_bytes());
    }

    fn insert_at(&mut self, pos: usize, key: &[u8]) {
        let old_count = self.count() as usize;
        let mut keys: Vec<Vec<u8>> = Vec::with_capacity(old_count + 1);
        for i in 0..old_count {
            keys.push(self.key_at(i).to_vec());
        }
        keys.insert(pos, key.to_vec());
        self.rebuild(&keys);
    }

    fn remove_at(&mut self, pos: usize) {
        let old_count = self.count() as usize;
        let mut keys: Vec<Vec<u8>> = Vec::with_capacity(old_count - 1);
        for i in 0..old_count {
            if i == pos {
                continue;
            }
            keys.push(self.key_at(i).to_vec());
        }
        self.rebuild(&keys);
    }

    fn rebuild(&mut self, keys: &[Vec<u8>]) {
        let count = keys.len() as u32;
        let blob_size: usize = keys.iter().map(|k| k.len()).sum();
        let total = 4 + count as usize * ENTRY_SIZE + blob_size;
        let mut new_data = Vec::with_capacity(total);

        new_data.extend_from_slice(&count.to_le_bytes());
        let entries_start = new_data.len();
        new_data.resize(entries_start + count as usize * ENTRY_SIZE, 0);
        let blob_start = new_data.len();

        let mut blob_cursor = 0u32;
        let mut table: Vec<SetEntry> = Vec::with_capacity(count as usize);
        for k in keys {
            let key_off = blob_cursor;
            new_data.extend_from_slice(k);
            blob_cursor += k.len() as u32;
            table.push(SetEntry { key_off, key_len: k.len() as u32 });
        }

        let table_bytes: &[u8] = bytemuck::cast_slice(&table);
        new_data[entries_start..blob_start].copy_from_slice(table_bytes);

        self.data = new_data;
        self.set_count(count);
    }
}

pub struct SetIter<'a> {
    set: &'a Set,
    cursor: usize,
}

impl<'a> Iterator for SetIter<'a> {
    type Item = &'a [u8];
    fn next(&mut self) -> Option<Self::Item> {
        if self.cursor >= self.set.size() {
            return None;
        }
        let i = self.cursor;
        self.cursor += 1;
        Some(self.set.key_at(i))
    }
}
