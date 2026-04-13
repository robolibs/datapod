use std::collections::{BTreeMap, BTreeSet, HashMap};

use crate::Vector;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct BinaryTree<T> {
    pub value: Option<T>,
    pub left: Option<Box<BinaryTree<T>>>,
    pub right: Option<Box<BinaryTree<T>>>,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct NaryTree<T> {
    pub value: Option<T>,
    pub children: Vector<NaryTree<T>>,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Trie<V> {
    pub values: HashMap<String, V>,
}

pub type OrderedMap<K, V> = BTreeMap<K, V>;
pub type OrderedSet<T> = BTreeSet<T>;
