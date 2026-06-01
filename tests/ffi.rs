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

#[test]
fn ffi_fixed_phase1_types_expose_wire_helpers() {
    let euler = datapod_euler_new(0.1, 0.2, 0.3);
    let mut euler_header = vec![0_u8; datapod_euler_header_size()];
    assert!(datapod_euler_type_hash() != 0);
    assert!(datapod_euler_to_header_bytes(
        euler,
        euler_header.as_mut_ptr(),
        euler_header.len()
    ));
    let mut euler_out = DatapodEuler::default();
    assert!(datapod_euler_from_header_bytes(
        euler_header.as_ptr(),
        euler_header.len(),
        &mut euler_out
    ));
    assert_eq!(euler, euler_out);

    let pose = datapod_pose_new(
        datapod_point_new(1.0, 2.0, 3.0),
        datapod_quaternion_identity(),
    );
    let mut pose_header = vec![0_u8; datapod_pose_header_size()];
    assert!(datapod_pose_type_hash() != 0);
    assert!(datapod_pose_to_header_bytes(
        pose,
        pose_header.as_mut_ptr(),
        pose_header.len()
    ));
    let mut pose_out = DatapodPose::default();
    assert!(datapod_pose_from_header_bytes(
        pose_header.as_ptr(),
        pose_header.len(),
        &mut pose_out
    ));
    assert_eq!(pose, pose_out);

    let triangle = datapod_triangle_new(
        datapod_point_new(0.0, 0.0, 0.0),
        datapod_point_new(1.0, 0.0, 0.0),
        datapod_point_new(0.0, 1.0, 0.0),
    );
    assert_eq!(datapod_triangle_area(triangle), 0.5);

    let ip = datapod_ip_v4(127, 0, 0, 1);
    let mut ip_header = vec![0_u8; datapod_ip_header_size()];
    assert!(datapod_ip_to_header_bytes(
        ip,
        ip_header.as_mut_ptr(),
        ip_header.len()
    ));
    let mut ip_out = DatapodIp::default();
    assert!(datapod_ip_from_header_bytes(
        ip_header.as_ptr(),
        ip_header.len(),
        &mut ip_out
    ));
    assert_eq!(ip, ip_out);
}

#[test]
fn ffi_heap_backed_values_expose_payloads_and_headers() {
    let bytes = [1_u8, 2, 3, 4];
    let value = datapod_bytes_value_new(bytes.as_ptr(), bytes.len());
    assert!(!value.is_null());
    assert!(datapod_bytes_value_type_hash() != 0);
    let mut header = vec![0_u8; datapod_bytes_value_header_size()];
    assert!(datapod_bytes_value_to_header_bytes(
        value,
        header.as_mut_ptr(),
        header.len()
    ));
    let payload = datapod_bytes_value_payload(value);
    assert_eq!(payload.len, bytes.len());
    datapod_bytes_value_free(value);

    let points = [
        datapod_point_new(0.0, 0.0, 0.0),
        datapod_point_new(1.0, 0.0, 0.0),
    ];
    let line = datapod_linestring_new(points.as_ptr(), points.len());
    assert!(!line.is_null());
    assert!(datapod_linestring_type_hash() != 0);
    assert_eq!(
        datapod_linestring_payload(line).len,
        points.len() * std::mem::size_of::<datapod::Point>()
    );
    datapod_linestring_free(line);

    let vector = datapod_vector_from_bytes(1, bytes.as_ptr(), bytes.len());
    assert!(!vector.is_null());
    assert!(datapod_vector_type_hash() != 0);
    assert_eq!(datapod_vector_payload(vector).len, bytes.len());
    datapod_vector_free(vector);
}

#[test]
fn ffi_assoc_entry_values_are_available_as_raw_pods() {
    let entry = datapod_map_entry_new(4, 8, 12, 16);
    assert!(datapod_map_entry_type_hash() != 0);
    assert_eq!(
        datapod_map_entry_byte_size(),
        std::mem::size_of::<datapod::MapEntry>()
    );
    let mut bytes = vec![0_u8; datapod_map_entry_byte_size()];
    assert!(datapod_map_entry_to_bytes(
        entry,
        bytes.as_mut_ptr(),
        bytes.len()
    ));
    let mut out = DatapodMapEntry::default();
    assert!(datapod_map_entry_from_bytes(
        bytes.as_ptr(),
        bytes.len(),
        &mut out
    ));
    assert_eq!(entry, out);

    let set_entry = datapod_set_entry_new(20, 24);
    assert!(datapod_set_entry_type_hash() != 0);
    assert_eq!(
        datapod_set_entry_byte_size(),
        std::mem::size_of::<datapod::SetEntry>()
    );
    let mut set_bytes = vec![0_u8; datapod_set_entry_byte_size()];
    assert!(datapod_set_entry_to_bytes(
        set_entry,
        set_bytes.as_mut_ptr(),
        set_bytes.len()
    ));
    let mut set_out = DatapodSetEntry::default();
    assert!(datapod_set_entry_from_bytes(
        set_bytes.as_ptr(),
        set_bytes.len(),
        &mut set_out
    ));
    assert_eq!(set_entry, set_out);
}

#[test]
fn ffi_remaining_fixed_robot_and_shape_types_expose_wire_helpers() {
    let model = datapod_model_new_default();
    assert!(!model.is_null());
    let mut model_header = vec![0_u8; datapod_model_header_size()];
    assert!(datapod_model_type_hash() != 0);
    assert!(datapod_model_to_header_bytes(
        model,
        model_header.as_mut_ptr(),
        model_header.len()
    ));
    let model_out = datapod_model_from_header_bytes(model_header.as_ptr(), model_header.len());
    assert!(!model_out.is_null());
    datapod_model_free(model);
    datapod_model_free(model_out);

    let joint = datapod_joint_new_default();
    assert!(!joint.is_null());
    let mut joint_header = vec![0_u8; datapod_joint_header_size()];
    assert!(datapod_joint_type_hash() != 0);
    assert!(datapod_joint_to_header_bytes(
        joint,
        joint_header.as_mut_ptr(),
        joint_header.len()
    ));
    datapod_joint_free(joint);

    let geom = datapod_geometry_new_default();
    assert!(!geom.is_null());
    let mut geom_header = vec![0_u8; datapod_geometry_header_size()];
    assert!(datapod_geometry_type_hash() != 0);
    assert!(datapod_geometry_to_header_bytes(
        geom,
        geom_header.as_mut_ptr(),
        geom_header.len()
    ));
    datapod_geometry_free(geom);

    let size = datapod_size_new(1.0, 2.0, 3.0);
    let mut size_header = vec![0_u8; datapod_size_value_header_size()];
    assert!(datapod_size_value_type_hash() != 0);
    assert!(datapod_size_value_to_header_bytes(
        size,
        size_header.as_mut_ptr(),
        size_header.len()
    ));
}
