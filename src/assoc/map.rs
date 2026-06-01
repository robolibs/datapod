//! Sorted key→value Pod map. Keys and values are arbitrary byte runs.
//!
//! ## Wire layout (single `#[dp(bytes)]` payload)
//!
//! ```text
//! [count : u32]                                     ← number of entries
//! [Entry_0] [Entry_1] ... [Entry_{N-1}]             ← fixed 16-byte entries (sorted by key bytes)
//! [key_0 bytes][value_0 bytes][key_1 bytes][...]    ← blob region
//! ```
//!
//! Each `MapEntry` carries `(key_off, key_len, value_off, value_len)`,
//! all u32, where `key_off` / `value_off` are byte offsets into the blob
//! region (the bytes after the entry table). The entry array is kept
//! sorted by the actual key bytes (lexicographic) so lookups are O(log n).
//!
//! ## Usage
//!
//! Three insert/get layers stacked on top of the raw bytes API:
//!
//! - `insert_str(k: &str, v: &str)` / `get_str(k: &str) -> Option<&str>`
//! - `insert_pod::<K: Pod, V: Pod>(&K, &V)` / `get_pod::<K, V>(&K) -> Option<V>`
//! - raw `insert(&[u8], &[u8])` / `get(&[u8]) -> Option<&[u8]>`
//!
//! `OMap` is an alias — the wire format is already ordered.

use std::cmp::Ordering;

/// Fixed-size sorted-array entry. `(key_off, key_len, value_off, value_len)`
/// are all u32 byte offsets/lengths into the blob region that follows the
/// entry table.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct MapEntry {
    pub key_off: u32,
    pub key_len: u32,
    pub value_off: u32,
    pub value_len: u32,
}

const ENTRY_SIZE: usize = std::mem::size_of::<MapEntry>();

#[datapod::datapod]
pub struct Map {
    #[dp(bytes)]
    pub data: Vec<u8>,
}

/// "Ordered map" alias — same wire format, sorted by key bytes.
pub type OMap = Map;

impl Default for Map {
    fn default() -> Self {
        // Bootstrap with count = 0 and no entries / no blob.
        Self {
            data: 0u32.to_le_bytes().to_vec(),
        }
    }
}

impl Map {
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

    fn entries(&self) -> &[MapEntry] {
        let start = 4;
        let end = self.blob_offset();
        bytemuck::cast_slice(&self.data[start..end])
    }

    fn entries_mut(&mut self) -> &mut [MapEntry] {
        let start = 4;
        let end = self.blob_offset();
        bytemuck::cast_slice_mut(&mut self.data[start..end])
    }

    pub fn size(&self) -> usize {
        self.count() as usize
    }

    pub fn empty(&self) -> bool {
        self.count() == 0
    }

    /// Get the key bytes for entry `i`.
    pub fn key_at(&self, i: usize) -> &[u8] {
        let e = self.entries()[i];
        let base = self.blob_offset();
        &self.data[base + e.key_off as usize..base + (e.key_off + e.key_len) as usize]
    }

    /// Get the value bytes for entry `i`.
    pub fn value_at(&self, i: usize) -> &[u8] {
        let e = self.entries()[i];
        let base = self.blob_offset();
        &self.data[base + e.value_off as usize..base + (e.value_off + e.value_len) as usize]
    }

    /// Binary-search for `key`. Returns `Ok(idx)` if found, `Err(idx)` for
    /// the insertion point.
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

    pub fn contains_key(&self, key: &[u8]) -> bool {
        self.binary_search(key).is_ok()
    }

    pub fn get(&self, key: &[u8]) -> Option<&[u8]> {
        self.binary_search(key).ok().map(|i| self.value_at(i))
    }

    pub fn get_str(&self, key: &str) -> Option<&str> {
        self.get(key.as_bytes())
            .and_then(|b| std::str::from_utf8(b).ok())
    }

    pub fn get_pod<K: bytemuck::Pod, V: bytemuck::Pod>(&self, key: &K) -> Option<V> {
        // Use pod_read_unaligned: blob entries are densely packed with no
        // per-entry alignment padding, so the V bytes may not be at an
        // address aligned for V.
        self.get(bytemuck::bytes_of(key))
            .map(bytemuck::pod_read_unaligned::<V>)
    }

    /// Insert or replace. Returns the old value bytes if a key match was
    /// found and replaced.
    pub fn insert(&mut self, key: &[u8], value: &[u8]) -> Option<Vec<u8>> {
        match self.binary_search(key) {
            Ok(idx) => {
                let old = self.value_at(idx).to_vec();
                self.replace_value_at(idx, value);
                Some(old)
            }
            Err(idx) => {
                self.insert_at(idx, key, value);
                None
            }
        }
    }

    pub fn insert_str(&mut self, key: &str, value: &str) -> Option<Vec<u8>> {
        self.insert(key.as_bytes(), value.as_bytes())
    }

    pub fn insert_pod<K: bytemuck::Pod, V: bytemuck::Pod>(
        &mut self,
        key: &K,
        value: &V,
    ) -> Option<Vec<u8>> {
        self.insert(bytemuck::bytes_of(key), bytemuck::bytes_of(value))
    }

    /// Remove a key. Returns the removed value bytes if present.
    pub fn remove(&mut self, key: &[u8]) -> Option<Vec<u8>> {
        let idx = self.binary_search(key).ok()?;
        let old = self.value_at(idx).to_vec();
        self.remove_at(idx);
        Some(old)
    }

    /// Iterate `(key_bytes, value_bytes)` in sorted-key order.
    pub fn iter(&self) -> MapIter<'_> {
        MapIter {
            map: self,
            cursor: 0,
        }
    }

    pub fn clear(&mut self) {
        self.data.clear();
        self.data.extend_from_slice(&0u32.to_le_bytes());
    }

    // -------------------------------------------------------------------
    // Mutation helpers — they rebuild the data buffer because every
    // insert/remove shifts the blob region and every offset.
    // -------------------------------------------------------------------

    /// Insert a new entry at `pos` in the sorted order.
    fn insert_at(&mut self, pos: usize, key: &[u8], value: &[u8]) {
        let old_count = self.count() as usize;

        // Snapshot existing entries + their blob slices so we can rebuild.
        let mut entries: Vec<(Vec<u8>, Vec<u8>)> = Vec::with_capacity(old_count + 1);
        for i in 0..old_count {
            entries.push((self.key_at(i).to_vec(), self.value_at(i).to_vec()));
        }
        entries.insert(pos, (key.to_vec(), value.to_vec()));

        self.rebuild(&entries);
    }

    fn remove_at(&mut self, pos: usize) {
        let old_count = self.count() as usize;
        let mut entries: Vec<(Vec<u8>, Vec<u8>)> = Vec::with_capacity(old_count - 1);
        for i in 0..old_count {
            if i == pos {
                continue;
            }
            entries.push((self.key_at(i).to_vec(), self.value_at(i).to_vec()));
        }
        self.rebuild(&entries);
    }

    fn replace_value_at(&mut self, pos: usize, value: &[u8]) {
        let old_count = self.count() as usize;
        let mut entries: Vec<(Vec<u8>, Vec<u8>)> = Vec::with_capacity(old_count);
        for i in 0..old_count {
            let v = if i == pos {
                value.to_vec()
            } else {
                self.value_at(i).to_vec()
            };
            entries.push((self.key_at(i).to_vec(), v));
        }
        self.rebuild(&entries);
    }

    /// Rebuild the entire wire buffer from a snapshot.
    fn rebuild(&mut self, entries: &[(Vec<u8>, Vec<u8>)]) {
        let count = entries.len() as u32;

        // First pass: compute total blob size.
        let mut blob_size = 0usize;
        for (k, v) in entries {
            blob_size += k.len() + v.len();
        }

        let total = 4 + (count as usize * ENTRY_SIZE) + blob_size;
        let mut new_data = Vec::with_capacity(total);

        // count
        new_data.extend_from_slice(&count.to_le_bytes());

        // Entry table (will fill in after we know offsets).
        let entries_start = new_data.len();
        new_data.resize(entries_start + count as usize * ENTRY_SIZE, 0);

        // Blob region.
        let blob_start = new_data.len();
        let mut blob_cursor = 0u32;
        let mut table: Vec<MapEntry> = Vec::with_capacity(count as usize);
        for (k, v) in entries {
            let key_off = blob_cursor;
            new_data.extend_from_slice(k);
            blob_cursor += k.len() as u32;
            let value_off = blob_cursor;
            new_data.extend_from_slice(v);
            blob_cursor += v.len() as u32;
            table.push(MapEntry {
                key_off,
                key_len: k.len() as u32,
                value_off,
                value_len: v.len() as u32,
            });
        }

        // Write entry table.
        let table_bytes: &[u8] = bytemuck::cast_slice(&table);
        new_data[entries_start..blob_start].copy_from_slice(table_bytes);

        self.data = new_data;
        self.set_count(count);
    }
}

pub struct MapIter<'a> {
    map: &'a Map,
    cursor: usize,
}

impl<'a> Iterator for MapIter<'a> {
    type Item = (&'a [u8], &'a [u8]);
    fn next(&mut self) -> Option<Self::Item> {
        if self.cursor >= self.map.size() {
            return None;
        }
        let i = self.cursor;
        self.cursor += 1;
        Some((self.map.key_at(i), self.map.value_at(i)))
    }
}
