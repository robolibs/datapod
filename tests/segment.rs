use datapod::{Point, Segment};

fn approx_eq(left: f64, right: f64) {
    assert!((left - right).abs() < 1e-9, "{left} != {right}");
}

#[test]
fn segment_length_and_midpoint() {
    let segment = Segment::new(Point::new(0.0, 0.0, 0.0), Point::new(3.0, 4.0, 0.0));
    approx_eq(segment.length(), 5.0);
    assert_eq!(segment.midpoint(), Point::new(1.5, 2.0, 0.0));
}

#[test]
fn segment_closest_point_clamps_to_segment() {
    let segment = Segment::new(Point::new(0.0, 0.0, 0.0), Point::new(10.0, 0.0, 0.0));
    assert_eq!(
        segment.closest_point(Point::new(5.0, 3.0, 0.0)),
        Point::new(5.0, 0.0, 0.0)
    );
    assert_eq!(
        segment.closest_point(Point::new(-1.0, 0.0, 0.0)),
        Point::new(0.0, 0.0, 0.0)
    );
    assert_eq!(
        segment.closest_point(Point::new(15.0, 0.0, 0.0)),
        Point::new(10.0, 0.0, 0.0)
    );
}

#[test]
fn segment_distance_to_point() {
    let segment = Segment::new(Point::new(0.0, 0.0, 0.0), Point::new(10.0, 0.0, 0.0));
    approx_eq(segment.distance_to(Point::new(5.0, 4.0, 3.0)), 5.0);
}
