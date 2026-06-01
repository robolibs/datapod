use datapod::{Euler, Geo, Linestring, Point, Polygon, Quaternion, Size};

fn approx_eq(left: f64, right: f64, epsilon: f64) {
    assert!((left - right).abs() < epsilon, "{left} != {right}");
}

#[test]
fn point_is_set_tracks_non_zero_coordinates() {
    assert!(!Point::default().is_set());
    assert!(Point::new(0.0, 1.0, 0.0).is_set());
}

#[test]
fn point_round_trips_through_mat() {
    let point = Point::new(1.0, 2.0, 3.0);
    assert_eq!(Point::from_mat(point.to_mat()), point);
}

#[test]
fn point_scalar_mul_and_div_work() {
    let point = Point::new(2.0, -4.0, 6.0);
    assert_eq!(point * 0.5, Point::new(1.0, -2.0, 3.0));
    assert_eq!(point / 2.0, Point::new(1.0, -2.0, 3.0));
}

#[test]
fn geo_is_set_and_validation_cover_edges() {
    assert!(!Geo::default().is_set());
    assert!(Geo::new(90.0, 180.0, 0.0).is_valid());
    assert!(!Geo::new(91.0, 0.0, 0.0).is_valid());
    assert!(!Geo::new(0.0, 181.0, 0.0).is_valid());
}

#[test]
fn geo_distance_to_self_is_zero() {
    let geo = Geo::new(52.0, 5.0, 1.0);
    approx_eq(geo.distance_to(geo), 0.0, 1e-9);
}

#[test]
fn euler_is_set_and_make_helper_work() {
    assert!(!Euler::default().is_set());
    assert_eq!(
        Euler::new(1.0, 2.0, 3.0),
        Euler {
            roll: 1.0,
            pitch: 2.0,
            yaw: 3.0
        }
    );
}

#[test]
fn euler_add_sub_mul_and_mat_round_trip_work() {
    let left = Euler::new(1.0, 2.0, 3.0);
    let right = Euler::new(0.5, 1.0, 1.5);
    assert_eq!(left + right, Euler::new(1.5, 3.0, 4.5));
    assert_eq!(left - right, Euler::new(0.5, 1.0, 1.5));
    assert_eq!(right * 2.0, Euler::new(1.0, 2.0, 3.0));
    assert_eq!(Euler::from_mat(left.to_mat()), left);
}

#[test]
fn quaternion_is_set_tracks_identity() {
    assert!(!Quaternion::identity().is_set());
    assert!(Quaternion::new(0.0, 1.0, 0.0, 0.0).is_set());
}

#[test]
fn quaternion_conjugate_inverse_and_normalization_work() {
    let q = Quaternion::new(2.0, 0.0, 0.0, 0.0);
    assert_eq!(q.conjugate(), Quaternion::new(2.0, -0.0, -0.0, -0.0));
    assert_eq!(q.inverse(), Quaternion::new(0.5, -0.0, -0.0, -0.0));
    assert_eq!(q.normalized(), Quaternion::identity());
}

#[test]
fn quaternion_add_sub_and_scalar_ops_work() {
    let left = Quaternion::new(1.0, 2.0, 3.0, 4.0);
    let right = Quaternion::new(0.5, 1.0, 1.5, 2.0);
    assert_eq!(left + right, Quaternion::new(1.5, 3.0, 4.5, 6.0));
    assert_eq!(left - right, Quaternion::new(0.5, 1.0, 1.5, 2.0));
    assert_eq!(right * 2.0, Quaternion::new(1.0, 2.0, 3.0, 4.0));
    assert_eq!(left / 2.0, Quaternion::new(0.5, 1.0, 1.5, 2.0));
}

#[test]
fn quaternion_identity_keeps_rotated_vector_unchanged() {
    let mut x = 1.0;
    let mut y = 2.0;
    let mut z = 3.0;
    Quaternion::identity().rotate_vector(&mut x, &mut y, &mut z);
    assert_eq!((x, y, z), (1.0, 2.0, 3.0));
}

#[test]
fn linestring_empty_and_num_points_work() {
    let empty = Linestring::default();
    assert!(empty.empty());
    assert_eq!(empty.num_points(), 0);
    let line = Linestring::new(vec![Point::new(0.0, 0.0, 0.0), Point::new(3.0, 4.0, 0.0)]);
    assert!(!line.empty());
    assert_eq!(line.num_points(), 2);
    approx_eq(line.length(), 5.0, 1e-9);
}

#[test]
fn polygon_empty_validity_and_num_vertices_work() {
    let empty = Polygon::default();
    assert!(empty.empty());
    assert!(!empty.is_valid());
    let polygon = Polygon::new(vec![
        Point::new(0.0, 0.0, 0.0),
        Point::new(1.0, 0.0, 0.0),
        Point::new(0.0, 1.0, 0.0),
    ]);
    assert_eq!(polygon.num_vertices(), 3);
    assert!(polygon.is_valid());
}

#[test]
fn size_round_trips_through_mat() {
    let size = Size::new(1.0, 2.0, 3.0);
    assert_eq!(Size::from_mat(size.to_mat()), size);
}

#[test]
fn size_arithmetic_and_extrema_work() {
    let left = Size::new(-1.0, 2.0, -3.0);
    let right = Size::new(4.0, -5.0, 6.0);
    assert_eq!(left.abs(), Size::new(1.0, 2.0, 3.0));
    assert_eq!(left.max(right), Size::new(4.0, 2.0, 6.0));
    assert_eq!(left.min(right), Size::new(-1.0, -5.0, -3.0));
    assert_eq!(left + right, Size::new(3.0, -3.0, 3.0));
    assert_eq!(right - left, Size::new(5.0, -7.0, 9.0));
    assert_eq!(right * 2.0, Size::new(8.0, -10.0, 12.0));
    assert_eq!(right / 2.0, Size::new(2.0, -2.5, 3.0));
    assert_eq!(
        Size::new(2.0, 3.0, 4.0) * Size::new(5.0, 6.0, 7.0),
        Size::new(10.0, 18.0, 28.0)
    );
}
