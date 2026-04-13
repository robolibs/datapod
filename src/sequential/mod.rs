mod vector;

pub use vector::Vector;

use std::collections::{BinaryHeap, LinkedList, VecDeque};
use std::ffi::CString as StdCString;

pub type Array<T, const N: usize> = [T; N];
pub type BitVec = Vec<bool>;
pub type Bytes = Vec<u8>;
pub type CString = StdCString;
pub type Deque<T> = VecDeque<T>;
pub type ForwardList<T> = LinkedList<T>;
pub type Heap<T> = BinaryHeap<T>;
pub type IndexedHeap<T> = BinaryHeap<T>;
pub type List<T> = LinkedList<T>;
pub type NVec<T, const N: usize> = [T; N];
pub type Queue<T> = VecDeque<T>;
pub type Stack<T> = Vec<T>;
pub type String = std::string::String;
pub type Vectra<T> = Vec<T>;
pub type Vecvec<T> = Vec<Vec<T>>;
pub type PagedVecvec<T> = Vec<Vec<T>>;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct FixedQueue<T> {
    pub values: VecDeque<T>,
    pub capacity: usize,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct FlatMatrix<T> {
    pub rows: usize,
    pub cols: usize,
    pub values: Vec<T>,
}
