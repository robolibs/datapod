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
        let mut data = Vec::new();
        data.extend_from_slice(&0u32.to_le_bytes());
        Self { data }
    }
}

impl Set {
    pub fn new() -> Self {
        Self::default()
    }

    fn count(&self) -> u32 {
        self.try_count().unwrap_or(0)
    }

    fn try_count(&self) -> Result<u32, WireError> {
        let Some(raw) = self.data.get(0..4) else {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "set payload too short: got {}, need at least 4",
                self.data.len()
            )));
        };
        let mut bytes = [0u8; 4];
        bytes.copy_from_slice(raw);
        Ok(u32::from_le_bytes(bytes))
    }

    fn set_count(&mut self, count: u32) -> Result<(), WireError> {
        let slot = self.data.get_mut(0..4).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("set count range is out of bounds")
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

    pub fn key_at(&self, i: usize) -> &[u8] {
        self.try_key_at(i).unwrap_or(&[])
    }

    pub fn try_key_at(&self, i: usize) -> Result<&[u8], WireError> {
        self.validate_owned()?;
        let len = self.try_size()?;
        if i >= len {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "set entry index out of bounds: {i} for len {len}"
            )));
        }
        self.key_at_validated(i)
    }

    fn key_at_validated(&self, i: usize) -> Result<&[u8], WireError> {
        let e = self.entry_at_validated(i)?;
        self.blob_range_validated(e.key_off, e.key_len)
    }

    fn entry_at_validated(&self, i: usize) -> Result<SetEntry, WireError> {
        let start = i
            .checked_mul(ENTRY_SIZE)
            .and_then(|offset| 4usize.checked_add(offset))
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("set entry offset overflowed"))?;
        let end = start
            .checked_add(ENTRY_SIZE)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("set entry end overflowed"))?;
        let bytes = self.data.get(start..end).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("set entry range is out of bounds")
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

    fn blob_range_validated(&self, offset: u32, len: u32) -> Result<&[u8], WireError> {
        let base = self.blob_offset_validated()?;
        let offset = u32_to_usize::<Self>(offset, "blob offset")?;
        let len = u32_to_usize::<Self>(len, "blob length")?;
        let start = base
            .checked_add(offset)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("set key start overflowed"))?;
        let end = start
            .checked_add(len)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("set key end overflowed"))?;
        self.data
            .get(start..end)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("set key range is out of bounds"))
    }

    fn try_binary_search(&self, key: &[u8]) -> Result<Result<usize, usize>, WireError> {
        self.validate_owned()?;
        let n = self.try_size()?;
        let mut lo = 0usize;
        let mut hi = n;
        while lo < hi {
            let span = hi.checked_sub(lo).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("set binary-search span underflowed")
            })?;
            let mid = lo.checked_add(span / 2).ok_or_else(|| {
                crate::wire::invalid_payload::<Self>("set binary-search midpoint overflowed")
            })?;
            match self.key_at_validated(mid)?.cmp(key) {
                Ordering::Less => {
                    lo = mid.checked_add(1).ok_or_else(|| {
                        crate::wire::invalid_payload::<Self>("set binary-search cursor overflowed")
                    })?
                }
                Ordering::Greater => hi = mid,
                Ordering::Equal => return Ok(Ok(mid)),
            }
        }
        Ok(Err(lo))
    }

    pub fn contains(&self, key: &[u8]) -> bool {
        self.try_contains(key).unwrap_or(false)
    }

    pub fn try_contains(&self, key: &[u8]) -> Result<bool, WireError> {
        Ok(self.try_binary_search(key)?.is_ok())
    }

    pub fn contains_str(&self, key: &str) -> bool {
        self.try_contains_str(key).unwrap_or(false)
    }

    pub fn try_contains_str(&self, key: &str) -> Result<bool, WireError> {
        self.try_contains(key.as_bytes())
    }

    pub fn contains_pod<K: bytemuck::Pod>(&self, key: &K) -> bool {
        self.try_contains_pod(key).unwrap_or(false)
    }

    pub fn try_contains_pod<K: bytemuck::Pod>(&self, key: &K) -> Result<bool, WireError> {
        self.try_contains(bytemuck::bytes_of(key))
    }

    /// Insert a key. Returns `true` if the key was newly added, `false` if
    /// it already existed.
    pub fn insert(&mut self, key: &[u8]) -> bool {
        self.try_insert(key).unwrap_or(false)
    }

    /// Fallible insert. This refuses to mutate if the existing owned buffer is
    /// malformed or if the rebuilt wire buffer would overflow u32 count/offset
    /// fields.
    pub fn try_insert(&mut self, key: &[u8]) -> Result<bool, WireError> {
        match self.try_binary_search(key)? {
            Ok(_) => Ok(false),
            Err(idx) => {
                self.try_insert_at(idx, key)?;
                Ok(true)
            }
        }
    }

    pub fn insert_str(&mut self, key: &str) -> bool {
        self.insert(key.as_bytes())
    }

    pub fn try_insert_str(&mut self, key: &str) -> Result<bool, WireError> {
        self.try_insert(key.as_bytes())
    }

    pub fn insert_pod<K: bytemuck::Pod>(&mut self, key: &K) -> bool {
        self.insert(bytemuck::bytes_of(key))
    }

    pub fn try_insert_pod<K: bytemuck::Pod>(&mut self, key: &K) -> Result<bool, WireError> {
        self.try_insert(bytemuck::bytes_of(key))
    }

    /// Remove a key. Returns `true` if it was present.
    pub fn remove(&mut self, key: &[u8]) -> bool {
        self.try_remove(key).unwrap_or(false)
    }

    pub fn try_remove(&mut self, key: &[u8]) -> Result<bool, WireError> {
        let idx = match self.try_binary_search(key)? {
            Ok(idx) => idx,
            Err(_) => return Ok(false),
        };
        self.try_remove_at(idx)?;
        Ok(true)
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

    fn try_insert_at(&mut self, pos: usize, key: &[u8]) -> Result<(), WireError> {
        self.validate_owned()?;
        let old_count = self.try_count_usize()?;
        let mut keys: Vec<Vec<u8>> = Vec::new();
        let snapshot_count = old_count
            .checked_add(1)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("set insert count overflowed"))?;
        Self::try_reserve_vec(&mut keys, snapshot_count, "set key snapshot")?;
        for i in 0..old_count {
            keys.push(Self::try_copy_bytes(
                self.key_at_validated(i)?,
                "set key snapshot",
            )?);
        }
        keys.insert(pos, Self::try_copy_bytes(key, "new set key")?);
        self.try_rebuild(&keys)
    }

    fn try_remove_at(&mut self, pos: usize) -> Result<(), WireError> {
        self.validate_owned()?;
        let old_count = self.try_count_usize()?;
        let mut keys: Vec<Vec<u8>> = Vec::new();
        let snapshot_count = old_count
            .checked_sub(1)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("set remove count underflowed"))?;
        Self::try_reserve_vec(&mut keys, snapshot_count, "set key snapshot")?;
        for i in 0..old_count {
            if i == pos {
                continue;
            }
            keys.push(Self::try_copy_bytes(
                self.key_at_validated(i)?,
                "set key snapshot",
            )?);
        }
        self.try_rebuild(&keys)
    }

    fn try_rebuild(&mut self, keys: &[Vec<u8>]) -> Result<(), WireError> {
        let count = u32::try_from(keys.len())
            .map_err(|_| crate::wire::invalid_payload::<Self>("entry count exceeds u32"))?;
        let count_usize = u32_to_usize::<Self>(count, "entry count")?;
        let blob_size = keys.iter().try_fold(0usize, |acc, k| {
            acc.checked_add(k.len())
                .ok_or_else(|| crate::wire::invalid_payload::<Self>("set blob length overflowed"))
        })?;
        let table_len = count_usize
            .checked_mul(ENTRY_SIZE)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("entry table length overflowed"))?;
        let total = 4usize
            .checked_add(table_len)
            .and_then(|base| base.checked_add(blob_size))
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("set wire length overflowed"))?;
        let mut new_data = Vec::new();
        Self::try_reserve_vec(&mut new_data, total, "set wire buffer")?;

        new_data.extend_from_slice(&count.to_le_bytes());
        let entries_start = new_data.len();
        let entries_end = entries_start.checked_add(table_len).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("set entry table end overflowed")
        })?;
        new_data.resize(entries_end, 0);
        let blob_start = new_data.len();

        let mut blob_cursor = 0u32;
        let mut table: Vec<SetEntry> = Vec::new();
        Self::try_reserve_vec(&mut table, count_usize, "set entry table")?;
        for k in keys {
            let key_off = blob_cursor;
            let key_len = u32::try_from(k.len())
                .map_err(|_| crate::wire::invalid_payload::<Self>("key length exceeds u32"))?;
            new_data.extend_from_slice(k);
            blob_cursor = blob_cursor
                .checked_add(key_len)
                .ok_or_else(|| crate::wire::invalid_payload::<Self>("key offset overflowed u32"))?;
            table.push(SetEntry { key_off, key_len });
        }

        let table_bytes: &[u8] = bytemuck::cast_slice(&table);
        let table_slot = new_data.get_mut(entries_start..blob_start).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("set entry table range is out of bounds")
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
        <Self as DataPodValidate>::validate_wire_parts(&SetHeader {}, &self.data)
    }

    fn try_count_usize(&self) -> Result<usize, WireError> {
        u32_to_usize::<Self>(self.try_count()?, "count")
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
        self.try_size().unwrap_or(0)
    }

    pub fn try_size(&self) -> Result<usize, WireError> {
        Set::validate_wire_parts(&SetHeader {}, self.data)?;
        u32_to_usize::<Set>(self.count()?, "count")
    }

    pub fn is_empty(&self) -> bool {
        self.try_size().map_or(true, |size| size == 0)
    }

    pub fn key_at(&self, index: usize) -> Result<&'a [u8], WireError> {
        let entry = self.entry_at(index)?;
        let base = self.blob_offset()?;
        let key_off = u32_to_usize::<Set>(entry.key_off, "key_off")?;
        let key_len = u32_to_usize::<Set>(entry.key_len, "key_len")?;
        let start = base
            .checked_add(key_off)
            .ok_or_else(|| crate::wire::invalid_payload::<Set>("set key start overflowed"))?;
        let end = start
            .checked_add(key_len)
            .ok_or_else(|| crate::wire::invalid_payload::<Set>("set key end overflowed"))?;
        self.data
            .get(start..end)
            .ok_or_else(|| crate::wire::invalid_payload::<Set>("set key range is out of bounds"))
    }

    fn count(&self) -> Result<u32, WireError> {
        let Some(raw) = self.data.get(0..4) else {
            return Err(crate::wire::invalid_payload::<Set>(format!(
                "set payload too short: got {}, need at least 4",
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
            .ok_or_else(|| crate::wire::invalid_payload::<Set>("entry table length overflowed"))?;
        4usize
            .checked_add(table_len)
            .ok_or_else(|| crate::wire::invalid_payload::<Set>("blob offset overflowed"))
    }

    fn entry_at(&self, index: usize) -> Result<SetEntry, WireError> {
        let size = self.try_size()?;
        if index >= size {
            return Err(crate::wire::invalid_header::<Set>(format!(
                "set entry index out of bounds: {index} for len {}",
                size
            )));
        }
        let start = index
            .checked_mul(ENTRY_SIZE)
            .and_then(|offset| 4usize.checked_add(offset))
            .ok_or_else(|| crate::wire::invalid_payload::<Set>("set entry offset overflowed"))?;
        let end = start
            .checked_add(ENTRY_SIZE)
            .ok_or_else(|| crate::wire::invalid_payload::<Set>("set entry end overflowed"))?;
        let Some(bytes) = self.data.get(start..end) else {
            return Err(crate::wire::invalid_payload::<Set>(format!(
                "set entry {index} outside payload: range {start}..{end}, payload len {}",
                self.data.len()
            )));
        };
        Ok(bytemuck::pod_read_unaligned(bytes))
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
    let mut count_bytes = [0u8; 4];
    let count_raw = payload
        .get(0..4)
        .ok_or_else(|| crate::wire::invalid_payload::<Set>("set count range is out of bounds"))?;
    count_bytes.copy_from_slice(count_raw);
    let count = u32_to_usize::<Set>(u32::from_le_bytes(count_bytes), "count")?;
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

    let blob_len = payload
        .len()
        .checked_sub(blob_offset)
        .ok_or_else(|| crate::wire::invalid_payload::<Set>("set blob length underflowed"))?;
    let mut previous_key: Option<&[u8]> = None;
    let mut expected_blob_offset = 0usize;
    for index in 0..count {
        let entry = read_set_entry(payload, index)?;
        let key_off = u32_to_usize::<Set>(entry.key_off, "key_off")?;
        if key_off != expected_blob_offset {
            return Err(crate::wire::invalid_payload::<Set>(format!(
                "entry {index} key offset {} does not match canonical blob offset {expected_blob_offset}",
                entry.key_off
            )));
        }
        let key = set_blob_range(payload, blob_offset, blob_len, entry.key_off, entry.key_len)
            .ok_or_else(|| {
                crate::wire::invalid_payload::<Set>(format!(
                    "entry {index} key range is out of bounds"
                ))
            })?;
        let key_len = u32_to_usize::<Set>(entry.key_len, "key_len")?;
        expected_blob_offset = expected_blob_offset
            .checked_add(key_len)
            .ok_or_else(|| crate::wire::invalid_payload::<Set>("key offset overflowed"))?;
        if let Some(previous_key) = previous_key
            && previous_key >= key
        {
            return Err(crate::wire::invalid_payload::<Set>(format!(
                "entry {index} key is not strictly sorted"
            )));
        }
        previous_key = Some(key);
    }
    if expected_blob_offset != blob_len {
        return Err(crate::wire::invalid_payload::<Set>(format!(
            "canonical set blob length {expected_blob_offset} does not match payload blob length {blob_len}"
        )));
    }
    Ok(())
}

fn read_set_entry(payload: &[u8], index: usize) -> Result<SetEntry, WireError> {
    let start = index
        .checked_mul(ENTRY_SIZE)
        .and_then(|offset| 4usize.checked_add(offset))
        .ok_or_else(|| crate::wire::invalid_payload::<Set>("set entry offset overflowed"))?;
    let end = start
        .checked_add(ENTRY_SIZE)
        .ok_or_else(|| crate::wire::invalid_payload::<Set>("set entry end overflowed"))?;
    let bytes = payload
        .get(start..end)
        .ok_or_else(|| crate::wire::invalid_payload::<Set>("set entry range is out of bounds"))?;
    Ok(bytemuck::pod_read_unaligned(bytes))
}

fn set_blob_range(
    payload: &[u8],
    blob_offset: usize,
    blob_len: usize,
    offset: u32,
    len: u32,
) -> Option<&[u8]> {
    let start = u32_to_usize::<Set>(offset, "blob offset").ok()?;
    let len = u32_to_usize::<Set>(len, "blob length").ok()?;
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
