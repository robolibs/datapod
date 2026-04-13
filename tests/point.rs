use datapod::Point;

fn approx_eq(left: f64, right: f64) {
    assert!((left - right).abs() < 1e-9, "{left} != {right}");
}

#[test]
fn point_distance_and_magnitude() {
    let point = Point::new(3.0, 4.0, 0.0);
    approx_eq(point.magnitude(), 5.0);
    approx_eq(point.distance_to(Point::default()), 5.0);
}

#[test]
fn point_2d_distance_ignores_z() {
    let p1 = Point::new(0.0, 0.0, 0.0);
    let p2 = Point::new(3.0, 4.0, 100.0);
    approx_eq(p1.distance_to_2d(p2), 5.0);
}

#[test]
fn point_arithmetic() {
    let p1 = Point::new(1.0, 2.0, 3.0);
    let p2 = Point::new(4.0, 5.0, 6.0);
    assert_eq!(p1 + p2, Point::new(5.0, 7.0, 9.0));
    assert_eq!(p2 - p1, Point::new(3.0, 3.0, 3.0));
    assert_eq!(p1 * 2.0, Point::new(2.0, 4.0, 6.0));
}
