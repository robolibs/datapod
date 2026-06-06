use datapod::ffi::*;

fn last_error_string() -> Option<String> {
    let ptr = datapod_last_error_message();
    if ptr.is_null() {
        None
    } else {
        Some(
            unsafe { std::ffi::CStr::from_ptr(ptr) }
                .to_str()
                .expect("datapod last error is utf-8")
                .to_string(),
        )
    }
}

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
fn ffi_spatial_query_helpers_clear_stale_last_error_on_success() {
    macro_rules! assert_clears_stale_error {
        ($call:expr, $message:literal $(,)?) => {{
            assert_eq!(datapod_header_size(u64::MAX), 0);
            assert!(last_error_string().is_some());
            let _value = $call;
            assert!(last_error_string().is_none(), $message);
        }};
    }

    let a = datapod_point_new(0.0, 0.0, 0.0);
    let b = datapod_point_new(3.0, 4.0, 0.0);
    let segment = datapod_segment_new(a, b);
    let geo = datapod_geo_new(52.0, 4.0, 0.0);
    let velocity = datapod_velocity_new(1.0, 2.0, 2.0);
    let acceleration = datapod_acceleration_new(2.0, 3.0, 6.0);
    let rectangle = datapod_rectangle_new(a, b, a, b);
    let aabb = datapod_aabb_new(a, b);
    let circle = datapod_circle_new(a, 2.0);
    let triangle = datapod_triangle_new(a, b, datapod_point_new(0.0, 4.0, 0.0));

    assert_clears_stale_error!(
        datapod_point_magnitude(b),
        "point magnitude should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_point_distance_to(a, b),
        "point distance should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_point_distance_to_2d(a, b),
        "point 2d distance should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_geo_is_valid(geo),
        "geo validity query should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_geo_distance_to(geo, geo),
        "geo distance should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_geo_bearing_to(geo, geo),
        "geo bearing should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_segment_length(segment),
        "segment length should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_segment_midpoint(segment),
        "segment midpoint should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_segment_closest_point(segment, b),
        "segment closest-point should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_segment_distance_to(segment, b),
        "segment distance should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_velocity_speed(velocity),
        "velocity speed should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_acceleration_magnitude(acceleration),
        "acceleration magnitude should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_rectangle_area(rectangle),
        "rectangle area should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_aabb_center(aabb),
        "aabb center should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_circle_area(circle),
        "circle area should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_triangle_area(triangle),
        "triangle area should clear stale last-error state"
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
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let vertex_bytes = datapod_polygon_vertices(polygon);
    assert_eq!(
        vertex_bytes.len,
        vertices.len() * std::mem::size_of::<DatapodPoint>()
    );
    assert!(!vertex_bytes.ptr.is_null());
    assert!(
        last_error_string().is_none(),
        "polygon vertex byte view should clear stale last-error state on success"
    );

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
fn ffi_dpstr_constructor_rejects_invalid_utf8_without_lossy_replacement() {
    let invalid = [0xff, 0xfe];
    let dpstr = datapod_dpstr_new(invalid.as_ptr(), invalid.len());
    assert!(dpstr.is_null());
    assert!(!datapod_last_error_message().is_null());
}

#[test]
fn ffi_dpstring_constructor_rejects_invalid_utf8() {
    let invalid = [0xff, 0xfe];
    let dpstring = datapod_dpstring_new(invalid.as_ptr(), invalid.len());
    assert!(dpstring.is_null());
    assert!(!datapod_last_error_message().is_null());
}

#[test]
fn ffi_heap_array_constructors_reject_oversized_lengths_before_slicing() {
    let point_len = usize::MAX / core::mem::size_of::<DatapodPoint>() + 1;
    let point_ptr = std::ptr::NonNull::<DatapodPoint>::dangling().as_ptr();
    let polygon = datapod_polygon_new(point_ptr, point_len);
    assert!(polygon.is_null());
    assert!(!datapod_last_error_message().is_null());

    let pose_len = usize::MAX / core::mem::size_of::<DatapodPose>() + 1;
    let pose_ptr = std::ptr::NonNull::<DatapodPose>::dangling().as_ptr();
    let path = datapod_path_new(pose_ptr, pose_len);
    assert!(path.is_null());
    assert!(!datapod_last_error_message().is_null());

    let state_len = usize::MAX / core::mem::size_of::<DatapodState>() + 1;
    let state_ptr = std::ptr::NonNull::<DatapodState>::dangling().as_ptr();
    let trajectory = datapod_trajectory_new(state_ptr, state_len);
    assert!(trajectory.is_null());
    assert!(!datapod_last_error_message().is_null());
}

#[test]
fn ffi_heap_array_constructors_reject_misaligned_typed_pointers_before_slicing() {
    let point_storage = [DatapodPoint::default(); 2];
    let misaligned_points =
        unsafe { (point_storage.as_ptr() as *const u8).add(1) as *const DatapodPoint };
    let polygon = datapod_polygon_new(misaligned_points, 1);
    assert!(polygon.is_null());
    assert!(!datapod_last_error_message().is_null());

    let pose_storage = [DatapodPose::default(); 2];
    let misaligned_poses =
        unsafe { (pose_storage.as_ptr() as *const u8).add(1) as *const DatapodPose };
    let path = datapod_path_new(misaligned_poses, 1);
    assert!(path.is_null());
    assert!(!datapod_last_error_message().is_null());

    let state_storage = [DatapodState::default(); 2];
    let misaligned_states =
        unsafe { (state_storage.as_ptr() as *const u8).add(1) as *const DatapodState };
    let trajectory = datapod_trajectory_new(misaligned_states, 1);
    assert!(trajectory.is_null());
    assert!(!datapod_last_error_message().is_null());
}

#[test]
fn ffi_slice_helpers_reject_pointer_ranges_that_wrap_address_space() {
    let overflowing_input = usize::MAX as *const u8;
    let mac = datapod_mac_addr_new(overflowing_input, 6);
    assert_eq!(mac, DatapodMacAddr::default());
    assert!(
        last_error_string()
            .expect("overflowing input pointer range must set last error")
            .contains("overflows address space")
    );

    let overflowing_output = usize::MAX as *mut u8;
    assert!(!datapod_point_to_header_bytes(
        datapod_point_new(1.0, 2.0, 3.0),
        overflowing_output,
        1,
    ));
    assert!(
        last_error_string()
            .expect("overflowing output pointer range must set last error")
            .contains("overflows address space")
    );

    let overflowing_points = usize::MAX as *const DatapodPoint;
    let polygon = datapod_polygon_new(overflowing_points, 1);
    assert!(polygon.is_null());
    assert!(
        last_error_string()
            .expect("overflowing typed pointer range must set last error")
            .contains("overflows address space")
    );

    let point_hash = datapod_point_type_hash();
    let point_header_len = datapod_point_header_size();
    let overflowing_message =
        datapod_wire_message_borrow(point_hash, overflowing_input, point_header_len);
    assert!(!datapod_wire_message_validate_v1(overflowing_message));
    assert!(
        last_error_string()
            .expect("overflowing wire-message pointer range must set last error")
            .contains("overflows address space")
    );

    let overflowing_header_frame = datapod_wire_frame_borrow(
        point_hash,
        overflowing_input,
        point_header_len,
        std::ptr::null(),
        0,
    );
    assert!(!datapod_wire_frame_validate_v1(overflowing_header_frame));
    assert!(
        last_error_string()
            .expect("overflowing wire-frame header pointer range must set last error")
            .contains("overflows address space")
    );

    let bytes_hash = datapod_emitted_type_hash(datapod_bytes_value_type_hash());
    let overflowing_payload_frame =
        datapod_wire_frame_borrow(bytes_hash, std::ptr::null(), 0, overflowing_input, 1);
    assert!(!datapod_wire_frame_validate_v1(overflowing_payload_frame));
    assert!(
        last_error_string()
            .expect("overflowing wire-frame payload pointer range must set last error")
            .contains("overflows address space")
    );

    let mut overflowing_join = DatapodOwnedBytes {
        ptr: std::ptr::NonNull::<u8>::dangling().as_ptr(),
        len: 11,
        capacity: 11,
    };
    assert!(!datapod_wire_message_join_v1(
        point_hash,
        overflowing_input,
        point_header_len,
        std::ptr::null(),
        0,
        &mut overflowing_join,
    ));
    assert!(
        overflowing_join.ptr.is_null()
            && overflowing_join.len == 0
            && overflowing_join.capacity == 0,
        "failed join with overflowing header pointer must clear stale owned-byte output"
    );
    assert!(
        last_error_string()
            .expect("overflowing join header pointer range must set last error")
            .contains("overflows address space")
    );

    let mut overflowing_payload_join = DatapodOwnedBytes {
        ptr: std::ptr::NonNull::<u8>::dangling().as_ptr(),
        len: 12,
        capacity: 12,
    };
    assert!(!datapod_wire_message_join(
        bytes_hash,
        std::ptr::null(),
        0,
        overflowing_input,
        1,
        &mut overflowing_payload_join,
    ));
    assert!(
        overflowing_payload_join.ptr.is_null()
            && overflowing_payload_join.len == 0
            && overflowing_payload_join.capacity == 0,
        "failed join with overflowing payload pointer must clear stale owned-byte output"
    );
    assert!(
        last_error_string()
            .expect("overflowing join payload pointer range must set last error")
            .contains("overflows address space")
    );
}

#[test]
fn ffi_owned_bytes_free_rejects_impossible_length_capacity_without_dropping() {
    let invalid = DatapodOwnedBytes {
        ptr: std::ptr::NonNull::<u8>::dangling().as_ptr(),
        len: 1,
        capacity: 0,
    };
    datapod_owned_bytes_free(invalid);
    assert!(!datapod_last_error_message().is_null());

    let null_non_empty = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 1,
        capacity: 1,
    };
    datapod_owned_bytes_free(null_non_empty);
    assert!(!datapod_last_error_message().is_null());

    let overflowing_range = DatapodOwnedBytes {
        ptr: usize::MAX as *mut u8,
        len: 1,
        capacity: 1,
    };
    datapod_owned_bytes_free(overflowing_range);
    assert!(
        last_error_string()
            .expect("overflowing owned bytes allocation must set last error")
            .contains("overflows address space")
    );

    let empty = DatapodOwnedBytes {
        ptr: std::ptr::NonNull::<u8>::dangling().as_ptr(),
        len: 0,
        capacity: 0,
    };
    datapod_owned_bytes_free(empty);
    assert!(datapod_last_error_message().is_null());
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
    assert!(!datapod_ip_from_header_bytes(
        ip_header.as_ptr(),
        ip_header.len().saturating_sub(1),
        &mut ip_out
    ));
    assert_eq!(
        ip_out,
        DatapodIp::default(),
        "failed fixed header decode must clear stale output values"
    );
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
    let mut vector_short_header = vec![0xaa_u8; datapod_vector_header_size().saturating_sub(1)];
    assert!(!datapod_vector_to_header_bytes(
        vector,
        vector_short_header.as_mut_ptr(),
        vector_short_header.len()
    ));
    assert!(
        vector_short_header.iter().all(|byte| *byte == 0),
        "failed heap header write must clear caller output bytes"
    );
    datapod_vector_free(vector);

    let invalid_vector = datapod_vector_from_bytes(2, bytes.as_ptr(), bytes.len() - 1);
    assert!(invalid_vector.is_null());
    assert!(!datapod_last_error_message().is_null());

    let invalid_matrix = datapod_matrix_from_bytes(2, 2, 1, bytes.as_ptr(), bytes.len() - 1);
    assert!(invalid_matrix.is_null());
    assert!(!datapod_last_error_message().is_null());
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
    let mut short_bytes = vec![0xaa_u8; datapod_map_entry_byte_size().saturating_sub(1)];
    assert!(!datapod_map_entry_to_bytes(
        entry,
        short_bytes.as_mut_ptr(),
        short_bytes.len()
    ));
    assert!(
        short_bytes.iter().all(|byte| *byte == 0),
        "failed raw POD write must clear caller output bytes"
    );
    let mut out = DatapodMapEntry::default();
    assert!(datapod_map_entry_from_bytes(
        bytes.as_ptr(),
        bytes.len(),
        &mut out
    ));
    assert_eq!(entry, out);
    let mut trailing_map_entry = bytes.clone();
    trailing_map_entry.push(0);
    assert!(!datapod_map_entry_from_bytes(
        trailing_map_entry.as_ptr(),
        trailing_map_entry.len(),
        &mut out
    ));
    assert_eq!(
        out,
        DatapodMapEntry::default(),
        "failed raw POD decode must clear stale output values"
    );
    assert!(!datapod_last_error_message().is_null());
    assert!(!datapod_map_entry_from_bytes(
        bytes.as_ptr(),
        bytes.len().saturating_sub(1),
        &mut out
    ));
    assert!(!datapod_last_error_message().is_null());
    assert!(!datapod_map_entry_from_bytes(
        bytes.as_ptr(),
        bytes.len(),
        std::ptr::null_mut()
    ));
    assert!(!datapod_last_error_message().is_null());

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
    let mut trailing_set_entry = set_bytes.clone();
    trailing_set_entry.push(0);
    assert!(!datapod_set_entry_from_bytes(
        trailing_set_entry.as_ptr(),
        trailing_set_entry.len(),
        &mut set_out
    ));
    assert_eq!(
        set_out,
        DatapodSetEntry::default(),
        "failed raw POD decode must clear stale output values"
    );
    assert!(!datapod_last_error_message().is_null());
}

#[test]
fn ffi_generic_registry_reports_exported_type_metadata() {
    let point_hash = datapod_point_type_hash();
    let point_canonical_hash = datapod::bind::type_hash_canonical::<datapod::Point>().unwrap();
    assert_eq!(point_hash, point_canonical_hash);
    assert!(datapod_type_exists(point_hash));
    assert!(datapod_type_exists(point_canonical_hash));
    assert_eq!(
        datapod_canonical_type_hash(point_hash),
        point_canonical_hash
    );
    assert_eq!(
        datapod_canonical_type_hash(point_canonical_hash),
        point_canonical_hash
    );
    assert_eq!(datapod_emitted_type_hash(point_hash), point_canonical_hash);
    assert_eq!(
        datapod_emitted_hash_kind(point_hash),
        datapod_hash_kind_canonical_name()
    );
    let current_format = unsafe { std::ffi::CStr::from_ptr(datapod_current_wire_format_name()) };
    assert_eq!(current_format.to_str().unwrap(), "datapod-wire-v1/le");
    let hash_policy = unsafe { std::ffi::CStr::from_ptr(datapod_builtin_hash_policy()) };
    assert!(hash_policy.to_str().unwrap().contains("canonical-name"));
    assert_eq!(datapod_header_size(point_hash), datapod_point_header_size());
    assert_eq!(
        datapod_header_size(point_canonical_hash),
        datapod_point_header_size()
    );
    assert_eq!(
        datapod_header_size_v1(point_canonical_hash),
        datapod_point_header_size()
    );
    assert_eq!(
        datapod_header_size_v1(point_hash),
        datapod_point_header_size()
    );
    assert_eq!(datapod_payload_kind(point_hash), 0);
    assert_eq!(datapod_format_version(point_hash), 1);
    assert_eq!(datapod_wire_format(point_hash), 1);
    assert_eq!(datapod_endian(point_hash), datapod_endian_little());
    assert_eq!(
        datapod_alignment_policy(point_hash),
        datapod_alignment_unaligned_wire()
    );
    assert_eq!(
        datapod_validator_kind(point_hash),
        datapod_validator_builtin()
    );
    assert!(datapod_has_archive(point_hash));
    assert!(datapod_has_view(point_hash));
    assert!(datapod_has_owned_decode(point_hash));
    assert_eq!(
        datapod_archive_shape(point_hash),
        datapod_archive_shape_fixed()
    );
    let name = datapod_type_name(point_hash);
    assert!(!name.is_null());
    let name = unsafe { std::ffi::CStr::from_ptr(name) }
        .to_str()
        .expect("registry names are utf-8");
    assert_eq!(name, "datapod.point.v1");
    assert_eq!(datapod_payload_kind(datapod_grid_type_hash()), 1);
    assert_eq!(
        datapod_archive_shape(datapod_grid_type_hash()),
        datapod_archive_shape_single_payload()
    );

    assert!(!datapod_type_exists(u64::MAX));
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert_eq!(datapod_header_size_v1(u64::MAX), 0);
    assert_eq!(datapod_canonical_type_hash(u64::MAX), 0);
    assert_eq!(datapod_payload_kind(u64::MAX), u32::MAX);
    assert_eq!(datapod_format_version(u64::MAX), u32::MAX);
    assert_eq!(datapod_wire_format(u64::MAX), u32::MAX);
    assert_eq!(datapod_emitted_type_hash(u64::MAX), 0);
    assert_eq!(datapod_emitted_hash_kind(u64::MAX), u32::MAX);
    assert_eq!(datapod_endian(u64::MAX), u32::MAX);
    assert_eq!(datapod_alignment_policy(u64::MAX), u32::MAX);
    assert_eq!(datapod_validator_kind(u64::MAX), u32::MAX);
    assert!(!datapod_has_archive(u64::MAX));
    assert!(!datapod_has_view(u64::MAX));
    assert!(!datapod_has_owned_decode(u64::MAX));
    assert_eq!(datapod_archive_shape(u64::MAX), u32::MAX);
    assert!(datapod_type_name(u64::MAX).is_null());

    let zero_header_name = std::ffi::CString::new("acme.empty_runtime_ffi.v1").unwrap();
    let zero_header_hash =
        datapod_register_type_name(zero_header_name.as_ptr(), 0, datapod_payload_kind_bytes());
    assert_ne!(zero_header_hash, 0);
    let zero_header_message = datapod_wire_message_borrow(zero_header_hash, std::ptr::null(), 0);
    assert!(datapod_wire_message_validate_v1(zero_header_message));
    let zero_header = datapod_wire_message_header_v1(zero_header_message);
    let zero_payload = datapod_wire_message_payload_v1(zero_header_message);
    assert_eq!(zero_header.len, 0);
    assert_eq!(zero_payload.len, 0);
    assert!(zero_header.ptr.is_null());
    assert!(zero_payload.ptr.is_null());

    let huge_header_name = std::ffi::CString::new("acme.huge_runtime_ffi.v1").unwrap();
    assert_eq!(
        datapod_register_type_name(
            huge_header_name.as_ptr(),
            isize::MAX as usize + 1,
            datapod_payload_kind_bytes()
        ),
        0
    );
    assert!(!datapod_last_error_message().is_null());
    let huge_direct_hash = datapod_type_hash_name(huge_header_name.as_ptr());
    assert!(!datapod_register_type(
        huge_direct_hash,
        huge_header_name.as_ptr(),
        isize::MAX as usize + 1,
        datapod_payload_kind_bytes(),
    ));
    assert!(
        last_error_string()
            .expect("direct huge-header registration must set last error")
            .contains("header size")
    );
    assert!(
        !datapod_type_exists_name(huge_header_name.as_ptr()),
        "huge-header C ABI runtime canonical name must not be registered"
    );

    let invalid_kind_name = std::ffi::CString::new("acme.bad_payload_kind_ffi.v1").unwrap();
    assert_eq!(
        datapod_register_type_name(invalid_kind_name.as_ptr(), 0, u32::MAX),
        0
    );
    assert!(
        last_error_string()
            .expect("invalid payload-kind registration must set last error")
            .contains("payload kind")
    );
    let invalid_kind_hash = datapod_type_hash_name(invalid_kind_name.as_ptr());
    assert!(!datapod_register_type(
        invalid_kind_hash,
        invalid_kind_name.as_ptr(),
        0,
        u32::MAX,
    ));
    assert!(
        last_error_string()
            .expect("direct invalid payload-kind registration must set last error")
            .contains("payload kind")
    );
    let mismatch_name = std::ffi::CString::new("acme.bad_hash_ffi.v1").unwrap();
    let mismatch_expected_hash = datapod_type_hash_name(mismatch_name.as_ptr());
    let mismatch_wrong_hash = mismatch_expected_hash ^ 0x55aa_55aa_55aa_55aa;
    assert!(!datapod_register_type(
        mismatch_wrong_hash,
        mismatch_name.as_ptr(),
        0,
        datapod_payload_kind_bytes(),
    ));
    assert!(
        last_error_string()
            .expect("direct mismatched hash registration must set last error")
            .contains("provided hash")
    );
    assert!(
        !datapod_type_exists(mismatch_wrong_hash),
        "mismatched C ABI runtime type hash must not be registered"
    );
    assert!(
        !datapod_type_exists_name(mismatch_name.as_ptr()),
        "mismatched C ABI runtime canonical name must not be registered"
    );
    let zero_hash_name = std::ffi::CString::new("acme.zero_hash_ffi.v1").unwrap();
    assert!(!datapod_register_type(
        0,
        zero_hash_name.as_ptr(),
        0,
        datapod_payload_kind_bytes(),
    ));
    assert!(
        last_error_string()
            .expect("direct zero-hash registration must set last error")
            .contains("non-zero")
    );
    assert!(
        !datapod_type_exists_name(zero_hash_name.as_ptr()),
        "zero-hash C ABI runtime canonical name must not be registered"
    );

    let mut point_v1_bytes = Vec::new();
    point_v1_bytes.extend_from_slice(&1.0f64.to_bits().to_le_bytes());
    point_v1_bytes.extend_from_slice(&2.0f64.to_bits().to_le_bytes());
    point_v1_bytes.extend_from_slice(&3.0f64.to_bits().to_le_bytes());
    let point_v1_message = datapod_wire_message_borrow(
        point_canonical_hash,
        point_v1_bytes.as_ptr(),
        point_v1_bytes.len(),
    );
    assert!(datapod_wire_message_validate_v1(point_v1_message));
    assert!(datapod_wire_message_is_valid_v1(point_v1_message));
    assert_eq!(
        datapod_wire_message_header_v1(point_v1_message).len,
        datapod_header_size_v1(point_canonical_hash)
    );
    assert_eq!(datapod_wire_message_payload_v1(point_v1_message).len, 0);
    let mut joined_v1 = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(datapod_wire_message_join_v1(
        point_canonical_hash,
        point_v1_bytes.as_ptr(),
        point_v1_bytes.len(),
        std::ptr::null(),
        0,
        &mut joined_v1,
    ));
    assert_eq!(joined_v1.len, point_v1_bytes.len());
    datapod_owned_bytes_free(joined_v1);
    let wrong_v1_message = datapod_wire_message_borrow(
        datapod::bind::rust_type_hash::<datapod::Point>(),
        point_v1_bytes.as_ptr(),
        point_v1_bytes.len(),
    );
    assert!(!datapod_wire_message_validate_v1(wrong_v1_message));
}

#[test]
fn ffi_metadata_queries_clear_success_and_report_unknown_hashes() {
    let unknown_hash = u64::MAX;
    let point_hash = datapod_point_type_hash();

    assert_eq!(datapod_header_size(unknown_hash), 0);
    assert!(
        last_error_string()
            .expect("unknown metadata lookup must set last error")
            .contains("unknown datapod type hash")
    );

    assert!(datapod_type_exists(point_hash));
    assert!(
        last_error_string().is_none(),
        "successful type existence query must clear stale metadata errors"
    );

    assert!(!datapod_type_exists(unknown_hash));
    assert!(
        last_error_string().is_none(),
        "type existence checks report absence via bool, not stale last-error state"
    );

    assert_eq!(datapod_header_size(point_hash), datapod_point_header_size());
    assert!(
        last_error_string().is_none(),
        "successful metadata lookup must leave last-error empty"
    );

    macro_rules! assert_unknown_metadata {
        ($expr:expr, $expected:expr $(,)?) => {{
            assert_eq!($expr, $expected);
            assert!(
                last_error_string()
                    .expect("unknown metadata lookup must set last error")
                    .contains("unknown datapod type hash")
            );
        }};
    }

    assert_unknown_metadata!(datapod_canonical_type_hash(unknown_hash), 0);
    assert_unknown_metadata!(datapod_emitted_type_hash(unknown_hash), 0);
    assert_unknown_metadata!(datapod_header_size(unknown_hash), 0);
    assert_unknown_metadata!(datapod_header_size_v1(unknown_hash), 0);
    assert_unknown_metadata!(datapod_payload_kind(unknown_hash), u32::MAX);
    assert_unknown_metadata!(datapod_format_version(unknown_hash), u32::MAX);
    assert_unknown_metadata!(datapod_wire_format(unknown_hash), u32::MAX);
    assert_unknown_metadata!(datapod_emitted_hash_kind(unknown_hash), u32::MAX);
    assert_unknown_metadata!(datapod_endian(unknown_hash), u32::MAX);
    assert_unknown_metadata!(datapod_alignment_policy(unknown_hash), u32::MAX);
    assert_unknown_metadata!(datapod_validator_kind(unknown_hash), u32::MAX);
    assert_unknown_metadata!(datapod_has_archive(unknown_hash), false);
    assert_unknown_metadata!(datapod_has_view(unknown_hash), false);
    assert_unknown_metadata!(datapod_has_owned_decode(unknown_hash), false);
    assert_unknown_metadata!(datapod_archive_shape(unknown_hash), u32::MAX);
    assert_unknown_metadata!(datapod_type_name(unknown_hash), std::ptr::null());
}

#[test]
fn ffi_canonical_name_inputs_reject_empty_and_invalid_utf8() {
    assert_eq!(datapod_type_hash_name(std::ptr::null()), 0);
    assert!(
        last_error_string()
            .expect("null canonical type hash lookup must set last error")
            .contains("null")
    );
    assert!(!datapod_type_exists_name(std::ptr::null()));
    assert!(
        last_error_string()
            .expect("null canonical type existence lookup must set last error")
            .contains("null")
    );
    assert_eq!(
        datapod_register_type_name(std::ptr::null(), 0, datapod_payload_kind_bytes()),
        0
    );
    assert!(
        last_error_string()
            .expect("null canonical type registration must set last error")
            .contains("null")
    );
    assert!(!datapod_register_type(
        1,
        std::ptr::null(),
        0,
        datapod_payload_kind_bytes(),
    ));
    assert!(
        last_error_string()
            .expect("direct null canonical type registration must set last error")
            .contains("null")
    );

    let empty = std::ffi::CString::new("").expect("empty CString is valid");
    assert_eq!(datapod_type_hash_name(empty.as_ptr()), 0);
    assert!(
        last_error_string()
            .expect("empty canonical type hash lookup must set last error")
            .contains("empty")
    );
    assert!(!datapod_type_exists_name(empty.as_ptr()));
    assert!(
        last_error_string()
            .expect("empty canonical type existence lookup must set last error")
            .contains("empty")
    );
    assert_eq!(
        datapod_register_type_name(empty.as_ptr(), 0, datapod_payload_kind_bytes()),
        0
    );
    assert!(
        last_error_string()
            .expect("empty canonical type registration must set last error")
            .contains("empty")
    );
    assert!(!datapod_register_type(
        1,
        empty.as_ptr(),
        0,
        datapod_payload_kind_bytes(),
    ));
    assert!(
        last_error_string()
            .expect("direct empty canonical type registration must set last error")
            .contains("empty")
    );

    let malformed = std::ffi::CString::new("Acme.bad-name.v1").unwrap();
    assert_eq!(datapod_type_hash_name(malformed.as_ptr()), 0);
    assert!(
        last_error_string()
            .expect("malformed canonical type hash lookup must set last error")
            .contains("ASCII lowercase")
    );
    assert!(!datapod_type_exists_name(malformed.as_ptr()));
    assert!(
        last_error_string()
            .expect("malformed canonical type existence lookup must set last error")
            .contains("ASCII lowercase")
    );
    assert_eq!(
        datapod_register_type_name(malformed.as_ptr(), 0, datapod_payload_kind_bytes()),
        0
    );
    assert!(
        last_error_string()
            .expect("malformed canonical type registration must set last error")
            .contains("ASCII lowercase")
    );
    assert!(!datapod_register_type(
        1,
        malformed.as_ptr(),
        0,
        datapod_payload_kind_bytes(),
    ));
    assert!(
        last_error_string()
            .expect("direct malformed canonical type registration must set last error")
            .contains("ASCII lowercase")
    );

    let invalid_utf8 =
        std::ffi::CString::from_vec_with_nul(vec![0xff, 0]).expect("explicitly NUL-terminated");
    assert_eq!(datapod_type_hash_name(invalid_utf8.as_ptr()), 0);
    assert!(
        last_error_string()
            .expect("invalid utf-8 canonical type hash lookup must set last error")
            .contains("not utf-8")
    );
    assert!(!datapod_type_exists_name(invalid_utf8.as_ptr()));
    assert!(
        last_error_string()
            .expect("invalid utf-8 canonical type existence lookup must set last error")
            .contains("not utf-8")
    );
    assert_eq!(
        datapod_register_type_name(invalid_utf8.as_ptr(), 0, datapod_payload_kind_bytes()),
        0
    );
    assert!(
        last_error_string()
            .expect("invalid utf-8 canonical type registration must set last error")
            .contains("not utf-8")
    );
    assert!(!datapod_register_type(
        1,
        invalid_utf8.as_ptr(),
        0,
        datapod_payload_kind_bytes(),
    ));
    assert!(
        last_error_string()
            .expect("direct invalid utf-8 canonical type registration must set last error")
            .contains("not utf-8")
    );
}

#[test]
fn ffi_canonical_name_byte_slice_inputs_are_bounded_and_validated() {
    assert_eq!(datapod_type_hash_name_bytes(std::ptr::null(), 1), 0);
    assert!(
        last_error_string()
            .expect("null non-empty byte-slice canonical type hash lookup must set last error")
            .contains("null")
    );
    assert_eq!(datapod_type_hash_name_bytes(std::ptr::null(), 0), 0);
    assert!(
        last_error_string()
            .expect("empty byte-slice canonical type hash lookup must set last error")
            .contains("empty")
    );

    let invalid_utf8 = [0xff_u8];
    assert_eq!(
        datapod_type_hash_name_bytes(invalid_utf8.as_ptr(), invalid_utf8.len()),
        0
    );
    assert!(
        last_error_string()
            .expect("invalid utf-8 byte-slice canonical type hash lookup must set last error")
            .contains("not utf-8")
    );

    let nul_name = b"acme\0bad.v1";
    assert_eq!(
        datapod_type_hash_name_bytes(nul_name.as_ptr(), nul_name.len()),
        0
    );
    assert!(
        last_error_string()
            .expect("NUL byte-slice canonical type hash lookup must set last error")
            .contains("NUL")
    );
    assert!(!datapod_type_exists_name_bytes(
        nul_name.as_ptr(),
        nul_name.len()
    ));
    assert!(
        last_error_string()
            .expect("NUL byte-slice canonical type existence lookup must set last error")
            .contains("NUL")
    );

    let malformed_name = b"acme.bad-name.v1";
    assert_eq!(
        datapod_type_hash_name_bytes(malformed_name.as_ptr(), malformed_name.len()),
        0
    );
    assert!(
        last_error_string()
            .expect("malformed byte-slice canonical type hash lookup must set last error")
            .contains("ASCII lowercase")
    );
    assert!(!datapod_type_exists_name_bytes(
        malformed_name.as_ptr(),
        malformed_name.len()
    ));
    assert!(
        last_error_string()
            .expect("malformed byte-slice canonical type existence lookup must set last error")
            .contains("ASCII lowercase")
    );

    let name = b"acme.byte_slice_runtime_ffi.v1";
    let hash = datapod_type_hash_name_bytes(name.as_ptr(), name.len());
    assert_ne!(hash, 0);
    assert!(last_error_string().is_none());
    assert!(datapod_register_type_bytes(
        hash,
        name.as_ptr(),
        name.len(),
        0,
        datapod_payload_kind_bytes(),
    ));
    assert!(last_error_string().is_none());
    assert!(datapod_type_exists_name_bytes(name.as_ptr(), name.len()));
    assert!(last_error_string().is_none());

    let named = b"acme.byte_slice_named_runtime_ffi.v1";
    let named_hash = datapod_register_type_name_bytes(
        named.as_ptr(),
        named.len(),
        0,
        datapod_payload_kind_bytes(),
    );
    assert_ne!(named_hash, 0);
    assert!(last_error_string().is_none());
    assert!(datapod_type_exists_name_bytes(named.as_ptr(), named.len()));
}

#[test]
fn ffi_is_valid_helpers_clear_stale_error_on_success_and_report_invalid_inputs() {
    let mut point_header = Vec::new();
    point_header.extend_from_slice(&1.0f64.to_bits().to_le_bytes());
    point_header.extend_from_slice(&2.0f64.to_bits().to_le_bytes());
    point_header.extend_from_slice(&3.0f64.to_bits().to_le_bytes());
    let point_hash = datapod_point_type_hash();
    assert_eq!(point_header.len(), datapod_point_header_size());

    let message =
        datapod_wire_message_borrow(point_hash, point_header.as_ptr(), point_header.len());
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    assert!(datapod_wire_message_is_valid(message));
    assert!(
        last_error_string().is_none(),
        "successful message is_valid should clear stale last-error state"
    );

    let short_message =
        datapod_wire_message_borrow(point_hash, point_header.as_ptr(), point_header.len() - 1);
    assert!(!datapod_wire_message_is_valid(short_message));
    assert!(
        last_error_string()
            .expect("invalid message is_valid should report a validation reason")
            .contains("short"),
        "invalid message is_valid should leave a useful validation error"
    );

    let frame = datapod_wire_frame_borrow(
        point_hash,
        point_header.as_ptr(),
        point_header.len(),
        std::ptr::null(),
        0,
    );
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    assert!(datapod_wire_frame_is_valid(frame));
    assert!(
        last_error_string().is_none(),
        "successful frame is_valid should clear stale last-error state"
    );

    let invalid_frame = datapod_wire_frame_borrow(
        point_hash,
        point_header.as_ptr(),
        point_header.len(),
        point_header.as_ptr(),
        1,
    );
    assert!(!datapod_wire_frame_is_valid(invalid_frame));
    assert!(
        last_error_string()
            .expect("invalid frame is_valid should report a validation reason")
            .contains("fixed-size"),
        "invalid frame is_valid should leave a useful validation error"
    );

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    assert!(datapod_archive_is_valid(frame));
    assert!(
        last_error_string().is_none(),
        "successful archive is_valid should clear stale last-error state"
    );

    assert!(!datapod_archive_is_valid(invalid_frame));
    assert!(
        last_error_string()
            .expect("invalid archive is_valid should report a validation reason")
            .contains("fixed-size"),
        "invalid archive is_valid should leave a useful validation error"
    );
}

#[test]
fn ffi_wire_message_views_reject_invalid_shapes_without_borrowing() {
    let point_hash = datapod_point_type_hash();
    let header_len = datapod_point_header_size();
    let good_header = vec![0_u8; header_len];

    let null_non_empty = datapod_wire_message_borrow(point_hash, std::ptr::null(), header_len);
    let header = datapod_wire_message_header(null_non_empty);
    assert_eq!(
        header,
        DatapodBytes {
            ptr: std::ptr::null(),
            len: 0
        },
        "header view must be empty when validation rejects a null non-empty message"
    );
    assert!(
        last_error_string()
            .expect("null non-empty message should set last error")
            .contains("null input byte array"),
        "null non-empty message should report the pointer-shape error"
    );

    let short = datapod_wire_message_borrow(point_hash, good_header.as_ptr(), header_len - 1);
    let payload = datapod_wire_message_payload(short);
    assert_eq!(
        payload,
        DatapodBytes {
            ptr: std::ptr::null(),
            len: 0
        },
        "payload view must be empty when validation rejects a short message"
    );
    assert!(
        last_error_string()
            .expect("short message should set last error")
            .contains("too short"),
        "short message should report the shape error before deriving payload pointers"
    );

    let fixed_with_payload_bytes = vec![0_u8; header_len.saturating_add(1)];
    let fixed_with_payload = datapod_wire_message_borrow(
        point_hash,
        fixed_with_payload_bytes.as_ptr(),
        fixed_with_payload_bytes.len(),
    );
    let header = datapod_wire_message_header_v1(fixed_with_payload);
    assert_eq!(
        header,
        DatapodBytes {
            ptr: std::ptr::null(),
            len: 0
        },
        "v1 header view must be empty when fixed-size messages carry payload bytes"
    );
    assert!(
        last_error_string()
            .expect("fixed-size message with payload should set last error")
            .contains("fixed-size"),
        "fixed-size message with payload should preserve the validation reason"
    );

    let unknown = datapod_wire_message_borrow(u64::MAX, good_header.as_ptr(), good_header.len());
    let payload = datapod_wire_message_payload_v1(unknown);
    assert_eq!(
        payload,
        DatapodBytes {
            ptr: std::ptr::null(),
            len: 0
        },
        "v1 payload view must be empty for unknown type hashes"
    );
    assert!(
        last_error_string()
            .expect("unknown type hash should set last error")
            .contains("unknown datapod type hash"),
        "unknown type hashes should leave a useful validation error"
    );
}

#[test]
fn ffi_metadata_constant_helpers_clear_stale_last_error() {
    macro_rules! assert_clears_stale_error {
        ($call:expr, $message:literal $(,)?) => {{
            assert_eq!(datapod_header_size(u64::MAX), 0);
            assert!(last_error_string().is_some());
            let _ = $call;
            assert!(last_error_string().is_none(), $message);
        }};
    }

    assert_clears_stale_error!(
        datapod_hash_kind_canonical_name(),
        "hash-kind constant helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_current_wire_format_name(),
        "wire-format name helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_builtin_hash_policy(),
        "hash-policy helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_payload_kind_fixed(),
        "fixed payload-kind helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_payload_kind_bytes(),
        "bytes payload-kind helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_endian_little(),
        "endian helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_alignment_unaligned_wire(),
        "alignment helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_validator_registry_only(),
        "registry-only validator helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_validator_builtin(),
        "builtin validator helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_validator_runtime_schema(),
        "runtime-schema validator helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_archive_shape_fixed(),
        "fixed archive-shape helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_archive_shape_single_payload(),
        "single-payload archive-shape helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_archive_shape_segmented_payload(),
        "segmented-payload archive-shape helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_archive_shape_runtime_schema(),
        "runtime-schema archive-shape helper should clear stale last-error state"
    );
}

#[test]
fn ffi_type_hash_and_size_helpers_clear_stale_last_error() {
    macro_rules! assert_clears_stale_error {
        ($call:expr, $message:literal $(,)?) => {{
            assert_eq!(datapod_header_size(u64::MAX), 0);
            assert!(last_error_string().is_some());
            let _value = $call;
            assert!(last_error_string().is_none(), $message);
        }};
    }

    assert_clears_stale_error!(
        datapod_point_type_hash(),
        "fixed-wire macro type-hash helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_point_header_size(),
        "fixed-wire macro header-size helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_geo_type_hash(),
        "hand-written spatial type-hash helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_geo_header_size(),
        "hand-written spatial header-size helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_mac_addr_type_hash(),
        "hand-written identifier type-hash helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_mac_addr_header_size(),
        "hand-written identifier header-size helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_dpstr_type_hash(),
        "hand-written byte-payload type-hash helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_dpstr_header_size(),
        "hand-written byte-payload header-size helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_tensor_type_hash(),
        "hand-written sequence type-hash helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_tensor_header_size(),
        "hand-written sequence header-size helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_point_key_type_hash(),
        "fixed handle type-hash helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_point_key_header_size(),
        "fixed handle header-size helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_robot_type_hash(),
        "robot-family type-hash helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_robot_header_size(),
        "robot-family header-size helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_envelope_type_hash(),
        "wire-envelope type-hash helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_envelope_header_size(),
        "wire-envelope header-size helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_size_value_type_hash(),
        "hand-written fixed alias type-hash helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_size_value_header_size(),
        "hand-written fixed alias header-size helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_map_entry_type_hash(),
        "raw POD type-hash helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_map_entry_byte_size(),
        "raw POD byte-size helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_polygon_type_hash(),
        "representative heap type-hash helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_polygon_header_size(),
        "representative heap header-size helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_grid_type_hash(),
        "representative shaped heap type-hash helper should clear stale last-error state"
    );
    assert_clears_stale_error!(
        datapod_grid_header_size(),
        "representative shaped heap header-size helper should clear stale last-error state"
    );
}

#[test]
fn ffi_representative_free_functions_clear_stale_errors_on_null() {
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    datapod_polygon_free(std::ptr::null_mut());
    assert!(
        last_error_string().is_none(),
        "polygon free(NULL) should clear stale last-error state"
    );

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    datapod_bytes_value_free(std::ptr::null_mut());
    assert!(
        last_error_string().is_none(),
        "bytes value free(NULL) should clear stale last-error state"
    );

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    datapod_point_key_free(std::ptr::null_mut());
    assert!(
        last_error_string().is_none(),
        "fixed-value handle free(NULL) should clear stale last-error state"
    );
}

#[test]
fn ffi_constructors_clear_stale_last_error_on_success() {
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let point = datapod_point_new(1.0, 2.0, 3.0);
    assert!(
        last_error_string().is_none(),
        "fixed-value constructors should clear stale last-error state on success"
    );

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let _identity = datapod_quaternion_identity();
    assert!(
        last_error_string().is_none(),
        "identity constructors should clear stale last-error state on success"
    );

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let _uuid = datapod_uuid_nil();
    assert!(
        last_error_string().is_none(),
        "nil constructors should clear stale last-error state on success"
    );

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let _ip = datapod_ip_v4(127, 0, 0, 1);
    assert!(
        last_error_string().is_none(),
        "IP constructors should clear stale last-error state on success"
    );

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let entry = datapod_map_entry_new(0, 1, 2, 3);
    assert_eq!(entry.key_len, 1);
    assert!(
        last_error_string().is_none(),
        "raw POD helper constructors should clear stale last-error state on success"
    );

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let set_entry = datapod_set_entry_new(4, 5);
    assert_eq!(set_entry.key_len, 5);
    assert!(
        last_error_string().is_none(),
        "set entry constructor should clear stale last-error state on success"
    );

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let map = datapod_map_new();
    assert!(!map.is_null());
    assert!(
        last_error_string().is_none(),
        "heap constructors should clear stale last-error state on success"
    );
    datapod_map_free(map);

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let set = datapod_set_new();
    assert!(!set.is_null());
    assert!(
        last_error_string().is_none(),
        "set constructor should clear stale last-error state on success"
    );
    datapod_set_free(set);

    let vertices = [
        point,
        datapod_point_new(2.0, 2.0, 3.0),
        datapod_point_new(1.0, 3.0, 3.0),
    ];
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let polygon = datapod_polygon_new(vertices.as_ptr(), vertices.len());
    assert!(!polygon.is_null());
    assert!(
        last_error_string().is_none(),
        "array-backed heap constructors should clear stale last-error state on success"
    );
    datapod_polygon_free(polygon);

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let point_key = datapod_point_key_new_default();
    assert!(!point_key.is_null());
    assert!(
        last_error_string().is_none(),
        "fixed-value default handle constructors should clear stale last-error state on success"
    );
    datapod_point_key_free(point_key);

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let model = datapod_model_new_default();
    assert!(!model.is_null());
    assert!(
        last_error_string().is_none(),
        "generated default handle constructors should clear stale last-error state on success"
    );
    datapod_model_free(model);
}

#[test]
fn ffi_borrow_helpers_clear_stale_last_error_on_success() {
    let bytes = [1_u8, 2, 3, 4];

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let borrowed = datapod_wire_message_borrow(123, bytes.as_ptr(), bytes.len());
    assert_eq!(borrowed.type_hash, 123);
    assert_eq!(borrowed.data, bytes.as_ptr());
    assert_eq!(borrowed.len, bytes.len());
    assert!(
        last_error_string().is_none(),
        "wire message borrow helper should clear stale last-error state on success"
    );

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let frame = datapod_wire_frame_borrow(456, bytes.as_ptr(), 2, bytes[2..].as_ptr(), 2);
    assert_eq!(frame.type_hash, 456);
    assert_eq!(frame.header, bytes.as_ptr());
    assert_eq!(frame.header_len, 2);
    assert_eq!(frame.payload, bytes[2..].as_ptr());
    assert_eq!(frame.payload_len, 2);
    assert!(
        last_error_string().is_none(),
        "wire frame borrow helper should clear stale last-error state on success"
    );

    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    let archive = datapod_archive_frame_borrow(789, bytes.as_ptr(), 1, bytes[1..].as_ptr(), 3);
    assert_eq!(archive.type_hash, 789);
    assert_eq!(archive.header, bytes.as_ptr());
    assert_eq!(archive.header_len, 1);
    assert_eq!(archive.payload, bytes[1..].as_ptr());
    assert_eq!(archive.payload_len, 3);
    assert!(
        last_error_string().is_none(),
        "archive frame borrow helper should clear stale last-error state on success"
    );
}

#[test]
fn ffi_payload_views_reject_null_handles_with_empty_sentinel_and_fresh_error() {
    macro_rules! assert_null_payload_view {
        ($call:expr, $message:literal $(,)?) => {{
            assert_eq!(datapod_header_size(u64::MAX), 0);
            assert!(
                last_error_string()
                    .expect("stale metadata error should be present")
                    .contains("unknown datapod type hash")
            );
            let view = $call;
            assert_eq!(
                view,
                DatapodBytes {
                    ptr: std::ptr::null(),
                    len: 0,
                },
                $message
            );
            let error = last_error_string().expect("null payload handle should set last error");
            assert!(
                error.contains("null") && error.contains("handle"),
                "payload view should replace stale metadata errors with a null-handle error, got {error:?}"
            );
        }};
    }

    assert_null_payload_view!(
        datapod_polygon_payload(std::ptr::null()),
        "polygon payload view should return the empty sentinel for null handles"
    );
    assert_null_payload_view!(
        datapod_bytes_value_payload(std::ptr::null()),
        "bytes payload view should return the empty sentinel for null handles"
    );
    assert_null_payload_view!(
        datapod_grid_payload(std::ptr::null()),
        "grid payload view should return the empty sentinel for null handles"
    );
    assert_null_payload_view!(
        datapod_matrix_payload(std::ptr::null()),
        "matrix payload view should return the empty sentinel for null handles"
    );
    assert_null_payload_view!(
        datapod_map_payload(std::ptr::null()),
        "map payload view should return the empty sentinel for null handles"
    );
    assert_null_payload_view!(
        datapod_set_payload(std::ptr::null()),
        "set payload view should return the empty sentinel for null handles"
    );
    assert_null_payload_view!(
        datapod_vector_payload(std::ptr::null()),
        "vector payload view should return the empty sentinel for null handles"
    );
    assert_null_payload_view!(
        datapod_tensor_payload(std::ptr::null()),
        "tensor payload view should return the empty sentinel for null handles"
    );
    assert_null_payload_view!(
        datapod_deque_payload(std::ptr::null()),
        "deque payload view should return the empty sentinel for null handles"
    );
}

#[test]
fn ffi_payload_views_clear_stale_last_error_on_success() {
    macro_rules! assert_success_payload_clears_error {
        ($call:expr, $expected_len:expr, $message:literal $(,)?) => {{
            assert_eq!(datapod_header_size(u64::MAX), 0);
            assert!(last_error_string().is_some());
            let view = $call;
            assert_eq!(view.len, $expected_len, $message);
            assert!(!view.ptr.is_null(), $message);
            assert!(last_error_string().is_none(), $message);
        }};
    }

    let bytes = [1_u8, 2, 3, 4];

    let byte_value = datapod_bytes_value_new(bytes.as_ptr(), bytes.len());
    assert!(!byte_value.is_null());
    assert_success_payload_clears_error!(
        datapod_bytes_value_payload(byte_value),
        bytes.len(),
        "bytes payload view should clear stale last-error state on success"
    );
    datapod_bytes_value_free(byte_value);

    let vector = datapod_vector_from_bytes(1, bytes.as_ptr(), bytes.len());
    assert!(!vector.is_null());
    assert_success_payload_clears_error!(
        datapod_vector_payload(vector),
        bytes.len(),
        "vector payload view should clear stale last-error state on success"
    );
    datapod_vector_free(vector);

    let matrix = datapod_matrix_from_bytes(2, 2, 1, bytes.as_ptr(), bytes.len());
    assert!(!matrix.is_null());
    assert_success_payload_clears_error!(
        datapod_matrix_payload(matrix),
        bytes.len(),
        "matrix payload view should clear stale last-error state on success"
    );
    datapod_matrix_free(matrix);

    let tensor = datapod_tensor_from_bytes(2, 1, 2, 1, bytes.as_ptr(), bytes.len());
    assert!(!tensor.is_null());
    assert_success_payload_clears_error!(
        datapod_tensor_payload(tensor),
        bytes.len(),
        "tensor payload view should clear stale last-error state on success"
    );
    datapod_tensor_free(tensor);
}

#[test]
fn ffi_typed_output_failures_clear_stale_values() {
    let mut point_out = datapod_point_new(9.0, 8.0, 7.0);
    let malformed = [0xaa_u8; 3];
    assert!(!datapod_point_from_wire(
        malformed.as_ptr(),
        malformed.len(),
        &mut point_out,
    ));
    assert_eq!(
        point_out,
        DatapodPoint::default(),
        "failed typed decode must clear stale caller output"
    );
    assert!(last_error_string().is_some());

    let vertices = [
        datapod_point_new(0.0, 0.0, 0.0),
        datapod_point_new(1.0, 0.0, 0.0),
        datapod_point_new(0.0, 1.0, 0.0),
    ];
    let polygon = datapod_polygon_new(vertices.as_ptr(), vertices.len());
    assert!(!polygon.is_null());

    let mut vertex_out = datapod_point_new(4.0, 5.0, 6.0);
    assert!(!datapod_polygon_vertex(polygon, 99, &mut vertex_out));
    assert_eq!(
        vertex_out,
        DatapodPoint::default(),
        "failed handle output lookup must clear stale caller output"
    );
    assert!(
        last_error_string()
            .expect("out-of-range polygon lookup must report an error")
            .contains("out of range")
    );

    vertex_out = datapod_point_new(4.0, 5.0, 6.0);
    assert!(!datapod_polygon_vertex(
        std::ptr::null(),
        0,
        &mut vertex_out
    ));
    assert_eq!(
        vertex_out,
        DatapodPoint::default(),
        "null-handle output lookup must clear stale caller output"
    );

    datapod_polygon_free(polygon);
}

#[test]
fn ffi_heap_to_wire_null_handles_clear_stale_outputs() {
    let dangling = std::ptr::NonNull::<u8>::dangling().as_ptr();
    let stale = || DatapodOwnedBytes {
        ptr: dangling,
        len: 123,
        capacity: 456,
    };

    let mut polygon_out = stale();
    assert!(!datapod_polygon_to_wire(std::ptr::null(), &mut polygon_out));
    assert!(polygon_out.ptr.is_null());
    assert_eq!(polygon_out.len, 0);
    assert_eq!(polygon_out.capacity, 0);
    assert!(
        last_error_string()
            .expect("null polygon to_wire must set last error")
            .contains("null polygon handle")
    );

    let mut bytes_out = stale();
    assert!(!datapod_bytes_value_to_wire(
        std::ptr::null(),
        &mut bytes_out
    ));
    assert!(bytes_out.ptr.is_null());
    assert_eq!(bytes_out.len, 0);
    assert_eq!(bytes_out.capacity, 0);
    assert!(
        last_error_string()
            .expect("null bytes to_wire must set last error")
            .contains("null bytes_value handle")
    );

    let mut matrix_out = stale();
    assert!(!datapod_matrix_to_wire(std::ptr::null(), &mut matrix_out));
    assert!(matrix_out.ptr.is_null());
    assert_eq!(matrix_out.len, 0);
    assert_eq!(matrix_out.capacity, 0);
    assert!(
        last_error_string()
            .expect("null matrix to_wire must set last error")
            .contains("null matrix handle")
    );
}

#[test]
fn ffi_generic_wire_helpers_split_join_and_copy_messages() {
    let point = datapod_point_new(4.0, 5.0, 6.0);
    let mut header = vec![0_u8; datapod_point_header_size()];
    assert!(datapod_point_to_header_bytes(
        point,
        header.as_mut_ptr(),
        header.len()
    ));
    let mut short_header = vec![0xaa_u8; datapod_point_header_size().saturating_sub(1)];
    assert!(!datapod_point_to_header_bytes(
        point,
        short_header.as_mut_ptr(),
        short_header.len()
    ));
    assert!(
        short_header.iter().all(|byte| *byte == 0),
        "failed fixed header write must clear caller output bytes"
    );
    assert!(!datapod_point_to_header_bytes(
        point,
        header.as_mut_ptr(),
        usize::MAX
    ));
    assert!(!datapod_last_error_message().is_null());

    let mut joined = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    let point_wire_hash = datapod_emitted_type_hash(datapod_point_type_hash());
    assert!(datapod_wire_message_join(
        point_wire_hash,
        header.as_ptr(),
        header.len(),
        std::ptr::null(),
        0,
        &mut joined,
    ));
    let mut invalid_fixed_body = header.clone();
    invalid_fixed_body.push(99);
    let invalid_fixed_message = datapod_wire_message_borrow(
        point_wire_hash,
        invalid_fixed_body.as_ptr(),
        invalid_fixed_body.len(),
    );
    let mut invalid_fixed_frame = DatapodArchiveFrame {
        type_hash: 0,
        header: std::ptr::null(),
        header_len: 0,
        payload: std::ptr::null(),
        payload_len: 0,
    };
    assert!(!datapod_archive_frame_from_message(
        invalid_fixed_message,
        &mut invalid_fixed_frame
    ));
    let dangling = std::ptr::NonNull::<u8>::dangling().as_ptr();
    let mut stale_invalid_fixed_frame = DatapodArchiveFrame {
        type_hash: 123,
        header: dangling,
        header_len: 456,
        payload: dangling,
        payload_len: 789,
    };
    assert!(!datapod_archive_frame_from_message(
        invalid_fixed_message,
        &mut stale_invalid_fixed_frame
    ));
    assert_eq!(stale_invalid_fixed_frame.type_hash, 0);
    assert!(stale_invalid_fixed_frame.header.is_null());
    assert_eq!(stale_invalid_fixed_frame.header_len, 0);
    assert!(stale_invalid_fixed_frame.payload.is_null());
    assert_eq!(stale_invalid_fixed_frame.payload_len, 0);
    assert!(!datapod_archive_split(
        point_wire_hash,
        invalid_fixed_body.as_ptr(),
        invalid_fixed_body.len(),
        &mut invalid_fixed_frame,
    ));
    assert!(!datapod_last_error_message().is_null());
    let mut stale_invalid_fixed_frame_v1 = DatapodArchiveFrame {
        type_hash: 321,
        header: dangling,
        header_len: 654,
        payload: dangling,
        payload_len: 987,
    };
    assert!(!datapod_archive_split_v1(
        point_wire_hash,
        invalid_fixed_body.as_ptr(),
        invalid_fixed_body.len(),
        &mut stale_invalid_fixed_frame_v1,
    ));
    assert_eq!(stale_invalid_fixed_frame_v1.type_hash, 0);
    assert!(stale_invalid_fixed_frame_v1.header.is_null());
    assert_eq!(stale_invalid_fixed_frame_v1.header_len, 0);
    assert!(stale_invalid_fixed_frame_v1.payload.is_null());
    assert_eq!(stale_invalid_fixed_frame_v1.payload_len, 0);

    assert!(!datapod_archive_frame_from_message(
        invalid_fixed_message,
        std::ptr::null_mut()
    ));
    assert!(
        last_error_string()
            .expect("null archive-frame output must set last error")
            .contains("archive-frame output"),
        "archive-frame-from-message should report archive-frame output errors"
    );
    assert!(!datapod_archive_split(
        point_wire_hash,
        invalid_fixed_body.as_ptr(),
        invalid_fixed_body.len(),
        std::ptr::null_mut(),
    ));
    assert!(
        last_error_string()
            .expect("null archive split output must set last error")
            .contains("archive-frame output"),
        "archive_split should report archive-frame output errors"
    );

    let mut invalid_copy = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(!datapod_wire_message_copy(
        invalid_fixed_message,
        &mut invalid_copy
    ));
    assert!(!datapod_last_error_message().is_null());
    let mut stale_copy_output = DatapodOwnedBytes {
        ptr: std::ptr::NonNull::<u8>::dangling().as_ptr(),
        len: 99,
        capacity: 99,
    };
    assert!(!datapod_wire_message_copy(
        invalid_fixed_message,
        &mut stale_copy_output
    ));
    assert!(
        stale_copy_output.ptr.is_null()
            && stale_copy_output.len == 0
            && stale_copy_output.capacity == 0,
        "failed owned-byte copy must clear stale output pointers"
    );
    assert!(!datapod_wire_message_copy(
        invalid_fixed_message,
        std::ptr::null_mut()
    ));
    assert!(
        last_error_string()
            .expect("null wire-message copy output must set last error")
            .contains("owned-byte output")
    );
    let payload = [1_u8];
    let mut invalid_fixed_join = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(!datapod_wire_message_join(
        point_wire_hash,
        header.as_ptr(),
        header.len(),
        payload.as_ptr(),
        payload.len(),
        &mut invalid_fixed_join,
    ));
    assert!(!datapod_wire_message_join_v1(
        point_wire_hash,
        header.as_ptr(),
        header.len(),
        payload.as_ptr(),
        payload.len(),
        &mut invalid_fixed_join,
    ));
    let mut stale_join_output = DatapodOwnedBytes {
        ptr: std::ptr::NonNull::<u8>::dangling().as_ptr(),
        len: 42,
        capacity: 42,
    };
    assert!(!datapod_wire_message_join(
        point_wire_hash,
        header.as_ptr(),
        header.len(),
        payload.as_ptr(),
        payload.len(),
        &mut stale_join_output,
    ));
    assert!(
        stale_join_output.ptr.is_null()
            && stale_join_output.len == 0
            && stale_join_output.capacity == 0,
        "failed owned-byte join must clear stale output pointers"
    );
    let mut stale_join_v1_output = DatapodOwnedBytes {
        ptr: std::ptr::NonNull::<u8>::dangling().as_ptr(),
        len: 43,
        capacity: 43,
    };
    assert!(!datapod_wire_message_join_v1(
        point_wire_hash,
        header.as_ptr(),
        header.len(),
        payload.as_ptr(),
        payload.len(),
        &mut stale_join_v1_output,
    ));
    assert!(
        stale_join_v1_output.ptr.is_null()
            && stale_join_v1_output.len == 0
            && stale_join_v1_output.capacity == 0,
        "failed v1 owned-byte join must clear stale output pointers"
    );
    assert!(!datapod_wire_message_join(
        point_wire_hash,
        header.as_ptr(),
        header.len(),
        payload.as_ptr(),
        payload.len(),
        std::ptr::null_mut(),
    ));
    assert!(
        last_error_string()
            .expect("null wire-message join output must set last error")
            .contains("owned-byte output")
    );
    assert!(!datapod_wire_message_join_v1(
        point_wire_hash,
        header.as_ptr(),
        header.len(),
        payload.as_ptr(),
        payload.len(),
        std::ptr::null_mut(),
    ));
    assert!(
        last_error_string()
            .expect("null v1 wire-message join output must set last error")
            .contains("owned-byte output")
    );
    let invalid_archive = datapod_archive_frame_borrow(
        point_wire_hash,
        header.as_ptr(),
        header.len(),
        payload.as_ptr(),
        payload.len(),
    );
    let mut stale_archive_join_output = DatapodOwnedBytes {
        ptr: std::ptr::NonNull::<u8>::dangling().as_ptr(),
        len: 24,
        capacity: 24,
    };
    assert!(!datapod_archive_to_message(
        invalid_archive,
        &mut stale_archive_join_output
    ));
    assert!(
        stale_archive_join_output.ptr.is_null()
            && stale_archive_join_output.len == 0
            && stale_archive_join_output.capacity == 0,
        "failed archive-to-message join must clear stale output pointers"
    );
    let mut stale_archive_join_v1_output = DatapodOwnedBytes {
        ptr: std::ptr::NonNull::<u8>::dangling().as_ptr(),
        len: 25,
        capacity: 25,
    };
    assert!(!datapod_archive_to_message_v1(
        invalid_archive,
        &mut stale_archive_join_v1_output
    ));
    assert!(
        stale_archive_join_v1_output.ptr.is_null()
            && stale_archive_join_v1_output.len == 0
            && stale_archive_join_v1_output.capacity == 0,
        "failed v1 archive-to-message join must clear stale output pointers"
    );
    assert!(!datapod_archive_to_message(
        invalid_archive,
        std::ptr::null_mut()
    ));
    assert!(
        last_error_string()
            .expect("null archive-to-message output must set last error")
            .contains("owned-byte output")
    );

    let matrix_payload = [1_u8, 2, 3, 4];
    let matrix = datapod_matrix_from_bytes(2, 2, 1, matrix_payload.as_ptr(), matrix_payload.len());
    assert!(!matrix.is_null());
    let mut matrix_header = vec![0_u8; datapod_matrix_header_size()];
    assert!(datapod_matrix_to_header_bytes(
        matrix,
        matrix_header.as_mut_ptr(),
        matrix_header.len()
    ));
    assert!(!datapod_wire_message_join(
        datapod_emitted_type_hash(datapod_matrix_type_hash()),
        matrix_header.as_ptr(),
        matrix_header.len(),
        matrix_payload.as_ptr(),
        matrix_payload.len() - 1,
        &mut invalid_fixed_join,
    ));
    datapod_matrix_free(matrix);

    let message = datapod_wire_message_borrow(point_wire_hash, joined.ptr, joined.len);
    assert!(datapod_wire_message_is_valid(message));
    let oversized_message =
        datapod_wire_message_borrow(point_wire_hash, header.as_ptr(), usize::MAX);
    let mut oversized_copy = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(!datapod_wire_message_copy(
        oversized_message,
        &mut oversized_copy
    ));
    assert!(!datapod_last_error_message().is_null());
    let header_view = datapod_wire_message_header(message);
    assert_eq!(header_view.len, header.len());
    let payload_view = datapod_wire_message_payload(message);
    assert_eq!(payload_view.len, 0);

    let archive = datapod_archive_frame_borrow(
        point_wire_hash,
        header.as_ptr(),
        header.len(),
        std::ptr::null(),
        0,
    );
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    assert!(datapod_archive_validate(archive));
    assert!(
        last_error_string().is_none(),
        "successful archive validation should clear stale metadata errors"
    );
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    assert!(datapod_archive_validate_as(point_wire_hash, archive));
    assert!(
        last_error_string().is_none(),
        "successful archive validate_as should clear stale metadata errors"
    );
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    assert!(datapod_archive_is_valid(archive));
    assert!(
        last_error_string().is_none(),
        "successful archive is_valid should clear stale metadata errors"
    );

    let mut archive_from_message = DatapodArchiveFrame {
        type_hash: 0,
        header: std::ptr::null(),
        header_len: 0,
        payload: std::ptr::null(),
        payload_len: 0,
    };
    assert!(datapod_archive_frame_from_message(
        message,
        &mut archive_from_message
    ));
    let mut archive_from_split = DatapodArchiveFrame {
        type_hash: 0,
        header: std::ptr::null(),
        header_len: 0,
        payload: std::ptr::null(),
        payload_len: 0,
    };
    assert!(datapod_archive_split(
        point_wire_hash,
        joined.ptr,
        joined.len,
        &mut archive_from_split,
    ));
    assert_eq!(archive_from_split.type_hash, point_wire_hash);
    assert_eq!(archive_from_split.header_len, header.len());
    assert_eq!(archive_from_split.payload_len, 0);
    assert_eq!(archive_from_message.type_hash, point_wire_hash);
    assert_eq!(archive_from_message.header_len, header.len());
    assert_eq!(archive_from_message.payload_len, 0);
    assert!(datapod_archive_validate_v1(archive_from_message));
    let mut archive_joined = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(datapod_archive_to_message(
        archive_from_message,
        &mut archive_joined
    ));
    assert!(
        last_error_string().is_none(),
        "successful archive-to-message should clear stale errors"
    );
    assert_eq!(archive_joined.len, joined.len);

    let mut copied = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(datapod_wire_message_copy(message, &mut copied));
    assert_eq!(copied.len, joined.len);

    datapod_owned_bytes_free(joined);
    datapod_owned_bytes_free(archive_joined);
    datapod_owned_bytes_free(copied);
}

#[test]
fn ffi_generic_fixed_value_wire_helpers_cover_all_fixed_c_structs() {
    let pose = datapod_pose_new(
        datapod_point_new(1.0, 2.0, 3.0),
        datapod_quaternion_identity(),
    );
    let mut pose_wire = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    let pose_wire_hash = datapod_emitted_type_hash(datapod_pose_type_hash());
    assert!(datapod_fixed_value_to_wire(
        pose_wire_hash,
        (&pose as *const DatapodPose).cast(),
        std::mem::size_of::<DatapodPose>(),
        &mut pose_wire,
    ));
    assert_eq!(pose_wire.len, datapod_pose_header_size());
    let mut pose_archive = DatapodArchiveFrame {
        type_hash: 0,
        header: std::ptr::null(),
        header_len: 0,
        payload: std::ptr::null(),
        payload_len: 0,
    };
    assert!(datapod_fixed_value_archive(
        pose_wire_hash,
        (&pose as *const DatapodPose).cast(),
        std::mem::size_of::<DatapodPose>(),
        &mut pose_archive,
    ));
    assert_eq!(pose_archive.type_hash, pose_wire_hash);
    assert_eq!(pose_archive.header_len, datapod_pose_header_size());
    assert_eq!(pose_archive.payload_len, 0);

    let mut pose_out = DatapodPose::default();
    assert!(datapod_fixed_value_from_wire(
        pose_wire_hash,
        pose_wire.ptr,
        pose_wire.len,
        (&mut pose_out as *mut DatapodPose).cast(),
        std::mem::size_of::<DatapodPose>(),
    ));
    assert_eq!(pose, pose_out);
    let mut stale_pose_out = pose;
    assert!(!datapod_fixed_value_from_wire(
        pose_wire_hash,
        pose_wire.ptr,
        pose_wire.len.saturating_sub(1),
        (&mut stale_pose_out as *mut DatapodPose).cast(),
        std::mem::size_of::<DatapodPose>(),
    ));
    assert_eq!(
        stale_pose_out,
        DatapodPose::default(),
        "failed fixed-value wire decode must clear stale output bytes"
    );
    let mut pose_archive_out = DatapodPose::default();
    assert!(datapod_fixed_value_from_archive(
        pose_wire_hash,
        pose_archive,
        (&mut pose_archive_out as *mut DatapodPose).cast(),
        std::mem::size_of::<DatapodPose>(),
    ));
    assert_eq!(pose, pose_archive_out);
    let mut stale_pose_archive_out = pose;
    let mut invalid_pose_archive = pose_archive;
    invalid_pose_archive.payload = pose_wire.ptr;
    invalid_pose_archive.payload_len = 1;
    assert!(!datapod_fixed_value_from_archive(
        pose_wire_hash,
        invalid_pose_archive,
        (&mut stale_pose_archive_out as *mut DatapodPose).cast(),
        std::mem::size_of::<DatapodPose>(),
    ));
    assert_eq!(
        stale_pose_archive_out,
        DatapodPose::default(),
        "failed fixed-value archive decode must clear stale output bytes"
    );
    let mut stale_wrong_type_wire_out = pose;
    assert!(!datapod_fixed_value_from_wire(
        datapod_bytes_value_type_hash(),
        pose_wire.ptr,
        pose_wire.len,
        (&mut stale_wrong_type_wire_out as *mut DatapodPose).cast(),
        std::mem::size_of::<DatapodPose>(),
    ));
    assert_eq!(
        stale_wrong_type_wire_out,
        DatapodPose::default(),
        "wrong-type fixed-value wire decode must clear stale output bytes"
    );
    let mut stale_wrong_type_archive_out = pose;
    assert!(!datapod_fixed_value_from_archive(
        datapod_bytes_value_type_hash(),
        pose_archive,
        (&mut stale_wrong_type_archive_out as *mut DatapodPose).cast(),
        std::mem::size_of::<DatapodPose>(),
    ));
    assert_eq!(
        stale_wrong_type_archive_out,
        DatapodPose::default(),
        "wrong-type fixed-value archive decode must clear stale output bytes"
    );

    let unknown_hash = u64::MAX;
    let dangling = std::ptr::NonNull::<u8>::dangling().as_ptr();
    let mut stale_unknown_wire = DatapodOwnedBytes {
        ptr: dangling,
        len: 123,
        capacity: 456,
    };
    assert!(!datapod_fixed_value_to_wire(
        unknown_hash,
        (&pose as *const DatapodPose).cast(),
        std::mem::size_of::<DatapodPose>(),
        &mut stale_unknown_wire,
    ));
    assert!(stale_unknown_wire.ptr.is_null());
    assert_eq!(stale_unknown_wire.len, 0);
    assert_eq!(stale_unknown_wire.capacity, 0);
    let unknown_error = unsafe { std::ffi::CStr::from_ptr(datapod_last_error_message()) }
        .to_str()
        .expect("last error is utf-8");
    assert!(unknown_error.contains("unknown datapod type hash"));

    let mut stale_unknown_archive = DatapodArchiveFrame {
        type_hash: 123,
        header: dangling,
        header_len: 456,
        payload: dangling,
        payload_len: 789,
    };
    assert!(!datapod_fixed_value_archive(
        unknown_hash,
        (&pose as *const DatapodPose).cast(),
        std::mem::size_of::<DatapodPose>(),
        &mut stale_unknown_archive,
    ));
    assert_eq!(stale_unknown_archive.type_hash, 0);
    assert!(stale_unknown_archive.header.is_null());
    assert_eq!(stale_unknown_archive.header_len, 0);
    assert!(stale_unknown_archive.payload.is_null());
    assert_eq!(stale_unknown_archive.payload_len, 0);

    let mut stale_unknown_wire_out = pose;
    assert!(!datapod_fixed_value_from_wire(
        unknown_hash,
        pose_wire.ptr,
        pose_wire.len,
        (&mut stale_unknown_wire_out as *mut DatapodPose).cast(),
        std::mem::size_of::<DatapodPose>(),
    ));
    assert_eq!(stale_unknown_wire_out, DatapodPose::default());

    let mut stale_unknown_archive_out = pose;
    assert!(!datapod_fixed_value_from_archive(
        unknown_hash,
        pose_archive,
        (&mut stale_unknown_archive_out as *mut DatapodPose).cast(),
        std::mem::size_of::<DatapodPose>(),
    ));
    assert_eq!(stale_unknown_archive_out, DatapodPose::default());
    datapod_owned_bytes_free(pose_wire);

    let bytes = [1_u8, 2, 3];
    let value = datapod_bytes_value_new(bytes.as_ptr(), bytes.len());
    assert!(!value.is_null());
    let mut should_fail = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(!datapod_fixed_value_to_wire(
        datapod_bytes_value_type_hash(),
        std::ptr::null(),
        0,
        &mut should_fail,
    ));
    assert!(!datapod_last_error_message().is_null());
    datapod_bytes_value_free(value);
}

#[test]
fn ffi_typed_wire_helpers_round_trip_fixed_and_heap_values() {
    let point = datapod_point_new(1.0, 2.0, 3.0);
    let mut point_wire = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(datapod_point_to_wire(point, &mut point_wire));
    assert!(!point_wire.ptr.is_null());
    let mut point_out = DatapodPoint::default();
    assert!(datapod_point_from_wire(
        point_wire.ptr,
        point_wire.len,
        &mut point_out
    ));
    assert_eq!(point, point_out);
    datapod_owned_bytes_free(point_wire);

    let pixels = [1_u8, 2, 3, 4];
    let grid = datapod_grid_new(
        2,
        2,
        13,
        false,
        1.0,
        datapod_pose_new(
            datapod_point_new(0.0, 0.0, 0.0),
            datapod_quaternion_identity(),
        ),
        pixels.as_ptr(),
        pixels.len(),
    );
    assert!(!grid.is_null());
    let mut grid_wire = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(datapod_grid_to_wire(grid, &mut grid_wire));
    let grid_out = datapod_grid_from_wire(grid_wire.ptr, grid_wire.len);
    assert!(!grid_out.is_null());
    assert_eq!(datapod_grid_payload(grid_out).len, pixels.len());
    let mut grid_view = DatapodGridView::default();
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    assert!(datapod_grid_view_from_wire(
        grid_wire.ptr,
        grid_wire.len,
        &mut grid_view,
    ));
    assert!(
        last_error_string().is_none(),
        "successful grid view decode should clear stale last-error state"
    );
    assert_eq!(grid_view.rows, 2);
    assert_eq!(grid_view.cols, 2);
    assert_eq!(grid_view.encoding, 13);
    assert_eq!(grid_view.data.len, pixels.len());
    assert_eq!(
        grid_view.data.ptr,
        unsafe { grid_wire.ptr.add(datapod_grid_header_size()) }.cast_const()
    );
    assert!(!datapod_grid_view_from_wire(
        grid_wire.ptr,
        grid_wire.len - 1,
        &mut grid_view,
    ));
    datapod_grid_free(grid);
    datapod_grid_free(grid_out);
    datapod_owned_bytes_free(grid_wire);

    let bytes = [9_u8, 8, 7, 6];
    let byte_value = datapod_bytes_value_new(bytes.as_ptr(), bytes.len());
    assert!(!byte_value.is_null());
    let mut byte_wire = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(datapod_bytes_value_to_wire(byte_value, &mut byte_wire));
    let byte_out = datapod_bytes_value_from_wire(byte_wire.ptr, byte_wire.len);
    assert!(!byte_out.is_null());
    assert_eq!(datapod_bytes_value_payload(byte_out).len, bytes.len());
    let mut byte_view = DatapodBytesView::default();
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    assert!(datapod_bytes_value_view_from_wire(
        byte_wire.ptr,
        byte_wire.len,
        &mut byte_view,
    ));
    assert!(
        last_error_string().is_none(),
        "successful bytes view decode should clear stale last-error state"
    );
    assert_eq!(byte_view.payload.len, bytes.len());
    assert_eq!(
        byte_view.payload.ptr,
        unsafe { byte_wire.ptr.add(datapod_bytes_value_header_size()) }.cast_const()
    );
    assert!(!datapod_bytes_value_view_from_wire(
        byte_wire.ptr,
        byte_wire.len,
        std::ptr::null_mut(),
    ));
    assert!(
        last_error_string()
            .expect("null bytes view output should set last error")
            .contains("null bytes view output")
    );
    let mut byte_archive = DatapodArchiveFrame {
        type_hash: 0,
        header: std::ptr::null(),
        header_len: 0,
        payload: std::ptr::null(),
        payload_len: 0,
    };
    assert!(datapod_bytes_value_archive(byte_value, &mut byte_archive));
    assert_eq!(
        byte_archive.payload,
        datapod_bytes_value_payload(byte_value).ptr
    );
    let mut byte_archive_view = DatapodBytesView::default();
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    assert!(datapod_bytes_value_view_from_archive(
        byte_archive,
        &mut byte_archive_view,
    ));
    assert!(
        last_error_string().is_none(),
        "successful bytes archive view should clear stale last-error state"
    );
    assert!(!datapod_bytes_value_view_from_archive(
        byte_archive,
        std::ptr::null_mut(),
    ));
    assert!(
        last_error_string()
            .expect("null bytes archive view output should set last error")
            .contains("null bytes view output")
    );
    assert_eq!(byte_archive_view.payload.ptr, byte_archive.payload);
    let byte_from_archive = datapod_bytes_value_from_archive(byte_archive);
    assert!(!byte_from_archive.is_null());
    assert_eq!(
        datapod_bytes_value_payload(byte_from_archive).len,
        bytes.len()
    );
    datapod_bytes_value_free(byte_value);
    datapod_bytes_value_free(byte_out);
    datapod_bytes_value_free(byte_from_archive);
    datapod_owned_bytes_free(byte_wire);

    let matrix = datapod_matrix_from_bytes(2, 2, 1, bytes.as_ptr(), bytes.len());
    assert!(!matrix.is_null());
    let mut matrix_wire = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(datapod_matrix_to_wire(matrix, &mut matrix_wire));
    let matrix_out = datapod_matrix_from_wire(matrix_wire.ptr, matrix_wire.len);
    assert!(!matrix_out.is_null());
    assert_eq!(datapod_matrix_payload(matrix_out).len, bytes.len());
    let mut matrix_archive = DatapodArchiveFrame {
        type_hash: 0,
        header: std::ptr::null(),
        header_len: 0,
        payload: std::ptr::null(),
        payload_len: 0,
    };
    assert!(datapod_matrix_archive(matrix, &mut matrix_archive));
    assert_eq!(
        matrix_archive.type_hash,
        datapod_emitted_type_hash(datapod_matrix_type_hash())
    );
    assert_eq!(matrix_archive.header_len, datapod_matrix_header_size());
    assert_eq!(matrix_archive.payload_len, bytes.len());
    assert_eq!(matrix_archive.payload, datapod_matrix_payload(matrix).ptr);
    let mut matrix_view = DatapodMatrixView::default();
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    assert!(datapod_matrix_view_from_wire(
        matrix_wire.ptr,
        matrix_wire.len,
        &mut matrix_view,
    ));
    assert!(
        last_error_string().is_none(),
        "successful matrix view decode should clear stale last-error state"
    );
    assert_eq!(matrix_view.rows, 2);
    assert_eq!(matrix_view.cols, 2);
    assert_eq!(matrix_view.element_size, 1);
    assert_eq!(matrix_view.payload.len, bytes.len());
    assert_eq!(
        matrix_view.payload.ptr,
        unsafe { matrix_wire.ptr.add(datapod_matrix_header_size()) }.cast_const()
    );
    assert!(!datapod_matrix_view_from_wire(
        matrix_wire.ptr,
        matrix_wire.len,
        std::ptr::null_mut(),
    ));
    assert!(
        last_error_string()
            .expect("null matrix view output should set last error")
            .contains("null matrix view output")
    );
    let mut matrix_archive_view = DatapodMatrixView::default();
    assert_eq!(datapod_header_size(u64::MAX), 0);
    assert!(last_error_string().is_some());
    assert!(datapod_matrix_view_from_archive(
        matrix_archive,
        &mut matrix_archive_view,
    ));
    assert!(
        last_error_string().is_none(),
        "successful matrix archive view should clear stale last-error state"
    );
    assert!(!datapod_matrix_view_from_archive(
        matrix_archive,
        std::ptr::null_mut(),
    ));
    assert!(
        last_error_string()
            .expect("null matrix archive view output should set last error")
            .contains("null matrix view output")
    );
    assert_eq!(matrix_archive_view.payload.ptr, matrix_archive.payload);
    let matrix_from_archive = datapod_matrix_from_archive(matrix_archive);
    assert!(!matrix_from_archive.is_null());
    assert_eq!(
        datapod_matrix_payload(matrix_from_archive).len,
        matrix_archive.payload_len
    );
    assert!(!datapod_matrix_view_from_wire(
        matrix_wire.ptr,
        matrix_wire.len - 1,
        &mut matrix_view,
    ));
    assert_eq!(
        matrix_view,
        DatapodMatrixView::default(),
        "failed matrix view decode must clear stale borrowed payload pointers"
    );
    datapod_matrix_free(matrix);
    datapod_matrix_free(matrix_out);
    datapod_matrix_free(matrix_from_archive);
    datapod_owned_bytes_free(matrix_wire);

    let vector = datapod_vector_from_bytes(1, bytes.as_ptr(), bytes.len());
    assert!(!vector.is_null());
    let mut vector_wire = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(datapod_vector_to_wire(vector, &mut vector_wire));
    let vector_out = datapod_vector_from_wire(vector_wire.ptr, vector_wire.len);
    assert!(!vector_out.is_null());
    assert_eq!(datapod_vector_payload(vector_out).len, bytes.len());
    let mut vector_view = DatapodVectorView::default();
    assert!(datapod_vector_view_from_wire(
        vector_wire.ptr,
        vector_wire.len,
        &mut vector_view,
    ));
    assert_eq!(vector_view.element_size, 1);
    assert_eq!(vector_view.payload.len, bytes.len());
    let mut vector_archive = DatapodArchiveFrame {
        type_hash: 0,
        header: std::ptr::null(),
        header_len: 0,
        payload: std::ptr::null(),
        payload_len: 0,
    };
    assert!(datapod_vector_archive(vector, &mut vector_archive));
    assert_eq!(vector_archive.payload, datapod_vector_payload(vector).ptr);
    let mut vector_archive_view = DatapodVectorView::default();
    assert!(datapod_vector_view_from_archive(
        vector_archive,
        &mut vector_archive_view,
    ));
    assert_eq!(vector_archive_view.payload.ptr, vector_archive.payload);
    let vector_from_archive = datapod_vector_from_archive(vector_archive);
    assert!(!vector_from_archive.is_null());
    assert_eq!(datapod_vector_payload(vector_from_archive).len, bytes.len());
    datapod_vector_free(vector);
    datapod_vector_free(vector_out);
    datapod_vector_free(vector_from_archive);
    datapod_owned_bytes_free(vector_wire);

    let tensor = datapod_tensor_from_bytes(1, 2, 2, 1, bytes.as_ptr(), bytes.len());
    assert!(!tensor.is_null());
    let mut tensor_wire = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(datapod_tensor_to_wire(tensor, &mut tensor_wire));
    let tensor_out = datapod_tensor_from_wire(tensor_wire.ptr, tensor_wire.len);
    assert!(!tensor_out.is_null());
    assert_eq!(datapod_tensor_payload(tensor_out).len, bytes.len());
    let mut tensor_view = DatapodTensorView::default();
    assert!(datapod_tensor_view_from_wire(
        tensor_wire.ptr,
        tensor_wire.len,
        &mut tensor_view,
    ));
    assert_eq!(tensor_view.rows, 1);
    assert_eq!(tensor_view.cols, 2);
    assert_eq!(tensor_view.layers, 2);
    assert_eq!(tensor_view.element_size, 1);
    assert_eq!(tensor_view.payload.len, bytes.len());
    assert!(!datapod_tensor_view_from_wire(
        tensor_wire.ptr,
        tensor_wire.len - 1,
        &mut tensor_view,
    ));
    datapod_tensor_free(tensor);
    datapod_tensor_free(tensor_out);
    datapod_owned_bytes_free(tensor_wire);

    let bit_payload = [0b0000_0101_u8];
    let bitvec = datapod_bitvec_from_bytes(3, bit_payload.as_ptr(), bit_payload.len());
    assert!(!bitvec.is_null());
    let mut bitvec_wire = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(datapod_bitvec_to_wire(bitvec, &mut bitvec_wire));
    let mut bitvec_view = DatapodBitVecView::default();
    assert!(datapod_bitvec_view_from_wire(
        bitvec_wire.ptr,
        bitvec_wire.len,
        &mut bitvec_view,
    ));
    assert_eq!(bitvec_view.bits, 3);
    assert_eq!(bitvec_view.data.len, 1);
    datapod_bitvec_free(bitvec);
    datapod_owned_bytes_free(bitvec_wire);

    let mut vecvec_payload = Vec::new();
    vecvec_payload.extend_from_slice(&1_u32.to_le_bytes());
    vecvec_payload.extend_from_slice(&0_u32.to_le_bytes());
    vecvec_payload.extend_from_slice(&4_u32.to_le_bytes());
    vecvec_payload.extend_from_slice(&[1_u8, 0, 2, 0]);
    let vecvec = datapod_vecvec_from_bytes(2, vecvec_payload.as_ptr(), vecvec_payload.len());
    assert!(!vecvec.is_null());
    let mut vecvec_wire = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(datapod_vecvec_to_wire(vecvec, &mut vecvec_wire));
    let mut vecvec_view = DatapodVecvecView::default();
    assert!(datapod_vecvec_view_from_wire(
        vecvec_wire.ptr,
        vecvec_wire.len,
        &mut vecvec_view,
    ));
    assert_eq!(vecvec_view.element_size, 2);
    assert_eq!(vecvec_view.bucket_count, 1);
    assert_eq!(vecvec_view.payload.len, vecvec_payload.len());
    datapod_vecvec_free(vecvec);
    datapod_owned_bytes_free(vecvec_wire);

    let map = datapod_map_new();
    assert!(!map.is_null());
    let map_key = b"k";
    let map_value = b"v";
    assert!(datapod_map_insert(
        map,
        map_key.as_ptr(),
        map_key.len(),
        map_value.as_ptr(),
        map_value.len(),
    ));
    let mut map_wire = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(datapod_map_to_wire(map, &mut map_wire));
    let mut map_view = DatapodMapView::default();
    assert!(datapod_map_view_from_wire(
        map_wire.ptr,
        map_wire.len,
        &mut map_view,
    ));
    assert_eq!(map_view.count, 1);
    assert_eq!(
        map_view.entries.len,
        std::mem::size_of::<datapod::MapEntry>()
    );
    assert_eq!(map_view.blob.len, map_key.len() + map_value.len());
    assert_eq!(
        map_view.payload.ptr,
        unsafe { map_wire.ptr.add(datapod_map_header_size()) }.cast_const()
    );
    assert!(!datapod_map_view_from_wire(
        map_wire.ptr,
        map_wire.len - 1,
        &mut map_view,
    ));
    let mut map_frame = DatapodArchiveFrame {
        type_hash: 0,
        header: std::ptr::null(),
        header_len: 0,
        payload: std::ptr::null(),
        payload_len: 0,
    };
    assert!(datapod_archive_split(
        datapod_emitted_type_hash(datapod_map_type_hash()),
        map_wire.ptr,
        map_wire.len,
        &mut map_frame,
    ));
    let mut bad_map_frame = map_frame;
    bad_map_frame.payload_len = bad_map_frame.payload_len.saturating_sub(1);
    assert!(!datapod_map_view_from_frame(bad_map_frame, &mut map_view,));
    assert_eq!(
        map_view,
        DatapodMapView::default(),
        "failed map view decode must clear stale borrowed payload pointers"
    );
    assert!(!datapod_last_error_message().is_null());
    datapod_map_free(map);
    datapod_owned_bytes_free(map_wire);

    let set = datapod_set_new();
    assert!(!set.is_null());
    let set_key = b"s";
    assert!(datapod_set_insert(set, set_key.as_ptr(), set_key.len()));
    assert!(!datapod_set_insert(set, set_key.as_ptr(), set_key.len()));
    let mut set_wire = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(datapod_set_to_wire(set, &mut set_wire));
    let mut set_view = DatapodSetView::default();
    assert!(datapod_set_view_from_wire(
        set_wire.ptr,
        set_wire.len,
        &mut set_view,
    ));
    assert_eq!(set_view.count, 1);
    assert_eq!(
        set_view.entries.len,
        std::mem::size_of::<datapod::SetEntry>()
    );
    assert_eq!(set_view.blob.len, set_key.len());
    assert_eq!(
        set_view.payload.ptr,
        unsafe { set_wire.ptr.add(datapod_set_header_size()) }.cast_const()
    );
    assert!(!datapod_set_view_from_wire(
        set_wire.ptr,
        set_wire.len - 1,
        &mut set_view,
    ));
    assert_eq!(
        set_view,
        DatapodSetView::default(),
        "failed set view decode must clear stale borrowed payload pointers"
    );
    let mut set_frame = DatapodArchiveFrame {
        type_hash: 0,
        header: std::ptr::null(),
        header_len: 0,
        payload: std::ptr::null(),
        payload_len: 0,
    };
    assert!(datapod_archive_split(
        datapod_emitted_type_hash(datapod_set_type_hash()),
        set_wire.ptr,
        set_wire.len,
        &mut set_frame,
    ));
    let mut bad_set_frame = set_frame;
    bad_set_frame.payload_len = bad_set_frame.payload_len.saturating_sub(1);
    assert!(!datapod_set_view_from_frame(bad_set_frame, &mut set_view,));
    assert!(!datapod_last_error_message().is_null());
    datapod_set_free(set);
    datapod_owned_bytes_free(set_wire);

    let vertices = [
        datapod_point_new(0.0, 0.0, 0.0),
        datapod_point_new(1.0, 0.0, 0.0),
        datapod_point_new(0.0, 1.0, 0.0),
    ];
    let polygon = datapod_polygon_new(vertices.as_ptr(), vertices.len());
    assert!(!polygon.is_null());
    let mut polygon_wire = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(datapod_polygon_to_wire(polygon, &mut polygon_wire));
    let polygon_out = datapod_polygon_from_wire(polygon_wire.ptr, polygon_wire.len);
    assert!(!polygon_out.is_null());
    assert_eq!(
        datapod_polygon_payload(polygon_out).len,
        datapod_polygon_payload(polygon).len
    );
    datapod_polygon_free(polygon);
    datapod_polygon_free(polygon_out);
    datapod_owned_bytes_free(polygon_wire);
}

#[test]
fn ffi_grid_and_layer_constructors_reject_invalid_owned_values() {
    let pose = datapod_pose_new(
        datapod_point_new(0.0, 0.0, 0.0),
        datapod_quaternion_identity(),
    );
    let pixels = [1_u8, 2, 3, 4];

    assert!(datapod_grid_new(0, 2, 13, false, 1.0, pose, pixels.as_ptr(), pixels.len()).is_null());
    assert!(!datapod_last_error_message().is_null());

    assert!(datapod_grid_new(2, 2, 13, false, 1.0, pose, pixels.as_ptr(), 3).is_null());
    assert!(!datapod_last_error_message().is_null());

    assert!(
        datapod_grid_new(
            2,
            2,
            13,
            false,
            f64::NAN,
            pose,
            pixels.as_ptr(),
            pixels.len()
        )
        .is_null()
    );
    assert!(!datapod_last_error_message().is_null());

    assert!(
        datapod_layer_new(
            2,
            2,
            0,
            13,
            false,
            1.0,
            1.0,
            pose,
            pixels.as_ptr(),
            pixels.len()
        )
        .is_null()
    );
    assert!(!datapod_last_error_message().is_null());

    assert!(
        datapod_layer_new(
            2,
            2,
            1,
            13,
            false,
            1.0,
            0.0,
            pose,
            pixels.as_ptr(),
            pixels.len()
        )
        .is_null()
    );
    assert!(!datapod_last_error_message().is_null());

    assert!(datapod_layer_new(2, 2, 1, 13, false, 1.0, 1.0, pose, pixels.as_ptr(), 3).is_null());
    assert!(!datapod_last_error_message().is_null());
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

#[test]
fn ffi_archive_helpers_reject_null_outputs_wrong_hashes_and_invalid_fixed_payloads() {
    let point = datapod_point_new(1.0, 2.0, 3.0);
    let point_hash = datapod_emitted_type_hash(datapod_point_type_hash());
    let mut point_header = vec![0_u8; datapod_point_header_size()];
    assert!(datapod_point_to_header_bytes(
        point,
        point_header.as_mut_ptr(),
        point_header.len()
    ));

    assert!(!datapod_fixed_value_archive(
        point_hash,
        (&point as *const DatapodPoint).cast(),
        std::mem::size_of::<DatapodPoint>(),
        std::ptr::null_mut(),
    ));
    assert!(!datapod_last_error_message().is_null());

    let payload = [9_u8];
    let fixed_with_payload = datapod_archive_frame_borrow(
        point_hash,
        point_header.as_ptr(),
        point_header.len(),
        payload.as_ptr(),
        payload.len(),
    );
    assert!(!datapod_archive_validate(fixed_with_payload));
    let mut point_out = DatapodPoint::default();
    assert!(!datapod_fixed_value_from_archive(
        point_hash,
        fixed_with_payload,
        (&mut point_out as *mut DatapodPoint).cast(),
        std::mem::size_of::<DatapodPoint>(),
    ));
    let dangling = std::ptr::NonNull::<u8>::dangling().as_ptr();
    let mut stale_fixed_archive = DatapodArchiveFrame {
        type_hash: 123,
        header: dangling,
        header_len: 456,
        payload: dangling,
        payload_len: 789,
    };
    assert!(!datapod_fixed_value_archive(
        point_hash,
        (&point as *const DatapodPoint).cast(),
        std::mem::size_of::<DatapodPoint>() - 1,
        &mut stale_fixed_archive,
    ));
    assert_eq!(stale_fixed_archive.type_hash, 0);
    assert!(stale_fixed_archive.header.is_null());
    assert_eq!(stale_fixed_archive.header_len, 0);
    assert!(stale_fixed_archive.payload.is_null());
    assert_eq!(stale_fixed_archive.payload_len, 0);

    let bytes = [1_u8, 2, 3, 4];
    let value = datapod_bytes_value_new(bytes.as_ptr(), bytes.len());
    assert!(!value.is_null());
    assert!(!datapod_bytes_value_archive(value, std::ptr::null_mut()));
    let mut stale_heap_archive = DatapodArchiveFrame {
        type_hash: 123,
        header: dangling,
        header_len: 456,
        payload: dangling,
        payload_len: 789,
    };
    assert!(!datapod_bytes_value_archive(
        std::ptr::null(),
        &mut stale_heap_archive,
    ));
    assert_eq!(stale_heap_archive.type_hash, 0);
    assert!(stale_heap_archive.header.is_null());
    assert_eq!(stale_heap_archive.header_len, 0);
    assert!(stale_heap_archive.payload.is_null());
    assert_eq!(stale_heap_archive.payload_len, 0);

    let mut archive = DatapodArchiveFrame {
        type_hash: 0,
        header: std::ptr::null(),
        header_len: 0,
        payload: std::ptr::null(),
        payload_len: 0,
    };
    assert!(datapod_bytes_value_archive(value, &mut archive));
    archive.type_hash ^= 1;

    let mut view = DatapodBytesView::default();
    assert!(!datapod_bytes_value_view_from_archive(archive, &mut view));
    assert!(datapod_bytes_value_from_archive(archive).is_null());
    datapod_bytes_value_free(value);

    let text = b"abc";
    let dpstr = datapod_dpstr_new(text.as_ptr(), text.len());
    assert!(!dpstr.is_null());
    let mut dpstr_archive = DatapodArchiveFrame {
        type_hash: 0,
        header: std::ptr::null(),
        header_len: 0,
        payload: std::ptr::null(),
        payload_len: 0,
    };
    assert!(datapod_dpstr_archive(dpstr, &mut dpstr_archive));
    assert_eq!(
        dpstr_archive.type_hash,
        datapod_emitted_type_hash(datapod_dpstr_type_hash())
    );
    assert!(!datapod_bytes_value_view_from_archive(
        dpstr_archive,
        &mut view
    ));
    assert!(datapod_bytes_value_from_archive(dpstr_archive).is_null());
    datapod_dpstr_free(dpstr);

    let invalid_utf8 = [0xff_u8];
    let invalid_dpstr_archive = DatapodArchiveFrame {
        type_hash: datapod_emitted_type_hash(datapod_dpstr_type_hash()),
        header: std::ptr::null(),
        header_len: 0,
        payload: invalid_utf8.as_ptr(),
        payload_len: invalid_utf8.len(),
    };
    let mut stale_dpstr_view = DatapodDpStrView {
        utf8: DatapodBytes {
            ptr: dangling,
            len: 99,
        },
    };
    assert!(!datapod_dpstr_view_from_archive(
        invalid_dpstr_archive,
        &mut stale_dpstr_view,
    ));
    assert_eq!(stale_dpstr_view, DatapodDpStrView::default());
    assert!(
        last_error_string()
            .expect("invalid UTF-8 DpStr archive view should set last error")
            .contains("utf-8")
    );
    assert!(datapod_dpstr_from_archive(invalid_dpstr_archive).is_null());
    assert!(
        last_error_string()
            .expect("invalid UTF-8 DpStr archive decode should set last error")
            .contains("utf-8")
    );

    let invalid_dpstring_archive = DatapodArchiveFrame {
        type_hash: datapod_emitted_type_hash(datapod_dpstring_type_hash()),
        header: std::ptr::null(),
        header_len: 0,
        payload: invalid_utf8.as_ptr(),
        payload_len: invalid_utf8.len(),
    };
    let mut stale_dpstring_view = DatapodDpStringView {
        bytes: DatapodBytes {
            ptr: dangling,
            len: 100,
        },
    };
    assert!(!datapod_dpstring_view_from_archive(
        invalid_dpstring_archive,
        &mut stale_dpstring_view,
    ));
    assert_eq!(stale_dpstring_view, DatapodDpStringView::default());
    assert!(
        last_error_string()
            .expect("invalid UTF-8 DpString archive view should set last error")
            .contains("utf-8")
    );
    assert!(datapod_dpstring_from_archive(invalid_dpstring_archive).is_null());
    assert!(
        last_error_string()
            .expect("invalid UTF-8 DpString archive decode should set last error")
            .contains("utf-8")
    );
}

#[test]
fn ffi_frame_validation_rejects_bad_shapes_before_touching_frame_pointers() {
    let dangling = std::ptr::NonNull::<u8>::dangling().as_ptr();
    let point_hash = datapod_emitted_type_hash(datapod_point_type_hash());

    let unknown_message = datapod_wire_message_borrow(u64::MAX, dangling, 1);
    assert!(!datapod_wire_message_validate_v1(unknown_message));
    assert!(!datapod_last_error_message().is_null());

    let short_message = datapod_wire_message_borrow(
        point_hash,
        dangling,
        datapod_point_header_size().saturating_sub(1),
    );
    assert!(!datapod_wire_message_validate_v1(short_message));
    assert!(!datapod_last_error_message().is_null());

    let wrong_header_len = DatapodArchiveFrame {
        type_hash: point_hash,
        header: dangling,
        header_len: datapod_point_header_size() + 1,
        payload: std::ptr::null(),
        payload_len: 0,
    };
    assert!(!datapod_archive_validate_as(
        point_hash ^ 1,
        wrong_header_len
    ));
    assert!(
        last_error_string()
            .expect("wrong archive type should set last error before frame slicing")
            .contains("wrong datapod frame type hash"),
        "validate_as should reject type mismatch before inspecting malformed frame pointers"
    );
    assert!(!datapod_archive_validate(wrong_header_len));
    assert!(!datapod_last_error_message().is_null());

    let huge_total_len = DatapodArchiveFrame {
        type_hash: point_hash,
        header: dangling,
        header_len: datapod_point_header_size(),
        payload: dangling,
        payload_len: usize::MAX,
    };
    assert!(!datapod_archive_validate(huge_total_len));
    assert!(!datapod_last_error_message().is_null());

    let bytes_hash = datapod_emitted_type_hash(datapod_bytes_value_type_hash());
    let wrong_heap_header_len = DatapodArchiveFrame {
        type_hash: bytes_hash,
        header: dangling,
        header_len: datapod_bytes_value_header_size() + 1,
        payload: dangling,
        payload_len: 4,
    };
    assert!(!datapod_bytes_value_view_from_archive(
        wrong_heap_header_len,
        &mut DatapodBytesView::default()
    ));
    assert!(!datapod_last_error_message().is_null());

    let matrix_hash = datapod_emitted_type_hash(datapod_matrix_type_hash());
    let wrong_matrix_header_len = DatapodArchiveFrame {
        type_hash: matrix_hash,
        header: dangling,
        header_len: datapod_matrix_header_size() + 1,
        payload: dangling,
        payload_len: 4,
    };
    let mut stale_matrix_view = DatapodMatrixView {
        rows: 99,
        cols: 88,
        element_size: 77,
        payload: DatapodBytes {
            ptr: dangling,
            len: 66,
        },
    };
    assert!(!datapod_matrix_view_from_archive(
        wrong_matrix_header_len,
        &mut stale_matrix_view,
    ));
    assert_eq!(stale_matrix_view, DatapodMatrixView::default());
    assert!(!datapod_last_error_message().is_null());

    let grid_hash = datapod_emitted_type_hash(datapod_grid_type_hash());
    let wrong_grid_header_len = DatapodArchiveFrame {
        type_hash: grid_hash,
        header: dangling,
        header_len: datapod_grid_header_size() + 1,
        payload: dangling,
        payload_len: 4,
    };
    let mut stale_grid_view = DatapodGridView {
        rows: 99,
        cols: 88,
        encoding: 77,
        centered: true,
        resolution: 1.0,
        pose: DatapodPose::default(),
        data: DatapodBytes {
            ptr: dangling,
            len: 66,
        },
    };
    assert!(!datapod_grid_view_from_archive(
        wrong_grid_header_len,
        &mut stale_grid_view,
    ));
    assert_eq!(stale_grid_view, DatapodGridView::default());
    assert!(!datapod_last_error_message().is_null());

    let mut joined = DatapodOwnedBytes {
        ptr: std::ptr::null_mut(),
        len: 0,
        capacity: 0,
    };
    assert!(!datapod_wire_message_join_v1(
        bytes_hash,
        dangling,
        datapod_bytes_value_header_size(),
        dangling,
        usize::MAX,
        &mut joined,
    ));
    assert!(!datapod_last_error_message().is_null());

    let mut unknown_join_output = DatapodOwnedBytes {
        ptr: dangling,
        len: 44,
        capacity: 44,
    };
    assert!(!datapod_wire_message_join_v1(
        u64::MAX,
        dangling,
        usize::MAX,
        dangling,
        usize::MAX,
        &mut unknown_join_output,
    ));
    assert!(
        unknown_join_output.ptr.is_null()
            && unknown_join_output.len == 0
            && unknown_join_output.capacity == 0,
        "unknown-type join must clear stale output before returning"
    );
    assert!(
        last_error_string()
            .expect("unknown-type join should report before touching raw pointers")
            .contains("unknown datapod type hash"),
        "unknown-type join should reject type metadata before borrowing dangling input"
    );

    let mut wrong_header_join_output = DatapodOwnedBytes {
        ptr: dangling,
        len: 45,
        capacity: 45,
    };
    assert!(!datapod_wire_message_join_v1(
        point_hash,
        dangling,
        datapod_point_header_size().saturating_sub(1),
        dangling,
        usize::MAX,
        &mut wrong_header_join_output,
    ));
    assert!(
        wrong_header_join_output.ptr.is_null()
            && wrong_header_join_output.len == 0
            && wrong_header_join_output.capacity == 0,
        "wrong-header join must clear stale output before returning"
    );
    assert!(
        last_error_string()
            .expect("wrong-header join should report before touching raw pointers")
            .contains("wrong v1 header length"),
        "wrong-header join should reject shape before borrowing dangling input"
    );

    let paged_hash = datapod_emitted_type_hash(datapod_paged_vecvec_type_hash());
    let mut paged_header = Vec::new();
    paged_header.extend_from_slice(&1_u32.to_le_bytes());
    paged_header.extend_from_slice(&0_u32.to_le_bytes());
    assert_eq!(paged_header.len(), datapod_paged_vecvec_header_size());
    let short_paged_payload = [0_u8, 0, 0];
    let mut paged_wire = paged_header.clone();
    paged_wire.extend_from_slice(&short_paged_payload);
    let mut paged_view = DatapodPagedVecvecView::default();
    assert!(!datapod_paged_vecvec_view_from_wire(
        paged_wire.as_ptr(),
        paged_wire.len(),
        &mut paged_view,
    ));
    assert!(!datapod_last_error_message().is_null());

    let bad_paged_frame = DatapodArchiveFrame {
        type_hash: paged_hash,
        header: paged_header.as_ptr(),
        header_len: paged_header.len(),
        payload: short_paged_payload.as_ptr(),
        payload_len: short_paged_payload.len(),
    };
    assert!(!datapod_paged_vecvec_view_from_frame(
        bad_paged_frame,
        &mut paged_view,
    ));
    assert!(!datapod_last_error_message().is_null());
}
