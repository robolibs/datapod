use crate::Vector;
use std::collections::VecDeque;

pub const INVALID_INDEX: usize = usize::MAX;

pub type NodeId = usize;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Node<T> {
    pub value: T,
    pub left: usize,
    pub right: usize,
    pub parent: usize,
}

impl<T: Default> Default for Node<T> {
    fn default() -> Self {
        Self {
            value: T::default(),
            left: INVALID_INDEX,
            right: INVALID_INDEX,
            parent: INVALID_INDEX,
        }
    }
}

impl<T> Node<T> {
    pub fn new(value: T, parent: usize) -> Self {
        Self {
            value,
            left: INVALID_INDEX,
            right: INVALID_INDEX,
            parent,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct BinaryTree<T> {
    nodes: Vector<Node<T>>,
    root: usize,
    size: usize,
    free_list: Vector<usize>,
}

impl<T> Default for BinaryTree<T> {
    fn default() -> Self {
        Self {
            nodes: Vector::new(),
            root: INVALID_INDEX,
            size: 0,
            free_list: Vector::new(),
        }
    }
}

impl<T> BinaryTree<T> {
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
            panic!("BinaryTree::get: invalid node ID");
        }
        &self.nodes[id].value
    }

    pub fn get_mut(&mut self, id: NodeId) -> &mut T {
        if !self.valid(id) {
            panic!("BinaryTree::get_mut: invalid node ID");
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
            panic!("BinaryTree::set: invalid node ID");
        }
        self.nodes[id].value = value;
    }

    pub fn left(&self, id: NodeId) -> NodeId {
        if !self.valid(id) {
            return INVALID_INDEX;
        }
        self.nodes[id].left
    }

    pub fn right(&self, id: NodeId) -> NodeId {
        if !self.valid(id) {
            return INVALID_INDEX;
        }
        self.nodes[id].right
    }

    pub fn parent(&self, id: NodeId) -> NodeId {
        if !self.valid(id) {
            return INVALID_INDEX;
        }
        self.nodes[id].parent
    }

    pub fn is_leaf(&self, id: NodeId) -> bool {
        if !self.valid(id) {
            return false;
        }
        self.nodes[id].left == INVALID_INDEX && self.nodes[id].right == INVALID_INDEX
    }

    pub fn has_left(&self, id: NodeId) -> bool {
        if !self.valid(id) {
            return false;
        }
        self.nodes[id].left != INVALID_INDEX
    }

    pub fn has_right(&self, id: NodeId) -> bool {
        if !self.valid(id) {
            return false;
        }
        self.nodes[id].right != INVALID_INDEX
    }

    pub fn is_root(&self, id: NodeId) -> bool {
        id == self.root && self.valid(id)
    }

    pub fn add_left(&mut self, parent_id: NodeId, value: T) -> NodeId {
        if !self.valid(parent_id) {
            panic!("BinaryTree::add_left: invalid parent ID");
        }
        if self.nodes[parent_id].left != INVALID_INDEX {
            panic!("BinaryTree::add_left: node already has left child");
        }
        let new_id = self.allocate_node(value, parent_id);
        self.nodes[parent_id].left = new_id;
        new_id
    }

    pub fn add_right(&mut self, parent_id: NodeId, value: T) -> NodeId {
        if !self.valid(parent_id) {
            panic!("BinaryTree::add_right: invalid parent ID");
        }
        if self.nodes[parent_id].right != INVALID_INDEX {
            panic!("BinaryTree::add_right: node already has right child");
        }
        let new_id = self.allocate_node(value, parent_id);
        self.nodes[parent_id].right = new_id;
        new_id
    }

    pub fn remove(&mut self, id: NodeId) {
        if !self.valid(id) {
            return;
        }
        let left = self.nodes[id].left;
        let right = self.nodes[id].right;
        if left != INVALID_INDEX {
            self.remove(left);
        }
        if right != INVALID_INDEX {
            self.remove(right);
        }
        let parent_id = self.nodes[id].parent;
        if parent_id != INVALID_INDEX {
            if self.nodes[parent_id].left == id {
                self.nodes[parent_id].left = INVALID_INDEX;
            } else if self.nodes[parent_id].right == id {
                self.nodes[parent_id].right = INVALID_INDEX;
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

    pub fn height_of(&self, id: NodeId) -> i32 {
        if !self.valid(id) {
            return -1;
        }
        let lh = self.height_of(self.nodes[id].left);
        let rh = self.height_of(self.nodes[id].right);
        1 + if lh > rh { lh } else { rh }
    }

    pub fn height(&self) -> i32 {
        self.height_of(self.root)
    }

    pub fn subtree_size(&self, id: NodeId) -> usize {
        if !self.valid(id) {
            return 0;
        }
        1 + self.subtree_size(self.nodes[id].left) + self.subtree_size(self.nodes[id].right)
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
        self.preorder_impl(self.nodes[id].left, func);
        self.preorder_impl(self.nodes[id].right, func);
    }

    pub fn inorder<F: FnMut(&T, NodeId)>(&self, mut func: F) {
        self.inorder_impl(self.root, &mut func);
    }

    pub fn inorder_from<F: FnMut(&T, NodeId)>(&self, start: NodeId, mut func: F) {
        self.inorder_impl(start, &mut func);
    }

    fn inorder_impl<F: FnMut(&T, NodeId)>(&self, id: NodeId, func: &mut F) {
        if !self.valid(id) {
            return;
        }
        self.inorder_impl(self.nodes[id].left, func);
        func(&self.nodes[id].value, id);
        self.inorder_impl(self.nodes[id].right, func);
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
        self.postorder_impl(self.nodes[id].left, func);
        self.postorder_impl(self.nodes[id].right, func);
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
            if self.nodes[current].left != INVALID_INDEX {
                queue.push_back(self.nodes[current].left);
            }
            if self.nodes[current].right != INVALID_INDEX {
                queue.push_back(self.nodes[current].right);
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

    pub fn to_inorder(&self) -> Vector<T>
    where
        T: Clone,
    {
        let mut result = Vector::new();
        self.inorder(|val, _| result.push_back(val.clone()));
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
