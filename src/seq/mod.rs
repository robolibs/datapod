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

pub use bitvec::BitVec;
pub use bytes::Bytes;
pub use deque::Deque;
pub use forward_list::{FORWARD_LIST_NIL, ForwardList};
pub use heap::{Heap, HeapOrder, MaxHeap, MinHeap, PriorityQueue};
pub use indexed_heap::IndexedHeap;
pub use list::{LIST_NIL, List};
pub use matrix::{Matrix, MatrixHeader};
pub use paged_vecvec::PagedVecvec;
pub use queue::{Fifo, Queue};
pub use stack::Stack;
pub use string::DpStr;
pub use tensor::Tensor;
pub use vector::Vector;
pub use vecvec::Vecvec;

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
