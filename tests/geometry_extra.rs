use datapod::{
    Aabb, BoundingSphere, Box, Circle, Euler, Obb, Point, Rectangle, Size, Square, Triangle,
};

fn approx_eq(left: f64, right: f64, epsilon: f64) {
    assert!((left - right).abs() < epsilon, "{left} != {right}");
}

#[test]
fn aabb_center_volume_and_surface_area_are_correct() {
    let aabb = Aabb::new(Point::new(0.0, 0.0, 0.0), Point::new(2.0, 3.0, 4.0));
    assert_eq!(aabb.center(), Point::new(1.0, 1.5, 2.0));
    approx_eq(aabb.volume(), 24.0, 1e-9);
    approx_eq(aabb.surface_area(), 52.0, 1e-9);
}

#[test]
fn aabb_contains_and_intersects_handle_false_cases() {
    let left = Aabb::new(Point::new(0.0, 0.0, 0.0), Point::new(1.0, 1.0, 1.0));
    let right = Aabb::new(Point::new(2.0, 2.0, 2.0), Point::new(3.0, 3.0, 3.0));
    assert!(!left.contains(Point::new(-1.0, 0.0, 0.0)));
    assert!(!left.intersects(right));
}

#[test]
fn aabb_expand_point_and_box_grow_bounds() {
    let mut aabb = Aabb::new(Point::new(0.0, 0.0, 0.0), Point::new(1.0, 1.0, 1.0));
    aabb.expand_point(Point::new(-1.0, 0.5, 2.0));
    assert_eq!(aabb.min_point, Point::new(-1.0, 0.0, 0.0));
    assert_eq!(aabb.max_point, Point::new(1.0, 1.0, 2.0));
    aabb.expand_box(Aabb::new(
        Point::new(-2.0, -3.0, -4.0),
        Point::new(0.0, 0.0, 0.0),
    ));
    assert_eq!(aabb.min_point, Point::new(-2.0, -3.0, -4.0));
}

#[test]
fn aabb_distance_is_zero_for_inside_points() {
    let aabb = Aabb::new(Point::new(0.0, 0.0, 0.0), Point::new(1.0, 1.0, 1.0));
    approx_eq(aabb.distance_to_point(Point::new(0.5, 0.5, 0.5)), 0.0, 1e-9);
}

#[test]
fn aabb_round_trips_through_mat() {
    let aabb = Aabb::new(Point::new(1.0, 2.0, 3.0), Point::new(4.0, 5.0, 6.0));
    assert_eq!(Aabb::from_mat(aabb.to_mat()), aabb);
}

#[test]
fn obb_volume_surface_area_and_full_size_are_correct() {
    let obb = Obb::new(Point::default(), Size::new(1.0, 2.0, 3.0), Euler::default());
    approx_eq(obb.volume(), 48.0, 1e-9);
    approx_eq(obb.surface_area(), 88.0, 1e-9);
    assert_eq!(obb.full_size(), Size::new(2.0, 4.0, 6.0));
}

#[test]
fn obb_contains_and_corners_work() {
    let obb = Obb::new(
        Point::new(1.0, 1.0, 1.0),
        Size::new(1.0, 1.0, 1.0),
        Euler::default(),
    );
    assert!(obb.contains(Point::new(2.0, 2.0, 2.0)));
    assert!(!obb.contains(Point::new(3.1, 1.0, 1.0)));
    assert_eq!(obb.corners().len(), 8);
}

#[test]
fn obb_round_trips_through_mat() {
    let obb = Obb::new(
        Point::new(1.0, 2.0, 3.0),
        Size::new(4.0, 5.0, 6.0),
        Euler::new(0.1, 0.2, 0.3),
    );
    assert_eq!(Obb::from_mat(obb.to_mat()), obb);
}

#[test]
fn box_center_corners_and_round_trip_work() {
    let bx = Box {
        pose: datapod::Pose {
            point: Point::new(1.0, 2.0, 3.0),
            rotation: datapod::Quaternion::identity(),
        },
        size: Size::new(2.0, 4.0, 6.0),
    };
    assert_eq!(bx.center(), Point::new(1.0, 2.0, 3.0));
    assert_eq!(bx.corners().len(), 8);
    assert_eq!(Box::from_mat(bx.to_mat()), bx);
}

#[test]
fn bounding_sphere_unit_has_origin_center_and_radius_one() {
    let sphere = BoundingSphere::unit();
    assert_eq!(sphere.center, Point::default());
    approx_eq(sphere.radius, 1.0, 1e-9);
}

#[test]
fn circle_round_trip_and_contains_boundary_point() {
    let circle = Circle {
        center: Point::new(1.0, 1.0, 0.0),
        radius: 2.0,
    };
    assert!(circle.contains(Point::new(3.0, 1.0, 0.0)));
    assert_eq!(Circle::from_mat(circle.to_mat()), circle);
}

#[test]
fn rectangle_round_trip_and_corners_order_work() {
    let rectangle = Rectangle {
        top_left: Point::new(0.0, 1.0, 0.0),
        top_right: Point::new(2.0, 1.0, 0.0),
        bottom_left: Point::new(0.0, 0.0, 0.0),
        bottom_right: Point::new(2.0, 0.0, 0.0),
    };
    assert_eq!(rectangle.get_corners()[0], rectangle.bottom_left);
    assert_eq!(Rectangle::from_mat(rectangle.to_mat()), rectangle);
}

#[test]
fn square_round_trip_and_corner_count_work() {
    let square = Square {
        center: Point::new(0.0, 0.0, 0.0),
        side: 2.0,
    };
    assert_eq!(square.get_corners().len(), 4);
    assert_eq!(Square::from_mat(square.to_mat()), square);
}

#[test]
fn triangle_round_trip_and_contains_interior_point() {
    let triangle = Triangle {
        a: Point::new(0.0, 0.0, 0.0),
        b: Point::new(2.0, 0.0, 0.0),
        c: Point::new(0.0, 2.0, 0.0),
    };
    assert!(triangle.contains(Point::new(0.25, 0.25, 0.0)));
    assert_eq!(Triangle::from_mat(triangle.to_mat()), triangle);
}
