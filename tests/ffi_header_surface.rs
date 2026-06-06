//! C ABI surface regression tests.
//!
//! The generated `include/datapod.h` is the contract C users compile against.
//! This test catches accidental drift where a public no-mangle Rust FFI symbol
//! exists in `src/ffi.rs` but the checked-in header no longer declares it.

fn exported_ffi_functions() -> Vec<&'static str> {
    const FFI_RS: &str = include_str!("../src/ffi.rs");
    const MARKER: &str = r#"pub extern "C" fn "#;

    let mut names = Vec::new();
    for line in FFI_RS.lines() {
        let Some(rest) = line.split_once(MARKER).map(|(_, rest)| rest) else {
            continue;
        };
        let Some(name) = rest
            .split(|ch: char| ch == '(' || ch.is_whitespace())
            .next()
        else {
            continue;
        };
        if name.is_empty() || name.contains('$') {
            continue;
        }
        names.push(name);
    }
    names.sort_unstable();
    names.dedup();
    names
}

#[test]
fn checked_in_c_header_declares_all_direct_exported_ffi_functions() {
    const HEADER: &str = include_str!("../include/datapod.h");

    let missing: Vec<_> = exported_ffi_functions()
        .into_iter()
        .filter(|name| !HEADER.contains(&format!("{name}(")))
        .collect();

    assert!(
        missing.is_empty(),
        "include/datapod.h is missing declarations for exported FFI symbols: {missing:?}"
    );
}

#[test]
fn checked_in_c_header_exposes_archive_frame_alias_and_core_archive_api() {
    const HEADER: &str = include_str!("../include/datapod.h");

    for needle in [
        "typedef DatapodWireFrame DatapodArchiveFrame;",
        "datapod_archive_frame_borrow(",
        "datapod_archive_frame_from_message(",
        "datapod_archive_split(",
        "datapod_archive_to_message(",
        "datapod_archive_validate(",
        "datapod_archive_is_valid(",
    ] {
        assert!(
            HEADER.contains(needle),
            "include/datapod.h is missing expected archive ABI item {needle:?}"
        );
    }
}

#[test]
fn exported_ffi_constructors_clear_stale_last_error_state() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");
    const MARKER: &str = r#"pub extern "C" fn "#;

    let mut missing = Vec::new();
    for chunk in FFI_RS.split(MARKER).skip(1) {
        let name = chunk
            .split(|ch: char| ch == '(' || ch.is_whitespace())
            .next()
            .expect("exported function chunk has a name");
        if !(name.ends_with("_new")
            || name.ends_with("_new_default")
            || name.ends_with("_identity")
            || name.ends_with("_nil")
            || name.ends_with("_v4")
            || name.ends_with("_borrow"))
        {
            continue;
        }
        let body = chunk
            .split("\n#[unsafe(no_mangle)]")
            .next()
            .unwrap_or(chunk);
        if !body.contains("clear_last_error()") {
            missing.push(name);
        }
    }

    assert!(
        missing.is_empty(),
        "successful C ABI constructors must clear stale last-error state: {missing:?}"
    );
}

#[test]
fn exported_ffi_functions_use_shared_output_preparers_for_typed_outputs() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");
    const MARKER: &str = r#"pub extern "C" fn "#;

    let mut direct_null_output_checks = Vec::new();
    for chunk in FFI_RS.split(MARKER).skip(1) {
        let name = chunk
            .split(|ch: char| ch == '(' || ch.is_whitespace())
            .next()
            .expect("exported function chunk has a name");
        let signature = chunk.split_once('{').map(|(sig, _)| sig).unwrap_or(chunk);
        if !signature.contains("out: *mut") {
            continue;
        }
        let body = chunk
            .split("\n#[unsafe(no_mangle)]")
            .next()
            .unwrap_or(chunk);
        if body.contains("if out.is_null()") {
            direct_null_output_checks.push(name);
        }
    }

    assert!(
        direct_null_output_checks.is_empty(),
        "exported FFI functions should use shared output preparers that clear stale outputs: {direct_null_output_checks:?}"
    );
}

#[test]
fn ffi_shared_output_preparers_zero_caller_outputs_before_validation() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    for (helper, required) in [
        (
            "fn prepare_owned_bytes_output",
            &[
                "if out.is_null()",
                "set_last_error(\"null owned-byte output\")",
                "*out = DatapodOwnedBytes::empty();",
            ][..],
        ),
        (
            "fn prepare_wire_frame_output",
            &[
                "if out.is_null()",
                "set_last_error(format!(\"null {label} output\"))",
                "*out = DatapodWireFrame::empty();",
            ][..],
        ),
        (
            "fn prepare_default_output",
            &[
                "if out.is_null()",
                "set_last_error(format!(\"null {label} output\"))",
                "*out = T::default();",
            ][..],
        ),
    ] {
        let start = FFI_RS
            .find(helper)
            .unwrap_or_else(|| panic!("missing C ABI output preparer {helper}"));
        let body = FFI_RS
            .get(start..start + 900)
            .unwrap_or_else(|| panic!("C ABI output preparer {helper} body window is truncated"));

        assert!(
            required.iter().all(|needle| body.contains(needle)),
            "{helper} must reject null output pointers and clear stale caller output before validation"
        );
    }
}

#[test]
fn exported_ffi_output_writers_prepare_outputs_before_assignment() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");
    const MARKER: &str = r#"pub extern "C" fn "#;

    let mut missing_prepare = Vec::new();
    for chunk in FFI_RS.split(MARKER).skip(1) {
        let name = chunk
            .split(|ch: char| ch == '(' || ch.is_whitespace())
            .next()
            .expect("exported function chunk has a name");
        let signature = chunk.split_once('{').map(|(sig, _)| sig).unwrap_or(chunk);
        if !signature.contains("out: *mut") {
            continue;
        }
        let body = chunk
            .split("\n#[unsafe(no_mangle)]")
            .next()
            .unwrap_or(chunk);
        let Some(write_pos) = body.find("*out =") else {
            continue;
        };

        let required_prepare = if signature.contains("out: *mut DatapodOwnedBytes") {
            "prepare_owned_bytes_output(out)"
        } else if signature.contains("out: *mut DatapodWireFrame")
            || signature.contains("out: *mut DatapodArchiveFrame")
        {
            "prepare_wire_frame_output(out,"
        } else {
            "prepare_default_output(out,"
        };

        let prepared_before_write = body
            .find(required_prepare)
            .is_some_and(|prepare_pos| prepare_pos < write_pos);
        if !prepared_before_write {
            missing_prepare.push((name, required_prepare));
        }
    }

    assert!(
        missing_prepare.is_empty(),
        "exported C ABI functions must clear output structs before assigning through out: {missing_prepare:?}"
    );
}

#[test]
fn ffi_empty_output_sentinals_are_all_zero_or_null() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    for (helper, required) in [
        (
            "impl DatapodWireFrame",
            &[
                "type_hash: 0",
                "header: ptr::null()",
                "header_len: 0",
                "payload: ptr::null()",
                "payload_len: 0",
            ][..],
        ),
        (
            "impl DatapodOwnedBytes",
            &["ptr: ptr::null_mut()", "len: 0", "capacity: 0"][..],
        ),
    ] {
        let start = FFI_RS
            .find(helper)
            .unwrap_or_else(|| panic!("missing C ABI empty sentinel impl {helper}"));
        let body = FFI_RS
            .get(start..start + 500)
            .unwrap_or_else(|| panic!("C ABI empty sentinel impl {helper} window is truncated"));

        assert!(
            required.iter().all(|needle| body.contains(needle)),
            "{helper} empty sentinel must be safe for C callers to inspect/free after failed calls"
        );
    }
}

#[test]
fn ffi_error_string_sanitization_does_not_collect_infallibly() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    let start = FFI_RS
        .find("fn c_string_without_nul(message: String) -> CString")
        .expect("C ABI CString sanitizer should exist");
    let body = &FFI_RS[start..];
    let end = body
        .find("\n}\n\nfn canonical_type_name_in")
        .expect("C ABI CString sanitizer body should be findable");
    let body = &body[..end];

    for needle in [
        "if !raw.contains(&0)",
        "let mut bytes = Vec::new();",
        "bytes.try_reserve_exact(raw.len())",
        "return CString::default();",
        "bytes.extend(raw.into_iter().filter(|b| *b != 0));",
    ] {
        assert!(
            body.contains(needle),
            "C ABI last-error sanitizer should avoid infallible allocation via {needle:?}"
        );
    }

    assert!(
        !body.contains(".collect()"),
        "C ABI last-error sanitizer should not use infallible collect allocation"
    );
}

#[test]
fn ffi_slice_constructors_convert_inputs_with_fallible_reservation() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    let helper_start = FFI_RS
        .find("fn collect_ffi_values<T, U>(items: &[T], label: &str) -> Result<Vec<U>, ()>")
        .expect("C ABI typed slice conversion helper should exist");
    let helper_body = &FFI_RS[helper_start..];
    let helper_end = helper_body
        .find("\n}\n\n#[unsafe(no_mangle)]")
        .expect("C ABI typed slice conversion helper body should be findable");
    let helper_body = &helper_body[..helper_end];

    for needle in [
        "let mut values = Vec::new();",
        "values.try_reserve_exact(items.len())",
        "set_last_error(format!(",
        "values.extend(items.iter().copied().map(U::from));",
        "Ok(values)",
    ] {
        assert!(
            helper_body.contains(needle),
            "C ABI typed slice conversion helper should reserve before copying via {needle:?}"
        );
    }

    for (name, label) in [
        ("datapod_polygon_new", "polygon vertices"),
        ("datapod_linestring_new", "linestring points"),
        ("datapod_multi_point_new", "multi_point points"),
        ("datapod_ring_new", "ring points"),
        ("datapod_path_new", "path waypoints"),
        ("datapod_trajectory_new", "trajectory states"),
    ] {
        let marker = format!("pub extern \"C\" fn {name}");
        let start = FFI_RS
            .find(&marker)
            .unwrap_or_else(|| panic!("C ABI constructor {name} should exist"));
        let body = &FFI_RS[start..];
        let end = body
            .find("\n#[unsafe(no_mangle)]")
            .unwrap_or_else(|| panic!("C ABI constructor {name} body should be findable"));
        let body = &body[..end];

        assert!(
            body.contains("collect_ffi_values(") && body.contains(label),
            "C ABI constructor {name} should use collect_ffi_values for {label}"
        );
        assert!(
            !body.contains(".collect()"),
            "C ABI constructor {name} should not collect converted inputs infallibly"
        );
    }
}

#[test]
fn ffi_raw_byte_clones_use_fallible_reservation() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    let start = FFI_RS
        .find("fn clone_bytes(ptr: *const u8, len: usize) -> Option<Vec<u8>>")
        .expect("C ABI byte clone helper should exist");
    let body = &FFI_RS[start..];
    let end = body
        .find("\n}\n\nfn ffi_type_hash")
        .expect("C ABI byte clone helper body should be findable");
    let body = &body[..end];

    for needle in [
        "let mut out = Vec::new();",
        "out.try_reserve_exact(bytes.len())",
        "failed to reserve {} copied bytes",
        "out.extend_from_slice(bytes);",
        "Some(out)",
    ] {
        assert!(
            body.contains(needle),
            "C ABI byte clone helper should reserve fallibly before copying via {needle:?}"
        );
    }

    assert!(
        !body.contains("bytes.to_vec()"),
        "C ABI byte clone helper should not use infallible to_vec"
    );
}

#[test]
fn ffi_fixed_and_pod_helpers_use_checked_copy_ranges() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    for needle in [
        "output.get_mut(..header_len)",
        "out.get_mut(..bytes.len())",
        "bytes.get(..std::mem::size_of::<R>())",
        "slot.copy_from_slice(input);",
        "slot.copy_from_slice(bytes);",
    ] {
        assert!(
            FFI_RS.contains(needle),
            "C ABI fixed/POD helpers should use checked copy/read range {needle:?}"
        );
    }

    for forbidden in [
        "output[..header_len].copy_from_slice(input)",
        "out[..bytes.len()].copy_from_slice(bytes)",
        "pod_read_unaligned::<R>(&bytes[..std::mem::size_of::<R>()])",
    ] {
        assert!(
            !FFI_RS.contains(forbidden),
            "C ABI fixed/POD helpers should not retain direct range pattern {forbidden:?}"
        );
    }
}

#[test]
fn ffi_raw_slice_creation_stays_centralized_and_range_checked() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    let slice_raw_parts = FFI_RS.matches("std::slice::from_raw_parts").count();
    assert_eq!(
        slice_raw_parts, 3,
        "FFI slice construction should stay centralized in typed_slice_in/bytes_in/bytes_out"
    );

    for (helper, required) in [
        (
            "unsafe fn typed_slice_in",
            "validate_pointer_range(ptr.cast::<u8>(), byte_len, label)?",
        ),
        (
            "unsafe fn bytes_out",
            "validate_pointer_range(ptr.cast_const(), len, \"output byte array\")?",
        ),
        (
            "unsafe fn bytes_in",
            "validate_pointer_range(ptr, len, \"input byte array\")?",
        ),
    ] {
        let start = FFI_RS
            .find(helper)
            .unwrap_or_else(|| panic!("missing FFI slice helper {helper}"));
        let body = &FFI_RS[start..];
        let end = body
            .find("\n}\n\n")
            .unwrap_or_else(|| panic!("could not find end of FFI slice helper {helper}"));
        let body = &body[..end];
        assert!(
            body.contains(required),
            "{helper} must validate pointer-range overflow before creating Rust slices"
        );
    }
}

#[test]
fn ffi_raw_slice_helpers_validate_before_raw_slice_creation() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    for (helper, slice_call, required_before_slice) in [
        (
            "unsafe fn typed_slice_in",
            "std::slice::from_raw_parts(ptr, len)",
            &[
                "len.checked_mul(std::mem::size_of::<T>())",
                "byte_len > isize::MAX as usize",
                "validate_pointer_range(ptr.cast::<u8>(), byte_len, label)?",
                "(ptr as usize) % align != 0",
            ][..],
        ),
        (
            "unsafe fn bytes_out",
            "std::slice::from_raw_parts_mut(ptr, len)",
            &[
                "len > isize::MAX as usize",
                "if ptr.is_null()",
                "validate_pointer_range(ptr.cast_const(), len, \"output byte array\")?",
            ][..],
        ),
        (
            "unsafe fn bytes_in",
            "std::slice::from_raw_parts(ptr, len)",
            &[
                "len > isize::MAX as usize",
                "if ptr.is_null()",
                "validate_pointer_range(ptr, len, \"input byte array\")?",
            ][..],
        ),
    ] {
        let start = FFI_RS
            .find(helper)
            .unwrap_or_else(|| panic!("missing FFI slice helper {helper}"));
        let body = &FFI_RS[start..];
        let end = body
            .find("\n}\n\n")
            .unwrap_or_else(|| panic!("could not find end of FFI slice helper {helper}"));
        let body = &body[..end];
        let slice_pos = body
            .find(slice_call)
            .unwrap_or_else(|| panic!("{helper} should contain {slice_call}"));

        for required in required_before_slice {
            let required_pos = body
                .find(required)
                .unwrap_or_else(|| panic!("{helper} missing pre-slice guard {required:?}"));
            assert!(
                required_pos < slice_pos,
                "{helper} must run {required:?} before creating a raw Rust slice"
            );
        }
    }
}

#[test]
fn ffi_canonical_name_byte_slice_apis_are_bounded_and_exported() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");
    const HEADER: &str = include_str!("../include/datapod.h");

    let helper = "fn canonical_type_name_bytes_in";
    let start = FFI_RS
        .find(helper)
        .unwrap_or_else(|| panic!("missing length-bounded canonical-name helper"));
    let body = &FFI_RS[start..];
    let end = body
        .find("\n}\n\n")
        .unwrap_or_else(|| panic!("could not find end of length-bounded canonical-name helper"));
    let body = &body[..end];
    for required in [
        "bytes_in(name, name_len)",
        "bytes.is_empty()",
        "bytes.contains(&0)",
        "std::str::from_utf8(bytes)",
    ] {
        assert!(
            body.contains(required),
            "length-bounded canonical-name helper should contain {required:?}"
        );
    }

    for name in [
        "datapod_type_hash_name_bytes",
        "datapod_register_type_bytes",
        "datapod_register_type_name_bytes",
        "datapod_type_exists_name_bytes",
    ] {
        assert!(
            FFI_RS.contains(&format!("pub extern \"C\" fn {name}")),
            "Rust C ABI should export {name}"
        );
        assert!(
            HEADER.contains(&format!("{name}(")),
            "public C header should declare {name}"
        );
    }
}

#[test]
fn ffi_frame_entrypoints_validate_shape_before_borrowing_frame_slices() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    for (name, guard, slice_calls) in [
        (
            "datapod_wire_frame_validate_v1",
            "prevalidate_wire_frame_shape(frame).is_err()",
            &[
                "bytes_in(frame.header, frame.header_len)",
                "bytes_in(frame.payload, frame.payload_len)",
            ][..],
        ),
        (
            "datapod_fixed_value_from_archive",
            "datapod_archive_validate_as(type_hash, frame)",
            &["bytes_in(frame.header, frame.header_len)"][..],
        ),
    ] {
        let marker = format!("pub extern \"C\" fn {name}");
        let start = FFI_RS
            .find(&marker)
            .unwrap_or_else(|| panic!("missing C ABI frame entrypoint {name}"));
        let body = &FFI_RS[start..];
        let end = body
            .find("\n}\n\n")
            .unwrap_or_else(|| panic!("could not find end of C ABI frame entrypoint {name}"));
        let body = &body[..end];
        let guard_pos = body
            .find(guard)
            .unwrap_or_else(|| panic!("{name} should contain frame-shape guard {guard:?}"));
        for slice_call in slice_calls {
            let slice_pos = body
                .find(slice_call)
                .unwrap_or_else(|| panic!("{name} should borrow frame slice via {slice_call:?}"));
            assert!(
                guard_pos < slice_pos,
                "{name} must validate frame shape before borrowing raw frame pointers"
            );
        }
    }

    let helper = "fn wire_frame_in";
    let start = FFI_RS
        .find(helper)
        .unwrap_or_else(|| panic!("missing FFI frame input helper {helper}"));
    let body = &FFI_RS[start..];
    let end = body
        .find("\n}\n\n")
        .unwrap_or_else(|| panic!("could not find end of FFI frame input helper {helper}"));
    let body = &body[..end];
    let guard_pos = body
        .find("prevalidate_wire_frame_shape(frame)?")
        .unwrap_or_else(|| panic!("{helper} should prevalidate frame shape"));
    for slice_call in [
        "bytes_in(frame.header, frame.header_len)",
        "bytes_in(frame.payload, frame.payload_len)",
    ] {
        let slice_pos = body
            .find(slice_call)
            .unwrap_or_else(|| panic!("{helper} should borrow frame slice via {slice_call:?}"));
        assert!(
            guard_pos < slice_pos,
            "{helper} must validate registered frame shape before borrowing raw frame pointers"
        );
    }
}

#[test]
fn ffi_join_entrypoints_validate_shape_before_borrowing_input_slices() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    for name in ["datapod_wire_message_join", "datapod_wire_message_join_v1"] {
        let marker = format!("pub extern \"C\" fn {name}");
        let start = FFI_RS
            .find(&marker)
            .unwrap_or_else(|| panic!("missing C ABI join entrypoint {name}"));
        let body = &FFI_RS[start..];
        let end = body
            .find("\n}\n\n")
            .unwrap_or_else(|| panic!("could not find end of C ABI join entrypoint {name}"));
        let body = &body[..end];
        let header_borrow = body
            .find("bytes_in(header, header_len)")
            .unwrap_or_else(|| panic!("{name} should borrow the header via bytes_in"));
        let payload_borrow = body
            .find("bytes_in(payload, payload_len)")
            .unwrap_or_else(|| panic!("{name} should borrow the payload via bytes_in"));

        for guard in [
            "prepare_owned_bytes_output(out).is_err()",
            "!crate::registry::type_exists(type_hash)",
            "header_len != expected_header_len",
            "datapod_payload_kind(type_hash) == datapod_payload_kind_fixed()",
            "prevalidate_joined_wire_len(header_len, payload_len).is_err()",
        ] {
            let guard_pos = body
                .find(guard)
                .unwrap_or_else(|| panic!("{name} should contain join guard {guard:?}"));
            assert!(
                guard_pos < header_borrow && guard_pos < payload_borrow,
                "{name} must run {guard:?} before borrowing raw header/payload pointers"
            );
        }
    }
}

#[test]
fn ffi_owned_bytes_vec_reconstruction_is_range_checked() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    assert_eq!(
        FFI_RS.matches("Vec::from_raw_parts").count(),
        2,
        "owned-byte Vec reconstruction should stay limited to the public free path and internal cleanup path"
    );

    for (helper, required) in [
        (
            "fn drop_owned_bytes_preserving_last_error",
            &["pointer_range_overflows(bytes.ptr.cast_const(), bytes.capacity)"][..],
        ),
        (
            "pub extern \"C\" fn datapod_owned_bytes_free",
            &[
                "validate_pointer_range(",
                "bytes.ptr.cast_const()",
                "bytes.capacity",
                "\"owned bytes allocation\"",
            ][..],
        ),
    ] {
        let start = FFI_RS
            .find(helper)
            .unwrap_or_else(|| panic!("missing owned-bytes helper {helper}"));
        let body = FFI_RS
            .get(start..start + 1200)
            .unwrap_or_else(|| panic!("owned-bytes helper {helper} body window is truncated"));
        assert!(
            required.iter().all(|required| body.contains(required)),
            "{helper} must validate allocation pointer-range overflow before Vec::from_raw_parts"
        );
    }
}

#[test]
fn ffi_borrowed_message_payload_derivation_avoids_unsafe_pointer_add() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    assert!(
        !FFI_RS.contains("message.data.add("),
        "borrowed message payload helpers should avoid unsafe raw-pointer add; validate first and use wrapping_add for address derivation"
    );
    assert!(
        FFI_RS.contains("message.data.wrapping_add(header_len)"),
        "borrowed message payload helpers should derive payload pointers with wrapping_add after validation"
    );
}

#[test]
fn ffi_canonical_c_string_decoding_stays_centralized() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    assert!(
        FFI_RS.contains("fn canonical_type_name_in"),
        "C ABI canonical type name decoding should have one shared helper"
    );
    assert_eq!(
        FFI_RS.matches("CStr::from_ptr").count(),
        1,
        "C ABI canonical type name decoding should stay centralized so null/utf8/empty checks remain consistent"
    );
    assert!(
        FFI_RS.contains("datapod canonical type name is empty"),
        "C ABI canonical type names should reject empty strings before hashing or lookup"
    );
}

#[test]
fn ffi_metadata_constant_helpers_clear_last_error() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    for name in [
        "datapod_hash_kind_canonical_name",
        "datapod_current_wire_format_name",
        "datapod_builtin_hash_policy",
        "datapod_payload_kind_fixed",
        "datapod_payload_kind_bytes",
        "datapod_endian_little",
        "datapod_alignment_unaligned_wire",
        "datapod_validator_registry_only",
        "datapod_validator_builtin",
        "datapod_validator_runtime_schema",
        "datapod_archive_shape_fixed",
        "datapod_archive_shape_single_payload",
        "datapod_archive_shape_segmented_payload",
        "datapod_archive_shape_runtime_schema",
    ] {
        let marker = format!("pub extern \"C\" fn {name}");
        let start = FFI_RS
            .find(&marker)
            .unwrap_or_else(|| panic!("missing metadata constant helper {name}"));
        let body = &FFI_RS[start..];
        let end = body
            .find("\n}\n\n")
            .unwrap_or_else(|| panic!("could not find end of metadata constant helper {name}"));
        let body = &body[..end];
        assert!(
            body.contains("clear_last_error();"),
            "{name} should clear stale C ABI last-error state on success"
        );
    }
}

#[test]
fn ffi_borrowed_byte_view_exports_clear_errors_and_empty_on_failure() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");
    const MARKER: &str = r#"pub extern "C" fn "#;

    let mut checked = 0usize;
    let mut missing_clear = Vec::new();
    let mut missing_empty_failure = Vec::new();

    for chunk in FFI_RS.split(MARKER).skip(1) {
        let name = chunk
            .split(|ch: char| ch == '(' || ch.is_whitespace())
            .next()
            .expect("exported function chunk has a name");
        let body = chunk
            .split("\n#[unsafe(no_mangle)]")
            .next()
            .unwrap_or(chunk);
        let signature = body.split_once('{').map(|(sig, _)| sig).unwrap_or(body);
        if !signature.contains("-> DatapodBytes") {
            continue;
        }

        checked += 1;
        if !body.contains("clear_last_error();") {
            missing_clear.push(name);
        }
        if !body.contains("DatapodBytes::empty()") {
            missing_empty_failure.push(name);
        }
    }

    assert!(
        missing_clear.is_empty(),
        "borrowed byte-view exports must clear stale last-error state before returning: {missing_clear:?}"
    );
    assert!(
        missing_empty_failure.is_empty(),
        "borrowed byte-view exports must return the null/zero sentinel on validation or handle failures: {missing_empty_failure:?}"
    );
    assert!(
        checked >= 30,
        "expected to inspect all public borrowed byte-view exports"
    );
}

#[test]
fn ffi_type_hash_and_size_helpers_route_through_error_clearing_helpers() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    for helper in [
        "fn ffi_type_hash<T: DataPod>() -> u64",
        "fn ffi_rust_type_hash<T: 'static>() -> u64",
        "fn ffi_header_size<T: DataPod>() -> usize",
        "fn ffi_pod_byte_size<T>() -> usize",
    ] {
        let start = FFI_RS
            .find(helper)
            .unwrap_or_else(|| panic!("missing shared metadata helper {helper}"));
        let body = &FFI_RS[start..];
        let end = body
            .find("\n}\n\n")
            .unwrap_or_else(|| panic!("could not find end of shared metadata helper {helper}"));
        let body = &body[..end];
        assert!(
            body.contains("clear_last_error();"),
            "{helper} should clear stale C ABI last-error state before returning metadata"
        );
    }

    let fixed_macro = FFI_RS
        .split("macro_rules! fixed_wire_fns")
        .nth(1)
        .expect("missing fixed_wire_fns macro");
    assert!(
        fixed_macro.contains("ffi_type_hash::<$rust_ty>()")
            && fixed_macro.contains("ffi_header_size::<$rust_ty>()"),
        "fixed_wire_fns metadata exports should route through shared error-clearing helpers"
    );

    for required in [
        "ffi_type_hash::<Size>()",
        "ffi_header_size::<Size>()",
        "ffi_type_hash::<PointKey>()",
        "ffi_header_size::<PointKey>()",
        "ffi_type_hash::<JointDynamics>()",
        "ffi_header_size::<JointDynamics>()",
        "ffi_type_hash::<Robot>()",
        "ffi_header_size::<Robot>()",
        "ffi_type_hash::<Envelope>()",
        "ffi_header_size::<Envelope>()",
        "ffi_type_hash::<Geo>()",
        "ffi_header_size::<Geo>()",
        "ffi_type_hash::<MacAddr>()",
        "ffi_header_size::<MacAddr>()",
        "ffi_type_hash::<DpStr>()",
        "ffi_header_size::<DpStr>()",
        "ffi_type_hash::<Trajectory>()",
        "ffi_header_size::<Trajectory>()",
        "ffi_type_hash::<Tensor>()",
        "ffi_header_size::<Tensor>()",
        "ffi_type_hash::<PagedVecvec>()",
        "ffi_header_size::<PagedVecvec>()",
        "ffi_rust_type_hash::<MapEntry>()",
        "ffi_pod_byte_size::<MapEntry>()",
        "ffi_type_hash::<Polygon>()",
        "ffi_header_size::<Polygon>()",
        "ffi_type_hash::<Grid>()",
        "ffi_header_size::<Grid>()",
        "ffi_type_hash::<Point>()",
        "ffi_header_size::<Point>()",
    ] {
        assert!(
            FFI_RS.contains(required),
            "representative hand-written metadata export should use shared helper {required}"
        );
    }

    let mut checked = 0usize;
    for chunk in FFI_RS.split(r#"pub extern "C" fn "#).skip(1) {
        let name = chunk
            .split(|ch: char| ch == '(' || ch.is_whitespace())
            .next()
            .expect("exported function chunk has a name");
        let body = chunk
            .split("\n#[unsafe(no_mangle)]")
            .next()
            .unwrap_or(chunk);
        let signature = body.split_once('{').map(|(sig, _)| sig).unwrap_or(body);
        if signature.contains(':') {
            continue;
        }
        if name.ends_with("_type_hash") {
            checked += 1;
            assert!(
                body.contains("ffi_type_hash::<") || body.contains("ffi_rust_type_hash::<"),
                "{name} should route through shared type-hash helpers that clear stale last-error"
            );
        } else if name.ends_with("_header_size") {
            checked += 1;
            assert!(
                body.contains("ffi_header_size::<"),
                "{name} should route through shared header-size helpers that clear stale last-error"
            );
        } else if name.ends_with("_byte_size") {
            checked += 1;
            assert!(
                body.contains("ffi_pod_byte_size::<"),
                "{name} should route through shared byte-size helpers that clear stale last-error"
            );
        }
    }
    assert!(
        checked >= 100,
        "expected to inspect all typed metadata exports"
    );
}

#[test]
fn ffi_archive_view_functions_delegate_to_validating_frame_views() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    let mut checked = 0usize;
    let mut rest = FFI_RS;
    while let Some(offset) = rest.find("pub extern \"C\" fn datapod_") {
        rest = &rest[offset..];
        let Some(name_end) = rest.find('(') else {
            break;
        };
        let signature_prefix = &rest[..name_end];
        let name = signature_prefix
            .rsplit_once(' ')
            .map(|(_, name)| name)
            .unwrap_or(signature_prefix);
        let next = rest.find("\n#[unsafe(no_mangle)]").unwrap_or(rest.len());
        let body = &rest[..next];
        rest = &rest[next..];
        if !name.ends_with("_view_from_archive") {
            continue;
        }
        checked += 1;
        let frame_view_name = name.replace("_view_from_archive", "_view_from_frame");
        let expected_call = format!("{frame_view_name}(frame, out)");
        assert!(
            body.contains(&expected_call),
            "{name} must delegate to {frame_view_name} so archive views share frame validation and output clearing"
        );
    }

    assert!(
        checked >= 25,
        "expected to inspect all heap-backed archive view fns"
    );
}

#[test]
fn ffi_archive_owned_decode_functions_route_through_shared_helper() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    let mut checked = 0usize;
    let mut rest = FFI_RS;
    while let Some(offset) = rest.find("pub extern \"C\" fn datapod_") {
        rest = &rest[offset..];
        let Some(name_end) = rest.find('(') else {
            break;
        };
        let signature_prefix = &rest[..name_end];
        let name = signature_prefix
            .rsplit_once(' ')
            .map(|(_, name)| name)
            .unwrap_or(signature_prefix);
        let next = rest.find("\n#[unsafe(no_mangle)]").unwrap_or(rest.len());
        let body = &rest[..next];
        rest = &rest[next..];
        if !name.ends_with("_from_archive")
            || name.ends_with("_view_from_archive")
            || name == "datapod_fixed_value_from_archive"
        {
            continue;
        }
        checked += 1;
        assert!(
            body.contains("heap_handle_from_archive::<"),
            "{name} must route through heap_handle_from_archive so archive owned-decode shares validation, collection, and cleanup"
        );
        assert!(
            !body.contains("datapod_archive_to_message_v1("),
            "{name} must not duplicate archive collection outside heap_handle_from_archive"
        );
    }

    assert!(
        checked >= 25,
        "expected to inspect all heap-backed archive owned-decode fns"
    );
}

#[test]
fn ffi_spatial_query_helpers_clear_last_error_on_success() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    for name in [
        "datapod_point_magnitude",
        "datapod_point_distance_to",
        "datapod_point_distance_to_2d",
        "datapod_geo_is_valid",
        "datapod_geo_distance_to",
        "datapod_geo_bearing_to",
        "datapod_segment_length",
        "datapod_segment_midpoint",
        "datapod_segment_closest_point",
        "datapod_segment_distance_to",
        "datapod_velocity_speed",
        "datapod_acceleration_magnitude",
        "datapod_rectangle_area",
        "datapod_aabb_center",
        "datapod_circle_area",
        "datapod_triangle_area",
    ] {
        let marker = format!("pub extern \"C\" fn {name}");
        let start = FFI_RS
            .find(&marker)
            .unwrap_or_else(|| panic!("missing spatial query helper {name}"));
        let body = &FFI_RS[start..];
        let end = body
            .find("\n}\n\n")
            .unwrap_or_else(|| panic!("could not find end of spatial query helper {name}"));
        let body = &body[..end];
        assert!(
            body.contains("clear_last_error();"),
            "{name} should clear stale C ABI last-error state on success"
        );
    }
}

#[test]
fn ffi_borrowed_byte_view_lengths_use_checked_arithmetic() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    let helper = "fn ffi_byte_len_for_count";
    let start = FFI_RS
        .find(helper)
        .unwrap_or_else(|| panic!("missing checked byte-length helper {helper}"));
    let body = &FFI_RS[start..];
    let end = body
        .find("\n}\n\n")
        .unwrap_or_else(|| panic!("could not find end of checked byte-length helper {helper}"));
    let body = &body[..end];
    assert!(
        body.contains("checked_mul") && body.contains("isize::MAX"),
        "checked byte-length helper should reject multiplication overflow and oversized slices"
    );

    assert!(
        !FFI_RS.contains("vertices.len() * std::mem::size_of::<Point>()"),
        "polygon vertex byte views should not use unchecked len * size arithmetic"
    );
    assert!(
        FFI_RS.contains("ffi_byte_len_for_count(") && FFI_RS.contains("\"polygon vertices\""),
        "polygon vertex byte views should route through checked byte-length arithmetic"
    );
}

#[test]
fn ffi_sequence_view_counts_use_checked_header_conversions() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    for needle in [
        "fn ffi_u32_to_usize<T: 'static>(value: u32, field: &'static str) -> Result<usize, ()>",
        "usize::try_from(value)",
        "fn ffi_element_count<T: 'static>(",
        "ffi_u32_to_usize::<T>(element_size, \"element_size\")?",
        "fn ffi_node_layout<T: 'static>(",
        "element_size.checked_add(8)",
        "fn ffi_indexed_heap_layout(payload: &[u8], priority_size: u32) -> Result<(usize, usize), ()>",
        "ffi_u32_to_usize::<IndexedHeap>(priority_size, \"priority_size\")?",
        "8usize.checked_add(priority_size)",
        "ffi_u32_to_usize::<Deque>(view.header.split_byte, \"split_byte\")",
        "ffi_element_count::<Queue>(payload, view.header.element_size, \"queue\")",
        "ffi_u32_to_usize::<Queue>(view.header.front, \"front\")",
        "raw_count.checked_sub(front)",
        "ffi_node_layout::<List>(payload, view.header.element_size, \"list\")",
        "ffi_node_layout::<ForwardList>(payload, view.header.element_size, \"forward_list\")",
        "ffi_element_count::<Heap>(payload, view.header.element_size, \"heap\")",
        "ffi_indexed_heap_layout(payload, view.header.priority_size)",
    ] {
        assert!(
            FFI_RS.contains(needle),
            "FFI sequence view helpers should use checked conversion/layout path {needle:?}"
        );
    }

    for forbidden in [
        "view.header.split_byte as usize",
        "view.header.element_size as usize",
        "view.header.front as usize",
        "view.header.priority_size as usize",
        "8 + view.header.priority_size as usize",
    ] {
        assert!(
            !FFI_RS.contains(forbidden),
            "FFI sequence view helpers should not retain unchecked cast pattern {forbidden:?}"
        );
    }
}

#[test]
fn ffi_assoc_view_counts_use_checked_u32_outputs() {
    const FFI_RS: &str = include_str!("../src/ffi.rs");

    for needle in [
        "fn ffi_usize_to_u32<T: 'static>(value: usize, field: &'static str) -> Result<u32, ()>",
        "u32::try_from(value)",
        "ffi_usize_to_u32::<Map>(count, \"map count\")",
        "ffi_usize_to_u32::<Set>(count, \"set count\")",
    ] {
        assert!(
            FFI_RS.contains(needle),
            "FFI assoc views should keep checked count output conversion {needle:?}"
        );
    }

    {
        let forbidden = "count: count as u32";
        assert!(
            !FFI_RS.contains(forbidden),
            "FFI assoc views should not retain unchecked count output cast {forbidden:?}"
        );
    }
}

#[test]
fn c_wire_bench_declares_posix_clock_source_before_system_headers() {
    const C_BENCH: &str = include_str!("c_wire_bench.c");

    let posix_define = C_BENCH
        .find("#define _POSIX_C_SOURCE 200809L")
        .expect("C wire bench should request POSIX clock_gettime declarations");
    let time_include = C_BENCH
        .find("#include <time.h>")
        .expect("C wire bench should include time.h");

    assert!(
        posix_define < time_include,
        "C wire bench should define _POSIX_C_SOURCE before including time.h so strict C builds expose clock_gettime"
    );
}
