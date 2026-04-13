use datapod::mat;

fn approx_eq(left: f64, right: f64, epsilon: f64) {
    assert!((left - right).abs() < epsilon, "{left} != {right}");
}

#[test]
fn mat_quaternion_basic_algebra() {
    let q = mat::Quaternion::new(1.0_f64, 2.0, 3.0, 4.0);
    assert_eq!(q.conjugate(), mat::Quaternion::new(1.0, -2.0, -3.0, -4.0));
    assert_eq!(q + q, mat::Quaternion::new(2.0, 4.0, 6.0, 8.0));
    assert_eq!(q * 2.0, mat::Quaternion::new(2.0, 4.0, 6.0, 8.0));
}

#[test]
fn mat_quaternion_rotation_construction_and_conversion() {
    let q = mat::Quaternion::from_euler(0.1, 0.2, 0.3);
    let (roll, pitch, yaw) = q.to_euler();
    approx_eq(roll, 0.1, 1e-9);
    approx_eq(pitch, 0.2, 1e-9);
    approx_eq(yaw, 0.3, 1e-9);
    assert!(q.is_unit(1e-9));
}

#[test]
fn mat_quaternion_interpolation_works() {
    let a = mat::Quaternion::identity();
    let b = mat::Quaternion::from_axis_angle(0.0, 0.0, 1.0, std::f64::consts::PI);
    let mid = mat::nlerp(a, b, 0.5);
    assert!(mid.is_unit(1e-9));
    let smooth = mat::slerp(a, b, 0.5);
    assert!(smooth.is_unit(1e-9));
}
