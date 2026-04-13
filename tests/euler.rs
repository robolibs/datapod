use datapod::{Euler, Quaternion};

fn approx_eq(left: f64, right: f64, epsilon: f64) {
    assert!((left - right).abs() < epsilon, "{left} != {right}");
}

#[test]
fn euler_normalization_wraps_angles() {
    let value = Euler::new(std::f64::consts::PI + 0.5, 0.0, 0.0).normalized();
    approx_eq(value.roll, -std::f64::consts::PI + 0.5, 1e-9);
}

#[test]
fn euler_trig_helpers_work() {
    let value = Euler::new(0.0, 0.0, std::f64::consts::FRAC_PI_4);
    approx_eq(value.yaw_cos(), std::f64::consts::FRAC_1_SQRT_2, 1e-12);
    approx_eq(value.yaw_sin(), std::f64::consts::FRAC_1_SQRT_2, 1e-12);
}

#[test]
fn euler_quaternion_round_trip() {
    let original = Euler::new(0.1, 0.2, 0.3);
    let quaternion = original.to_quaternion();
    let round_trip = quaternion.to_euler();
    approx_eq(round_trip.roll, original.roll, 1e-9);
    approx_eq(round_trip.pitch, original.pitch, 1e-9);
    approx_eq(round_trip.yaw, original.yaw, 1e-9);
}

#[test]
fn quaternion_identity_is_correct() {
    assert_eq!(Quaternion::identity(), Quaternion::new(1.0, 0.0, 0.0, 0.0));
}
