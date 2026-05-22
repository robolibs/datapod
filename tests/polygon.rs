use datapod::{Point, Polygon};

fn approx_eq(left: f64, right: f64, epsilon: f64) {
    assert!((left - right).abs() < epsilon, "{left} != {right}");
}

fn square() -> Polygon {
    Polygon::new(vec![
        Point::new(0.0, 0.0, 0.0),
        Point::new(1.0, 0.0, 0.0),
        Point::new(1.0, 1.0, 0.0),
        Point::new(0.0, 1.0, 0.0),
    ])
}

#[test]
fn polygon_area_perimeter_and_contains() {
    let p = square();
    approx_eq(p.area(), 1.0, 1e-9);
    approx_eq(p.perimeter(), 4.0, 1e-9);
    assert!(p.contains(Point::new(0.5, 0.5, 0.0)));
    assert!(!p.contains(Point::new(2.0, 2.0, 0.0)));
}

#[test]
fn polygon_aabb_matches_bounds() {
    let p = Polygon::new(vec![
        Point::new(1.0, 2.0, 3.0),
        Point::new(3.0, 4.0, -1.0),
        Point::new(-2.0, 5.0, 2.0),
    ]);
    let aabb = p.get_aabb();
    assert_eq!(aabb.min_point, Point::new(-2.0, 2.0, -1.0));
    assert_eq!(aabb.max_point, Point::new(3.0, 5.0, 3.0));
}

#[test]
fn polygon_obb_returns_positive_extents() {
    let p = square();
    let obb = p.get_obb();
    assert!(obb.half_extents.x > 0.0);
    assert!(obb.half_extents.y > 0.0);
}
