use datapod::{
    Aabb, BoundingSphere, Box, Circle, Obb, Point, Pose, Quaternion, Rectangle, Size, Square,
    Triangle,
};

fn approx_eq(left: f64, right: f64, epsilon: f64) {
    assert!((left - right).abs() < epsilon, "{left} != {right}");
}

#[test]
fn size_has_expected_geometry_helpers() {
    let size = Size::new(2.0, 3.0, 6.0);
    approx_eq(size.volume(), 36.0, 1e-9);
    approx_eq(size.diagonal(), 7.0, 1e-9);
    assert!(size.is_set());
}

#[test]
fn aabb_supports_queries_and_round_trip() {
    let mut aabb = Aabb::new(Point::new(0.0, 0.0, 0.0), Point::new(2.0, 3.0, 4.0));
    assert!(aabb.contains(Point::new(1.0, 1.0, 1.0)));
    approx_eq(aabb.volume(), 24.0, 1e-9);
    approx_eq(aabb.surface_area(), 52.0, 1e-9);
    aabb.expand_point(Point::new(3.0, 4.0, 5.0));
    assert_eq!(Aabb::from_mat(aabb.to_mat()), aabb);
}

#[test]
fn circle_square_triangle_and_rectangle_have_basic_geometry() {
    let circle = Circle {
        center: Point::default(),
        radius: 2.0,
    };
    approx_eq(circle.area(), std::f64::consts::PI * 4.0, 1e-9);
    assert!(circle.contains(Point::new(1.0, 1.0, 0.0)));

    let square = Square {
        center: Point::default(),
        side: 2.0,
    };
    approx_eq(square.area(), 4.0, 1e-9);
    assert!(square.contains(Point::new(0.5, 0.5, 0.0)));

    let triangle = Triangle {
        a: Point::new(0.0, 0.0, 0.0),
        b: Point::new(3.0, 0.0, 0.0),
        c: Point::new(0.0, 4.0, 0.0),
    };
    approx_eq(triangle.area(), 6.0, 1e-9);

    let rectangle = Rectangle {
        top_left: Point::new(0.0, 1.0, 0.0),
        top_right: Point::new(2.0, 1.0, 0.0),
        bottom_left: Point::new(0.0, 0.0, 0.0),
        bottom_right: Point::new(2.0, 0.0, 0.0),
    };
    approx_eq(rectangle.area(), 2.0, 1e-9);
    assert!(rectangle.contains(Point::new(1.0, 0.5, 0.0)));
}

#[test]
fn box_and_obb_support_basic_queries() {
    let pose = Pose {
        point: Point::new(1.0, 2.0, 3.0),
        rotation: Quaternion::identity(),
    };
    let box3 = Box {
        pose,
        size: Size::new(2.0, 4.0, 6.0),
    };
    approx_eq(box3.volume(), 48.0, 1e-9);
    assert!(box3.contains(Point::new(1.5, 2.0, 3.0)));
    assert_eq!(Box::from_mat(box3.to_mat()), box3);

    let obb = Obb::new(
        Point::default(),
        Size::new(1.0, 2.0, 3.0),
        Default::default(),
    );
    approx_eq(obb.volume(), 48.0, 1e-9);
    assert!(obb.contains(Point::new(0.5, 1.5, 2.5)));
    assert_eq!(Obb::from_mat(obb.to_mat()), obb);
}

#[test]
fn bounding_sphere_unit_exists() {
    let sphere = BoundingSphere::unit();
    assert_eq!(sphere.center, Point::default());
    approx_eq(sphere.radius, 1.0, 1e-9);
}
