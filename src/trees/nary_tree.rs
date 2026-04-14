use crate::Vector;
use std::collections::VecDeque;

pub const INVALID_INDEX: usize = usize::MAX;

pub type NodeId = usize;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Node<T> {
    pub value: T,
    pub parent: usize,
    pub first_child: usize,
    pub next_sibling: usize,
}

impl<T: Default> Default for Node<T> {
    fn default() -> Self {
        Self {
            value: T::default(),
            parent: INVALID_INDEX,
            first_child: INVALID_INDEX,
            next_sibling: INVALID_INDEX,
        }
    }
}

impl<T> Node<T> {
    pub fn new(value: T, parent: usize) -> Self {
        Self {
            value,
            parent,
            first_child: INVALID_INDEX,
            next_sibling: INVALID_INDEX,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct NaryTree<T> {
    nodes: Vector<Node<T>>,
    root: usize,
    size: usize,
    free_list: Vector<usize>,
}

impl<T> Default for NaryTree<T> {
    fn default() -> Self {
        Self {
            nodes: Vector::new(),
            root: INVALID_INDEX,
            size: 0,
            free_list: Vector::new(),
        }
    }
}

impl<T> NaryTree<T> {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn empty(&self) -> bool {
        self.size == 0
    }

    pub fn is_empty(&self) -> bool {
        self.empty()
    }

    pub fn size(&self) -> usize {
        self.size
    }

    pub fn len(&self) -> usize {
        self.size
    }

    pub fn valid(&self, id: NodeId) -> bool {
        if id == INVALID_INDEX || id >= self.nodes.len() {
            return false;
        }
        for i in 0..self.free_list.len() {
            if self.free_list[i] == id {
                return false;
            }
        }
        true
    }

    pub fn root(&self) -> NodeId {
        self.root
    }

    pub fn has_root(&self) -> bool {
        self.root != INVALID_INDEX
    }

    pub fn set_root(&mut self, value: T) -> NodeId {
        if self.root != INVALID_INDEX {
            self.nodes[self.root].value = value;
            return self.root;
        }
        self.root = self.allocate_node(value, INVALID_INDEX);
        self.root
    }

    pub fn get(&self, id: NodeId) -> &T {
        if !self.valid(id) {
            panic!("NaryTree::get: invalid node ID");
        }
        &self.nodes[id].value
    }

    pub fn get_mut(&mut self, id: NodeId) -> &mut T {
        if !self.valid(id) {
            panic!("NaryTree::get_mut: invalid node ID");
        }
        &mut self.nodes[id].value
    }

    pub fn try_get(&self, id: NodeId) -> Option<&T> {
        if !self.valid(id) {
            return None;
        }
        Some(&self.nodes[id].value)
    }

    pub fn set(&mut self, id: NodeId, value: T) {
        if !self.valid(id) {
            panic!("NaryTree::set: invalid node ID");
        }
        self.nodes[id].value = value;
    }

    pub fn parent(&self, id: NodeId) -> NodeId {
        if !self.valid(id) {
            return INVALID_INDEX;
        }
        self.nodes[id].parent
    }

    pub fn first_child(&self, id: NodeId) -> NodeId {
        if !self.valid(id) {
            return INVALID_INDEX;
        }
        self.nodes[id].first_child
    }

    pub fn next_sibling(&self, id: NodeId) -> NodeId {
        if !self.valid(id) {
            return INVALID_INDEX;
        }
        self.nodes[id].next_sibling
    }

    pub fn children(&self, id: NodeId) -> Vector<NodeId> {
        let mut result = Vector::new();
        if !self.valid(id) {
            return result;
        }
        let mut child = self.nodes[id].first_child;
        while child != INVALID_INDEX {
            result.push_back(child);
            child = self.nodes[child].next_sibling;
        }
        result
    }

    pub fn num_children(&self, id: NodeId) -> usize {
        if !self.valid(id) {
            return 0;
        }
        let mut count = 0;
        let mut child = self.nodes[id].first_child;
        while child != INVALID_INDEX {
            count += 1;
            child = self.nodes[child].next_sibling;
        }
        count
    }

    pub fn is_leaf(&self, id: NodeId) -> bool {
        if !self.valid(id) {
            return false;
        }
        self.nodes[id].first_child == INVALID_INDEX
    }

    pub fn is_root(&self, id: NodeId) -> bool {
        id == self.root && self.valid(id)
    }

    pub fn add_child(&mut self, parent_id: NodeId, value: T) -> NodeId {
        if !self.valid(parent_id) {
            panic!("NaryTree::add_child: invalid parent ID");
        }
        let new_id = self.allocate_node(value, parent_id);
        if self.nodes[parent_id].first_child == INVALID_INDEX {
            self.nodes[parent_id].first_child = new_id;
        } else {
            let mut last = self.nodes[parent_id].first_child;
            while self.nodes[last].next_sibling != INVALID_INDEX {
                last = self.nodes[last].next_sibling;
            }
            self.nodes[last].next_sibling = new_id;
        }
        new_id
    }

    pub fn remove(&mut self, id: NodeId) {
        if !self.valid(id) {
            return;
        }
        let mut child = self.nodes[id].first_child;
        while child != INVALID_INDEX {
            let next = self.nodes[child].next_sibling;
            self.remove(child);
            child = next;
        }
        let parent_id = self.nodes[id].parent;
        if parent_id != INVALID_INDEX {
            if self.nodes[parent_id].first_child == id {
                self.nodes[parent_id].first_child = self.nodes[id].next_sibling;
            } else {
                let mut prev = self.nodes[parent_id].first_child;
                while prev != INVALID_INDEX && self.nodes[prev].next_sibling != id {
                    prev = self.nodes[prev].next_sibling;
                }
                if prev != INVALID_INDEX {
                    self.nodes[prev].next_sibling = self.nodes[id].next_sibling;
                }
            }
        } else {
            self.root = INVALID_INDEX;
        }
        self.deallocate_node(id);
    }

    pub fn clear(&mut self) {
        self.nodes.clear();
        self.free_list.clear();
        self.root = INVALID_INDEX;
        self.size = 0;
    }

    pub fn depth(&self, id: NodeId) -> i32 {
        if !self.valid(id) {
            return -1;
        }
        let mut d = 0;
        let mut current = self.nodes[id].parent;
        while current != INVALID_INDEX {
            d += 1;
            current = self.nodes[current].parent;
        }
        d
    }

    pub fn height_of(&self, id: NodeId) -> i32 {
        if !self.valid(id) {
            return -1;
        }
        if self.nodes[id].first_child == INVALID_INDEX {
            return 0;
        }
        let mut max_child = -1;
        let mut child = self.nodes[id].first_child;
        while child != INVALID_INDEX {
            let h = self.height_of(child);
            if h > max_child {
                max_child = h;
            }
            child = self.nodes[child].next_sibling;
        }
        1 + max_child
    }

    pub fn height(&self) -> i32 {
        self.height_of(self.root)
    }

    pub fn subtree_size(&self, id: NodeId) -> usize {
        if !self.valid(id) {
            return 0;
        }
        let mut count = 1;
        let mut child = self.nodes[id].first_child;
        while child != INVALID_INDEX {
            count += self.subtree_size(child);
            child = self.nodes[child].next_sibling;
        }
        count
    }

    pub fn preorder<F: FnMut(&T, NodeId)>(&self, mut func: F) {
        self.preorder_impl(self.root, &mut func);
    }

    pub fn preorder_from<F: FnMut(&T, NodeId)>(&self, start: NodeId, mut func: F) {
        self.preorder_impl(start, &mut func);
    }

    fn preorder_impl<F: FnMut(&T, NodeId)>(&self, id: NodeId, func: &mut F) {
        if !self.valid(id) {
            return;
        }
        func(&self.nodes[id].value, id);
        let mut child = self.nodes[id].first_child;
        while child != INVALID_INDEX {
            self.preorder_impl(child, func);
            child = self.nodes[child].next_sibling;
        }
    }

    pub fn postorder<F: FnMut(&T, NodeId)>(&self, mut func: F) {
        self.postorder_impl(self.root, &mut func);
    }

    pub fn postorder_from<F: FnMut(&T, NodeId)>(&self, start: NodeId, mut func: F) {
        self.postorder_impl(start, &mut func);
    }

    fn postorder_impl<F: FnMut(&T, NodeId)>(&self, id: NodeId, func: &mut F) {
        if !self.valid(id) {
            return;
        }
        let mut child = self.nodes[id].first_child;
        while child != INVALID_INDEX {
            self.postorder_impl(child, func);
            child = self.nodes[child].next_sibling;
        }
        func(&self.nodes[id].value, id);
    }

    pub fn levelorder<F: FnMut(&T, NodeId)>(&self, mut func: F) {
        if self.root == INVALID_INDEX {
            return;
        }
        let mut queue: VecDeque<NodeId> = VecDeque::new();
        queue.push_back(self.root);
        while let Some(current) = queue.pop_front() {
            func(&self.nodes[current].value, current);
            let mut child = self.nodes[current].first_child;
            while child != INVALID_INDEX {
                queue.push_back(child);
                child = self.nodes[child].next_sibling;
            }
        }
    }

    pub fn to_preorder(&self) -> Vector<T>
    where
        T: Clone,
    {
        let mut result = Vector::new();
        self.preorder(|val, _| result.push_back(val.clone()));
        result
    }

    pub fn to_postorder(&self) -> Vector<T>
    where
        T: Clone,
    {
        let mut result = Vector::new();
        self.postorder(|val, _| result.push_back(val.clone()));
        result
    }

    pub fn to_levelorder(&self) -> Vector<T>
    where
        T: Clone,
    {
        let mut result = Vector::new();
        self.levelorder(|val, _| result.push_back(val.clone()));
        result
    }

    fn allocate_node(&mut self, value: T, parent: usize) -> NodeId {
        let idx = if !self.free_list.is_empty() {
            let idx = *self.free_list.back();
            self.free_list.pop_back();
            self.nodes[idx] = Node::new(value, parent);
            idx
        } else {
            let idx = self.nodes.len();
            self.nodes.push_back(Node::new(value, parent));
            idx
        };
        self.size += 1;
        idx
    }

    fn deallocate_node(&mut self, idx: NodeId) {
        self.free_list.push_back(idx);
        self.size -= 1;
    }
}
