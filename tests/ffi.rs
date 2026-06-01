use datapod::ffi::*;

#[test]
fn ffi_point_geo_segment_values_work() {
    let a = datapod_point_new(0.0, 0.0, 0.0);
    let b = datapod_point_new(3.0, 4.0, 0.0);
    assert_eq!(datapod_point_distance_to(a, b), 5.0);
    assert_eq!(datapod_point_magnitude(b), 5.0);

    let ams = datapod_geo_new(52.0, 4.0, 0.0);
    let bad = datapod_geo_new(100.0, 4.0, 0.0);
    assert!(datapod_geo_is_valid(ams));
    assert!(!datapod_geo_is_valid(bad));

    let segment = datapod_segment_new(a, b);
    assert_eq!(datapod_segment_length(segment), 5.0);
    assert_eq!(
        datapod_segment_midpoint(segment),
        DatapodPoint {
            x: 1.5,
            y: 2.0,
            z: 0.0
        }
    );
}

#[test]
fn ffi_polygon_handle_reports_metrics_and_errors() {
    let vertices = [
        datapod_point_new(0.0, 0.0, 0.0),
        datapod_point_new(2.0, 0.0, 0.0),
        datapod_point_new(2.0, 2.0, 0.0),
        datapod_point_new(0.0, 2.0, 0.0),
    ];
    let polygon = datapod_polygon_new(vertices.as_ptr(), vertices.len());
    assert!(!polygon.is_null());

    assert_eq!(datapod_polygon_len(polygon), 4);
    assert_eq!(datapod_polygon_area(polygon), 4.0);
    assert_eq!(datapod_polygon_perimeter(polygon), 8.0);
    assert!(datapod_polygon_contains(
        polygon,
        datapod_point_new(1.0, 1.0, 0.0)
    ));

    let mut out = DatapodPoint::default();
    assert!(datapod_polygon_vertex(polygon, 2, &mut out));
    assert_eq!(out, vertices[2]);

    assert!(!datapod_polygon_vertex(polygon, 99, &mut out));
    assert!(!datapod_last_error_message().is_null());

    datapod_polygon_free(polygon);
}

#[test]
fn ffi_polygon_constructor_rejects_null_non_empty_array() {
    let polygon = datapod_polygon_new(std::ptr::null(), 1);
    assert!(polygon.is_null());
    assert!(!datapod_last_error_message().is_null());
}
