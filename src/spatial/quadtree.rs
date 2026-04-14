use crate::Vector;

use super::{Aabb, Point};

const DEFAULT_CAPACITY: usize = 16;

#[derive(Debug, Clone, PartialEq)]
pub struct QuadtreeEntry<T> {
    pub point: Point,
    pub data: T,
}

impl<T> QuadtreeEntry<T> {
    pub fn new(point: Point, data: T) -> Self {
        Self { point, data }
    }
}

#[derive(Debug, Clone, PartialEq)]
struct Node<T> {
    boundary: Aabb,
    entries: Vector<QuadtreeEntry<T>>,
    children: Vector<Node<T>>,
}

impl<T> Node<T> {
    fn new(boundary: Aabb) -> Self {
        Self {
            boundary,
            entries: Vector::default(),
            children: Vector::default(),
        }
    }

    fn is_leaf(&self) -> bool {
        self.children.is_empty()
    }

    fn subdivide(&mut self) {
        let center = self.boundary.center();
        let b = self.boundary;

        // NW: min_x..center.x, center.y..max_y
        self.children.push_back(Node::new(Aabb::new(
            Point::new(b.min_point.x, center.y, b.min_point.z),
            Point::new(center.x, b.max_point.y, b.max_point.z),
        )));
        // NE: center.x..max_x, center.y..max_y
        self.children.push_back(Node::new(Aabb::new(
            Point::new(center.x, center.y, b.min_point.z),
            Point::new(b.max_point.x, b.max_point.y, b.max_point.z),
        )));
        // SW: min_x..center.x, min_y..center.y
        self.children.push_back(Node::new(Aabb::new(
            Point::new(b.min_point.x, b.min_point.y, b.min_point.z),
            Point::new(center.x, center.y, b.max_point.z),
        )));
        // SE: center.x..max_x, min_y..center.y
        self.children.push_back(Node::new(Aabb::new(
            Point::new(center.x, b.min_point.y, b.min_point.z),
            Point::new(b.max_point.x, center.y, b.max_point.z),
        )));
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct Quadtree<T> {
    root: Node<T>,
    capacity: usize,
}

impl<T> Default for Quadtree<T> {
    fn default() -> Self {
        Self {
            root: Node::new(Aabb::default()),
            capacity: DEFAULT_CAPACITY,
        }
    }
}

impl<T> Quadtree<T> {
    pub fn new(boundary: Aabb) -> Self {
        Self {
            root: Node::new(boundary),
            capacity: DEFAULT_CAPACITY,
        }
    }

    pub fn with_capacity(boundary: Aabb, capacity: usize) -> Self {
        Self {
            root: Node::new(boundary),
            capacity,
        }
    }

    pub fn boundary(&self) -> Aabb {
        self.root.boundary
    }

    pub fn is_empty(&self) -> bool {
        self.size() == 0
    }

    pub fn size(&self) -> usize {
        Self::count_entries(&self.root)
    }

    fn count_entries(node: &Node<T>) -> usize {
        let mut count = node.entries.len();
        if !node.is_leaf() {
            for child in node.children.iter() {
                count += Self::count_entries(child);
            }
        }
        count
    }

    pub fn clear(&mut self) {
        self.root.entries.clear();
        self.root.children.clear();
    }
}

impl<T: Clone + PartialEq> Quadtree<T> {
    pub fn insert(&mut self, point: Point, data: T) -> bool {
        let entry = QuadtreeEntry::new(point, data);
        Self::insert_node(&mut self.root, entry, self.capacity)
    }

    fn insert_node(node: &mut Node<T>, entry: QuadtreeEntry<T>, capacity: usize) -> bool {
        if !node.boundary.contains(entry.point) {
            return false;
        }

        if node.entries.len() < capacity && node.is_leaf() {
            node.entries.push_back(entry);
            return true;
        }

        if node.is_leaf() {
            node.subdivide();
            let old_entries: Vec<QuadtreeEntry<T>> = node.entries.iter().cloned().collect();
            node.entries.clear();
            for old_entry in old_entries {
                let mut inserted = false;
                for child in node.children.iter_mut() {
                    if Self::insert_node(child, old_entry.clone(), capacity) {
                        inserted = true;
                        break;
                    }
                }
                if !inserted {
                    node.entries.push_back(old_entry);
                }
            }
        }

        for child in node.children.iter_mut() {
            if Self::insert_node(child, entry.clone(), capacity) {
                return true;
            }
        }

        node.entries.push_back(entry);
        true
    }

    pub fn query(&self, range: Aabb) -> Vector<QuadtreeEntry<T>> {
        let mut results = Vector::default();
        Self::query_node(&self.root, range, &mut results);
        results
    }

    fn query_node(node: &Node<T>, range: Aabb, results: &mut Vector<QuadtreeEntry<T>>) {
        if !node.boundary.intersects(range) {
            return;
        }
        for entry in node.entries.iter() {
            if range.contains(entry.point) {
                results.push_back(entry.clone());
            }
        }
        if !node.is_leaf() {
            for child in node.children.iter() {
                Self::query_node(child, range, results);
            }
        }
    }

    pub fn query_radius(&self, center: Point, radius: f64) -> Vector<QuadtreeEntry<T>> {
        let mut results = Vector::default();
        Self::query_radius_node(&self.root, center, radius, &mut results);
        results
    }

    fn query_radius_node(
        node: &Node<T>,
        center: Point,
        radius: f64,
        results: &mut Vector<QuadtreeEntry<T>>,
    ) {
        let query_box = Aabb::new(
            Point::new(center.x - radius, center.y - radius, center.z - radius),
            Point::new(center.x + radius, center.y + radius, center.z + radius),
        );
        if !node.boundary.intersects(query_box) {
            return;
        }
        let radius_sq = radius * radius;
        for entry in node.entries.iter() {
            let dx = entry.point.x - center.x;
            let dy = entry.point.y - center.y;
            if dx * dx + dy * dy <= radius_sq {
                results.push_back(entry.clone());
            }
        }
        if !node.is_leaf() {
            for child in node.children.iter() {
                Self::query_radius_node(child, center, radius, results);
            }
        }
    }

    pub fn remove(&mut self, point: Point, data: &T) -> bool {
        Self::remove_node(&mut self.root, point, data)
    }

    fn remove_node(node: &mut Node<T>, point: Point, data: &T) -> bool {
        if !node.boundary.contains(point) {
            return false;
        }
        for i in 0..node.entries.len() {
            if node.entries[i].point == point && &node.entries[i].data == data {
                let last = node.entries.len() - 1;
                node.entries.swap(i, last);
                node.entries.pop_back();
                return true;
            }
        }
        if !node.is_leaf() {
            for child in node.children.iter_mut() {
                if Self::remove_node(child, point, data) {
                    return true;
                }
            }
        }
        false
    }

    pub fn k_nearest(&self, point: Point, k: usize) -> Vector<QuadtreeEntry<T>> {
        let mut candidates: Vec<(f64, QuadtreeEntry<T>)> = Vec::new();
        Self::k_nearest_node(&self.root, point, &mut candidates);
        candidates.sort_by(|a, b| a.0.partial_cmp(&b.0).unwrap_or(std::cmp::Ordering::Equal));
        let mut results = Vector::default();
        for (_, entry) in candidates.into_iter().take(k) {
            results.push_back(entry);
        }
        results
    }

    fn k_nearest_node(
        node: &Node<T>,
        point: Point,
        candidates: &mut Vec<(f64, QuadtreeEntry<T>)>,
    ) {
        for entry in node.entries.iter() {
            let dx = entry.point.x - point.x;
            let dy = entry.point.y - point.y;
            candidates.push((dx * dx + dy * dy, entry.clone()));
        }
        if !node.is_leaf() {
            for child in node.children.iter() {
                Self::k_nearest_node(child, point, candidates);
            }
        }
    }

    pub fn items(&self) -> Vector<QuadtreeEntry<T>> {
        let mut out = Vector::default();
        Self::collect(&self.root, &mut out);
        out
    }

    fn collect(node: &Node<T>, out: &mut Vector<QuadtreeEntry<T>>) {
        for entry in node.entries.iter() {
            out.push_back(entry.clone());
        }
        if !node.is_leaf() {
            for child in node.children.iter() {
                Self::collect(child, out);
            }
        }
    }
}
