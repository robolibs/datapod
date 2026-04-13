use datapod::Geo;

fn approx_eq(left: f64, right: f64, epsilon: f64) {
    assert!((left - right).abs() < epsilon, "{left} != {right}");
}

#[test]
fn geo_validation_and_altitude() {
    let geo = Geo::new(37.7749, -122.4194, 16.0);
    assert!(geo.is_valid());
    assert!(geo.has_altitude());
    assert!(geo.is_set());
    assert!(!Geo::new(91.0, 0.0, 0.0).is_valid());
}

#[test]
fn geo_distance_matches_expected_scale() {
    let sf = Geo::new(37.7749, -122.4194, 16.0);
    let ny = Geo::new(40.7128, -74.0060, 10.0);
    approx_eq(sf.distance_to(ny), 4_129_000.0, 50_000.0);
}

#[test]
fn geo_bearing_stays_in_range() {
    let origin = Geo::new(37.0, -122.0, 0.0);
    let east = Geo::new(37.0, -121.0, 0.0);
    let bearing = origin.bearing_to(east);
    assert!((0.0..std::f64::consts::TAU).contains(&bearing));
    approx_eq(bearing, std::f64::consts::FRAC_PI_2, 0.05);
}
