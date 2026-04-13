use datapod::{Euler, Point, Segment};

#[test]
fn point_round_trips_through_mat_vector() {
    let point = Point::new(1.0, 2.0, 3.0);
    let mat = point.to_mat();
    assert_eq!(Point::from_mat(mat), point);
}

#[test]
fn euler_round_trips_through_mat_vector() {
    let euler = Euler::new(0.1, 0.2, 0.3);
    let mat = euler.to_mat();
    assert_eq!(Euler::from_mat(mat), euler);
}

#[test]
fn segment_round_trips_through_mat_vector() {
    let segment = Segment::new(Point::new(1.0, 2.0, 3.0), Point::new(4.0, 5.0, 6.0));
    let mat = segment.to_mat();
    assert_eq!(Segment::from_mat(mat), segment);
}
