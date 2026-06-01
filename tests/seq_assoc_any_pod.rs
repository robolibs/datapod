//! Smoke tests that prove the seq/ and assoc/ Pods can hold arbitrary
//! `bytemuck::Pod` types — including domain Pods like `Point`.

use datapod::{Heap, IndexedHeap, List, Map, Matrix, Point, Queue, Set, Stack, Tensor, Vector};

#[test]
fn vector_holds_point() {
    let mut v: Vector = Vector::new::<Point>();
    v.push(Point::new(1.0, 2.0, 3.0));
    v.push(Point::new(4.0, 5.0, 6.0));
    assert_eq!(v.size(), 2);
    assert_eq!(v.get::<Point>(0), Point::new(1.0, 2.0, 3.0));
    assert_eq!(v.get::<Point>(1), Point::new(4.0, 5.0, 6.0));
    assert_eq!(v.pop::<Point>(), Some(Point::new(4.0, 5.0, 6.0)));
    assert_eq!(v.as_slice::<Point>(), &[Point::new(1.0, 2.0, 3.0)]);
}

#[test]
fn stack_holds_u64() {
    let mut s: Stack = Stack::new::<u64>();
    s.push(10u64);
    s.push(20u64);
    s.push(30u64);
    assert_eq!(s.top::<u64>(), Some(30));
    assert_eq!(s.pop::<u64>(), Some(30));
    assert_eq!(s.pop::<u64>(), Some(20));
    assert_eq!(s.size(), 1);
}

#[test]
fn queue_holds_point() {
    let mut q: Queue = Queue::new::<Point>();
    q.push_back(Point::new(1.0, 1.0, 1.0));
    q.push_back(Point::new(2.0, 2.0, 2.0));
    q.push_back(Point::new(3.0, 3.0, 3.0));
    assert_eq!(q.pop_front::<Point>(), Some(Point::new(1.0, 1.0, 1.0)));
    assert_eq!(q.pop_front::<Point>(), Some(Point::new(2.0, 2.0, 2.0)));
    assert_eq!(q.size(), 1);
}

#[test]
fn matrix_holds_f32() {
    let mut m: Matrix = Matrix::new::<f32>(3, 4);
    m.set::<f32>(1, 2, 7.5);
    assert_eq!(m.get::<f32>(1, 2), 7.5);
    assert_eq!(m.as_slice::<f32>().len(), 12);
}

#[test]
fn tensor_holds_i32() {
    let mut t: Tensor = Tensor::new::<i32>(2, 3, 4);
    t.set::<i32>(0, 0, 1, 42);
    assert_eq!(t.get::<i32>(0, 0, 1), 42);
    assert_eq!(t.size(), 24);
}

#[test]
fn heap_holds_f64_max() {
    let mut h: Heap = Heap::new::<f64>();
    h.push(1.0f64);
    h.push(5.0f64);
    h.push(3.0f64);
    h.push(2.0f64);
    assert_eq!(h.pop::<f64>(), Some(5.0));
    assert_eq!(h.pop::<f64>(), Some(3.0));
    assert_eq!(h.pop::<f64>(), Some(2.0));
    assert_eq!(h.pop::<f64>(), Some(1.0));
}

#[test]
fn indexed_heap_with_point_priorities() {
    // Priority is a Point (we sort by partial_cmp of Point, which goes
    // through derive's lexicographic PartialOrd).
    let mut h: IndexedHeap = IndexedHeap::new::<f64>();
    h.push(1u64, 0.5f64);
    h.push(2u64, 2.5f64);
    h.push(3u64, 1.5f64);
    let (k, p) = h.pop::<f64>().unwrap();
    assert_eq!(k, 2);
    assert_eq!(p, 2.5);
}

#[test]
fn list_holds_point() {
    let mut l: List = List::new::<Point>();
    l.push_back(Point::new(1.0, 0.0, 0.0));
    l.push_back(Point::new(2.0, 0.0, 0.0));
    l.push_front(Point::new(0.0, 0.0, 0.0));
    assert_eq!(l.size(), 3);
    let collected: Vec<Point> = l.iter::<Point>().collect();
    assert_eq!(
        collected,
        vec![
            Point::new(0.0, 0.0, 0.0),
            Point::new(1.0, 0.0, 0.0),
            Point::new(2.0, 0.0, 0.0),
        ]
    );
    assert_eq!(l.pop_back::<Point>(), Some(Point::new(2.0, 0.0, 0.0)));
    assert_eq!(l.pop_front::<Point>(), Some(Point::new(0.0, 0.0, 0.0)));
}

#[test]
fn map_string_to_string() {
    let mut m: Map = Map::new();
    assert!(m.insert_str("alpha", "one").is_none());
    assert!(m.insert_str("bravo", "two").is_none());
    assert!(m.insert_str("charlie", "three").is_none());
    assert_eq!(m.size(), 3);
    assert_eq!(m.get_str("bravo"), Some("two"));
    assert_eq!(m.get_str("alpha"), Some("one"));
    assert_eq!(m.get_str("delta"), None);

    // Replace existing key.
    let old = m.insert_str("bravo", "TWO");
    assert_eq!(old.as_deref(), Some(b"two".as_ref()));
    assert_eq!(m.get_str("bravo"), Some("TWO"));

    // Iteration is in sorted-key order.
    let collected: Vec<(String, String)> = m
        .iter()
        .map(|(k, v)| {
            (
                std::str::from_utf8(k).unwrap().to_string(),
                std::str::from_utf8(v).unwrap().to_string(),
            )
        })
        .collect();
    assert_eq!(
        collected,
        vec![
            ("alpha".to_string(), "one".to_string()),
            ("bravo".to_string(), "TWO".to_string()),
            ("charlie".to_string(), "three".to_string()),
        ]
    );

    // Remove.
    let removed = m.remove(b"alpha");
    assert_eq!(removed.as_deref(), Some(b"one".as_ref()));
    assert_eq!(m.size(), 2);
    assert!(!m.contains_key(b"alpha"));
}

#[test]
fn map_pod_keys_and_values() {
    // Use Point as both key and value via insert_pod/get_pod.
    let mut m: Map = Map::new();
    let k1 = Point::new(1.0, 0.0, 0.0);
    let k2 = Point::new(0.0, 1.0, 0.0);
    m.insert_pod(&k1, &Point::new(10.0, 10.0, 10.0));
    m.insert_pod(&k2, &Point::new(20.0, 20.0, 20.0));
    assert_eq!(
        m.get_pod::<Point, Point>(&k1),
        Some(Point::new(10.0, 10.0, 10.0))
    );
    assert_eq!(
        m.get_pod::<Point, Point>(&k2),
        Some(Point::new(20.0, 20.0, 20.0))
    );
}

#[test]
fn set_string_keys() {
    let mut s: Set = Set::new();
    assert!(s.insert_str("foo"));
    assert!(s.insert_str("bar"));
    assert!(s.insert_str("baz"));
    assert!(!s.insert_str("foo")); // duplicate
    assert_eq!(s.size(), 3);
    assert!(s.contains_str("foo"));
    assert!(!s.contains_str("quux"));

    let collected: Vec<String> = s
        .iter()
        .map(|k| std::str::from_utf8(k).unwrap().to_string())
        .collect();
    assert_eq!(collected, vec!["bar", "baz", "foo"]);

    assert!(s.remove(b"baz"));
    assert!(!s.remove(b"baz"));
    assert_eq!(s.size(), 2);
}
