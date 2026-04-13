use datapod::mat;

#[test]
fn dynamic_vector_from_vec_and_push_pop_work() {
    let mut vector = mat::DynamicVector::from_vec(vec![1, 2, 3]);
    vector.push(4);
    assert_eq!(vector.pop(), Some(4));
    assert_eq!(vector.as_slice(), &[1, 2, 3]);
}

#[test]
fn dynamic_vector_at_and_swap_work() {
    let mut left = mat::DynamicVector::from_vec(vec![1, 2, 3]);
    let mut right = mat::DynamicVector::from_vec(vec![4, 5]);
    assert_eq!(left.at(1), Ok(&2));
    assert!(left.at(3).is_err());
    left.swap(&mut right);
    assert_eq!(left.as_slice(), &[4, 5]);
    assert_eq!(right.as_slice(), &[1, 2, 3]);
}

#[test]
fn dynamic_matrix_empty_and_fill_work() {
    let mut matrix = mat::Dynamic::<i32>::new(0, 3);
    assert!(matrix.empty());
    matrix.resize(2, 2);
    matrix.fill(7);
    assert_eq!(matrix.as_slice(), &[7, 7, 7, 7]);
}

#[test]
fn dynamic_matrix_at_mut_and_iter_work() {
    let mut matrix = mat::Dynamic::<i32>::new(2, 2);
    *matrix.at_mut(1, 1).unwrap() = 9;
    let sum: i32 = matrix.iter().copied().sum();
    assert_eq!(sum, 9);
}

#[test]
fn dynamic_matrix_invalid_shape_is_detected() {
    let matrix = mat::Dynamic {
        rows: 2,
        cols: 3,
        values: vec![1, 2],
    };
    assert!(!matrix.is_valid());
}

#[test]
fn dynamic_matrix_swap_exchanges_shapes_and_values() {
    let mut left = mat::Dynamic::from_vec(1, 2, vec![1, 2]).unwrap();
    let mut right = mat::Dynamic::from_vec(2, 1, vec![3, 4]).unwrap();
    left.swap(&mut right);
    assert_eq!(
        (left.rows(), left.cols(), left.as_slice()),
        (2, 1, &[3, 4][..])
    );
    assert_eq!(
        (right.rows(), right.cols(), right.as_slice()),
        (1, 2, &[1, 2][..])
    );
}

#[test]
fn dynamic_tensor_new_builds_expected_default_storage() {
    let tensor = mat::Tensor::<i32>::new(vec![2, 2, 2]);
    assert_eq!(tensor.as_slice(), &[0; 8]);
}

#[test]
fn dynamic_tensor_dim_and_empty_work() {
    let tensor = mat::Tensor::<i32>::default();
    assert_eq!(tensor.rank(), 0);
    assert!(tensor.empty());
    assert_eq!(tensor.dim(0), None);
}

#[test]
fn dynamic_tensor_fill_and_swap_work() {
    let mut left = mat::Tensor::<i32>::new(vec![2, 2, 1]);
    let mut right = mat::Tensor::<i32>::new(vec![1, 1, 1]);
    left.fill(3);
    right.fill(9);
    left.swap(&mut right);
    assert_eq!(left.shape(), &[1, 1, 1]);
    assert_eq!(left.as_slice(), &[9]);
    assert_eq!(right.shape(), &[2, 2, 1]);
    assert_eq!(right.as_slice(), &[3, 3, 3, 3]);
}

#[test]
fn dynamic_tensor_at_and_rank_validation_errors_work() {
    let tensor = mat::Tensor::<i32>::from_vec(vec![2, 2, 2], vec![1, 2, 3, 4, 5, 6, 7, 8]).unwrap();
    assert_eq!(tensor.at(&[1, 1, 1]), Ok(&8));
    assert!(tensor.at(&[2, 0, 0]).is_err());
    assert!(tensor.at(&[0, 0]).is_err());
}

#[test]
fn dynamic_tensor_alias_is_available() {
    let tensor: mat::DynamicTensor<i32> = mat::Tensor::new(vec![1, 2, 3]);
    assert_eq!(tensor.rank(), 3);
}
