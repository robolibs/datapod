use datapod::Vector;

#[test]
fn vector_new_and_with_capacity_start_empty() {
    assert!(Vector::<i32>::new().empty());
    let vector = Vector::<i32>::with_capacity(8);
    assert!(vector.empty());
    assert!(vector.capacity() >= 8);
}

#[test]
fn vector_from_elem_and_clear_work() {
    let mut vector = Vector::from_elem(3, 7);
    assert_eq!(vector.as_slice(), &[7, 7, 7]);
    vector.clear();
    assert!(vector.empty());
}

#[test]
fn vector_push_back_and_pop_back_work() {
    let mut vector = Vector::default();
    vector.push_back(1);
    vector.push_back(2);
    assert_eq!(vector.pop_back(), Some(2));
    assert_eq!(vector.as_slice(), &[1]);
}

#[test]
fn vector_insert_remove_and_erase_work() {
    let mut vector = Vector::from([1, 3]);
    vector.insert(1, 2);
    assert_eq!(vector.as_slice(), &[1, 2, 3]);
    assert_eq!(vector.remove(1), 2);
    vector.insert(1, 4);
    assert_eq!(vector.erase(1), 4);
}

#[test]
fn vector_truncate_and_resize_work() {
    let mut vector = Vector::from([1, 2, 3, 4]);
    vector.truncate(2);
    assert_eq!(vector.as_slice(), &[1, 2]);
    vector.resize(4, 9);
    assert_eq!(vector.as_slice(), &[1, 2, 9, 9]);
}

#[test]
fn vector_assign_and_extend_from_slice_work() {
    let mut vector = Vector::from([1, 2]);
    vector.assign([3, 4, 5]);
    assert_eq!(vector.as_slice(), &[3, 4, 5]);
    vector.extend_from_slice(&[6, 7]);
    assert_eq!(vector.as_slice(), &[3, 4, 5, 6, 7]);
}

#[test]
fn vector_first_last_front_back_mut_work() {
    let mut vector = Vector::from([1, 2, 3]);
    *vector.first_mut().unwrap() = 4;
    *vector.last_mut().unwrap() = 5;
    assert_eq!(vector.front(), &4);
    assert_eq!(vector.back(), &5);
    *vector.front_mut() = 6;
    *vector.back_mut() = 7;
    assert_eq!(vector.as_slice(), &[6, 2, 7]);
}

#[test]
fn vector_data_and_data_mut_are_non_null_when_non_empty() {
    let mut vector = Vector::from([1, 2, 3]);
    assert!(!vector.data().is_null());
    assert!(!vector.data_mut().is_null());
}

#[test]
fn vector_drain_and_iter_mut_work() {
    let mut vector = Vector::from([1, 2, 3, 4]);
    let drained: Vec<_> = vector.drain(1..3).collect();
    assert_eq!(drained, vec![2, 3]);
    for value in &mut vector {
        *value *= 10;
    }
    assert_eq!(vector.as_slice(), &[10, 40]);
}

#[test]
fn vector_reserve_and_shrink_to_fit_work() {
    let mut vector = Vector::from([1, 2, 3]);
    vector.reserve(20);
    assert!(vector.capacity() >= 23);
    vector.shrink_to_fit();
    assert!(vector.capacity() >= vector.len());
}

#[test]
fn vector_into_iterator_collects_values() {
    let vector = Vector::from([1, 2, 3]);
    let values: Vec<_> = vector.into_iter().collect();
    assert_eq!(values, vec![1, 2, 3]);
}
