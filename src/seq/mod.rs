//! Sequential container Pods — wire-shippable containers that can hold
//! **any [`bytemuck::Pod`] element type**, not just scalars.
//!
//! Every container stores its payload as a single `Vec<u8>` (the
//! `#[dp(bytes)]` field) plus an `element_size: u32` in the header that
//! says how wide each element is in bytes. All access methods are generic
//! over `T: bytemuck::Pod` and runtime-check that `size_of::<T>()` matches
//! `element_size`. So `Vector<Point>` (24-byte elements), `Stack<u64>`
//! (8-byte), `List<MyPodStruct>` (any Pod) all work uniformly.
//!
//! ## Pattern
//!
//! ```
//! use datapod::{Point, Vector};
//!
//! let mut v = Vector::new::<Point>();        // sets element_size = 24
//! v.push(Point::new(1.0, 2.0, 3.0));
//! let p: Point = v.try_get(0)?;
//! let slice: &[Point] = v.try_as_slice::<Point>()?;
//!
//! assert_eq!(p, Point::new(1.0, 2.0, 3.0));
//! assert_eq!(slice, &[Point::new(1.0, 2.0, 3.0)]);
//! # Ok::<(), datapod::WireError>(())
//! ```
//!
//! Linked lists grow the value slot to fit any Pod. `List` nodes use
//! `[value: element_size bytes][prev: u32][next: u32]`; `ForwardList`
//! nodes use `[value: element_size bytes][next: u32]`.

mod bitvec;
mod bytes;
mod deque;
mod forward_list;
mod heap;
mod indexed_heap;
mod list;
mod matrix;
mod paged_vecvec;
mod queue;
mod stack;
mod string;
mod tensor;
mod vector;
mod vecvec;

pub use bitvec::{BitVec, BitVecHeader, BitVecView};
pub use bytes::{Bytes, BytesHeader, BytesView};
pub use deque::{Deque, DequeHeader};
pub use forward_list::{FORWARD_LIST_NIL, ForwardList, ForwardListHeader};
pub use heap::{Heap, HeapHeader, HeapOrder, MaxHeap, MinHeap, PriorityQueue};
pub use indexed_heap::{IndexedHeap, IndexedHeapHeader};
pub use list::{LIST_NIL, List, ListHeader};
pub use matrix::{Matrix, MatrixHeader, MatrixView};
pub use paged_vecvec::{PagedVecvec, PagedVecvecHeader};
pub use queue::{Fifo, Queue, QueueHeader};
pub use stack::{Stack, StackHeader};
pub use string::{DpStr, DpStrHeader, DpStrView};
pub use tensor::{Tensor, TensorHeader, TensorView};
pub use vector::{Vector, VectorHeader, VectorView};
pub use vecvec::{Vecvec, VecvecHeader, VecvecView};

use crate::{BytePayloadView, DataPodAccess, DataPodValidate, WireError};
use std::collections::HashSet;

pub type DequeView<'a> = BytePayloadView<'a, DequeHeader>;
pub type ForwardListView<'a> = BytePayloadView<'a, ForwardListHeader>;
pub type HeapView<'a> = BytePayloadView<'a, HeapHeader>;
pub type IndexedHeapView<'a> = BytePayloadView<'a, IndexedHeapHeader>;
pub type ListView<'a> = BytePayloadView<'a, ListHeader>;
pub type PagedVecvecView<'a> = BytePayloadView<'a, PagedVecvecHeader>;
pub type QueueView<'a> = BytePayloadView<'a, QueueHeader>;
pub type StackView<'a> = BytePayloadView<'a, StackHeader>;

pub(crate) fn checked_pod_element_size<Container: 'static, T: bytemuck::Pod>()
-> Result<u32, WireError> {
    let size = std::mem::size_of::<T>();
    u32::try_from(size).map_err(|_| {
        crate::wire::invalid_payload::<Container>(format!(
            "element size {size} exceeds u32 wire header field"
        ))
    })
}

macro_rules! impl_byte_payload_access {
    ($ty:ty, $header:ty) => {
        impl DataPodAccess for $ty {
            type View<'a> = BytePayloadView<'a, $header>;

            fn access_wire_parts<'a>(
                header: Self::Header,
                payload: &'a [u8],
            ) -> Result<Self::View<'a>, WireError> {
                Self::validate_wire_parts(&header, payload)?;
                Ok(BytePayloadView { header, payload })
            }

            unsafe fn access_wire_parts_unchecked<'a>(
                header: Self::Header,
                payload: &'a [u8],
            ) -> Self::View<'a> {
                BytePayloadView { header, payload }
            }
        }
    };
}

impl DataPodValidate for Stack {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        if header._pad != 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "reserved _pad field must be zero",
            ));
        }
        validate_sized_payload::<Self>(header.element_size, payload).map(|_| ())
    }
}

impl_byte_payload_access!(Stack, StackHeader);

impl DataPodValidate for Queue {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        let raw_len = validate_sized_payload::<Self>(header.element_size, payload)?;
        let front = checked_u32_to_usize::<Self>(header.front, "front")?;
        if front > raw_len {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "front index {} exceeds raw element count {raw_len}",
                header.front
            )));
        }
        Ok(())
    }
}

impl_byte_payload_access!(Queue, QueueHeader);

impl DataPodValidate for Deque {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        validate_sized_payload::<Self>(header.element_size, payload)?;
        let split = checked_u32_to_usize::<Self>(header.split_byte, "split_byte")?;
        if split > payload.len() {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "split_byte {split} exceeds payload length {}",
                payload.len()
            )));
        }
        let element_size = checked_u32_to_usize::<Self>(header.element_size, "element_size")?;
        if element_size != 0 && split % element_size != 0 {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "split_byte {split} is not a multiple of element_size {}",
                header.element_size
            )));
        }
        Ok(())
    }
}

impl_byte_payload_access!(Deque, DequeHeader);

impl DataPodValidate for Heap {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        if !header.order.is_valid() {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "unknown heap order tag {}",
                header.order.0
            )));
        }
        if header._pad != [0; 3] {
            return Err(crate::wire::invalid_header::<Self>(
                "reserved _pad field must be zero",
            ));
        }
        validate_sized_payload::<Self>(header.element_size, payload)?;
        Ok(())
    }
}

impl_byte_payload_access!(Heap, HeapHeader);

impl DataPodValidate for IndexedHeap {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        if !header.order.is_valid() {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "unknown heap order tag {}",
                header.order.0
            )));
        }
        if header._pad != [0; 3] {
            return Err(crate::wire::invalid_header::<Self>(
                "reserved _pad field must be zero",
            ));
        }
        if header.priority_size == 0 {
            return if payload.is_empty() {
                Ok(())
            } else {
                Err(crate::wire::invalid_payload::<Self>(
                    "zero priority_size requires empty payload",
                ))
            };
        }
        let priority_size = checked_u32_to_usize::<Self>(header.priority_size, "priority_size")?;
        let entry_size = 8usize
            .checked_add(priority_size)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("entry size overflowed"))?;
        if payload.len() % entry_size != 0 {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "{} bytes is not a multiple of indexed heap entry_size {entry_size}",
                payload.len()
            )));
        }
        let entry_count = payload.len() / entry_size;
        let mut keys = HashSet::new();
        keys.try_reserve(entry_count).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!(
                "failed to reserve {entry_count} indexed heap key slots: {err}"
            ))
        })?;
        for entry in payload.chunks_exact(entry_size) {
            let key = read_checked_u64_le::<Self>(entry, 0)?;
            if !keys.insert(key) {
                return Err(crate::wire::invalid_payload::<Self>(format!(
                    "duplicate indexed heap key {key}"
                )));
            }
        }
        Ok(())
    }
}

impl_byte_payload_access!(IndexedHeap, IndexedHeapHeader);

impl DataPodValidate for List {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        if header._pad != 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "reserved _pad field must be zero",
            ));
        }
        validate_linked_list_payload::<Self>(
            header.element_size,
            payload,
            header.head,
            Some(header.tail),
            header.free_head,
            checked_u32_to_usize::<Self>(header.size_, "size_")?,
            true,
        )
    }
}

impl_byte_payload_access!(List, ListHeader);

impl DataPodValidate for ForwardList {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        validate_linked_list_payload::<Self>(
            header.element_size,
            payload,
            header.head,
            None,
            header.free_head,
            checked_u32_to_usize::<Self>(header.size_, "size_")?,
            false,
        )
    }
}

impl_byte_payload_access!(ForwardList, ForwardListHeader);

impl DataPodValidate for PagedVecvec {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        if header._pad != 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "reserved _pad field must be zero",
            ));
        }
        validate_vecvec_payload::<Self>(header.element_size, payload)
    }
}

impl_byte_payload_access!(PagedVecvec, PagedVecvecHeader);

fn validate_sized_payload<T: 'static>(
    element_size: u32,
    payload: &[u8],
) -> Result<usize, WireError> {
    if element_size == 0 {
        return if payload.is_empty() {
            Ok(0)
        } else {
            Err(crate::wire::invalid_payload::<T>(
                "zero element_size requires empty payload",
            ))
        };
    }
    let element_size = checked_u32_to_usize::<T>(element_size, "element_size")?;
    if payload.len() % element_size != 0 {
        return Err(crate::wire::invalid_payload::<T>(format!(
            "{} bytes is not a multiple of element_size {element_size}",
            payload.len()
        )));
    }
    Ok(payload.len() / element_size)
}

fn checked_u32_to_usize<T: 'static>(value: u32, field: &'static str) -> Result<usize, WireError> {
    usize::try_from(value)
        .map_err(|_| crate::wire::invalid_header::<T>(format!("{field} does not fit in usize")))
}

fn validate_linked_list_payload<T: 'static>(
    element_size: u32,
    payload: &[u8],
    head: u32,
    tail: Option<u32>,
    free_head: u32,
    size: usize,
    has_prev: bool,
) -> Result<(), WireError> {
    let nil = u32::MAX;
    if element_size == 0 {
        if !payload.is_empty()
            || size != 0
            || head != nil
            || free_head != nil
            || tail.is_some_and(|tail| tail != nil)
        {
            return Err(crate::wire::invalid_header::<T>(
                "zero element_size requires empty list, nil links, and empty payload",
            ));
        }
        return Ok(());
    }

    let element_size = checked_u32_to_usize::<T>(element_size, "element_size")?;
    let link_bytes = 8usize;
    let node_size = element_size
        .checked_add(link_bytes)
        .ok_or_else(|| crate::wire::invalid_payload::<T>("node size overflowed"))?;
    if payload.len() % node_size != 0 {
        return Err(crate::wire::invalid_payload::<T>(format!(
            "{} bytes is not a multiple of node_size {node_size}",
            payload.len()
        )));
    }
    let slots = payload.len() / node_size;
    if size > slots {
        return Err(crate::wire::invalid_header::<T>(format!(
            "logical size {size} exceeds slot count {slots}"
        )));
    }
    validate_index_or_nil::<T>(head, slots, "head")?;
    if let Some(tail) = tail {
        validate_index_or_nil::<T>(tail, slots, "tail")?;
    }
    validate_index_or_nil::<T>(free_head, slots, "free_head")?;

    if size == 0 {
        if head != nil || tail.is_some_and(|tail| tail != nil) {
            return Err(crate::wire::invalid_header::<T>(
                "empty list must have nil head and tail",
            ));
        }
    } else if head == nil {
        return Err(crate::wire::invalid_header::<T>(
            "non-empty list must have a head",
        ));
    }

    let mut seen = bool_scratch::<T>(slots, "active linked-list slots")?;
    let mut cursor = head;
    let mut previous = nil;
    let mut count = 0usize;
    let mut last = nil;
    while cursor != nil {
        let index = checked_u32_to_usize::<T>(cursor, "node index")?;
        if index >= slots {
            return Err(crate::wire::invalid_payload::<T>(format!(
                "node index {index} exceeds slot count {slots}"
            )));
        }
        if seen[index] {
            return Err(crate::wire::invalid_payload::<T>(
                "active list contains a cycle",
            ));
        }
        seen[index] = true;
        if has_prev {
            let prev = read_checked_u32_le::<T>(
                payload,
                node_offset::<T>(index, node_size, element_size)?,
            )?;
            if prev != previous {
                return Err(crate::wire::invalid_payload::<T>(format!(
                    "node {index} prev link {prev} does not match previous {previous}"
                )));
            }
        } else {
            let pad = read_checked_u32_le::<T>(
                payload,
                node_link_offset::<T>(index, node_size, element_size, 4)?,
            )?;
            if pad != 0 {
                return Err(crate::wire::invalid_payload::<T>(format!(
                    "node {index} reserved pad field must be zero"
                )));
            }
        }
        previous = cursor;
        last = cursor;
        cursor = read_checked_u32_le::<T>(
            payload,
            node_link_offset::<T>(index, node_size, element_size, if has_prev { 4 } else { 0 })?,
        )?;
        validate_index_or_nil::<T>(cursor, slots, "next")?;
        count += 1;
        if count > size {
            return Err(crate::wire::invalid_payload::<T>(
                "active list is longer than logical size",
            ));
        }
    }
    if count != size {
        return Err(crate::wire::invalid_header::<T>(format!(
            "active list count {count} does not match logical size {size}"
        )));
    }
    if let Some(tail) = tail
        && last != tail
    {
        return Err(crate::wire::invalid_header::<T>(format!(
            "tail {tail} does not match last active node {last}"
        )));
    }

    let mut free_seen = bool_scratch::<T>(slots, "free linked-list slots")?;
    let mut cursor = free_head;
    let mut free_count = 0usize;
    while cursor != nil {
        let index = checked_u32_to_usize::<T>(cursor, "free node index")?;
        if index >= slots {
            return Err(crate::wire::invalid_payload::<T>(format!(
                "free node index {index} exceeds slot count {slots}"
            )));
        }
        if seen[index] {
            return Err(crate::wire::invalid_payload::<T>(
                "free list overlaps active list",
            ));
        }
        if free_seen[index] {
            return Err(crate::wire::invalid_payload::<T>(
                "free list contains a cycle",
            ));
        }
        free_seen[index] = true;
        if has_prev {
            let prev = read_checked_u32_le::<T>(
                payload,
                node_offset::<T>(index, node_size, element_size)?,
            )?;
            if prev != 0 {
                return Err(crate::wire::invalid_payload::<T>(format!(
                    "free node {index} reserved prev field must be zero"
                )));
            }
        } else {
            let pad = read_checked_u32_le::<T>(
                payload,
                node_link_offset::<T>(index, node_size, element_size, 4)?,
            )?;
            if pad != 0 {
                return Err(crate::wire::invalid_payload::<T>(format!(
                    "free node {index} reserved pad field must be zero"
                )));
            }
        }
        cursor = read_checked_u32_le::<T>(
            payload,
            node_link_offset::<T>(index, node_size, element_size, if has_prev { 4 } else { 0 })?,
        )?;
        validate_index_or_nil::<T>(cursor, slots, "free next")?;
        free_count += 1;
    }

    if count.checked_add(free_count) != Some(slots) {
        return Err(crate::wire::invalid_payload::<T>(format!(
            "active ({count}) plus free ({free_count}) slots do not cover slot count {slots}"
        )));
    }

    Ok(())
}

fn bool_scratch<T: 'static>(slots: usize, label: &'static str) -> Result<Vec<bool>, WireError> {
    let mut scratch = Vec::new();
    scratch.try_reserve_exact(slots).map_err(|err| {
        crate::wire::invalid_payload::<T>(format!("failed to reserve {slots} {label}: {err}"))
    })?;
    scratch.resize(slots, false);
    Ok(scratch)
}

fn validate_index_or_nil<T: 'static>(
    value: u32,
    slots: usize,
    field: &'static str,
) -> Result<(), WireError> {
    if value == u32::MAX {
        return Ok(());
    }
    let value = checked_u32_to_usize::<T>(value, field)?;
    if slots > 0 && value < slots {
        Ok(())
    } else {
        Err(crate::wire::invalid_header::<T>(format!(
            "{field} index {value} exceeds slot count {slots}"
        )))
    }
}

fn node_offset<T: 'static>(
    index: usize,
    node_size: usize,
    element_size: usize,
) -> Result<usize, WireError> {
    index
        .checked_mul(node_size)
        .and_then(|base| base.checked_add(element_size))
        .ok_or_else(|| {
            crate::wire::invalid_payload::<T>(format!("node {index} link offset overflowed"))
        })
}

fn node_link_offset<T: 'static>(
    index: usize,
    node_size: usize,
    element_size: usize,
    link_offset: usize,
) -> Result<usize, WireError> {
    node_offset::<T>(index, node_size, element_size)?
        .checked_add(link_offset)
        .ok_or_else(|| {
            crate::wire::invalid_payload::<T>(format!("node {index} link field offset overflowed"))
        })
}

fn read_checked_u32_le<T: 'static>(bytes: &[u8], offset: usize) -> Result<u32, WireError> {
    let end = offset
        .checked_add(4)
        .ok_or_else(|| crate::wire::invalid_payload::<T>("u32 offset overflowed"))?;
    let Some(raw) = bytes.get(offset..end) else {
        return Err(crate::wire::invalid_payload::<T>(format!(
            "need 4 bytes at offset {offset}, payload has {} bytes",
            bytes.len()
        )));
    };
    let mut le = [0u8; 4];
    le.copy_from_slice(raw);
    Ok(u32::from_le_bytes(le))
}

fn read_checked_u64_le<T: 'static>(bytes: &[u8], offset: usize) -> Result<u64, WireError> {
    let end = offset
        .checked_add(8)
        .ok_or_else(|| crate::wire::invalid_payload::<T>("u64 offset overflowed"))?;
    let Some(raw) = bytes.get(offset..end) else {
        return Err(crate::wire::invalid_payload::<T>(format!(
            "need 8 bytes at offset {offset}, payload has {} bytes",
            bytes.len()
        )));
    };
    let mut le = [0u8; 8];
    le.copy_from_slice(raw);
    Ok(u64::from_le_bytes(le))
}

fn validate_vecvec_payload<T: 'static>(element_size: u32, payload: &[u8]) -> Result<(), WireError> {
    if payload.len() < 8 {
        return Err(crate::wire::invalid_payload::<T>(
            "payload must contain bucket count and at least one offset",
        ));
    }
    let bucket_count =
        checked_u32_to_usize::<T>(read_checked_u32_le::<T>(payload, 0)?, "bucket_count")?;
    let header_bytes = 4usize
        .checked_add(
            bucket_count
                .checked_add(1)
                .and_then(|count| count.checked_mul(4))
                .ok_or_else(|| {
                    crate::wire::invalid_payload::<T>("offset table length overflowed")
                })?,
        )
        .ok_or_else(|| crate::wire::invalid_payload::<T>("header length overflowed"))?;
    if payload.len() < header_bytes {
        return Err(crate::wire::invalid_payload::<T>(format!(
            "payload too short for offset table: got {}, need at least {header_bytes}",
            payload.len()
        )));
    }
    let data_len = payload
        .len()
        .checked_sub(header_bytes)
        .ok_or_else(|| crate::wire::invalid_payload::<T>("data length underflowed"))?;
    let mut previous = None;
    for index in 0..=bucket_count {
        let offset_start = index
            .checked_mul(4)
            .and_then(|offset| 4usize.checked_add(offset))
            .ok_or_else(|| crate::wire::invalid_payload::<T>("offset slot overflowed"))?;
        let offset =
            checked_u32_to_usize::<T>(read_checked_u32_le::<T>(payload, offset_start)?, "offset")?;
        if offset > data_len {
            return Err(crate::wire::invalid_payload::<T>(format!(
                "bucket offset {offset} exceeds data length {data_len}"
            )));
        }
        if let Some(previous) = previous
            && offset < previous
        {
            return Err(crate::wire::invalid_payload::<T>(
                "bucket offsets must be non-decreasing",
            ));
        }
        if index == 0 && offset != 0 {
            return Err(crate::wire::invalid_payload::<T>(
                "first bucket offset must be zero",
            ));
        }
        previous = Some(offset);
    }
    let last = previous.unwrap_or(0);
    if last != data_len {
        return Err(crate::wire::invalid_payload::<T>(format!(
            "last bucket offset {last} must equal data length {data_len}"
        )));
    }
    if element_size == 0 {
        return if data_len == 0 {
            Ok(())
        } else {
            Err(crate::wire::invalid_payload::<T>(
                "zero element_size requires empty bucket data",
            ))
        };
    }
    let element_size = checked_u32_to_usize::<T>(element_size, "element_size")?;
    for index in 0..bucket_count {
        let start_offset = index
            .checked_mul(4)
            .and_then(|offset| 4usize.checked_add(offset))
            .ok_or_else(|| crate::wire::invalid_payload::<T>("bucket start offset overflowed"))?;
        let end_offset = index
            .checked_add(1)
            .and_then(|next| next.checked_mul(4))
            .and_then(|offset| 4usize.checked_add(offset))
            .ok_or_else(|| crate::wire::invalid_payload::<T>("bucket end offset overflowed"))?;
        let start =
            checked_u32_to_usize::<T>(read_checked_u32_le::<T>(payload, start_offset)?, "start")?;
        let end = checked_u32_to_usize::<T>(read_checked_u32_le::<T>(payload, end_offset)?, "end")?;
        let bucket_len = end
            .checked_sub(start)
            .ok_or_else(|| crate::wire::invalid_payload::<T>("bucket length underflowed"))?;
        if bucket_len % element_size != 0 {
            return Err(crate::wire::invalid_payload::<T>(format!(
                "bucket {index} byte length is not a multiple of element_size {element_size}",
            )));
        }
    }
    Ok(())
}
