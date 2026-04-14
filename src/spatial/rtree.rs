use crate::Vector;

use super::{Aabb, Point};

const MAX_ITEMS: usize = 16;
const MIN_ITEMS: usize = MAX_ITEMS * 40 / 100;

#[derive(Debug, Clone, PartialEq)]
pub struct RTreeEntry<T> {
    pub bounds: Aabb,
    pub data: T,
}

impl<T> RTreeEntry<T> {
    pub fn new(bounds: Aabb, data: T) -> Self {
        Self { bounds, data }
    }
}

#[derive(Debug, Clone, PartialEq)]
enum NodeKind {
    Leaf,
    Branch,
}

#[derive(Debug, Clone, PartialEq)]
struct Node<T> {
    kind: NodeKind,
    bounds: Aabb,
    entries: Vector<RTreeEntry<T>>,
    children: Vector<Node<T>>,
}

impl<T> Node<T> {
    fn empty_leaf() -> Self {
        Self {
            kind: NodeKind::Leaf,
            bounds: Aabb::default(),
            entries: Vector::default(),
            children: Vector::default(),
        }
    }

    fn empty_branch() -> Self {
        Self {
            kind: NodeKind::Branch,
            bounds: Aabb::default(),
            entries: Vector::default(),
            children: Vector::default(),
        }
    }

    fn is_leaf(&self) -> bool {
        matches!(self.kind, NodeKind::Leaf)
    }

    fn recompute_bounds(&mut self) {
        if self.is_leaf() {
            if self.entries.is_empty() {
                self.bounds = Aabb::default();
                return;
            }
            let mut b = self.entries[0].bounds;
            for e in self.entries.iter().skip(1) {
                b.expand_box(e.bounds);
            }
            self.bounds = b;
        } else {
            if self.children.is_empty() {
                self.bounds = Aabb::default();
                return;
            }
            let mut b = self.children[0].bounds;
            for c in self.children.iter().skip(1) {
                b.expand_box(c.bounds);
            }
            self.bounds = b;
        }
    }
}

fn enlargement(existing: Aabb, to_include: Aabb) -> f64 {
    let mut expanded = existing;
    expanded.expand_box(to_include);
    expanded.volume() - existing.volume()
}

#[derive(Debug, Clone, PartialEq)]
pub struct RTree<T> {
    root: Node<T>,
    max_items: usize,
}

impl<T> Default for RTree<T> {
    fn default() -> Self {
        Self {
            root: Node::<T>::empty_leaf(),
            max_items: MAX_ITEMS,
        }
    }
}

impl<T> RTree<T> {
    pub fn new() -> Self {
        Self {
            root: Node::<T>::empty_leaf(),
            max_items: MAX_ITEMS,
        }
    }

    pub fn with_max_items(max_items: usize) -> Self {
        Self {
            root: Node::<T>::empty_leaf(),
            max_items,
        }
    }

    pub fn is_empty(&self) -> bool {
        self.size() == 0
    }

    pub fn size(&self) -> usize {
        Self::count_node(&self.root)
    }

    fn count_node(node: &Node<T>) -> usize {
        if node.is_leaf() {
            node.entries.len()
        } else {
            let mut n = 0;
            for c in node.children.iter() {
                n += Self::count_node(c);
            }
            n
        }
    }

    pub fn bounds(&self) -> Aabb {
        self.root.bounds
    }

    pub fn clear(&mut self) {
        self.root = Node::<T>::empty_leaf();
    }
}

impl<T: Clone + PartialEq> RTree<T> {
    pub fn insert(&mut self, bounds: Aabb, data: T) {
        let entry = RTreeEntry::new(bounds, data);
        if let Some(split) = Self::insert_into(&mut self.root, entry, self.max_items) {
            let old_root = std::mem::replace(&mut self.root, Node::empty_branch());
            self.root.children.push_back(old_root);
            self.root.children.push_back(split);
            self.root.recompute_bounds();
        } else {
            self.root.recompute_bounds();
        }
    }

    fn insert_into(
        node: &mut Node<T>,
        entry: RTreeEntry<T>,
        max_items: usize,
    ) -> Option<Node<T>> {
        if node.is_leaf() {
            node.entries.push_back(entry);
            node.recompute_bounds();
            if node.entries.len() > max_items {
                return Some(Self::split_leaf(node));
            }
            return None;
        }

        // Choose child with smallest enlargement
        let mut best_idx = 0usize;
        let mut best_enlarge = f64::INFINITY;
        for (i, child) in node.children.iter().enumerate() {
            let e = enlargement(child.bounds, entry.bounds);
            if e < best_enlarge {
                best_enlarge = e;
                best_idx = i;
            }
        }

        let split = {
            let child = &mut node.children[best_idx];
            Self::insert_into(child, entry, max_items)
        };
        if let Some(new_child) = split {
            node.children.push_back(new_child);
        }
        node.recompute_bounds();
        if node.children.len() > max_items {
            return Some(Self::split_branch(node));
        }
        None
    }

    fn split_leaf(node: &mut Node<T>) -> Node<T> {
        // Simple split: pick two seeds, distribute greedily
        let count = node.entries.len();
        let (s1, s2) = Self::pick_seeds_entries(&node.entries);
        let mut new_node = Node::empty_leaf();
        // Grab entries into local vec then distribute
        let mut entries: Vec<RTreeEntry<T>> = Vec::with_capacity(count);
        while let Some(e) = node.entries.pop_back() {
            entries.push(e);
        }
        entries.reverse();
        node.entries.clear();
        let seed1 = entries.remove(if s1 < s2 { s1 } else { s2 });
        let seed2 = entries.remove(if s1 < s2 { s2 - 1 } else { s1 });
        node.entries.push_back(seed1);
        new_node.entries.push_back(seed2);
        node.recompute_bounds();
        new_node.recompute_bounds();

        for e in entries {
            let enl1 = enlargement(node.bounds, e.bounds);
            let enl2 = enlargement(new_node.bounds, e.bounds);
            if enl1 < enl2 && node.entries.len() + (count - node.entries.len() - new_node.entries.len()) > MIN_ITEMS
            {
                node.entries.push_back(e);
                node.recompute_bounds();
            } else {
                new_node.entries.push_back(e);
                new_node.recompute_bounds();
            }
        }
        new_node
    }

    fn split_branch(node: &mut Node<T>) -> Node<T> {
        let count = node.children.len();
        let (s1, s2) = Self::pick_seeds_children(&node.children);
        let mut new_node = Node::empty_branch();
        let mut children: Vec<Node<T>> = Vec::with_capacity(count);
        while let Some(c) = node.children.pop_back() {
            children.push(c);
        }
        children.reverse();
        node.children.clear();
        let seed1 = children.remove(if s1 < s2 { s1 } else { s2 });
        let seed2 = children.remove(if s1 < s2 { s2 - 1 } else { s1 });
        node.children.push_back(seed1);
        new_node.children.push_back(seed2);
        node.recompute_bounds();
        new_node.recompute_bounds();

        for c in children {
            let enl1 = enlargement(node.bounds, c.bounds);
            let enl2 = enlargement(new_node.bounds, c.bounds);
            if enl1 < enl2 {
                node.children.push_back(c);
                node.recompute_bounds();
            } else {
                new_node.children.push_back(c);
                new_node.recompute_bounds();
            }
        }
        new_node
    }

    fn pick_seeds_entries(entries: &Vector<RTreeEntry<T>>) -> (usize, usize) {
        let n = entries.len();
        let mut worst = f64::NEG_INFINITY;
        let mut s1 = 0;
        let mut s2 = 1.min(n.saturating_sub(1));
        for i in 0..n {
            for j in (i + 1)..n {
                let mut combined = entries[i].bounds;
                combined.expand_box(entries[j].bounds);
                let waste = combined.volume() - entries[i].bounds.volume() - entries[j].bounds.volume();
                if waste > worst {
                    worst = waste;
                    s1 = i;
                    s2 = j;
                }
            }
        }
        (s1, s2)
    }

    fn pick_seeds_children(children: &Vector<Node<T>>) -> (usize, usize) {
        let n = children.len();
        let mut worst = f64::NEG_INFINITY;
        let mut s1 = 0;
        let mut s2 = 1.min(n.saturating_sub(1));
        for i in 0..n {
            for j in (i + 1)..n {
                let mut combined = children[i].bounds;
                combined.expand_box(children[j].bounds);
                let waste = combined.volume() - children[i].bounds.volume() - children[j].bounds.volume();
                if waste > worst {
                    worst = waste;
                    s1 = i;
                    s2 = j;
                }
            }
        }
        (s1, s2)
    }

    pub fn search(&self, query: Aabb) -> Vector<RTreeEntry<T>> {
        let mut results = Vector::default();
        Self::search_node(&self.root, query, &mut results);
        results
    }

    fn search_node(node: &Node<T>, query: Aabb, results: &mut Vector<RTreeEntry<T>>) {
        if !node.bounds.intersects(query) {
            return;
        }
        if node.is_leaf() {
            for entry in node.entries.iter() {
                if entry.bounds.intersects(query) {
                    results.push_back(entry.clone());
                }
            }
        } else {
            for child in node.children.iter() {
                Self::search_node(child, query, results);
            }
        }
    }

    pub fn search_point(&self, point: Point) -> Vector<RTreeEntry<T>> {
        let q = Aabb::new(point, point);
        self.search(q)
    }

    pub fn remove(&mut self, bounds: Aabb, data: &T) -> bool {
        let removed = Self::remove_from(&mut self.root, bounds, data);
        if removed {
            // If root is a branch with single child, collapse
            if !self.root.is_leaf() && self.root.children.len() == 1 {
                let child = self.root.children.pop_back().unwrap();
                self.root = child;
            }
            self.root.recompute_bounds();
        }
        removed
    }

    fn remove_from(node: &mut Node<T>, bounds: Aabb, data: &T) -> bool {
        if !node.bounds.intersects(bounds) {
            return false;
        }
        if node.is_leaf() {
            let mut idx = None;
            for (i, e) in node.entries.iter().enumerate() {
                if e.bounds == bounds && &e.data == data {
                    idx = Some(i);
                    break;
                }
            }
            if let Some(i) = idx {
                let last = node.entries.len() - 1;
                node.entries.swap(i, last);
                node.entries.pop_back();
                node.recompute_bounds();
                return true;
            }
            return false;
        }
        for child in node.children.iter_mut() {
            if Self::remove_from(child, bounds, data) {
                node.recompute_bounds();
                return true;
            }
        }
        false
    }

    pub fn items(&self) -> Vector<RTreeEntry<T>> {
        let mut out = Vector::default();
        Self::collect(&self.root, &mut out);
        out
    }

    fn collect(node: &Node<T>, out: &mut Vector<RTreeEntry<T>>) {
        if node.is_leaf() {
            for e in node.entries.iter() {
                out.push_back(e.clone());
            }
        } else {
            for c in node.children.iter() {
                Self::collect(c, out);
            }
        }
    }
}
