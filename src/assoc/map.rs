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

use crate::{DataPodAccess, DataPodValidate, WireError};

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
#[dp(manual_access)]
pub struct Map {
    #[dp(bytes)]
    pub data: Vec<u8>,
}

/// "Ordered map" alias — same wire format, sorted by key bytes.
pub type OMap = Map;

impl Default for Map {
    fn default() -> Self {
        // Bootstrap with count = 0 and no entries / no blob.
        let mut data = Vec::new();
        data.extend_from_slice(&0u32.to_le_bytes());
        Self { data }
    }
}

impl Map {
    pub fn new() -> Self {
        Self::default()
    }

    fn count(&self) -> u32 {
        self.try_count().unwrap_or(0)
    }

    fn try_count(&self) -> Result<u32, WireError> {
        let Some(raw) = self.data.get(0..4) else {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "map payload too short: got {}, need at least 4",
                self.data.len()
            )));
        };
        let mut bytes = [0u8; 4];
        bytes.copy_from_slice(raw);
        Ok(u32::from_le_bytes(bytes))
    }

    fn set_count(&mut self, count: u32) -> Result<(), WireError> {
        let slot = self.data.get_mut(0..4).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("map count range is out of bounds")
        })?;
        slot.copy_from_slice(&count.to_le_bytes());
        Ok(())
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    pub fn try_size(&self) -> Result<usize, WireError> {
        self.validate_owned()?;
        self.try_count_usize()
    }

    pub fn empty(&self) -> bool {
        self.try_empty().unwrap_or(true)
    }

    pub fn try_empty(&self) -> Result<bool, WireError> {
        Ok(self.try_size()? == 0)
    }

    /// Get the key bytes for entry `i`.
    pub fn key_at(&self, i: usize) -> &[u8] {
        self.try_key_at(i).unwrap_or(&[])
    }

    pub fn try_key_at(&self, i: usize) -> Result<&[u8], WireError> {
        self.validate_owned()?;
        let len = self.try_size()?;
        if i >= len {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "map entry index out of bounds: {i} for len {len}"
            )));
        }
        self.key_at_validated(i)
    }

    fn key_at_validated(&self, i: usize) -> Result<&[u8], WireError> {
        let e = self.entry_at_validated(i)?;
        self.blob_range_validated(e.key_off, e.key_len, "map key")
    }

    /// Get the value bytes for entry `i`.
    pub fn value_at(&self, i: usize) -> &[u8] {
        self.try_value_at(i).unwrap_or(&[])
    }

    pub fn try_value_at(&self, i: usize) -> Result<&[u8], WireError> {
        self.validate_owned()?;
        let len = self.try_size()?;
        if i >= len {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "map entry index out of bounds: {i} for len {len}"
            )));
        }
        self.value_at_validated(i)
    }

    fn value_at_validated(&self, i: usize) -> Result<&[u8], WireError> {
        let e = self.entry_at_validated(i)?;
        self.blob_range_validated(e.value_off, e.value_len, "map value")
    }

    fn entry_at_validated(&self, i: usize) -> Result<MapEntry, WireError> {
        let start = i
            .checked_mul(ENTRY_SIZE)
            .and_then(|offset| 4usize.checked_add(offset))
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("map entry offset overflowed"))?;
        let end = start
            .checked_add(ENTRY_SIZE)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("map entry end overflowed"))?;
        let bytes = self.data.get(start..end).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("map entry range is out of bounds")
        })?;
        Ok(bytemuck::pod_read_unaligned(bytes))
    }

    fn blob_offset_validated(&self) -> Result<usize, WireError> {
        let table_len = self
            .try_count_usize()?
            .checked_mul(ENTRY_SIZE)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("entry table length overflowed"))?;
        4usize
            .checked_add(table_len)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("blob offset overflowed"))
    }

    fn blob_range_validated(
        &self,
        offset: u32,
        len: u32,
        context: &'static str,
    ) -> Result<&[u8], WireError> {
        let base = self.blob_offset_validated()?;
        let offset = u32_to_usize::<Self>(offset, "blob offset")?;
        let len = u32_to_usize::<Self>(len, "blob length")?;
        let start = base.checked_add(offset).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>(format!("{context} start overflowed"))
        })?;
        let end = start.checked_add(len).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>(format!("{context} end overflowed"))
        })?;
        self.data.get(start..end).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>(format!("{context} range is out of bounds"))
        })
    }

    fn try_binary_search(&self, key: &[u8]) -> Result<Result<usize, usize>, WireError> {
        self.validate_owned()?;
        let n = self.try_size()?;
        let mut lo = 0usize;
        let mut hi = n;
        while lo < hi {
            let span = hi.checked_sub(lo).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("map binary-search span underflowed")
            })?;
            let mid = lo.checked_add(span / 2).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("map binary-search midpoint overflowed")
            })?;
            match self.key_at_validated(mid)?.cmp(key) {
                Ordering::Less => {
                    lo = mid.checked_add(1).ok_or_else(|| {
                        crate::wire::invalid_payload::<Self>("map binary-search cursor overflowed")
                    })?
                }
                Ordering::Greater => hi = mid,
                Ordering::Equal => return Ok(Ok(mid)),
            }
        }
        Ok(Err(lo))
    }

    pub fn contains_key(&self, key: &[u8]) -> bool {
        self.try_contains_key(key).unwrap_or(false)
    }

    pub fn try_contains_key(&self, key: &[u8]) -> Result<bool, WireError> {
        Ok(self.try_binary_search(key)?.is_ok())
    }

    pub fn get(&self, key: &[u8]) -> Option<&[u8]> {
        self.try_get(key).unwrap_or(None)
    }

    pub fn try_get(&self, key: &[u8]) -> Result<Option<&[u8]>, WireError> {
        Ok(match self.try_binary_search(key)? {
            Ok(i) => Some(self.try_value_at(i)?),
            Err(_) => None,
        })
    }

    pub fn get_str(&self, key: &str) -> Option<&str> {
        self.try_get_str(key).unwrap_or(None)
    }

    pub fn try_get_str(&self, key: &str) -> Result<Option<&str>, WireError> {
        self.try_get(key.as_bytes())?
            .map(|bytes| {
                std::str::from_utf8(bytes)
                    .map_err(|err| crate::wire::invalid_payload::<Self>(err.to_string()))
            })
            .transpose()
    }

    pub fn get_pod<K: bytemuck::Pod, V: bytemuck::Pod>(&self, key: &K) -> Option<V> {
        self.try_get_pod(key).unwrap_or(None)
    }

    pub fn try_get_pod<K: bytemuck::Pod, V: bytemuck::Pod>(
        &self,
        key: &K,
    ) -> Result<Option<V>, WireError> {
        let Some(bytes) = self.try_get(bytemuck::bytes_of(key))? else {
            return Ok(None);
        };
        let expected = core::mem::size_of::<V>();
        if bytes.len() != expected {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "pod value size mismatch: got {} bytes, expected {expected}",
                bytes.len()
            )));
        }
        // Use pod_read_unaligned: blob entries are densely packed with no
        // per-entry alignment padding, so the V bytes may not be at an
        // address aligned for V.
        Ok(Some(bytemuck::pod_read_unaligned::<V>(bytes)))
    }

    /// Insert or replace. Returns the old value bytes if a key match was
    /// found and replaced.
    pub fn insert(&mut self, key: &[u8], value: &[u8]) -> Option<Vec<u8>> {
        self.try_insert(key, value).unwrap_or(None)
    }

    /// Fallible insert/replace. This refuses to mutate if the existing owned
    /// buffer is malformed or if the rebuilt wire buffer would overflow any
    /// u32 count/offset/length fields.
    pub fn try_insert(&mut self, key: &[u8], value: &[u8]) -> Result<Option<Vec<u8>>, WireError> {
        match self.try_binary_search(key)? {
            Ok(idx) => {
                let old = Self::try_copy_bytes(self.try_value_at(idx)?, "old map value")?;
                self.try_replace_value_at(idx, value)?;
                Ok(Some(old))
            }
            Err(idx) => {
                self.try_insert_at(idx, key, value)?;
                Ok(None)
            }
        }
    }

    pub fn insert_str(&mut self, key: &str, value: &str) -> Option<Vec<u8>> {
        self.insert(key.as_bytes(), value.as_bytes())
    }

    pub fn try_insert_str(&mut self, key: &str, value: &str) -> Result<Option<Vec<u8>>, WireError> {
        self.try_insert(key.as_bytes(), value.as_bytes())
    }

    pub fn insert_pod<K: bytemuck::Pod, V: bytemuck::Pod>(
        &mut self,
        key: &K,
        value: &V,
    ) -> Option<Vec<u8>> {
        self.insert(bytemuck::bytes_of(key), bytemuck::bytes_of(value))
    }

    pub fn try_insert_pod<K: bytemuck::Pod, V: bytemuck::Pod>(
        &mut self,
        key: &K,
        value: &V,
    ) -> Result<Option<Vec<u8>>, WireError> {
        self.try_insert(bytemuck::bytes_of(key), bytemuck::bytes_of(value))
    }

    /// Remove a key. Returns the removed value bytes if present.
    pub fn remove(&mut self, key: &[u8]) -> Option<Vec<u8>> {
        self.try_remove(key).unwrap_or(None)
    }

    pub fn try_remove(&mut self, key: &[u8]) -> Result<Option<Vec<u8>>, WireError> {
        let idx = match self.try_binary_search(key)? {
            Ok(idx) => idx,
            Err(_) => return Ok(None),
        };
        let old = Self::try_copy_bytes(self.try_value_at(idx)?, "removed map value")?;
        self.try_remove_at(idx)?;
        Ok(Some(old))
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

    fn try_insert_at(&mut self, pos: usize, key: &[u8], value: &[u8]) -> Result<(), WireError> {
        self.validate_owned()?;
        let old_count = self.try_count_usize()?;

        // Snapshot existing entries + their blob slices so we can rebuild.
        let mut entries: Vec<(Vec<u8>, Vec<u8>)> = Vec::new();
        let snapshot_count = old_count
            .checked_add(1)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("map insert count overflowed"))?;
        Self::try_reserve_vec(&mut entries, snapshot_count, "map entry snapshot")?;
        for i in 0..old_count {
            entries.push((
                Self::try_copy_bytes(self.key_at_validated(i)?, "map key snapshot")?,
                Self::try_copy_bytes(self.value_at_validated(i)?, "map value snapshot")?,
            ));
        }
        entries.insert(
            pos,
            (
                Self::try_copy_bytes(key, "new map key")?,
                Self::try_copy_bytes(value, "new map value")?,
            ),
        );

        self.try_rebuild(&entries)
    }

    fn try_remove_at(&mut self, pos: usize) -> Result<(), WireError> {
        self.validate_owned()?;
        let old_count = self.try_count_usize()?;
        let mut entries: Vec<(Vec<u8>, Vec<u8>)> = Vec::new();
        let snapshot_count = old_count
            .checked_sub(1)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("map remove count underflowed"))?;
        Self::try_reserve_vec(&mut entries, snapshot_count, "map entry snapshot")?;
        for i in 0..old_count {
            if i == pos {
                continue;
            }
            entries.push((
                Self::try_copy_bytes(self.key_at_validated(i)?, "map key snapshot")?,
                Self::try_copy_bytes(self.value_at_validated(i)?, "map value snapshot")?,
            ));
        }
        self.try_rebuild(&entries)
    }

    fn try_replace_value_at(&mut self, pos: usize, value: &[u8]) -> Result<(), WireError> {
        self.validate_owned()?;
        let old_count = self.try_count_usize()?;
        let mut entries: Vec<(Vec<u8>, Vec<u8>)> = Vec::new();
        Self::try_reserve_vec(&mut entries, old_count, "map entry snapshot")?;
        for i in 0..old_count {
            let v = if i == pos {
                Self::try_copy_bytes(value, "new map value")?
            } else {
                Self::try_copy_bytes(self.value_at_validated(i)?, "map value snapshot")?
            };
            entries.push((
                Self::try_copy_bytes(self.key_at_validated(i)?, "map key snapshot")?,
                v,
            ));
        }
        self.try_rebuild(&entries)
    }

    fn try_rebuild(&mut self, entries: &[(Vec<u8>, Vec<u8>)]) -> Result<(), WireError> {
        let count = u32::try_from(entries.len())
            .map_err(|_| crate::wire::invalid_payload::<Self>("entry count exceeds u32"))?;
        let count_usize = u32_to_usize::<Self>(count, "entry count")?;

        // First pass: compute total blob size.
        let mut blob_size = 0usize;
        for (k, v) in entries {
            let pair_len = k.len().checked_add(v.len()).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("entry blob length overflowed")
            })?;
            blob_size = blob_size.checked_add(pair_len).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("map blob length overflowed")
            })?;
        }

        let table_len = count_usize
            .checked_mul(ENTRY_SIZE)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("entry table length overflowed"))?;
        let total = 4usize
            .checked_add(table_len)
            .and_then(|base| base.checked_add(blob_size))
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("map wire length overflowed"))?;
        let mut new_data = Vec::new();
        Self::try_reserve_vec(&mut new_data, total, "map wire buffer")?;

        // count
        new_data.extend_from_slice(&count.to_le_bytes());

        // Entry table (will fill in after we know offsets).
        let entries_start = new_data.len();
        let entries_end = entries_start.checked_add(table_len).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("map entry table end overflowed")
        })?;
        new_data.resize(entries_end, 0);

        // Blob region.
        let blob_start = new_data.len();
        let mut blob_cursor = 0u32;
        let mut table: Vec<MapEntry> = Vec::new();
        Self::try_reserve_vec(&mut table, count_usize, "map entry table")?;
        for (k, v) in entries {
            let key_off = blob_cursor;
            let key_len = u32::try_from(k.len())
                .map_err(|_| crate::wire::invalid_payload::<Self>("key length exceeds u32"))?;
            new_data.extend_from_slice(k);
            blob_cursor = blob_cursor
                .checked_add(key_len)
                .ok_or_else(|| crate::wire::invalid_payload::<Self>("key offset overflowed u32"))?;
            let value_off = blob_cursor;
            let value_len = u32::try_from(v.len())
                .map_err(|_| crate::wire::invalid_payload::<Self>("value length exceeds u32"))?;
            new_data.extend_from_slice(v);
            blob_cursor = blob_cursor.checked_add(value_len).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("value offset overflowed u32")
            })?;
            table.push(MapEntry {
                key_off,
                key_len,
                value_off,
                value_len,
            });
        }

        // Write entry table.
        let table_bytes: &[u8] = bytemuck::cast_slice(&table);
        let table_slot = new_data.get_mut(entries_start..blob_start).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("map entry table range is out of bounds")
        })?;
        table_slot.copy_from_slice(table_bytes);

        self.data = new_data;
        self.set_count(count)?;
        Ok(())
    }

    fn try_reserve_vec<T>(
        vec: &mut Vec<T>,
        additional: usize,
        context: &'static str,
    ) -> Result<(), WireError> {
        vec.try_reserve_exact(additional).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "failed to reserve {additional} items for {context}: {err}"
            ))
        })
    }

    fn try_copy_bytes(bytes: &[u8], context: &'static str) -> Result<Vec<u8>, WireError> {
        let mut out = Vec::new();
        Self::try_reserve_vec(&mut out, bytes.len(), context)?;
        out.extend_from_slice(bytes);
        Ok(out)
    }

    fn validate_owned(&self) -> Result<(), WireError> {
        <Self as DataPodValidate>::validate_wire_parts(&MapHeader {}, &self.data)
    }

    fn try_count_usize(&self) -> Result<usize, WireError> {
        u32_to_usize::<Self>(self.try_count()?, "count")
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

/// Borrowed, validation-backed view over a `Map` wire payload.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct MapView<'a> {
    pub data: &'a [u8],
}

impl<'a> MapView<'a> {
    pub fn payload_bytes(&self) -> &'a [u8] {
        self.data
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    pub fn try_size(&self) -> Result<usize, WireError> {
        Map::validate_wire_parts(&MapHeader {}, self.data)?;
        u32_to_usize::<Map>(self.count()?, "count")
    }

    pub fn is_empty(&self) -> bool {
        self.try_size().map_or(true, |size| size == 0)
    }

    pub fn key_at(&self, index: usize) -> Result<&'a [u8], WireError> {
        let entry = self.entry_at(index)?;
        let base = self.blob_offset()?;
        let key_off = u32_to_usize::<Map>(entry.key_off, "key_off")?;
        let key_len = u32_to_usize::<Map>(entry.key_len, "key_len")?;
        let start = base
            .checked_add(key_off)
            .ok_or_else(|| crate::wire::invalid_payload::<Map>("map key start overflowed"))?;
        let end = start
            .checked_add(key_len)
            .ok_or_else(|| crate::wire::invalid_payload::<Map>("map key end overflowed"))?;
        self.data
            .get(start..end)
            .ok_or_else(|| crate::wire::invalid_payload::<Map>("map key range is out of bounds"))
    }

    pub fn value_at(&self, index: usize) -> Result<&'a [u8], WireError> {
        let entry = self.entry_at(index)?;
        let base = self.blob_offset()?;
        let value_off = u32_to_usize::<Map>(entry.value_off, "value_off")?;
        let value_len = u32_to_usize::<Map>(entry.value_len, "value_len")?;
        let start = base
            .checked_add(value_off)
            .ok_or_else(|| crate::wire::invalid_payload::<Map>("map value start overflowed"))?;
        let end = start
            .checked_add(value_len)
            .ok_or_else(|| crate::wire::invalid_payload::<Map>("map value end overflowed"))?;
        self.data
            .get(start..end)
            .ok_or_else(|| crate::wire::invalid_payload::<Map>("map value range is out of bounds"))
    }

    fn count(&self) -> Result<u32, WireError> {
        let Some(raw) = self.data.get(0..4) else {
            return Err(crate::wire::invalid_payload::<Map>(format!(
                "map payload too short: got {}, need at least 4",
                self.data.len()
            )));
        };
        let mut bytes = [0u8; 4];
        bytes.copy_from_slice(raw);
        Ok(u32::from_le_bytes(bytes))
    }

    fn blob_offset(&self) -> Result<usize, WireError> {
        let table_len = self
            .try_size()?
            .checked_mul(ENTRY_SIZE)
            .ok_or_else(|| crate::wire::invalid_payload::<Map>("entry table length overflowed"))?;
        4usize
            .checked_add(table_len)
            .ok_or_else(|| crate::wire::invalid_payload::<Map>("blob offset overflowed"))
    }

    fn entry_at(&self, index: usize) -> Result<MapEntry, WireError> {
        let size = self.try_size()?;
        if index >= size {
            return Err(crate::wire::invalid_header::<Map>(format!(
                "map entry index out of bounds: {index} for len {}",
                size
            )));
        }
        let start = index
            .checked_mul(ENTRY_SIZE)
            .and_then(|offset| 4usize.checked_add(offset))
            .ok_or_else(|| crate::wire::invalid_payload::<Map>("map entry offset overflowed"))?;
        let end = start
            .checked_add(ENTRY_SIZE)
            .ok_or_else(|| crate::wire::invalid_payload::<Map>("map entry end overflowed"))?;
        let Some(bytes) = self.data.get(start..end) else {
            return Err(crate::wire::invalid_payload::<Map>(format!(
                "map entry {index} outside payload: range {start}..{end}, payload len {}",
                self.data.len()
            )));
        };
        Ok(bytemuck::pod_read_unaligned(bytes))
    }
}

impl DataPodValidate for Map {
    fn validate_wire_parts(_header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        validate_map_payload(payload)
    }
}

impl DataPodAccess for Map {
    type View<'a> = MapView<'a>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Self::validate_wire_parts(&header, payload)?;
        Ok(MapView { data: payload })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        _header: Self::Header,
        payload: &'a [u8],
    ) -> Self::View<'a> {
        MapView { data: payload }
    }
}

fn validate_map_payload(payload: &[u8]) -> Result<(), WireError> {
    if payload.len() < 4 {
        return Err(crate::wire::invalid_payload::<Map>(format!(
            "map payload too short: got {}, need at least 4",
            payload.len()
        )));
    }
    let mut count_bytes = [0u8; 4];
    let count_raw = payload
        .get(0..4)
        .ok_or_else(|| crate::wire::invalid_payload::<Map>("map count range is out of bounds"))?;
    count_bytes.copy_from_slice(count_raw);
    let count = u32_to_usize::<Map>(u32::from_le_bytes(count_bytes), "count")?;
    let table_len = count
        .checked_mul(ENTRY_SIZE)
        .ok_or_else(|| crate::wire::invalid_payload::<Map>("entry table length overflowed"))?;
    let blob_offset = 4usize
        .checked_add(table_len)
        .ok_or_else(|| crate::wire::invalid_payload::<Map>("blob offset overflowed"))?;
    if payload.len() < blob_offset {
        return Err(crate::wire::invalid_payload::<Map>(format!(
            "map payload too short for {count} entries: got {}, need at least {blob_offset}",
            payload.len()
        )));
    }

    let blob_len = payload
        .len()
        .checked_sub(blob_offset)
        .ok_or_else(|| crate::wire::invalid_payload::<Map>("map blob length underflowed"))?;
    let mut previous_key: Option<&[u8]> = None;
    let mut expected_blob_offset = 0usize;
    for index in 0..count {
        let entry = read_map_entry(payload, index)?;
        let key_off = u32_to_usize::<Map>(entry.key_off, "key_off")?;
        if key_off != expected_blob_offset {
            return Err(crate::wire::invalid_payload::<Map>(format!(
                "entry {index} key offset {} does not match canonical blob offset {expected_blob_offset}",
                entry.key_off
            )));
        }
        let key = map_blob_range(payload, blob_offset, blob_len, entry.key_off, entry.key_len)
            .ok_or_else(|| {
                crate::wire::invalid_payload::<Map>(format!(
                    "entry {index} key range is out of bounds"
                ))
            })?;
        let key_len = u32_to_usize::<Map>(entry.key_len, "key_len")?;
        expected_blob_offset = expected_blob_offset
            .checked_add(key_len)
            .ok_or_else(|| crate::wire::invalid_payload::<Map>("key offset overflowed"))?;
        let value_off = u32_to_usize::<Map>(entry.value_off, "value_off")?;
        if value_off != expected_blob_offset {
            return Err(crate::wire::invalid_payload::<Map>(format!(
                "entry {index} value offset {} does not match canonical blob offset {expected_blob_offset}",
                entry.value_off
            )));
        }
        let _value = map_blob_range(
            payload,
            blob_offset,
            blob_len,
            entry.value_off,
            entry.value_len,
        )
        .ok_or_else(|| {
            crate::wire::invalid_payload::<Map>(format!(
                "entry {index} value range is out of bounds"
            ))
        })?;
        let value_len = u32_to_usize::<Map>(entry.value_len, "value_len")?;
        expected_blob_offset = expected_blob_offset
            .checked_add(value_len)
            .ok_or_else(|| crate::wire::invalid_payload::<Map>("value offset overflowed"))?;
        if let Some(previous_key) = previous_key
            && previous_key >= key
        {
            return Err(crate::wire::invalid_payload::<Map>(format!(
                "entry {index} key is not strictly sorted"
            )));
        }
        previous_key = Some(key);
    }
    if expected_blob_offset != blob_len {
        return Err(crate::wire::invalid_payload::<Map>(format!(
            "canonical map blob length {expected_blob_offset} does not match payload blob length {blob_len}"
        )));
    }
    Ok(())
}

fn read_map_entry(payload: &[u8], index: usize) -> Result<MapEntry, WireError> {
    let start = index
        .checked_mul(ENTRY_SIZE)
        .and_then(|offset| 4usize.checked_add(offset))
        .ok_or_else(|| crate::wire::invalid_payload::<Map>("map entry offset overflowed"))?;
    let end = start
        .checked_add(ENTRY_SIZE)
        .ok_or_else(|| crate::wire::invalid_payload::<Map>("map entry end overflowed"))?;
    let bytes = payload
        .get(start..end)
        .ok_or_else(|| crate::wire::invalid_payload::<Map>("map entry range is out of bounds"))?;
    Ok(bytemuck::pod_read_unaligned(bytes))
}

fn map_blob_range(
    payload: &[u8],
    blob_offset: usize,
    blob_len: usize,
    offset: u32,
    len: u32,
) -> Option<&[u8]> {
    let start = u32_to_usize::<Map>(offset, "blob offset").ok()?;
    let len = u32_to_usize::<Map>(len, "blob length").ok()?;
    let end = start.checked_add(len)?;
    if end > blob_len {
        return None;
    }
    let absolute_start = blob_offset.checked_add(start)?;
    let absolute_end = blob_offset.checked_add(end)?;
    payload.get(absolute_start..absolute_end)
}

fn u32_to_usize<P: 'static>(value: u32, field: &'static str) -> Result<usize, WireError> {
    usize::try_from(value)
        .map_err(|_| crate::wire::invalid_payload::<P>(format!("{field} does not fit in usize")))
}
