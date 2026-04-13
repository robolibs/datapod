use datapod::mat;

#[test]
fn dynamic_vector_supports_runtime_shape_and_mutation() {
    let mut vector = mat::DVector::<f64>::new(3);
    assert_eq!(vector.size(), 3);
    assert!(!vector.empty());
    vector[0] = 1.0;
    vector[1] = 2.0;
    vector[2] = 3.0;
    assert_eq!(vector.as_slice(), &[1.0, 2.0, 3.0]);
    assert_eq!(vector.front(), Some(&1.0));
    assert_eq!(vector.back(), Some(&3.0));
    vector.resize(5, 9.0);
    assert_eq!(vector.as_slice(), &[1.0, 2.0, 3.0, 9.0, 9.0]);
    vector.fill(4.0);
    assert_eq!(vector.as_slice(), &[4.0, 4.0, 4.0, 4.0, 4.0]);
}

#[test]
fn dynamic_matrix_supports_rows_cols_and_indexing() {
    let mut matrix = mat::DMatrix::<f64>::new(2, 3);
    assert_eq!(matrix.rows(), 2);
    assert_eq!(matrix.cols(), 3);
    assert!(matrix.is_valid());

    matrix[(0, 0)] = 1.0;
    matrix[(0, 1)] = 2.0;
    matrix[(1, 2)] = 6.0;
    assert_eq!(matrix[(1, 2)], 6.0);
    assert_eq!(matrix.row(0).unwrap(), &[1.0, 2.0, 0.0]);
    assert_eq!(*matrix.at(0, 1).unwrap(), 2.0);

    matrix.resize(3, 2);
    assert_eq!(matrix.rows(), 3);
    assert_eq!(matrix.cols(), 2);
    assert_eq!(matrix.size(), 6);
}

#[test]
fn dynamic_matrix_can_be_constructed_from_flat_storage() {
    let matrix = mat::DMatrix::from_vec(2, 2, vec![1_i32, 2, 3, 4]).unwrap();
    assert_eq!(matrix[(0, 0)], 1);
    assert_eq!(matrix[(0, 1)], 2);
    assert_eq!(matrix[(1, 0)], 3);
    assert_eq!(matrix[(1, 1)], 4);
    assert!(mat::DMatrix::<i32>::from_vec(2, 3, vec![1, 2]).is_err());
}

#[test]
fn dynamic_tensor_supports_runtime_rank_and_indexing() {
    let mut tensor = mat::DTensor::<i32>::new(vec![2, 3, 2]);
    assert_eq!(tensor.rank(), 3);
    assert_eq!(tensor.shape(), &[2, 3, 2]);
    assert_eq!(tensor.strides(), &[1, 2, 6]);
    assert_eq!(tensor.size(), 12);

    *tensor.at_mut(&[1, 2, 1]).unwrap() = 7;
    assert_eq!(*tensor.at(&[1, 2, 1]).unwrap(), 7);
    assert_eq!(tensor.linear_index(&[1, 2, 1]).unwrap(), 11);

    tensor.resize(vec![2, 2, 2, 2]);
    assert_eq!(tensor.rank(), 4);
    assert_eq!(tensor.shape(), &[2, 2, 2, 2]);
    assert_eq!(tensor.size(), 16);
}

#[test]
fn dynamic_tensor_validates_shape_mismatch() {
    assert!(mat::DTensor::<i32>::from_vec(vec![2, 2, 2], vec![1, 2, 3]).is_err());
    let tensor = mat::DTensor::<i32>::from_vec(vec![2, 2], vec![1, 2, 3, 4]).unwrap();
    assert_eq!(*tensor.at(&[1, 1]).unwrap(), 4);
}
