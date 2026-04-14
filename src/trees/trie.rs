use crate::Vector;
use std::collections::BTreeMap;

pub const INVALID_INDEX: usize = usize::MAX;

pub type NodeId = usize;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Node<T> {
    pub children: BTreeMap<char, usize>,
    pub is_end: bool,
    pub value: Option<T>,
}

impl<T> Default for Node<T> {
    fn default() -> Self {
        Self {
            children: BTreeMap::new(),
            is_end: false,
            value: None,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Trie<T> {
    nodes: Vector<Node<T>>,
    root: usize,
    size: usize,
}

impl<T> Default for Trie<T> {
    fn default() -> Self {
        let mut nodes: Vector<Node<T>> = Vector::new();
        nodes.push_back(Node::default());
        Self {
            nodes,
            root: 0,
            size: 0,
        }
    }
}

impl<T> Trie<T> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn empty(&self) -> bool {
        self.size == 0
    }

    pub fn is_empty(&self) -> bool {
        self.size == 0
    }

    pub fn size(&self) -> usize {
        self.size
    }

    pub fn len(&self) -> usize {
        self.size
    }

    pub fn clear(&mut self) {
        self.nodes.clear();
        self.nodes.push_back(Node::default());
        self.root = 0;
        self.size = 0;
    }

    pub fn insert(&mut self, key: &str, value: T) {
        let mut node = self.root;
        for c in key.chars() {
            let next = self.nodes[node].children.get(&c).copied();
            node = match next {
                Some(idx) => idx,
                None => {
                    let new_node = self.allocate_node();
                    self.nodes[node].children.insert(c, new_node);
                    new_node
                }
            };
        }
        if !self.nodes[node].is_end {
            self.size += 1;
        }
        self.nodes[node].is_end = true;
        self.nodes[node].value = Some(value);
    }

    pub fn insert_key(&mut self, key: &str)
    where
        T: Default,
    {
        self.insert(key, T::default());
    }

    pub fn erase(&mut self, key: &str) -> bool {
        let mut node = self.root;
        for c in key.chars() {
            match self.nodes[node].children.get(&c).copied() {
                Some(idx) => node = idx,
                None => return false,
            }
        }
        if !self.nodes[node].is_end {
            return false;
        }
        self.nodes[node].is_end = false;
        self.nodes[node].value = None;
        self.size -= 1;
        true
    }

    pub fn remove(&mut self, key: &str) -> bool {
        self.erase(key)
    }

    pub fn contains(&self, key: &str) -> bool {
        let node = self.find_node(key);
        node != INVALID_INDEX && self.nodes[node].is_end
    }

    pub fn find(&self, key: &str) -> Option<&T> {
        let node = self.find_node(key);
        if node == INVALID_INDEX || !self.nodes[node].is_end {
            return None;
        }
        self.nodes[node].value.as_ref()
    }

    pub fn at(&self, key: &str) -> &T {
        self.find(key).expect("Trie::at: key not found")
    }

    pub fn starts_with(&self, prefix: &str) -> bool {
        self.find_node(prefix) != INVALID_INDEX
    }

    pub fn autocomplete(&self, prefix: &str) -> Vector<String> {
        let mut results: Vector<String> = Vector::new();
        let node = self.find_node(prefix);
        if node == INVALID_INDEX {
            return results;
        }
        let mut current = String::from(prefix);
        self.collect_keys(node, &mut current, &mut results);
        results
    }

    pub fn prefix_search(&self, prefix: &str) -> Vector<String> {
        self.autocomplete(prefix)
    }

    pub fn keys(&self) -> Vector<String> {
        self.autocomplete("")
    }

    fn allocate_node(&mut self) -> NodeId {
        let idx = self.nodes.len();
        self.nodes.push_back(Node::default());
        idx
    }

    fn find_node(&self, key: &str) -> NodeId {
        if self.root == INVALID_INDEX {
            return INVALID_INDEX;
        }
        let mut node = self.root;
        for c in key.chars() {
            match self.nodes[node].children.get(&c).copied() {
                Some(idx) => node = idx,
                None => return INVALID_INDEX,
            }
        }
        node
    }

    fn collect_keys(&self, node: NodeId, current: &mut String, results: &mut Vector<String>) {
        if self.nodes[node].is_end {
            results.push_back(current.clone());
        }
        for (c, &child) in self.nodes[node].children.iter() {
            current.push(*c);
            self.collect_keys(child, current, results);
            current.pop();
        }
    }
}

pub type TrieSet = Trie<bool>;
