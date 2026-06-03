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

use crate::{DataPodAccess, DataPodValidate, WireError};

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct SetEntry {
    pub key_off: u32,
    pub key_len: u32,
}

const ENTRY_SIZE: usize = std::mem::size_of::<SetEntry>();

#[datapod::datapod]
#[dp(manual_access)]
pub struct Set {
    #[dp(bytes)]
    pub data: Vec<u8>,
}

pub type OSet = Set;

impl Default for Set {
    fn default() -> Self {
        Self {
            data: 0u32.to_le_bytes().to_vec(),
        }
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
        SetIter {
            set: self,
            cursor: 0,
        }
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
            table.push(SetEntry {
                key_off,
                key_len: k.len() as u32,
            });
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

/// Borrowed, validation-backed view over a `Set` wire payload.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct SetView<'a> {
    pub data: &'a [u8],
}

impl<'a> SetView<'a> {
    pub fn payload_bytes(&self) -> &'a [u8] {
        self.data
    }

    pub fn size(&self) -> usize {
        self.count() as usize
    }

    pub fn is_empty(&self) -> bool {
        self.count() == 0
    }

    pub fn key_at(&self, index: usize) -> &'a [u8] {
        let entry = self.entry_at(index);
        let base = self.blob_offset();
        let start = base + entry.key_off as usize;
        let end = start + entry.key_len as usize;
        &self.data[start..end]
    }

    fn count(&self) -> u32 {
        u32::from_le_bytes(self.data[0..4].try_into().unwrap())
    }

    fn blob_offset(&self) -> usize {
        4 + self.size() * ENTRY_SIZE
    }

    fn entry_at(&self, index: usize) -> SetEntry {
        let start = 4 + index * ENTRY_SIZE;
        let end = start + ENTRY_SIZE;
        bytemuck::pod_read_unaligned(&self.data[start..end])
    }
}

impl DataPodValidate for Set {
    fn validate_wire_parts(_header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        validate_set_payload(payload)
    }
}

impl DataPodAccess for Set {
    type View<'a> = SetView<'a>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Self::validate_wire_parts(&header, payload)?;
        Ok(SetView { data: payload })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        _header: Self::Header,
        payload: &'a [u8],
    ) -> Self::View<'a> {
        SetView { data: payload }
    }
}

fn validate_set_payload(payload: &[u8]) -> Result<(), WireError> {
    if payload.len() < 4 {
        return Err(crate::wire::invalid_payload::<Set>(format!(
            "set payload too short: got {}, need at least 4",
            payload.len()
        )));
    }
    let count = u32::from_le_bytes(payload[0..4].try_into().unwrap()) as usize;
    let table_len = count
        .checked_mul(ENTRY_SIZE)
        .ok_or_else(|| crate::wire::invalid_payload::<Set>("entry table length overflowed"))?;
    let blob_offset = 4usize
        .checked_add(table_len)
        .ok_or_else(|| crate::wire::invalid_payload::<Set>("blob offset overflowed"))?;
    if payload.len() < blob_offset {
        return Err(crate::wire::invalid_payload::<Set>(format!(
            "set payload too short for {count} entries: got {}, need at least {blob_offset}",
            payload.len()
        )));
    }

    let blob_len = payload.len() - blob_offset;
    let mut previous_key: Option<&[u8]> = None;
    for index in 0..count {
        let entry = read_set_entry(payload, index);
        let key = set_blob_range(payload, blob_offset, blob_len, entry.key_off, entry.key_len)
            .ok_or_else(|| {
                crate::wire::invalid_payload::<Set>(format!(
                    "entry {index} key range is out of bounds"
                ))
            })?;
        if let Some(previous_key) = previous_key
            && previous_key >= key
        {
            return Err(crate::wire::invalid_payload::<Set>(format!(
                "entry {index} key is not strictly sorted"
            )));
        }
        previous_key = Some(key);
    }
    Ok(())
}

fn read_set_entry(payload: &[u8], index: usize) -> SetEntry {
    let start = 4 + index * ENTRY_SIZE;
    let end = start + ENTRY_SIZE;
    bytemuck::pod_read_unaligned(&payload[start..end])
}

fn set_blob_range(
    payload: &[u8],
    blob_offset: usize,
    blob_len: usize,
    offset: u32,
    len: u32,
) -> Option<&[u8]> {
    let start = offset as usize;
    let end = start.checked_add(len as usize)?;
    if end > blob_len {
        return None;
    }
    Some(&payload[blob_offset + start..blob_offset + end])
}
