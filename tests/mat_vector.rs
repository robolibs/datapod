use datapod::mat;

#[test]
fn mat_vector_basic_surface() {
    let vector = mat::Vector::<f64, 3>::from([1.0, 2.0, 3.0]);
    assert_eq!(vector[0], 1.0);
    assert_eq!(vector[1], 2.0);
    assert_eq!(vector[2], 3.0);
    assert_eq!(vector.size(), 3);
    assert_eq!(vector.length(), 3);
    assert!(!vector.empty());
}

#[test]
fn mat_vector_fill_and_swap() {
    let mut left = mat::Vector::<i32, 3>::default();
    let mut right = mat::Vector::<i32, 3>::from([10, 20, 30]);
    left.fill(7);
    assert_eq!(left.as_slice(), &[7, 7, 7]);
    left.swap(&mut right);
    assert_eq!(left.as_slice(), &[10, 20, 30]);
    assert_eq!(right.as_slice(), &[7, 7, 7]);
}

#[test]
fn mat_vector_alignment_and_aliases() {
    let vector = mat::Vector3d::default();
    assert_eq!((vector.data() as usize) % 32, 0);
    let state: mat::Vector6f = mat::Vector::from([0.0, 1.0, 2.0, 3.0, 4.0, 5.0]);
    assert_eq!(state[5], 5.0);
}

#[test]
fn scalar_wrapper_is_trivial() {
    let scalar = mat::Scalar::new(42.0_f64);
    assert_eq!(scalar.value, 42.0);
}
