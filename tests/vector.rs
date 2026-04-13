use datapod::Vector;

#[test]
fn vector_basic_operations() {
    let mut values = Vector::new();
    assert_eq!(values.size(), 0);
    assert!(values.empty());

    values.push_back(1);
    values.push_back(2);
    values.push_back(3);

    assert_eq!(values.size(), 3);
    assert_eq!(values[0], 1);
    assert_eq!(values.front(), &1);
    assert_eq!(values.back(), &3);
}

#[test]
fn vector_capacity_and_resize() {
    let mut values = Vector::with_capacity(8);
    assert!(values.capacity() >= 8);

    values.resize(3, 42);
    assert_eq!(values.as_slice(), &[42, 42, 42]);

    values.insert(1, 7);
    assert_eq!(values.as_slice(), &[42, 7, 42, 42]);

    assert_eq!(values.erase(2), 42);
    assert_eq!(values.as_slice(), &[42, 7, 42]);
}

#[test]
fn vector_from_array_and_iteration() {
    let values = Vector::from([10, 20, 30]);
    let sum: i32 = values.iter().copied().sum();
    assert_eq!(sum, 60);
}
