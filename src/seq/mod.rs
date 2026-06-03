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
//! ```ignore
//! let mut v = Vector::new::<Point>();        // sets element_size = 24
//! v.push(Point::new(1.0, 2.0, 3.0));
//! let p: Point = v.get(0);
//! let slice: &[Point] = v.as_slice::<Point>();
//! ```
//!
//! Linked lists (`List`, `ForwardList`) lay out their nodes as
//! `[value: element_size bytes][prev: u32, next: u32]` so the value slot
//! grows to fit any Pod.

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

/// Assert at runtime (debug builds) that `T`'s size matches the container's
/// stored element_size. Use in every typed accessor / mutator.
#[inline]
#[track_caller]
pub(crate) fn assert_element_size<T>(stored: u32) {
    debug_assert_eq!(
        std::mem::size_of::<T>(),
        stored as usize,
        "element size mismatch: T is {}, container expects {}",
        std::mem::size_of::<T>(),
        stored
    );
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
        if header.front as usize > raw_len {
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
        let split = header.split_byte as usize;
        if split > payload.len() {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "split_byte {split} exceeds payload length {}",
                payload.len()
            )));
        }
        if header.element_size != 0 && split % header.element_size as usize != 0 {
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
        let entry_size = 8usize
            .checked_add(header.priority_size as usize)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("entry size overflowed"))?;
        if payload.len() % entry_size != 0 {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "{} bytes is not a multiple of indexed heap entry_size {entry_size}",
                payload.len()
            )));
        }
        let mut keys = HashSet::new();
        for entry in payload.chunks_exact(entry_size) {
            let key = read_u64_le(entry, 0);
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
            header.size_ as usize,
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
            header.size_ as usize,
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
    if payload.len() % element_size as usize != 0 {
        return Err(crate::wire::invalid_payload::<T>(format!(
            "{} bytes is not a multiple of element_size {element_size}",
            payload.len()
        )));
    }
    Ok(payload.len() / element_size as usize)
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

    let link_bytes = 8usize;
    let node_size = (element_size as usize)
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

    let mut seen = vec![false; slots];
    let mut cursor = head;
    let mut previous = nil;
    let mut count = 0usize;
    let mut last = nil;
    while cursor != nil {
        let index = cursor as usize;
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
            let prev = read_u32_le(
                payload,
                node_offset(index, node_size, element_size as usize),
            );
            if prev != previous {
                return Err(crate::wire::invalid_payload::<T>(format!(
                    "node {index} prev link {prev} does not match previous {previous}"
                )));
            }
        }
        previous = cursor;
        last = cursor;
        cursor = read_u32_le(
            payload,
            node_offset(index, node_size, element_size as usize) + if has_prev { 4 } else { 0 },
        );
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

    let mut free_seen = vec![false; slots];
    let mut cursor = free_head;
    while cursor != nil {
        let index = cursor as usize;
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
        cursor = read_u32_le(
            payload,
            node_offset(index, node_size, element_size as usize) + if has_prev { 4 } else { 0 },
        );
        validate_index_or_nil::<T>(cursor, slots, "free next")?;
    }

    Ok(())
}

fn validate_index_or_nil<T: 'static>(
    value: u32,
    slots: usize,
    field: &'static str,
) -> Result<(), WireError> {
    if value == u32::MAX || (slots > 0 && (value as usize) < slots) {
        Ok(())
    } else {
        Err(crate::wire::invalid_header::<T>(format!(
            "{field} index {value} exceeds slot count {slots}"
        )))
    }
}

fn node_offset(index: usize, node_size: usize, element_size: usize) -> usize {
    index * node_size + element_size
}

fn validate_vecvec_payload<T: 'static>(element_size: u32, payload: &[u8]) -> Result<(), WireError> {
    if payload.len() < 8 {
        return Err(crate::wire::invalid_payload::<T>(
            "payload must contain bucket count and at least one offset",
        ));
    }
    let bucket_count = read_u32_le(payload, 0) as usize;
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
    let data_len = payload.len() - header_bytes;
    let mut previous = None;
    for index in 0..=bucket_count {
        let offset = read_u32_le(payload, 4 + index * 4) as usize;
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
    for index in 0..bucket_count {
        let start = read_u32_le(payload, 4 + index * 4) as usize;
        let end = read_u32_le(payload, 4 + (index + 1) * 4) as usize;
        if (end - start) % element_size as usize != 0 {
            return Err(crate::wire::invalid_payload::<T>(format!(
                "bucket {index} byte length is not a multiple of element_size {element_size}",
            )));
        }
    }
    Ok(())
}

fn read_u32_le(bytes: &[u8], offset: usize) -> u32 {
    u32::from_le_bytes(bytes[offset..offset + 4].try_into().unwrap())
}

fn read_u64_le(bytes: &[u8], offset: usize) -> u64 {
    u64::from_le_bytes(bytes[offset..offset + 8].try_into().unwrap())
}
