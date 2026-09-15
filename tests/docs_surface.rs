//! Public documentation surface checks for the wire/archive API.
//!
//! These tests are intentionally small string checks. They catch stale docs
//! around high-risk guarantees: zero-copy wording, Archive/View/Owned naming,
//! and validation strength across Rust/C/Python.

#[test]
fn readme_mentions_archive_view_owned_and_zero_copy_contract() {
    const README: &str = include_str!("../README.md");

    for needle in [
        "Archive/View/Owned",
        "Zero-copy model",
        "DatapodArchiveFrame",
        "obj.archive()",
        "memoryview",
        "OwnedWireMessage",
        "WireMessage",
        "Fallible owned container APIs",
        "no-mutate-on-error",
    ] {
        assert!(README.contains(needle), "README missing {needle:?}");
    }
}

#[test]
fn wire_policy_documents_strong_validation_and_binding_frame_helpers() {
    const WIRE: &str = include_str!("../docs/WIRE_FORMAT.md");

    for needle in [
        "Generic registry validation uses the strongest validator known",
        "fixed-size schemas carry no payload bytes",
        "built-in schemas run their semantic `DataPodValidate` checks",
        "runtime byte-payload schemas prove only registered header shape",
        "datapod_wire_frame_validate_v1",
        "validate_wire_frame_v1",
        "DatapodArchiveFrame",
        "ArchiveFrame` is the primary zero-copy shape",
        "Owned Rust container mutation follows the same trust-boundary rule",
        "try_push",
        "no-mutate-on-error",
    ] {
        assert!(
            WIRE.contains(needle),
            "docs/WIRE_FORMAT.md missing {needle:?}"
        );
    }

    assert!(
        !WIRE.contains("Generic registry validation can only prove"),
        "docs/WIRE_FORMAT.md still contains stale weak-validation wording"
    );
}

#[test]
fn bind_write_header_uses_checked_output_slice() {
    const BIND: &str = include_str!("../src/bind.rs");

    assert!(
        BIND.contains("out.get_mut(..bytes.len())"),
        "bind::write_header should acquire its output range through get_mut"
    );
    assert!(
        !BIND.contains("out[..bytes.len()].copy_from_slice(bytes)"),
        "bind::write_header should not write through direct range indexing"
    );
}

#[test]
fn core_datapod_impls_avoid_obvious_panic_shortcuts() {
    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR"));
    let files = [
        "src/assoc/map.rs",
        "src/assoc/set.rs",
        "src/id/ip.rs",
        "src/id/mac_addr.rs",
        "src/id/string.rs",
        "src/raster/grid.rs",
        "src/raster/layer.rs",
        "src/seq/bitvec.rs",
        "src/seq/bytes.rs",
        "src/seq/deque.rs",
        "src/seq/forward_list.rs",
        "src/seq/heap.rs",
        "src/seq/indexed_heap.rs",
        "src/seq/list.rs",
        "src/seq/matrix.rs",
        "src/seq/mod.rs",
        "src/seq/paged_vecvec.rs",
        "src/seq/queue.rs",
        "src/seq/stack.rs",
        "src/seq/string.rs",
        "src/seq/tensor.rs",
        "src/seq/vector.rs",
        "src/seq/vecvec.rs",
    ];
    let forbidden = [
        concat!(".expect", "("),
        concat!("unwrap", "()"),
        concat!("todo", "!"),
        concat!("unimplemented", "!"),
        concat!("unreachable", "!"),
        concat!("panic", "!("),
    ];

    for file in files {
        let source = std::fs::read_to_string(root.join(file))
            .unwrap_or_else(|err| panic!("failed to read {file}: {err}"));
        for needle in forbidden {
            assert!(
                !source.contains(needle),
                "{file} still contains panic shortcut {needle:?}"
            );
        }
    }
}

#[test]
fn runtime_sources_avoid_direct_range_indexing_regressions() {
    fn visit_rs_files(dir: &std::path::Path, files: &mut Vec<std::path::PathBuf>) {
        for entry in std::fs::read_dir(dir)
            .unwrap_or_else(|err| panic!("failed to read source directory {dir:?}: {err}"))
        {
            let entry = entry.unwrap_or_else(|err| panic!("failed to read directory entry: {err}"));
            let path = entry.path();
            if path.is_dir() {
                visit_rs_files(&path, files);
            } else if path.extension().and_then(|ext| ext.to_str()) == Some("rs") {
                files.push(path);
            }
        }
    }

    fn code_without_comment_or_string(line: &str) -> Option<&str> {
        let trimmed = line.trim_start();
        if trimmed.starts_with("//") {
            return None;
        }
        let code = line.split_once("//").map_or(line, |(code, _)| code);
        if code.contains('"') {
            return None;
        }
        Some(code)
    }

    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR"));
    let mut files = Vec::new();
    visit_rs_files(&root.join("src"), &mut files);
    visit_rs_files(&root.join("crates/datapod-macros/src"), &mut files);

    let mut hits = Vec::new();
    for path in files {
        let source = std::fs::read_to_string(&path)
            .unwrap_or_else(|err| panic!("failed to read source file {path:?}: {err}"));
        for (line_index, line) in source.lines().enumerate() {
            let Some(code) = code_without_comment_or_string(line) else {
                continue;
            };
            let has_range_index = code.contains('[') && code.contains("..") && code.contains(']');
            let has_range_copy = code.contains("].copy_from_slice");
            let has_pod_read_from_index = code.contains("pod_read_unaligned(&") && has_range_index;
            if has_range_index || has_range_copy || has_pod_read_from_index {
                let relative = path.strip_prefix(root).unwrap_or(&path);
                hits.push(format!(
                    "{}:{}:{}",
                    relative.display(),
                    line_index + 1,
                    line.trim()
                ));
            }
        }
    }

    assert!(
        hits.is_empty(),
        "runtime source should avoid direct range indexing; use checked get/get_mut or iterators instead: {hits:#?}"
    );
}

#[test]
fn ffi_python_registry_and_polygon_avoid_obvious_panic_shortcuts() {
    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR"));
    let files = [
        "src/ffi.rs",
        "src/geom/polygon.rs",
        "src/python/mod.rs",
        "src/registry.rs",
    ];
    let forbidden = [
        concat!(".expect", "("),
        concat!("unwrap", "()"),
        concat!("todo", "!"),
        concat!("unimplemented", "!"),
        concat!("unreachable", "!"),
        concat!("panic", "!("),
    ];

    for file in files {
        let source = std::fs::read_to_string(root.join(file))
            .unwrap_or_else(|err| panic!("failed to read {file}: {err}"));
        for needle in forbidden {
            assert!(
                !source.contains(needle),
                "{file} still contains panic shortcut {needle:?}"
            );
        }
    }
}

#[test]
fn python_archive_alias_installer_checks_callable_hooks_before_dispatch() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "if not callable(to_wire_frame):",
        "if not callable(to_wire_frame_v1):",
        "if not callable(from_wire_frame):",
        "if not callable(view_from_wire_frame):",
        "if not callable(from_wire):",
        "_require_wire_frame_arg(frame, \"archive frame\")",
        "_require_wire_frame_arg(frame, \"wire frame\")",
        "cls.from_wire_frame = make_from_wire_frame(cls)",
        "cls.from_wire_frame_v1 = make_from_wire_frame(cls)",
        "cls.view_from_wire_frame = make_view_from_wire_frame(cls)",
        "cls.view_from_wire_frame_v1 = make_view_from_wire_frame(cls)",
        "def frame_part_bytes(frame, attr, label):",
        "raise ValueError(f\"{label} must expose {attr}.tobytes()\") from error",
        "raise ValueError(\n                    f\"{_safe_datapod_label(inner_cls)} frame validation failed\"",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python archive alias installer missing guard {needle:?}"
        );
    }
}

#[test]
fn python_declarative_class_frame_methods_reject_non_wireframes_before_attribute_access() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "frame = _require_wire_frame_arg(frame, \"wire frame\")",
        "frame = _require_wire_frame_arg(frame, \"archive frame\")",
        "incoming_hash = _u64_type_hash(incoming_hash)",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python declarative classmethods missing validation guard {needle:?}"
        );
    }
}

#[test]
fn python_generic_frame_helpers_validate_returned_wireframes() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    let start = PYTHON
        .find("def _require_wire_frame_result(value, label, source=None):")
        .expect("Python frame-result helper should exist");
    let body = &PYTHON[start..];
    let end = body
        .find("\n\n\ndef _require_wire_frame_arg")
        .expect("Python frame-result helper body should be findable");
    let body = &body[..end];

    for needle in [
        "must return datapod.WireFrame",
        "_source_validated_declarative_frame(source, value, label)",
        "_validate_wire_frame_shape_v1(value.type_hash, value.header, value.payload)",
        "validate_wire_frame_v1(value.type_hash, value.header, value.payload)",
    ] {
        assert!(
            body.contains(needle),
            "Python generic wire_frame/archive helpers should validate returned frames via {needle:?}"
        );
    }
}

#[test]
fn python_declarative_encode_paths_avoid_duplicate_custom_validation() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "def _make_generated_hook_registry():",
        "\"frame_encoder\": set()",
        "\"message_encoder\": set()",
        "\"wire_decoder\": set()",
        "return func in registries[kind]",
        "def _source_uses_generated_method(source, label, kind):",
        "func = _method_function(class_attr)",
        "bound_attr = getattr(source, label, None)",
        "_method_function(bound_attr) is func",
        "def _method_function(value):",
        "return getattr(value, \"__func__\", value)",
        "def _safe_callable_attr(obj, name):",
        "return value if callable(value) else None",
        "def _source_uses_generated_frame_encoder(source, label):",
        "_is_generated_hook(kind, func)",
        "return _source_uses_generated_method(source, label, \"frame_encoder\")",
        "def _class_type_hash_matches(cls, type_hash):",
        "class_type_hash = getattr(cls, \"TYPE_HASH\")",
        "except Exception:",
        "return _u64_type_hash(class_type_hash) == type_hash",
        "except (TypeError, ValueError):",
        "if _safe_callable_attr(cls, \"__datapod_validate_object__\") is None:",
        "def _safe_type_name(obj):",
        "return type(obj).__name__",
        "def _validation_shell(cls):",
        "return object.__new__(cls)",
        "_safe_type_name(cls)",
        "cannot be allocated for declarative validation",
        "def _set_validation_field(obj, name, value):",
        "object.__setattr__(obj, name, value)",
        "_safe_type_name(obj)",
        "cannot set declarative field",
        "obj = _validation_shell(cls)",
        "_set_validation_field(obj, name, value)",
        "obj = _validation_shell(inner_cls)",
        "if not _class_type_hash_matches(cls, type_hash):",
        "return _class_type_hash_matches(cls, frame.type_hash)",
        "def _join_wire_frame_shape_v1(frame):",
        "return frame.type_hash, _join_wire_frame_shape_v1(frame)",
        "_register_generated_hook(\"frame_encoder\", to_wire_frame)",
        "_register_generated_hook(\"message_encoder\", to_wire_message)",
        "_register_generated_hook(\"wire_decoder\", from_wire_message.__func__)",
        "_require_wire_frame_result(to_wire_frame(), \"to_wire_frame\", obj)",
        "_require_wire_frame_result(archive_method(), \"archive\", obj)",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python declarative encode/archive helpers should avoid duplicate custom validation via {needle:?}"
        );
    }

    for forbidden in [
        "__datapod_generated_frame_encoder__",
        "__datapod_generated_message_encoder__",
        "__datapod_generated_wire_decoder__",
    ] {
        assert!(
            !PYTHON.contains(forbidden),
            "Python generated hook trust should not use forgeable attribute marker {forbidden:?}"
        );
    }
}

#[test]
fn python_wireframe_to_wire_message_validates_before_joining() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    let start = PYTHON
        .find("def _wire_frame_to_wire_message")
        .expect("Python WireFrame.to_wire_message implementation should exist");
    let body = &PYTHON[start..];
    let end = body
        .find("\n\nWireFrame.to_wire_message")
        .expect("Python WireFrame helper body should be findable");
    let body = &body[..end];

    for needle in [
        "def _wire_frame_to_wire_message(self, _validate=validate_wire_frame_v1):",
        "_validate(self.type_hash, self.header, self.payload)",
    ] {
        assert!(
            body.contains(needle),
            "Python WireFrame.to_wire_message should validate before joining via {needle:?}"
        );
    }
}

#[test]
fn rust_backed_python_frame_decoders_normalize_bad_frame_parts_to_value_error() {
    for (file, helper, needles) in [
        (
            "src/python/fixed.rs",
            "fn fixed_frame_header_payload<'py>(",
            &[
                "frame header must support tobytes()",
                "frame header tobytes() must return bytes",
                "frame payload must support tobytes()",
                "frame payload tobytes() must return bytes",
            ][..],
        ),
        (
            "src/python/geometry.rs",
            "fn frame_header_payload<'py>(",
            &[
                "frame header must support tobytes()",
                "frame header tobytes() must return bytes",
                "frame payload must support tobytes()",
                "frame payload tobytes() must return bytes",
            ][..],
        ),
        (
            "src/python/geometry.rs",
            "fn payload_view_object(",
            &["frame validation failed"][..],
        ),
        (
            "src/python/heap.rs",
            "fn frame_header_and_payload<'py>(",
            &[
                "frame header must support tobytes()",
                "frame header tobytes() must return bytes",
            ][..],
        ),
        (
            "src/python/heap.rs",
            "fn frame_payload_to_bytes(",
            &[
                "frame payload must support tobytes()",
                "frame payload tobytes() must return bytes",
            ][..],
        ),
        (
            "src/python/heap.rs",
            "fn payload_view_from_frame(",
            &["frame validation failed"][..],
        ),
    ] {
        let source = std::fs::read_to_string(file)
            .unwrap_or_else(|err| panic!("failed to read {file}: {err}"));
        let start = source
            .find(helper)
            .unwrap_or_else(|| panic!("missing Python frame decoder helper {helper}"));
        let body = &source[start..];
        let end = body.find("\n}\n\n").unwrap_or(body.len());
        let body = &body[..end];

        for needle in needles {
            assert!(
                body.contains(needle),
                "{file} helper {helper} should normalize malformed frame parts via {needle:?}"
            );
        }
    }
}

#[test]
fn rust_backed_python_frame_builders_validate_owned_values_before_exposing_archives() {
    for (file, helper) in [
        (
            "src/python/fixed.rs",
            "fn fixed_wire_frame_object<T>(py: Python<'_>, owner: PyObject, value: &T) -> PyResult<PyObject>",
        ),
        (
            "src/python/geometry.rs",
            "fn frame_object<T>(py: Python<'_>, owner: PyObject, value: &T) -> PyResult<PyObject>",
        ),
        (
            "src/python/heap.rs",
            "fn wire_frame_object<T>(py: Python<'_>, owner: PyObject, value: &T) -> PyResult<PyObject>",
        ),
    ] {
        let source = std::fs::read_to_string(file)
            .unwrap_or_else(|err| panic!("failed to read {file}: {err}"));
        let start = source
            .find(helper)
            .unwrap_or_else(|| panic!("missing Python frame builder helper {helper}"));
        let body = &source[start..];
        let end = body
            .find("\n}\n\n")
            .unwrap_or_else(|| panic!("Python frame builder helper {helper} body is truncated"));
        let body = &body[..end];

        for needle in [
            "DataPodValidate",
            "validate_wire_parts",
            "map_err(|error| PyValueError::new_err(error.to_string()))?",
        ] {
            assert!(
                body.contains(needle),
                "{file} frame builder {helper} should validate owned value before exposing archive via {needle:?}"
            );
        }
    }
}

#[test]
fn python_registration_rejects_empty_canonical_names_before_hashing() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "def _validate_canonical_name(canonical_name):",
        "if canonical_name == \"\":",
        "if \"\\x00\" in canonical_name:",
        "previous_was_dot = True",
        "\"a\" <= ch <= \"z\" or \"0\" <= ch <= \"9\" or ch == \"_\"",
        "datapod canonical type name is empty",
        "datapod canonical type name must not contain NUL bytes",
        "datapod canonical type name must not contain empty dot-separated segments",
        "datapod canonical type name must contain only ASCII lowercase letters",
        "crate::registry::validate_canonical_name(canonical_name)",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python registration path missing canonical-name guard {needle:?}"
        );
    }
}

#[test]
fn python_type_hash_validation_uses_strict_integer_protocol() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "import operator as _datapod_operator",
        "def _strict_index(value, label, description):",
        "raw = value.__index__()",
        "except Exception as error:",
        "if isinstance(raw, bool):",
        "return _datapod_operator.index(raw)",
        "def _metadata_attr(obj, name, default=_MISSING):",
        "label = getattr(obj, \"__name__\")",
        "label = _safe_type_name(obj)",
        "raise ValueError(f\"{label} metadata {name} lookup failed\") from error",
        "value = _strict_index(value, label, \"an unsigned 64-bit integer\")",
        "type_hash_value = _metadata_attr(cls, \"TYPE_HASH\")",
        "canonical_hash = _metadata_attr(cls, \"CANONICAL_TYPE_HASH\", None)",
        "_metadata_attr(cls, \"__datapod_fields__\", ())",
        "_metadata_attr(cls, \"__datapod_field_arities__\", ())",
        "\"canonical_name\": _metadata_attr(cls, \"__datapod_canonical_name__\", None)",
        "\"header_format\": _metadata_attr(cls, \"__datapod_header_format__\", None)",
        "\"payload_field\": _metadata_attr(cls, \"__datapod_payload_field__\", None)",
        "or _safe_callable_attr(target, \"__index__\") is not None",
        "_datapod_raw_register_schema",
        "_datapod_raw_validate_wire_message_v1",
        "_datapod_raw_decode_as",
        "_datapod_raw_from_wire_message",
        "def register_schema(canonical_name, header_size, payload_kind, type_hash=None):",
        "register_type cls must be a datapod class",
        "def validate_wire_message_v1(",
        "def decode_as(cls, type_hash, wire,",
        "def from_wire_message(type_hash, wire,",
        "decode_as cls must be a datapod class",
        "class_type_hash = _metadata_attr(cls, \"TYPE_HASH\")",
        "expected = _u64_type_hash(class_type_hash)",
        "wrong decode_as type hash",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python type-hash validation should reject bool/float coercion via {needle:?}"
        );
    }
}

#[test]
fn python_public_wire_helpers_require_byte_buffers_before_raw_dispatch() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "_byte_memoryview(wire, \"wire message\")",
        "_byte_memoryview(header, \"wire message header\")",
        "_byte_memoryview(b\"\" if payload is None else payload, \"wire message payload\")",
        "_byte_memoryview(header, \"wire frame header\")",
        "_byte_memoryview(payload, \"wire frame payload\")",
        "_datapod_raw_split_wire_message(",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python public wire helper should reject non-byte or non-contiguous buffers via {needle:?}"
        );
    }
}

#[test]
fn python_generic_encode_return_type_hash_uses_strict_u64_protocol() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "fn extract_wire_message_return",
        "PyBool",
        "type_hash.is_instance_of::<PyBool>()",
        "type_hash must be an unsigned 64-bit integer, not bool",
        "type_hash must fit in an unsigned 64-bit integer",
        "returned invalid wire message",
        "crate::validate_registered_wire_v1(type_hash, &wire)",
        "fn extract_contiguous_bytes_like",
        "memoryview",
        "c_contiguous",
        "bytes must be a C-contiguous byte buffer",
        "extract_contiguous_bytes_like(tuple.get_item(1)?, label)",
        "_source_validated_declarative_message",
        "_run_registered_declarative_validator",
        "SOURCE_VALIDATED_DECLARATIVE_MESSAGE",
        "RUN_REGISTERED_DECLARATIVE_VALIDATOR",
        "datapod declarative validation helper is not initialized",
        "extract_wire_message_return(method.call0()?, \"to_wire_message\", obj)",
        "extract_wire_message_return(method.call0()?, \"to_wire_message_v1\", obj)",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python generic encode return validation should reject bool/float/out-of-range type hashes via {needle:?}"
        );
    }

    for forbidden in [
        "m.add(\n        \"_source_validated_declarative_message\"",
        "m.add(\n        \"_run_registered_declarative_validator\"",
    ] {
        assert!(
            !PYTHON.contains(forbidden),
            "Rust generic encode should call stored helper objects, not mutable public module helper {forbidden:?}"
        );
    }
}

#[test]
fn rust_backed_python_raw_decoders_validate_class_type_hash() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "fn validate_python_class_type_hash",
        "class TYPE_HASH must be an unsigned 64-bit integer, not bool",
        "class TYPE_HASH must fit in an unsigned 64-bit integer",
        "class TYPE_HASH mismatch",
        "validate_python_class_type_hash(cls, type_hash, \"decode_as\")?",
        "validate_python_class_type_hash(&class, type_hash, \"from_wire_message\")?",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Rust-backed Python raw decoder dispatch should validate class TYPE_HASH via {needle:?}"
        );
    }
}

#[test]
fn rust_backed_python_raw_decoders_validate_class_metadata_before_method_dispatch() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for (start_marker, end_marker, validation_needle) in [
        (
            "fn decode_as(cls: &Bound<'_, PyAny>, type_hash: u64, wire: Vec<u8>)",
            "\n\n#[pyfunction]\nfn from_wire_message",
            "validate_python_class_type_hash(cls, type_hash, \"decode_as\")?",
        ),
        (
            "fn from_wire_message(py: Python<'_>, type_hash: u64, wire: Vec<u8>)",
            "\n\n#[pyfunction]\nfn type_hash_name",
            "validate_python_class_type_hash(&class, type_hash, \"from_wire_message\")?",
        ),
    ] {
        let start = PYTHON
            .find(start_marker)
            .unwrap_or_else(|| panic!("Rust-backed Python helper {start_marker:?} should exist"));
        let body = &PYTHON[start..];
        let end = body.find(end_marker).unwrap_or_else(|| {
            panic!("Rust-backed Python helper {start_marker:?} body should end")
        });
        let body = &body[..end];
        let validation_pos = body.find(validation_needle).unwrap_or_else(|| {
            panic!("Rust-backed Python helper {start_marker:?} should validate TYPE_HASH")
        });
        let method_lookup_pos = body
            .find("getattr(\"from_wire_message\")")
            .unwrap_or_else(|| {
                panic!(
                    "Rust-backed Python helper {start_marker:?} should look up from_wire_message"
                )
            });

        assert!(
            validation_pos < method_lookup_pos,
            "Rust-backed Python helper {start_marker:?} must validate class TYPE_HASH before method lookup/dispatch"
        );
    }
}

#[test]
fn rust_backed_python_generic_encoder_validates_buffer_before_wire_validation() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    let start = PYTHON
        .find("fn extract_wire_message_return(")
        .expect("Rust-backed tuple-return extractor should exist");
    let body = &PYTHON[start..];
    let end = body
        .find("\n\nfn extract_contiguous_bytes_like")
        .expect("Rust-backed tuple-return extractor body should end");
    let body = &body[..end];
    let buffer_pos = body
        .find("extract_contiguous_bytes_like(tuple.get_item(1)?, label)?")
        .expect("tuple-return extractor should require contiguous bytes-like buffers");
    let validation_pos = body
        .find("crate::validate_registered_wire_v1(type_hash, &wire)")
        .expect("tuple-return extractor should validate returned wire bytes");

    assert!(
        buffer_pos < validation_pos,
        "Rust-backed tuple-return extractor must prove bytes-like contiguity before registered wire validation"
    );
}

#[test]
fn python_generic_validate_wire_message_runs_declarative_validator() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "def _make_declarative_validator_registry():",
        "registry = {}",
        "def register(",
        "def get(type_hash):",
        "return registry.get(type_hash)",
        "_registered_declarative_validator_type,",
        ") = _make_declarative_validator_registry()",
        "_register_declarative_validator_type(",
        "registered = _registered_lookup(type_hash)",
        "_registered_lookup=_registered_declarative_validator_type",
        "_validate_frame=_run_registered_declarative_frame_validator",
        "_run_declarative=_run_registered_declarative_validator",
        "def validate_wire_message(type_hash, wire, _validate=validate_wire_message_v1):",
        "def validate_wire_frame_v1(type_hash, header, payload=b\"\", _validate=validate_wire_frame):",
        "def _run_registered_declarative_frame_validator(",
        "def _run_registered_declarative_validator(",
        "_metadata_attr(cls, \"__datapod_validator__\", None)",
        "_required_callable_metadata_attr(",
        "\"__datapod_unpack_kwargs__\",",
        "\"__datapod_validate_object__\",",
        "unpack_kwargs(header)",
        "payload.toreadonly()",
        "validate_object(obj)",
        "_run_declarative(type_hash, wire)",
        "_validate_frame(type_hash, header, payload)",
        "def validate_wire_message(type_hash, wire, _validate=validate_wire_message_v1):",
        "validate_wire_message_v1(type_hash, wire)",
    ] {
        assert!(
            PYTHON.contains(needle),
            "generic Python validate_wire_message should run declarative validators via {needle:?}"
        );
    }

    assert!(
        !PYTHON.contains("__datapod_declarative_validator_registry__"),
        "generic Python validation should keep captured validators in a closure-local registry, not a global dict"
    );
}

#[test]
fn python_view_payloads_are_readonly_even_for_mutable_wire_buffers() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for (start_marker, end_marker, needle) in [
        (
            "def _run_registered_declarative_frame_validator(",
            "\n\n\ndef _run_registered_declarative_validator",
            "payload.toreadonly()",
        ),
        (
            "def view_from_wire_v1(inner_cls, incoming_hash, wire):",
            "\n\n        @classmethod\n        def view_from_wire_frame",
            "payload.toreadonly()",
        ),
        (
            "def view_from_wire_frame_v1(inner_cls, frame):",
            "\n\n        @classmethod\n        def view_archive",
            "frame.payload.toreadonly()",
        ),
    ] {
        let start = PYTHON
            .find(start_marker)
            .unwrap_or_else(|| panic!("Python helper {start_marker:?} should exist"));
        let body = &PYTHON[start..];
        let end = body
            .find(end_marker)
            .unwrap_or_else(|| panic!("Python helper {start_marker:?} body should end"));
        let body = &body[..end];

        assert!(
            body.contains(needle),
            "Python helper {start_marker:?} should expose borrowed payloads as read-only views via {needle:?}"
        );
    }
}

#[test]
fn python_public_split_and_decode_helpers_run_full_declarative_validation() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for (start_marker, end_marker) in [
        (
            "def split_wire_message(type_hash, wire,",
            "\n\n\ndef split_wire_message_v1",
        ),
        (
            "def split_wire_message_v1(type_hash, wire,",
            "\n\n\ndef validate_wire_frame_parts_v1",
        ),
        (
            "def decode_as(cls, type_hash, wire,",
            "\n\n\ndef from_wire_message",
        ),
        (
            "def from_wire_message(type_hash, wire,",
            "\n\n\nclass WireFrame",
        ),
    ] {
        let start = PYTHON
            .find(start_marker)
            .unwrap_or_else(|| panic!("Python helper {start_marker:?} should exist"));
        let body = &PYTHON[start..];
        let end = body
            .find(end_marker)
            .unwrap_or_else(|| panic!("Python helper {start_marker:?} body should end"));
        let body = &body[..end];

        assert!(
            body.contains("validate_wire_message_v1(type_hash, wire)")
                || body.contains("_validate(type_hash, wire)"),
            "Python helper {start_marker:?} should run full declarative validation before dispatch/splitting"
        );
    }
}

#[test]
fn python_public_wire_helpers_capture_validator_hooks_against_monkey_patching() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for (start_marker, end_marker, capture_needle, call_needle) in [
        (
            "def join_wire_message(\n    type_hash,",
            "\n\n\ndef split_wire_message(",
            "_validate_frame=_run_registered_declarative_frame_validator",
            "_validate_frame(type_hash, header, payload)",
        ),
        (
            "def split_wire_message(type_hash, wire,",
            "\n\n\ndef split_wire_message_v1(",
            "_validate=validate_wire_message_v1",
            "_validate(type_hash, wire)",
        ),
        (
            "def split_wire_message_v1(type_hash, wire,",
            "\n\n\ndef validate_wire_frame_parts_v1",
            "_validate=validate_wire_message_v1",
            "_validate(type_hash, wire)",
        ),
        (
            "def decode_as(cls, type_hash, wire,",
            "\n\n\ndef from_wire_message(",
            "_validate=validate_wire_message_v1",
            "_validate(type_hash, wire)",
        ),
        (
            "def from_wire_message(type_hash, wire,",
            "\n\n\nclass WireFrame:",
            "_validate=validate_wire_message_v1",
            "_validate(type_hash, wire)",
        ),
        (
            "def is_valid_wire_message(type_hash, wire,",
            "\n\n\ndef is_valid_wire_message_v1",
            "_validate=validate_wire_message_v1",
            "_validate(type_hash, wire)",
        ),
        (
            "def is_valid_wire_message_v1(type_hash, wire,",
            "\n\n\ndef join_wire_message(",
            "_validate=validate_wire_message_v1",
            "_validate(type_hash, wire)",
        ),
        (
            "def split_wire_message_view(\n    type_hash,",
            "\n\n\ndef split_wire_message_view_v1",
            "_validate_frame=_run_registered_declarative_frame_validator",
            "_validate_frame(type_hash, header, payload)",
        ),
        (
            "def split_wire_message_view_v1(type_hash, wire,",
            "\n\n\ndef datapod_type(",
            "_split=split_wire_message_view",
            "return _split(type_hash, wire)",
        ),
        (
            "def _wire_frame_to_wire_message(self,",
            "\n\n\nWireFrame.to_wire_message = _wire_frame_to_wire_message",
            "_validate=validate_wire_frame_v1",
            "_validate(self.type_hash, self.header, self.payload)",
        ),
        (
            "def validate_wire_frame(\n    type_hash,",
            "\n\n\ndef validate_wire_frame_v1",
            "_validate_shape=_validate_wire_frame_shape_v1",
            "_validate_shape(type_hash, header, payload)",
        ),
        (
            "def is_valid_wire_frame(type_hash, header, payload=b\"\",",
            "\n\n\ndef is_valid_wire_frame_v1(",
            "_validate=validate_wire_frame",
            "_validate(type_hash, header, payload)",
        ),
        (
            "def is_valid_wire_frame_v1(",
            "\n\n\ndef _split_wire_message_view_shape_v1",
            "_validate=validate_wire_frame_v1",
            "_validate(type_hash, header, payload)",
        ),
        (
            "def _split_wire_message_view_shape_v1(",
            "\n\n\ndef split_wire_message_view(",
            "_validate_shape=_validate_wire_frame_shape_v1",
            "_validate_shape(type_hash, header, payload)",
        ),
        (
            "def _join_wire_frame_shape_v1_hardened(",
            "\n\n\n_join_wire_frame_shape_v1 = _join_wire_frame_shape_v1_hardened",
            "_validate_shape=_validate_wire_frame_shape_v1",
            "_validate_shape(frame.type_hash, frame.header, frame.payload)",
        ),
        (
            "def _require_wire_frame_result_hardened(",
            "\n\n\ndef _wire_frame_hardened(",
            "_validate=validate_wire_frame_v1",
            "_validate(value.type_hash, value.header, value.payload)",
        ),
        (
            "def _wire_frame_hardened(",
            "\n\n\ndef _archive_hardened(",
            "_require_result=_require_wire_frame_result_hardened",
            "return _require_result(to_wire_frame(), \"to_wire_frame\", obj)",
        ),
        (
            "def _archive_hardened(",
            "\n\n\nwire_frame = _wire_frame_hardened",
            "_require_result=_require_wire_frame_result_hardened",
            "return _require_result(archive_method(), \"archive\", obj)",
        ),
        (
            "def _view_wire_frame_hardened(",
            "\n\n\ndef _view_archive_hardened(",
            "_validate=validate_wire_frame",
            "_validate(expected, frame.header, frame.payload)",
        ),
        (
            "def _view_archive_hardened(",
            "\n\n\ndef _from_archive_hardened(",
            "_validate=validate_wire_frame",
            "_validate(expected, archive_frame.header, archive_frame.payload)",
        ),
        (
            "def _from_archive_hardened(",
            "\n\n\nview_wire_frame = _view_wire_frame_hardened",
            "_validate=validate_wire_frame",
            "_validate(expected, archive_frame.header, archive_frame.payload)",
        ),
    ] {
        let start = PYTHON
            .find(start_marker)
            .unwrap_or_else(|| panic!("Python helper {start_marker:?} should exist"));
        let body = &PYTHON[start..];
        let end = body
            .find(end_marker)
            .unwrap_or_else(|| panic!("Python helper {start_marker:?} body should end"));
        let body = &body[..end];
        assert!(
            body.contains(capture_needle),
            "Python helper {start_marker:?} should capture validator helper {capture_needle:?}"
        );
        assert!(
            body.contains(call_needle),
            "Python helper {start_marker:?} should use captured validator helper via {call_needle:?}"
        );
    }
}

#[test]
fn python_custom_validators_must_return_bool() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "valid = validate(obj)",
        "if not isinstance(valid, bool):",
        "datapod custom validator must return bool",
        "if not valid:",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python custom validators should reject truthy non-bool results via {needle:?}"
        );
    }
}

#[test]
fn python_declarative_encoders_fail_closed_on_hostile_field_lookup() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "object cannot read header field",
        "object cannot read payload field",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python declarative encoders should normalize hostile field lookup failures via {needle:?}"
        );
    }
}

#[test]
fn python_is_valid_helpers_swallow_validator_exceptions_as_false() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for start_marker in [
        "def is_valid_wire_message(type_hash, wire,",
        "def is_valid_wire_message_v1(type_hash, wire,",
        "def is_valid_wire_frame(type_hash, header, payload=b\"\",",
        "def is_valid_wire_frame_v1(",
    ] {
        let start = PYTHON
            .find(start_marker)
            .unwrap_or_else(|| panic!("Python helper {start_marker:?} should exist"));
        let body = &PYTHON[start..];
        let end = body
            .find("\n\n\ndef ")
            .unwrap_or_else(|| panic!("Python helper {start_marker:?} body should end"));
        let body = &body[..end];

        assert!(
            body.contains("except Exception:"),
            "Python predicate helper {start_marker:?} should return False for arbitrary validator exceptions"
        );
    }
}

#[test]
fn python_typed_validate_wire_message_runs_declarative_validator() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    let start = PYTHON
        .find("def validate_wire_message_v1_for_type(inner_cls, incoming_hash, wire):")
        .expect("typed Python validate helper should exist");
    let body = &PYTHON[start..];
    let end = body
        .find("\n\n        @classmethod\n        def view_from_wire")
        .expect("typed Python validate helper body should be findable");
    let body = &body[..end];

    let needle = "validate_wire_message_v1(incoming_hash, wire)";
    assert!(
        body.contains(needle),
        "typed Python validate_wire_message should delegate to generic validation via {needle:?}"
    );
    assert!(
        !body.contains("view_from_wire_v1.__func__(inner_cls, incoming_hash, wire)"),
        "typed Python validate_wire_message should not run declarative validators twice"
    );
}

#[test]
fn python_typed_frame_view_and_owned_decode_validate_shape_once() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for (start_marker, end_marker) in [
        (
            "def from_wire_message_v1(inner_cls, incoming_hash, wire):",
            "\n\n        @classmethod\n        def from_wire_frame",
        ),
        (
            "def from_wire_frame_v1(inner_cls, frame):",
            "\n\n        @classmethod\n        def from_archive",
        ),
        (
            "def view_from_wire_v1(inner_cls, incoming_hash, wire):",
            "\n\n        @classmethod\n        def view_from_wire_frame",
        ),
        (
            "def view_from_wire_frame_v1(inner_cls, frame):",
            "\n\n        @classmethod\n        def view_archive",
        ),
    ] {
        let start = PYTHON
            .find(start_marker)
            .unwrap_or_else(|| panic!("typed Python helper {start_marker:?} should exist"));
        let body = &PYTHON[start..];
        let end = body
            .find(end_marker)
            .unwrap_or_else(|| panic!("typed Python helper {start_marker:?} body should end"));
        let body = &body[..end];

        assert!(
            body.contains("_validate_wire_frame_shape_v1")
                || body.contains("_split_wire_message_view_shape_v1"),
            "typed Python helper {start_marker:?} should use raw shape validation before its own custom validator"
        );
        assert!(
            !body.contains("validate_wire_frame_v1(")
                && !body.contains("split_wire_message_v1(")
                && !body.contains("split_wire_message_view_v1("),
            "typed Python helper {start_marker:?} should not call public validators and run custom validators twice"
        );
    }
}

#[test]
fn python_declarative_layout_merges_inherited_annotations_without_inherited_schema_leakage() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "def _merged_annotations(cls):",
        "for base in reversed(_metadata_attr(cls, \"__mro__\", ()))",
        "annotations.update(_metadata_attr(base, \"__annotations__\", {}))",
        "if dataclass and \"__dataclass_fields__\" not in cls.__dict__:",
        "annotations = _merged_annotations(cls)",
        "cls.__dict__.get(\"__datapod_fields__\", None)",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python declarative datapod layout should support inherited annotations without reusing inherited generated schema attrs; missing {needle:?}"
        );
    }
    assert!(
        !PYTHON.contains("annotations = getattr(cls, \"__annotations__\", {})"),
        "Python declarative datapod layout should not ignore base-class annotations"
    );
    assert!(
        !PYTHON.contains("local_fields = getattr(cls, \"__datapod_fields__\", None)"),
        "Python declarative datapod layout should not inherit stale generated field metadata from a base class"
    );
    assert!(
        !PYTHON.contains("if dataclass and not _datapod_dataclasses.is_dataclass(cls):"),
        "Python declarative datapod layout should dataclass-decorate subclasses that only inherit dataclass metadata"
    );
}

#[test]
fn python_declarative_nested_schema_validates_annotation_type_hashes() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "def _annotation_schema(annotation):",
        "def _annotation_name(annotation):",
        "return _metadata_attr(annotation, \"__name__\", None)",
        "def _is_fixed_datapod_annotation(annotation):",
        "_metadata_attr(annotation, \"__datapod_header_format__\")",
        "_metadata_attr(annotation, \"__datapod_payload_field__\", None) is None",
        "\"type_hash\": _u64_type_hash(",
        "_metadata_attr(annotation, \"TYPE_HASH\")",
        "_metadata_attr(annotation, \"__datapod_fields__\")",
        "arity = _metadata_attr(annotation, \"__datapod_header_arity__\", None)",
        "_struct_value_count(",
        "_strict_index(arity, \"datapod annotation header arity\", \"a positive integer\")",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python declarative schema introspection should reject bool/float/out-of-range nested datapod annotation hashes via {needle:?}"
        );
    }

    assert!(
        !PYTHON.contains("\"type_hash\": annotation.TYPE_HASH"),
        "Python declarative schema introspection should not expose unchecked nested annotation TYPE_HASH values"
    );
    assert!(
        !PYTHON.contains("return getattr(annotation, \"__name__\", None)"),
        "Python declarative annotation naming should not use hostile-descriptor-prone raw getattr"
    );
    assert!(
        !PYTHON.contains("hasattr(annotation, \"__datapod_header_format__\")"),
        "Python declarative annotation detection should not use hostile-descriptor-prone hasattr"
    );
}

#[test]
fn python_generic_archive_helpers_validate_before_custom_dispatch() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for (start_marker, end_marker, dispatch_needle) in [
        (
            "def view_wire_frame(type_or_hash, frame):",
            "\n\n\ndef view_archive",
            "return view_from_wire_frame(frame)",
        ),
        (
            "def view_archive(type_or_hash, archive_frame):",
            "\n\n\ndef from_archive",
            "return view_archive_method(archive_frame)",
        ),
        (
            "def from_archive(type_or_hash, archive_frame):",
            "\n\n\ndef validate_wire_frame",
            "return from_archive_method(archive_frame)",
        ),
    ] {
        let start = PYTHON
            .find(start_marker)
            .unwrap_or_else(|| panic!("Python helper {start_marker:?} should exist"));
        let body = &PYTHON[start..];
        let end = body
            .find(end_marker)
            .unwrap_or_else(|| panic!("Python helper {start_marker:?} body should end"));
        let body = &body[..end];
        let validation_pos = body
            .find("validate_wire_frame(")
            .unwrap_or_else(|| panic!("Python helper {start_marker:?} should validate frames"));
        let dispatch_pos = body.find(dispatch_needle).unwrap_or_else(|| {
            panic!("Python helper {start_marker:?} should dispatch via {dispatch_needle:?}")
        });

        assert!(
            validation_pos < dispatch_pos,
            "Python helper {start_marker:?} must validate archive/frame bytes before custom hook dispatch"
        );
    }
}

#[test]
fn python_generic_archive_helpers_validate_class_type_hash_strictly() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for (start_marker, end_marker) in [
        (
            "def _view_wire_frame_hardened(",
            "\n\n\ndef _view_archive_hardened",
        ),
        (
            "def _view_archive_hardened(",
            "\n\n\ndef _from_archive_hardened",
        ),
        (
            "def _from_archive_hardened(",
            "\n\n\nview_wire_frame = _view_wire_frame_hardened",
        ),
    ] {
        let start = PYTHON
            .find(start_marker)
            .unwrap_or_else(|| panic!("Python helper {start_marker:?} should exist"));
        let body = &PYTHON[start..];
        let end = body
            .find(end_marker)
            .unwrap_or_else(|| panic!("Python helper {start_marker:?} body should end"));
        let body = &body[..end];

        assert!(
            body.contains("type_hash_value = _metadata_attr(type_or_hash, \"TYPE_HASH\")")
                && body.contains("expected = _u64_type_hash(type_hash_value)"),
            "Python helper {start_marker:?} should reject hostile/bool/float/out-of-range class TYPE_HASH values"
        );
    }

    assert!(
        PYTHON.contains("_registry=None") && PYTHON.contains("if _registry is None:"),
        "Python archive helpers should not bind __datapod_registry__ as a default before module injection"
    );
    assert!(
        !PYTHON.contains("_registry=__datapod_registry__"),
        "Python archive helpers should not reference injected registry during declarative module import"
    );
}

#[test]
fn python_array_counts_use_strict_positive_integer_protocol() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    let start = PYTHON
        .find("class _DatapodArray:")
        .expect("declarative Python array helper should exist");
    let body = &PYTHON[start..];
    let end = body
        .find("\n\nu8 = _DatapodScalar")
        .expect("declarative Python array helper body should be findable");
    let body = &body[..end];

    for needle in [
        "count = _strict_index(count, \"datapod.array count\", \"a positive int\")",
        "if count <= 0:",
        "if count > _datapod_sys.maxsize:",
        "datapod.array count exceeds maximum supported length",
    ] {
        assert!(
            body.contains(needle),
            "Python datapod.array count validation missing strict guard {needle:?}"
        );
    }
}

#[test]
fn python_fixed_array_encoding_is_bounded_and_fail_closed() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    let start = PYTHON
        .find("def _pack_header_field(annotation, value, name, arity):")
        .expect("Python header pack helper should exist");
    let body = &PYTHON[start..];
    let end = body
        .find("\n\n\ndef _unpack_header_field")
        .expect("Python header pack helper body should be findable");
    let body = &body[..end];

    for needle in [
        "iterator = iter(value)",
        "for index in range(arity):",
        "items.append(next(iterator))",
        "f\"field {name} expected {arity} items, got more than {arity}\"",
        "f\"field {name} failed while reading array item {index}\"",
        "f\"field {name} failed while checking array length\"",
        "return tuple(items)",
    ] {
        assert!(
            body.contains(needle),
            "Python fixed-array packing should be bounded/fail-closed via {needle:?}"
        );
    }
    assert!(
        !body.contains("items = tuple(value)"),
        "Python fixed-array packing should not exhaust arbitrary iterables just to check arity"
    );
}

#[test]
fn python_runtime_registration_uses_strict_header_size_protocol() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "import sys as _datapod_sys",
        "def _usize_header_size(value, label=\"header_size\"):",
        "value = _strict_index(value, label, \"a non-negative platform-sized integer\")",
        "if value > _datapod_sys.maxsize:",
        "_usize_header_size(header_size)",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python runtime registration should reject bool/float/impossible header sizes via {needle:?}"
        );
    }
}

#[test]
fn python_registry_metadata_helpers_propagate_registry_lookup_errors() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "crate::registry::try_find_type_info(type_hash)",
        ".map_err(|error| pyo3::exceptions::PyValueError::new_err(error.to_string()))?",
        "unknown datapod type hash: {type_hash}",
        ".map(|info| info.canonical_type_hash)",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python registry metadata path should use fallible registry lookup via {needle:?}"
        );
    }

    assert!(
        !PYTHON.contains("crate::registry::find_type_info(type_hash)"),
        "Python metadata helpers should not erase registry lookup errors through Option-only lookup"
    );
}

#[test]
fn python_declarative_registration_rejects_malformed_field_metadata() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "def _field_names_tuple(fields, label):",
        "if isinstance(fields, str):",
        "entries must be str",
        "entries must not be empty",
        "entries must be unique",
        "got conflicting header_format and header values",
        "got conflicting payload_field and payload values",
        "payload_field/payload must be a str",
        "payload_field/payload must not be empty",
        "payload_field is not None and payload_field is not True",
        "datapod_type decorator expects a class",
        "def _resolve_header_annotation(annotation):",
        "raise ValueError(\"datapod annotation metadata lookup failed\") from error",
        "if not isinstance(dataclass, bool):",
        "payload field must not also be a header field",
        "header_format/header must be a str",
        "byte_order must be a str",
        "byte_order must be one of",
        "datapod-wire-v1/le requires header_format/header to start with '<'",
        "datapod_type needs annotations for inferred header fields",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python declarative registration should reject malformed field metadata via {needle:?}"
        );
    }
}

#[test]
fn python_split_wire_message_view_v1_delegates_to_unversioned_helper() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");
    let start = PYTHON
        .find("def split_wire_message_view_v1(type_hash, wire,")
        .expect("split_wire_message_view_v1 helper should exist");
    let body = &PYTHON[start..];
    let end = body
        .find("\n\n\ndef datapod_type(")
        .expect("split_wire_message_view_v1 helper body should be findable");
    let body = &body[..end];

    assert!(
        body.contains("_split=split_wire_message_view")
            && body.contains("return _split(type_hash, wire)"),
        "Python v1 split-view helper should delegate to the unversioned helper to avoid drift"
    );
    assert!(
        !body.contains("header_size_v1(type_hash)") && !body.contains("validate_wire_frame_v1"),
        "Python v1 split-view helper should not duplicate split/validate logic"
    );
}

#[test]
fn ffi_handle_free_functions_route_through_shared_error_clearing_helper() {
    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR"));
    let source = std::fs::read_to_string(root.join("src/ffi.rs"))
        .unwrap_or_else(|err| panic!("failed to read src/ffi.rs: {err}"));
    assert!(source.contains("fn free_boxed_handle<T>(handle: *mut T)"));

    let mut checked = 0usize;
    let mut rest = source.as_str();
    while let Some(offset) = rest.find("pub extern \"C\" fn datapod_") {
        rest = &rest[offset..];
        let Some(name_end) = rest.find('(') else {
            break;
        };
        let name = &rest[..name_end];
        let next = rest.find("\n#[unsafe(no_mangle)]").unwrap_or(rest.len());
        let body = &rest[..next];
        rest = &rest[next..];
        if !name.ends_with("_free") || name.contains("owned_bytes_free") {
            continue;
        }
        checked += 1;
        assert!(
            body.contains("free_boxed_handle("),
            "{name} must route through free_boxed_handle so NULL free clears stale C ABI errors"
        );
    }
    assert!(
        checked >= 50,
        "expected to inspect all C ABI handle free fns"
    );
    assert!(
        !source.contains("std::boxed::Box::from_raw(handle)"),
        "handle free fns should not bypass free_boxed_handle"
    );
}

#[test]
fn ffi_heap_to_wire_functions_clear_outputs_before_null_handle_checks() {
    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR"));
    let source = std::fs::read_to_string(root.join("src/ffi.rs"))
        .unwrap_or_else(|err| panic!("failed to read src/ffi.rs: {err}"));

    let mut checked = 0usize;
    let mut rest = source.as_str();
    while let Some(offset) = rest.find("pub extern \"C\" fn datapod_") {
        rest = &rest[offset..];
        let Some(name_end) = rest.find('(') else {
            break;
        };
        let name = &rest[..name_end];
        let next = rest.find("\n#[unsafe(no_mangle)]").unwrap_or(rest.len());
        let body = &rest[..next];
        rest = &rest[next..];
        if !name.ends_with("_to_wire") || !body.contains("handle: *const") {
            continue;
        }
        checked += 1;
        let prepare = body
            .find("prepare_owned_bytes_output(out)")
            .unwrap_or_else(|| panic!("{name} must clear owned-byte output before validation"));
        let null_check = body
            .find("handle.is_null()")
            .unwrap_or_else(|| panic!("{name} must check null handle"));
        assert!(
            prepare < null_check,
            "{name} must clear stale DatapodOwnedBytes output before null-handle failure"
        );
    }
    assert!(
        checked >= 25,
        "expected to inspect all heap handle to_wire fns"
    );
}

#[test]
fn ffi_archive_owned_decode_cleanup_preserves_decode_errors() {
    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR"));
    let source = std::fs::read_to_string(root.join("src/ffi.rs"))
        .unwrap_or_else(|err| panic!("failed to read src/ffi.rs: {err}"));

    assert!(
        source.contains("fn drop_owned_bytes_preserving_last_error"),
        "src/ffi.rs should have an internal owned-byte cleanup that does not clear last-error"
    );
    let helper_start = source
        .find("fn heap_handle_from_archive")
        .expect("heap_handle_from_archive helper must exist");
    let helper_end = source[helper_start..]
        .find("\n}\n\n/// FFI-safe point value")
        .map(|offset| helper_start + offset)
        .expect("heap_handle_from_archive helper end must be findable");
    let helper = &source[helper_start..helper_end];
    assert!(
        helper.contains("drop_owned_bytes_preserving_last_error(owned)"),
        "archive owned-decode cleanup must preserve errors raised by from_wire"
    );
    assert!(
        !helper.contains("datapod_owned_bytes_free(owned)"),
        "archive owned-decode cleanup must not call the public free helper because it clears last-error"
    );
}

#[test]
fn proc_macro_source_avoids_obvious_panic_shortcuts() {
    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR"));
    let source = std::fs::read_to_string(root.join("crates/datapod-macros/src/lib.rs"))
        .unwrap_or_else(|err| panic!("failed to read crates/datapod-macros/src/lib.rs: {err}"));
    let forbidden = [
        concat!(".expect", "("),
        concat!("unwrap", "()"),
        concat!("todo", "!"),
        concat!("unimplemented", "!"),
        concat!("unreachable", "!"),
        concat!("panic", "!("),
    ];

    for needle in forbidden {
        assert!(
            !source.contains(needle),
            "crates/datapod-macros/src/lib.rs still contains panic shortcut {needle:?}"
        );
    }
}

#[test]
fn wire_module_avoids_obvious_panic_shortcuts() {
    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR"));
    let source = std::fs::read_to_string(root.join("src/wire.rs"))
        .unwrap_or_else(|err| panic!("failed to read src/wire.rs: {err}"));
    let forbidden = [
        concat!(".expect", "("),
        concat!("unwrap", "()"),
        concat!("todo", "!"),
        concat!("unimplemented", "!"),
        concat!("panic", "!("),
        concat!("assert", "!("),
    ];

    for needle in forbidden {
        assert!(
            !source.contains(needle),
            "src/wire.rs still contains panic shortcut {needle:?}"
        );
    }
}

#[test]
fn little_endian_array_header_decode_uses_fallible_reservation() {
    const WIRE_RS: &str = include_str!("../src/wire.rs");

    let start = WIRE_RS
        .find("impl<T, const N: usize> LeWireHeader for [T; N]")
        .expect("array LeWireHeader impl should exist");
    let body = &WIRE_RS[start..];
    let end = body
        .find("\n/// Read a single `LeWireHeader` field")
        .expect("array LeWireHeader body should be findable");
    let body = &body[..end];

    for needle in [
        "let mut items = Vec::new();",
        "items.try_reserve_exact(N)",
        "failed to reserve {N} little-endian array field items",
        "items.push(T::read_le(chunk)?);",
    ] {
        assert!(
            body.contains(needle),
            "array header decode should reserve fallibly before materializing items via {needle:?}"
        );
    }

    assert!(
        !body.contains("Vec::with_capacity(N)"),
        "array header decode should not use infallible Vec::with_capacity"
    );
}

#[test]
fn wire_len_reporting_uses_non_panicking_arithmetic() {
    const WIRE_RS: &str = include_str!("../src/wire.rs");

    for needle in [
        "self.header.len().saturating_add(self.payload.len())",
        ".fold(0usize, |total, payload| total.saturating_add(payload.len()))",
        "self.header.len().saturating_add(self.payload_len())",
        "pub fn try_joined_len(&self) -> Result<usize, WireError>",
        "\"WireParts\"",
        "checked_wire_len(",
    ] {
        assert!(
            WIRE_RS.contains(needle),
            "wire length helpers should avoid debug-overflow panics and keep fallible exact checks via {needle:?}"
        );
    }

    assert!(
        !WIRE_RS.contains("self.header.len() + self.payload.len()")
            && !WIRE_RS.contains("self.header.len() + self.payload_len()"),
        "wire length helpers should not use unchecked usize addition"
    );
}

#[test]
fn wire_split_and_decode_helpers_use_checked_ranges() {
    const WIRE_RS: &str = include_str!("../src/wire.rs");

    for needle in [
        "let field = bytes.get(*offset..end).ok_or(WireError::ShortHeader",
        "let header = bytes",
        ".get(..info.header_size)",
        ".ok_or(WireError::ShortHeader",
        ".get(info.header_size..)",
        "let Some(header_bytes) = bytes.get(..T::Header::LE_WIRE_SIZE) else",
        "let Some(payload) = bytes.get(T::Header::LE_WIRE_SIZE..) else",
        "let header_bytes = bytes.get(..header_size).ok_or(WireError::ShortHeader",
        ".get(header_size..)",
    ] {
        assert!(
            WIRE_RS.contains(needle),
            "wire split/decode helper should use checked range pattern {needle:?}"
        );
    }

    for forbidden in [
        "T::read_le(&bytes[*offset..end])",
        "header: &bytes[..info.header_size]",
        "payload: &bytes[info.header_size..]",
        "bytes.get(..T::Header::LE_WIRE_SIZE).unwrap_or(&[])",
        "bytes.get(T::Header::LE_WIRE_SIZE..).unwrap_or(&[])",
        "T::Header::read_le(&bytes[..T::Header::LE_WIRE_SIZE])",
        "&bytes[T::Header::LE_WIRE_SIZE..]",
        "T::Header::read_le(&bytes[..header_size])",
        "&bytes[header_size..]",
    ] {
        assert!(
            !WIRE_RS.contains(forbidden),
            "wire split/decode helper should not retain direct indexing pattern {forbidden:?}"
        );
    }
}

#[test]
fn datapod_default_payload_writer_uses_fallible_reservation() {
    const WIRE_RS: &str = include_str!("../src/wire.rs");

    let start = WIRE_RS
        .find("fn try_write_payload_bytes(&self, out: &mut Vec<u8>) -> Result<(), WireError>")
        .expect("DataPod::try_write_payload_bytes default should exist");
    let body = &WIRE_RS[start..];
    let end = body
        .find("\n    }\n\n    /// Borrow payload")
        .expect("DataPod::try_write_payload_bytes default body should be findable");
    let body = &body[..end];

    for needle in [
        "let payload = self.payload_bytes();",
        "out.try_reserve_exact(payload.len())",
        "failed to reserve {} payload bytes",
        "out.extend_from_slice(payload);",
    ] {
        assert!(
            body.contains(needle),
            "default DataPod payload writer should reserve fallibly before appending via {needle:?}"
        );
    }

    assert!(
        !body.contains("self.write_payload_bytes(out);"),
        "default DataPod payload writer should not delegate to the infallible writer"
    );
}

#[test]
fn sectioned_macro_overrides_fallible_payload_writer() {
    const MACROS_RS: &str = include_str!("../crates/datapod-macros/src/lib.rs");

    let start = MACROS_RS
        .find("fn try_write_payload_bytes")
        .expect("sectioned macro should generate a fallible payload writer override");
    let body = &MACROS_RS[start..];
    let end = body
        .find("fn with_payload_segments")
        .expect("sectioned macro writer should appear before segmented borrowing");
    let body = &body[..end];

    for needle in [
        "let __dp_len = self.try_payload_len()?",
        "out.try_reserve_exact(__dp_len)",
        "failed to reserve {} sectioned payload bytes",
        "#(#payload_try_write_steps)*",
        "::core::result::Result::Ok(())",
    ] {
        assert!(
            body.contains(needle),
            "sectioned generated writer should keep fallible owned-message collection on checked path {needle:?}"
        );
    }
}

#[test]
fn registered_wire_split_rejects_fixed_payload_bytes() {
    const WIRE_RS: &str = include_str!("../src/wire.rs");

    let start = WIRE_RS
        .find("pub fn split_wire_parts")
        .expect("split_wire_parts should exist");
    let body = &WIRE_RS[start..];
    let end = body
        .find("\n}\n\n/// Split a registered contiguous")
        .expect("split_wire_parts body should be findable");
    let body = &body[..end];

    for needle in [
        "info.payload_kind == crate::registry::PayloadKind::Fixed",
        "bytes.len() != info.header_size",
        "bytes.len().checked_sub(info.header_size)",
        "WireError::InvalidPayloadSize",
        "fixed-size datapod wire message cannot carry payload bytes",
    ] {
        assert!(
            body.contains(needle),
            "registered wire splitting should enforce fixed-size no-payload shape via {needle:?}"
        );
    }
    assert!(
        !body.contains("bytes.len() - info.header_size"),
        "registered wire splitting should not report fixed payload length with unchecked subtraction"
    );
}

#[test]
fn payload_vec_decode_uses_fallible_allocation() {
    const WIRE_RS: &str = include_str!("../src/wire.rs");

    let start = WIRE_RS
        .find("pub fn decode_payload_vec")
        .expect("decode_payload_vec should exist");
    let body = &WIRE_RS[start..];
    let end = body
        .find("\n}\n\n/// Universal transport envelope")
        .expect("decode_payload_vec body should be findable");
    let body = &body[..end];

    for needle in [
        "let count = payload.len() / elem_size;",
        "values.try_reserve_exact(count)",
        "failed to reserve {count} decoded payload elements",
        "values.extend(",
        ".chunks_exact(elem_size)",
        ".map(bytemuck::pod_read_unaligned::<T>)",
    ] {
        assert!(
            body.contains(needle),
            "payload Vec decoding should allocate fallibly before collecting decoded POD elements via {needle:?}"
        );
    }

    assert!(
        !body.contains(".collect()"),
        "payload Vec decoding should not use infallible collect allocation"
    );
}

#[test]
fn owned_wire_decode_copies_payload_with_fallible_reservation() {
    const WIRE_RS: &str = include_str!("../src/wire.rs");

    for (name, start_marker, end_marker, reserve_needle) in [
        (
            "from_wire_message_v1",
            "pub fn from_wire_message_v1<T>(msg: &WireMessage) -> Result<T, WireError>",
            "\n\n/// Decode a concrete datapod type from a borrowed",
            "failed to reserve {} decoded payload bytes",
        ),
        (
            "from_wire_frame_v1",
            "pub fn from_wire_frame_v1<T>(frame: WireFrame<'_>) -> Result<T, WireError>",
            "\n\n/// Split a registered datapod wire body",
            "failed to reserve {} decoded frame payload bytes",
        ),
    ] {
        let start = WIRE_RS
            .find(start_marker)
            .unwrap_or_else(|| panic!("{name} should exist"));
        let body = &WIRE_RS[start..];
        let end = body
            .find(end_marker)
            .unwrap_or_else(|| panic!("{name} body should be findable"));
        let body = &body[..end];

        for needle in [
            "let mut owned_payload = Vec::new();",
            ".try_reserve_exact(payload.len())",
            reserve_needle,
            "owned_payload.extend_from_slice(payload);",
            "T::from_wire_parts(header, owned_payload)",
        ] {
            assert!(
                body.contains(needle),
                "{name} should reserve fallibly before owned payload decode via {needle:?}"
            );
        }
        assert!(
            !body.contains("payload.to_vec()"),
            "{name} should not clone payload with infallible to_vec"
        );
    }
}

#[test]
fn payload_layout_builder_infallible_capacity_constructor_uses_fallible_path() {
    const LAYOUT_RS: &str = include_str!("../src/layout.rs");

    let start = LAYOUT_RS
        .find("pub fn with_capacity(section_capacity: usize, payload_capacity: usize) -> Self")
        .expect("PayloadLayoutBuilder::with_capacity should exist");
    let body = &LAYOUT_RS[start..];
    let end = body
        .find("\n\n    pub fn try_with_capacity")
        .expect("PayloadLayoutBuilder::with_capacity body should be findable");
    let body = &body[..end];

    for needle in [
        "match Self::try_with_capacity(section_capacity, payload_capacity)",
        "Ok(builder) => builder",
        "Err(_) => Self::new()",
    ] {
        assert!(
            body.contains(needle),
            "PayloadLayoutBuilder::with_capacity should reuse the fallible constructor via {needle:?}"
        );
    }

    assert!(
        !body.contains("Vec::with_capacity"),
        "PayloadLayoutBuilder::with_capacity should not call infallible Vec::with_capacity directly"
    );
}

#[test]
fn payload_section_and_kv_helpers_use_checked_ranges() {
    const LAYOUT_RS: &str = include_str!("../src/layout.rs");
    const KV_RS: &str = include_str!("../src/robot/kv.rs");

    for needle in [
        "bytes.get(..4)",
        "bytes.get(4..8)",
        "payload section offset range is out of bounds",
        "payload section length range is out of bounds",
    ] {
        assert!(
            LAYOUT_RS.contains(needle),
            "PayloadSection::read_le should use checked byte range {needle:?}"
        );
    }
    for forbidden in ["&bytes[..4]", "&bytes[4..8]"] {
        assert!(
            !LAYOUT_RS.contains(forbidden),
            "PayloadSection::read_le should not retain direct range pattern {forbidden:?}"
        );
    }

    for needle in [
        "pub fn try_key_str(&self) -> Result<&str, WireError>",
        "pub fn try_value_str(&self) -> Result<&str, WireError>",
        "self.try_key_str().unwrap_or(\"\")",
        "self.try_value_str().unwrap_or(\"\")",
        ".get(..end)",
        "key is not UTF-8 before NUL",
        "value is not UTF-8 before NUL",
        "dst.get_mut(..n)",
        "bytes.get(..n)",
        "out.copy_from_slice(input)",
    ] {
        assert!(
            KV_RS.contains(needle),
            "KV string helpers should use checked byte range {needle:?}"
        );
    }
    for forbidden in [
        "&self.key[..end]",
        "&self.value[..end]",
        "dst[..n].copy_from_slice(&bytes[..n])",
    ] {
        assert!(
            !KV_RS.contains(forbidden),
            "KV string helpers should not retain direct range pattern {forbidden:?}"
        );
    }
}

#[test]
fn hardened_runtime_surfaces_avoid_infallible_collection_shortcuts() {
    let root = std::path::Path::new(env!("CARGO_MANIFEST_DIR"));
    let files = [
        "src/ffi.rs",
        "src/id/mac_addr.rs",
        "src/layout.rs",
        "src/registry.rs",
        "src/python/fixed.rs",
        "src/python/geometry.rs",
        "src/python/heap.rs",
        "src/seq/indexed_heap.rs",
        "src/seq/paged_vecvec.rs",
        "src/seq/vecvec.rs",
        "src/wire.rs",
    ];
    let forbidden = [
        "Vec::with_capacity",
        "String::with_capacity",
        ".collect()",
        "collect::<",
    ];

    for file in files {
        let source = std::fs::read_to_string(root.join(file))
            .unwrap_or_else(|err| panic!("failed to read {file}: {err}"));
        for needle in forbidden {
            assert!(
                !source.contains(needle),
                "{file} should use fallible allocation paths instead of {needle:?}"
            );
        }
    }
}

#[test]
fn byte_counted_sequence_constructors_check_element_size_before_u32_header_cast() {
    const SEQ_MOD: &str = include_str!("../src/seq/mod.rs");
    assert!(
        SEQ_MOD.contains(
            "pub(crate) fn checked_pod_element_size<Container: 'static, T: bytemuck::Pod>"
        ),
        "sequence module should centralize checked Pod element-size narrowing"
    );
    assert!(
        SEQ_MOD.contains("u32::try_from(size)"),
        "sequence element-size helper should not truncate usize into u32"
    );

    for (name, source) in [
        ("Vector", include_str!("../src/seq/vector.rs")),
        ("Stack", include_str!("../src/seq/stack.rs")),
        ("Queue", include_str!("../src/seq/queue.rs")),
        ("Deque", include_str!("../src/seq/deque.rs")),
        ("Heap", include_str!("../src/seq/heap.rs")),
        ("IndexedHeap", include_str!("../src/seq/indexed_heap.rs")),
        ("List", include_str!("../src/seq/list.rs")),
        ("ForwardList", include_str!("../src/seq/forward_list.rs")),
        ("Vecvec", include_str!("../src/seq/vecvec.rs")),
        ("PagedVecvec", include_str!("../src/seq/paged_vecvec.rs")),
    ] {
        for needle in [
            "pub fn try_new<T: bytemuck::Pod>() -> Result<Self, WireError>",
            "Self::try_new::<T>().unwrap_or_default()",
            "super::checked_pod_element_size::<Self, T>()?",
        ] {
            assert!(
                source.contains(needle),
                "{name} constructor should use checked element-size setup via {needle:?}"
            );
        }
        assert!(
            !source.contains("std::mem::size_of::<T>() as u32"),
            "{name} constructor should not truncate size_of::<T>() into the u32 wire field"
        );
    }

    for (name, source) in [
        ("Matrix", include_str!("../src/seq/matrix.rs")),
        ("Tensor", include_str!("../src/seq/tensor.rs")),
    ] {
        assert!(
            source.contains("super::checked_pod_element_size::<Self, T>()?"),
            "{name} shaped constructors should use checked element-size setup"
        );
        assert!(
            !source.contains("element_size: es as u32"),
            "{name} shaped constructors should not truncate size_of::<T>() into the u32 wire field"
        );
    }
}

#[test]
fn registry_type_info_listing_reserves_fallibly() {
    const REGISTRY: &str = include_str!("../src/registry.rs");

    let start = REGISTRY
        .find("pub fn all_type_infos() -> Vec<TypeInfo>")
        .expect("registry all_type_infos should exist");
    let body = &REGISTRY[start..];
    let end = body
        .find("\n\npub fn find_type_info")
        .expect("registry all_type_infos body should be findable");
    let body = &body[..end];

    for needle in [
        "let mut infos = Vec::new();",
        "infos.try_reserve_exact(builtin_count)",
        "infos.push(type_info::<$ty>($canonical));",
        "infos.try_reserve_exact(custom.by_hash.len())",
        "infos.extend(custom.by_hash.values().cloned());",
    ] {
        assert!(
            body.contains(needle),
            "registry all_type_infos should reserve fallibly before populating via {needle:?}"
        );
    }
    for forbidden in ["vec![", ".collect()"] {
        assert!(
            !body.contains(forbidden),
            "registry all_type_infos should not use infallible allocation shortcut {forbidden:?}"
        );
    }
}

#[test]
fn registry_lookup_surface_exposes_fallible_lock_error_paths() {
    const REGISTRY: &str = include_str!("../src/registry.rs");

    for needle in [
        "pub fn try_find_type_info(type_hash: u64) -> Result<Option<TypeInfo>, RegistryError>",
        "pub fn try_find_type_info_by_name(canonical_name: &str) -> Result<Option<TypeInfo>, RegistryError>",
        ".map_err(|_| RegistryError::RegistryLockPoisoned)?",
        "try_find_type_info(type_hash).unwrap_or(None)",
        "try_find_type_info_by_name(canonical_name).unwrap_or(None)",
    ] {
        assert!(
            REGISTRY.contains(needle),
            "registry should expose checked lookup path {needle:?}"
        );
    }

    assert!(
        !REGISTRY.contains(".read().ok()"),
        "registry lookups should not silently erase poisoned-lock errors via read().ok()"
    );
}

#[test]
fn runtime_registry_c_and_python_share_canonical_name_grammar() {
    const REGISTRY: &str = include_str!("../src/registry.rs");
    const FFI: &str = include_str!("../src/ffi.rs");
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "pub fn validate_canonical_name(canonical_name: &str) -> Result<(), RegistryError>",
        "b'.' if previous_was_dot",
        "b'a'..=b'z' | b'0'..=b'9' | b'_'",
        "must contain only ASCII lowercase letters, digits, underscores, and dots",
        "validate_canonical_name(&canonical_name)?",
    ] {
        assert!(
            REGISTRY.contains(needle),
            "runtime registry should enforce canonical-name grammar through {needle:?}"
        );
    }

    assert!(
        FFI.matches("crate::registry::validate_canonical_name(name)")
            .count()
            >= 2,
        "C ABI string and byte-slice canonical-name paths should share registry grammar"
    );
    assert!(
        PYTHON.contains("crate::registry::validate_canonical_name(canonical_name)")
            && PYTHON.contains("def _validate_canonical_name(canonical_name):"),
        "Python raw and declarative registration should share the same canonical-name grammar"
    );
}

#[test]
fn heap_pop_moves_last_entry_without_allocating_a_temporary_vec() {
    const HEAP_RS: &str = include_str!("../src/seq/heap.rs");

    let start = HEAP_RS
        .find("pub fn try_pop<T: bytemuck::Pod + PartialOrd>")
        .expect("Heap::try_pop should exist");
    let body = &HEAP_RS[start..];
    let end = body
        .find("\n\n    pub fn top")
        .expect("Heap::try_pop body should be findable");
    let body = &body[..end];

    for needle in [
        ".checked_mul(es)",
        ".checked_add(es)",
        "self.data.get(last_start..last_end)",
        "self.data.copy_within(last_start..last_end, 0);",
        ".checked_sub(es)",
    ] {
        assert!(
            body.contains(needle),
            "Heap::try_pop should move the last entry in-place via {needle:?}"
        );
    }

    assert!(
        !body.contains(".to_vec()"),
        "Heap::try_pop should not allocate a temporary Vec while mutating"
    );
    for forbidden in [
        "let last_start = (n - 1) * es;",
        "self.data.copy_within(last_start..last_start + es, 0);",
        "self.data.truncate(self.data.len() - es);",
    ] {
        assert!(
            !body.contains(forbidden),
            "Heap::try_pop should avoid unchecked byte math pattern {forbidden:?}"
        );
    }
}

#[test]
fn vector_matrix_tensor_accessors_use_checked_byte_ranges() {
    for (name, source, required, forbidden) in [
        (
            "Vector",
            include_str!("../src/seq/vector.rs"),
            &[
                ".get(start..end)",
                ".get_mut(start..end)",
                ".get(start..)",
                ".get(offset..end)",
                ".checked_sub(es)",
                "self.try_size().unwrap_or(0)",
                "self.try_empty().unwrap_or(true)",
                "Ok(self.try_size()? == 0)",
                "fn try_element_size(&self) -> Result<usize, WireError>",
                "super::checked_u32_to_usize::<Self>(self.element_size, \"element_size\")",
                "super::checked_u32_to_usize::<Vector>(self.header.element_size, \"element_size\")",
                "super::checked_u32_to_usize::<P>(stored, \"element_size\")",
            ][..],
            &[
                "self.element_size as usize",
                "self.header.element_size as usize",
                "header.element_size as usize",
                "stored as usize",
                "&self.data[start..end]",
                "self.data[start..end].copy_from_slice",
                "&self.data[start..]",
                "&self.data[offset..end]",
                "Ok(self.size())",
                "Ok(self.empty())",
            ][..],
        ),
        (
            "Matrix",
            include_str!("../src/seq/matrix.rs"),
            &[
                ".get(offset)",
                ".get_mut(range)",
                ".get(offset..end)",
                ".get(start..end)",
                "matrix_dims::<Self>",
                "matrix_dims::<Matrix>",
                "u32_to_usize::<Self>(header.element_size",
                "u32_to_usize::<P>(stored, \"element_size\")",
            ][..],
            &[
                "self.rows as usize",
                "self.cols as usize",
                "header.rows as usize",
                "header.cols as usize",
                "header.element_size as usize",
                "self.header.rows as usize",
                "self.header.cols as usize",
                "stored as usize",
                "&self.data[offset]",
                "self.data[range].copy_from_slice",
                "&self.data[offset..end]",
                "&slice[start..end]",
            ][..],
        ),
        (
            "Tensor",
            include_str!("../src/seq/tensor.rs"),
            &[
                ".get(range)",
                ".get_mut(range)",
                ".get(offset..end)",
                ".get(start..end)",
                "tensor_dims::<Self>",
                "tensor_dims::<Tensor>",
                "u32_to_usize::<Self>(header.element_size",
                "u32_to_usize::<P>(stored, \"element_size\")",
            ][..],
            &[
                "self.rows as usize",
                "self.cols as usize",
                "self.layers as usize",
                "header.rows as usize",
                "header.cols as usize",
                "header.layers as usize",
                "header.element_size as usize",
                "self.header.rows as usize",
                "self.header.cols as usize",
                "self.header.layers as usize",
                "stored as usize",
                "&self.data[range]",
                "self.data[range].copy_from_slice",
                "&self.data[offset..end]",
                "&slice[start..end]",
            ][..],
        ),
    ] {
        for needle in required {
            assert!(
                source.contains(needle),
                "{name} byte accessors should use checked range helper {needle:?}"
            );
        }
        for needle in forbidden {
            assert!(
                !source.contains(needle),
                "{name} byte accessors should not retain direct indexing pattern {needle:?}"
            );
        }
    }
}

#[test]
fn shaped_owned_flat_index_helpers_validate_before_sentinel_fallback() {
    for (name, source, fallback) in [
        (
            "Matrix",
            include_str!("../src/seq/matrix.rs"),
            "self.try_flat_index(row, col).unwrap_or(usize::MAX)",
        ),
        (
            "Tensor",
            include_str!("../src/seq/tensor.rs"),
            "self.try_flat_index(row, col, layer).unwrap_or(usize::MAX)",
        ),
        (
            "Grid",
            include_str!("../src/raster/grid.rs"),
            "self.try_flat_index(row, col).unwrap_or(usize::MAX)",
        ),
        (
            "Layer",
            include_str!("../src/raster/layer.rs"),
            "self.try_flat_index(row, col, layer).unwrap_or(usize::MAX)",
        ),
    ] {
        assert!(
            source.contains(fallback),
            "{name} owned flat_index should route through fallible validation before returning the sentinel"
        );
        assert!(
            !source.contains("self.checked_flat_index(row, col).unwrap_or(usize::MAX)")
                && !source.contains(
                    "self.checked_flat_index(row, col, layer)\n            .unwrap_or(usize::MAX)"
                ),
            "{name} owned flat_index should not bypass owned payload validation"
        );
    }
}

#[test]
fn matrix_tensor_owned_is_empty_helpers_validate_before_defaulting() {
    for (name, source, fallback, checked) in [
        (
            "Matrix",
            include_str!("../src/seq/matrix.rs"),
            "self.try_is_empty().unwrap_or(true)",
            "Ok(self.rows == 0 || self.cols == 0)",
        ),
        (
            "Tensor",
            include_str!("../src/seq/tensor.rs"),
            "self.try_is_empty().unwrap_or(true)",
            "Ok(self.rows == 0 || self.cols == 0 || self.layers == 0)",
        ),
    ] {
        assert!(
            source.contains(fallback),
            "{name} owned is_empty should route through fallible validation before returning a safe default"
        );
        assert!(
            source.contains(checked),
            "{name} try_is_empty should compute dimensional emptiness without recursing"
        );
        assert!(
            !source.contains("pub fn is_empty(&self) -> bool {\n        self.rows == 0")
                && !source.contains("Ok(self.is_empty())"),
            "{name} owned is_empty should not bypass validation or recurse from try_is_empty"
        );
    }
}

#[test]
fn multipoint_and_ip_helpers_avoid_direct_tail_slices() {
    const MULTI_POINT: &str = include_str!("../src/geom/multi_point.rs");
    const POLYGON: &str = include_str!("../src/geom/polygon.rs");
    const RING: &str = include_str!("../src/geom/ring.rs");
    const IP: &str = include_str!("../src/id/ip.rs");

    assert!(
        MULTI_POINT.contains("self.points.iter().skip(1)"),
        "MultiPoint bbox should iterate tail points without direct range slicing"
    );
    assert!(
        !MULTI_POINT.contains("&self.points[1..]"),
        "MultiPoint bbox should not retain direct tail slice"
    );

    for needle in [
        ".windows(2)",
        ".zip(self.vertices.iter().cycle().skip(1))",
        "self.vertices.first().copied()",
    ] {
        assert!(
            POLYGON.contains(needle),
            "Polygon geometry helpers should use iterator-safe access pattern {needle:?}"
        );
    }
    for forbidden in [
        "self.vertices[i - 1]",
        "self.vertices[i]",
        "self.vertices[j]",
        "self.vertices[0]",
    ] {
        assert!(
            !POLYGON.contains(forbidden),
            "Polygon geometry helpers should not retain direct vector indexing pattern {forbidden:?}"
        );
    }

    assert!(
        RING.contains(".windows(2)"),
        "Ring area/length helpers should use windows instead of direct indexing"
    );
    for forbidden in ["self.points[i]", "self.points[i + 1]"] {
        assert!(
            !RING.contains(forbidden),
            "Ring helpers should not retain direct vector indexing pattern {forbidden:?}"
        );
    }

    assert!(
        IP.contains("header.bytes.get(4..)"),
        "Ip validation should inspect IPv4 padding through checked get(4..)"
    );
    assert!(
        !IP.contains("header.bytes[4..]"),
        "Ip validation should not retain direct IPv4 padding slice"
    );
}

#[test]
fn uuid_v4_generation_prefers_os_randomness_and_fallible_api() {
    const UUID: &str = include_str!("../src/id/uuid.rs");

    for needle in [
        "pub fn try_generate_v4() -> io::Result<Self>",
        "fill_os_random(&mut bytes)?",
        "std::fs::File::open(\"/dev/urandom\")?.read_exact(bytes)",
        "Self::try_generate_v4().unwrap_or_else(|_| Self::fallback_v4())",
        "static UUID_FALLBACK_COUNTER: AtomicU64",
        "Err(error) => error.duration().as_nanos() ^ u128::MAX",
        "let process_id = u128::from(std::process::id())",
        "fn mix_fallback_state(nanos: u128, counter: u64, process_id: u128) -> u128",
        "let counter = u128::from(counter)",
    ] {
        assert!(
            UUID.contains(needle),
            "Uuid v4 generation should prefer OS randomness and expose a fallible path via {needle:?}"
        );
    }

    assert!(
        !UUID.contains(".unwrap_or(0)"),
        "Uuid fallback generation should not silently collapse clock errors to zero"
    );
    for forbidden in ["as *const", "as usize", "stack_marker"] {
        assert!(
            !UUID.contains(forbidden),
            "Uuid fallback generation should not mix stack addresses via {forbidden:?}"
        );
    }
}

#[test]
fn deque_queue_stack_accessors_use_checked_byte_ranges() {
    for (name, source, required, forbidden) in [
        (
            "Deque",
            include_str!("../src/seq/deque.rs"),
            &[
                ".get(start..end)",
                ".get(..es)",
                "pub fn try_size(&self) -> Result<usize, WireError>",
                "self.try_size().unwrap_or(0)",
                "self.try_empty().unwrap_or(true)",
                "Ok(self.try_size()? == 0)",
                "if self.try_empty()?",
                ".checked_sub(es)",
                ".checked_sub(self.element_size)",
                "fn try_element_size(&self) -> Result<usize, WireError>",
                "fn try_split_byte(&self) -> Result<usize, WireError>",
                "super::checked_u32_to_usize::<Self>(self.element_size, \"element_size\")",
                "super::checked_u32_to_usize::<Self>(self.split_byte, \"split_byte\")",
                "super::checked_u32_to_usize::<P>(stored, \"element_size\")",
            ][..],
            &[
                "&self.data[start..end]",
                "&self.data[..es]",
                "self.data[start..end]",
                "self.data[..es]",
                "self.element_size as usize",
                "self.split_byte as usize",
                "stored as usize",
                "if self.empty()",
                "end - es",
                "self.split_byte -= es as u32",
                "Ok(self.size())",
                "Ok(self.empty())",
            ][..],
        ),
        (
            "Queue",
            include_str!("../src/seq/queue.rs"),
            &[
                ".get(byte_start..byte_end)",
                ".checked_sub(es)",
                "raw.checked_sub(self.try_front_usize()?)",
                "fn try_element_size(&self) -> Result<usize, WireError>",
                "fn try_front_usize(&self) -> Result<usize, WireError>",
                "fn try_raw_len(&self) -> Result<usize, WireError>",
                "u32_to_usize::<Self>(self.front, \"front\")",
                "u32_to_usize::<P>(stored, \"element_size\")",
                "queue front exceeds raw length",
                "Ok(self.try_size()? == 0)",
                ".checked_mul(2)",
                ".checked_mul(es)",
                "drop_bytes > self.data.len()",
                ".get(byte_start..byte_end)",
                "self.try_size().unwrap_or(0)",
                "self.try_empty().unwrap_or(true)",
            ][..],
            &[
                "&self.data[byte_start..byte_end]",
                "&self.data[byte_end - es..byte_end]",
                "self.data[byte_start..byte_end]",
                "self.data[byte_end - es..byte_end]",
                "self.element_size as usize",
                "self.front as usize",
                "stored as usize",
                "(self.front as usize) * 2",
                "self.front as usize * es",
                "Ok(self.size())",
                "Ok(self.empty())",
                "saturating_sub(self.front as usize)",
                "self.size() == 0",
            ][..],
        ),
        (
            "Stack",
            include_str!("../src/seq/stack.rs"),
            &[
                ".get(start..)",
                ".checked_sub(es)",
                "self.try_size().unwrap_or(0)",
                "self.try_empty().unwrap_or(true)",
                "Ok(self.try_size()? == 0)",
                "fn try_element_size(&self) -> Result<usize, WireError>",
                "super::checked_u32_to_usize::<Self>(self.element_size, \"element_size\")",
                "super::checked_u32_to_usize::<P>(stored, \"element_size\")",
            ][..],
            &[
                "&self.data[start..]",
                "self.data[start..]",
                "self.element_size as usize",
                "stored as usize",
                "Ok(self.size())",
                "Ok(self.empty())",
            ][..],
        ),
    ] {
        for needle in required {
            assert!(
                source.contains(needle),
                "{name} byte accessors should use checked range helper {needle:?}"
            );
        }
        for needle in forbidden {
            assert!(
                !source.contains(needle),
                "{name} byte accessors should not retain direct indexing pattern {needle:?}"
            );
        }
    }
}

#[test]
fn raster_world_conversions_use_checked_fallible_paths() {
    const RASTER_MOD_RS: &str = include_str!("../src/raster/mod.rs");

    for needle in [
        "pub(super) fn checked_world_axis<T: 'static>(",
        "fn rounded_axis_index_u32<T: 'static>(",
        "usize::try_from(index)",
        "f64::from(max_index)",
        "f64::from(mid)",
        "mid.checked_add(1)",
    ] {
        assert!(
            RASTER_MOD_RS.contains(needle),
            "shared raster world-axis conversion should use checked/clamped conversion path {needle:?}"
        );
    }
    {
        let forbidden = "value.clamp(0.0, max_index as f64) as usize";
        assert!(
            !RASTER_MOD_RS.contains(forbidden),
            "shared raster world-axis conversion should not retain unchecked float-to-usize cast {forbidden:?}"
        );
    }

    for (name, source, method, try_method, forbidden) in [
        (
            "Grid",
            include_str!("../src/raster/grid.rs"),
            "pub fn world_to_grid(&self, world_point: Point) -> (usize, usize)",
            "pub fn try_world_to_grid(&self, world_point: Point) -> Result<(usize, usize), WireError>",
            "saturating_sub(1)",
        ),
        (
            "Layer",
            include_str!("../src/raster/layer.rs"),
            "pub fn world_to_voxel(&self, world_point: Point) -> (usize, usize, usize)",
            "pub fn try_world_to_voxel(\n        &self,\n        world_point: Point,\n    ) -> Result<(usize, usize, usize), WireError>",
            "saturating_sub(1)",
        ),
    ] {
        for needle in [
            method,
            try_method,
            "self.validate_owned()?",
            ".checked_sub(1)",
            "super::checked_world_axis::<Self>",
            "coordinate is not finite",
        ] {
            assert!(
                source.contains(needle) || RASTER_MOD_RS.contains(needle),
                "{name} world conversion should expose/use checked fallible path via {needle:?}"
            );
        }
        assert!(
            !source.contains(forbidden),
            "{name} world conversion should not use saturating max-index arithmetic"
        );
    }
}

#[test]
fn utm_band_helpers_use_checked_constants_and_fallible_setter() {
    const UTM_RS: &str = include_str!("../src/world/utm.rs");

    for needle in [
        "const BAND_N: u32 = 78;",
        "pub fn try_set_band_char(&mut self, c: char) -> Result<(), WireError>",
        "let band = u32::from(c);",
        "if !is_valid_band_code(band)",
        "fn is_valid_band_code(band: u32) -> bool",
        "self.is_valid_band() && self.band >= BAND_N",
    ] {
        assert!(
            UTM_RS.contains(needle),
            "UTM band helpers should keep checked/fallible path {needle:?}"
        );
    }

    for forbidden in [
        "b'N' as u32",
        "c as u32",
        "b'C' as u32",
        "b'I' as u32",
        "b'O' as u32",
        "b'X' as u32",
        "self.band >= b'N' as u32",
    ] {
        assert!(
            !UTM_RS.contains(forbidden),
            "UTM band helpers should not retain unchecked literal/char cast {forbidden:?}"
        );
    }
}

#[test]
fn raster_dimension_helpers_use_checked_conversions() {
    for (name, source, required, forbidden) in [
        (
            "Grid",
            include_str!("../src/raster/grid.rs"),
            &[
                "pub fn try_new(",
                "grid_dims::<Self>",
                "grid_dims::<Grid>",
                "u32_to_usize::<P>(rows, \"rows\")",
                "u32_to_usize::<P>(cols, \"cols\")",
            ][..],
            &[
                "self.rows as usize",
                "self.cols as usize",
                "header.rows as usize",
                "header.cols as usize",
                "self.header.rows as usize",
                "self.header.cols as usize",
            ][..],
        ),
        (
            "Layer",
            include_str!("../src/raster/layer.rs"),
            &[
                "pub fn try_layer_count(&self) -> Result<usize, WireError>",
                "self.try_layer_count().unwrap_or(0)",
                "layer_dims::<Self>",
                "layer_dims::<Layer>",
                "u32_to_usize::<P>(rows, \"rows\")",
                "u32_to_usize::<P>(cols, \"cols\")",
                "u32_to_usize::<P>(layers, \"layers\")",
            ][..],
            &[
                "self.rows as usize",
                "self.cols as usize",
                "self.layers as usize",
                "header.rows as usize",
                "header.cols as usize",
                "header.layers as usize",
                "self.header.rows as usize",
                "self.header.cols as usize",
                "self.header.layers as usize",
            ][..],
        ),
    ] {
        for needle in required {
            assert!(
                source.contains(needle),
                "{name} dimension helpers should use checked conversion path {needle:?}"
            );
        }
        for needle in forbidden {
            assert!(
                !source.contains(needle),
                "{name} dimension helpers should not retain unchecked dimension cast {needle:?}"
            );
        }
    }
}

#[test]
fn sized_payload_validation_uses_checked_header_conversions() {
    let source = include_str!("../src/seq/mod.rs");
    for needle in [
        "checked_u32_to_usize::<Self>(header.front, \"front\")",
        "checked_u32_to_usize::<Self>(header.split_byte, \"split_byte\")",
        "checked_u32_to_usize::<Self>(header.element_size, \"element_size\")",
        "let element_size = checked_u32_to_usize::<T>(element_size, \"element_size\")?",
    ] {
        assert!(
            source.contains(needle),
            "shared sequence validation should use checked header conversion {needle:?}"
        );
    }
    for forbidden in [
        "header.front as usize",
        "header.split_byte as usize",
        "header.element_size as usize",
        "payload.len() / element_size as usize",
        "payload.len() % element_size as usize",
    ] {
        assert!(
            !source.contains(forbidden),
            "shared sequence validation should avoid unchecked header cast {forbidden:?}"
        );
    }
}

#[test]
fn list_and_forward_list_slot_helpers_use_checked_byte_ranges() {
    for (name, source, required, forbidden) in [
        (
            "List",
            include_str!("../src/seq/list.rs"),
            &[
                "fn try_node_size(&self) -> Result<usize, WireError>",
                "self.try_node_size().unwrap_or(0)",
                "let node_size = self.try_node_size().ok()?;",
                "let slot = super::checked_u32_to_usize::<Self>(slot, \"slot\").ok()?;",
                "slot.checked_mul(node_size)",
                "let element_size = self.try_element_size().ok()?;",
                "offset.checked_add(element_size)",
                "offset.checked_add(4)",
                ".checked_sub(1)",
                "self.data.get_mut(o..end)",
                "self.data.get(o..end)",
                "value_bytes.fill(0);",
                "self.try_size().unwrap_or(0)",
                "self.try_empty().unwrap_or(true)",
                "self.try_capacity().unwrap_or(0)",
                "Ok(self.try_size()? == 0)",
                "self.try_slot_count()",
                "let ns = self.try_node_size()?;",
                "fn try_element_size(&self) -> Result<usize, WireError>",
                "super::checked_u32_to_usize::<Self>(self.element_size, \"element_size\")",
                "super::checked_u32_to_usize::<Self>(self.size_, \"size_\")",
                "super::checked_u32_to_usize::<P>(stored, \"element_size\")",
            ][..],
            &[
                "slot as usize",
                "slot as usize * self.node_size()",
                "(slot as usize).checked_mul(self.node_size())",
                "self.value_offset(slot) + self.element_size as usize",
                "self.prev_offset(slot) + 4",
                "self.element_size as usize",
                "self.size_ as usize",
                "stored as usize",
                "self.size_ -= 1;",
                "self.data[o..o + 4].copy_from_slice",
                "&self.data[o..o + self.element_size as usize]",
                "self.data[o..o + es].copy_from_slice",
                "&mut self.data[o..o + es]",
                "Ok(self.size())",
                "Ok(self.empty())",
                "Ok(self.capacity())",
                "None => usize::MAX",
                "if ns == usize::MAX",
            ][..],
        ),
        (
            "ForwardList",
            include_str!("../src/seq/forward_list.rs"),
            &[
                "fn try_node_size(&self) -> Result<usize, WireError>",
                "self.try_node_size().unwrap_or(0)",
                "let node_size = self.try_node_size().ok()?;",
                "let slot = super::checked_u32_to_usize::<Self>(slot, \"slot\").ok()?;",
                "slot.checked_mul(node_size)",
                "let element_size = self.try_element_size().ok()?;",
                "offset.checked_add(element_size)",
                ".checked_sub(1)",
                "self.data.get_mut(o..end)",
                "self.data.get(o..end)",
                "value_bytes.fill(0);",
                "self.try_size().unwrap_or(0)",
                "self.try_empty().unwrap_or(true)",
                "self.try_capacity().unwrap_or(0)",
                "Ok(self.try_size()? == 0)",
                "self.try_slot_count()",
                "let ns = self.try_node_size()?;",
                "fn try_element_size(&self) -> Result<usize, WireError>",
                "super::checked_u32_to_usize::<Self>(self.element_size, \"element_size\")",
                "super::checked_u32_to_usize::<Self>(self.size_, \"size_\")",
                "super::checked_u32_to_usize::<P>(stored, \"element_size\")",
            ][..],
            &[
                "slot as usize",
                "slot as usize * self.node_size()",
                "(slot as usize).checked_mul(self.node_size())",
                "self.value_offset(slot) + self.element_size as usize",
                "self.element_size as usize",
                "self.size_ as usize",
                "stored as usize",
                "self.size_ -= 1;",
                "self.data[o..o + 4].copy_from_slice",
                "&self.data[o..o + self.element_size as usize]",
                "self.data[o..o + es].copy_from_slice",
                "&mut self.data[o..o + es]",
                "Ok(self.size())",
                "Ok(self.empty())",
                "Ok(self.capacity())",
                "None => usize::MAX",
                "if ns == usize::MAX",
            ][..],
        ),
    ] {
        for needle in required {
            assert!(
                source.contains(needle),
                "{name} slot helpers should use checked range helper {needle:?}"
            );
        }
        for needle in forbidden {
            assert!(
                !source.contains(needle),
                "{name} slot helpers should not retain direct indexing pattern {needle:?}"
            );
        }
    }
}

#[test]
fn heap_and_indexed_heap_helpers_use_checked_byte_ranges() {
    for (name, source, required, forbidden) in [
        (
            "Heap",
            include_str!("../src/seq/heap.rs"),
            &[
                "index.checked_mul(es)",
                "a.checked_mul(es)",
                "b.checked_mul(es)",
                "checked_add(offset)",
                "idx.checked_mul(2)",
                "let Some(r) = l.checked_add(1) else",
                "self.data.get(start..end)",
                "bytemuck::Zeroable::zeroed()",
                "pub fn try_size(&self) -> Result<usize, WireError>",
                "self.try_size().unwrap_or(0)",
                "self.try_empty().unwrap_or(true)",
                "Ok(self.try_size()? == 0)",
                "if self.try_empty()?",
                "let n = self.try_size()?",
                "fn try_sift_down<T: bytemuck::Pod + PartialOrd>",
                "self.try_sift_down::<T>(0)?;",
                "fn try_element_size(&self) -> Result<usize, WireError>",
                "super::checked_u32_to_usize::<Self>(self.element_size, \"element_size\")",
                "super::checked_u32_to_usize::<P>(stored, \"element_size\")",
            ][..],
            &[
                "&self.data[start..start + es]",
                "self.data[start..start + es]",
                "let oa = a * es;",
                "let ob = b * es;",
                "self.data.swap(oa + offset, ob + offset)",
                "let l = 2 * idx + 1;",
                "let r = 2 * idx + 2;",
                "checked_add(1).unwrap_or(usize::MAX)",
                "if self.empty()",
                "fn sift_down<T: bytemuck::Pod + PartialOrd>",
                "let n = self.size();",
                "Ok(self.size())",
                "Ok(self.empty())",
                "self.element_size as usize",
                "stored as usize",
            ][..],
        ),
        (
            "IndexedHeap",
            include_str!("../src/seq/indexed_heap.rs"),
            &[
                "i.checked_mul(self.entry_size())",
                "a.checked_mul(es)",
                "b.checked_mul(es)",
                "checked_add(k)",
                ".checked_sub(es)",
                "idx.checked_mul(2)",
                "let Some(r) = l.checked_add(1) else",
                "self.data.get_mut(off..end)",
                "self.data.get(off..end)",
                "bytemuck::Zeroable::zeroed()",
                "fn try_entry_size(&self) -> Result<usize, WireError>",
                "self.try_entry_size().unwrap_or(0)",
                "let entry_size = self.try_entry_size()?",
                "let last = self.try_size()?.checked_sub(1)",
                "let n = self.try_size()?",
                "if self.try_empty()?",
                "fn try_sift_down<T: bytemuck::Pod + PartialOrd>",
                "self.try_sift_down::<T>(0)?;",
                "self.try_sift_down::<T>(idx)?;",
                "pub fn try_size(&self) -> Result<usize, WireError>",
                "self.validate_owned()?",
                "self.try_size().unwrap_or(0)",
                "self.try_empty().unwrap_or(true)",
                "fn try_priority_size(&self) -> Result<usize, WireError>",
                "super::checked_u32_to_usize::<Self>(self.priority_size, \"priority_size\")",
                "super::checked_u32_to_usize::<P>(stored, \"priority_size\")",
            ][..],
            &[
                "fn key_at(&self, i: usize)",
                "self.try_key_at(i).unwrap_or(0)",
                "self.data[off..off + 8].copy_from_slice",
                "&self.data[off..off + self.priority_size as usize]",
                "self.data[off..off + ps].copy_from_slice",
                "i * self.entry_size()",
                "let oa = a * es;",
                "let ob = b * es;",
                "self.data.swap(oa + k, ob + k)",
                "let l = 2 * idx + 1;",
                "let r = 2 * idx + 2;",
                "checked_add(1).unwrap_or(usize::MAX)",
                "let last = self.size() - 1",
                "if self.empty()",
                "fn sift_down<T: bytemuck::Pod + PartialOrd>",
                "let n = self.size();",
                "self.priority_size as usize",
                "stored as usize",
            ][..],
        ),
    ] {
        for needle in required {
            assert!(
                source.contains(needle),
                "{name} byte helpers should use checked range helper {needle:?}"
            );
        }
        for needle in forbidden {
            assert!(
                !source.contains(needle),
                "{name} byte helpers should not retain direct indexing pattern {needle:?}"
            );
        }
    }
}

#[test]
fn assoc_map_set_copy_old_values_fallibly() {
    for (name, source, needles) in [
        (
            "Map",
            include_str!("../src/assoc/map.rs"),
            &[
                "let old = Self::try_copy_bytes(self.try_value_at(idx)?, \"old map value\")?;",
                "let old = Self::try_copy_bytes(self.try_value_at(idx)?, \"removed map value\")?;",
                "fn try_copy_bytes(bytes: &[u8], context: &'static str) -> Result<Vec<u8>, WireError>",
            ][..],
        ),
        (
            "Set",
            include_str!("../src/assoc/set.rs"),
            &[
                "let mut data = Vec::new();",
                "data.extend_from_slice(&0u32.to_le_bytes());",
            ][..],
        ),
    ] {
        for needle in needles {
            assert!(
                source.contains(needle),
                "{name} should use explicit/fallible copy initialization via {needle:?}"
            );
        }
    }

    for (name, source) in [
        ("Map", include_str!("../src/assoc/map.rs")),
        ("Set", include_str!("../src/assoc/set.rs")),
    ] {
        assert!(
            !source.contains("0u32.to_le_bytes().to_vec()"),
            "{name} default payload bootstrap should not use infallible to_vec"
        );
    }
}

#[test]
fn assoc_map_set_validated_accessors_use_checked_ranges() {
    for (name, source, forbidden) in [
        (
            "Map",
            include_str!("../src/assoc/map.rs"),
            &[
                "base + e.key_off as usize",
                "e.key_off + e.key_len",
                "base + e.value_off as usize",
                "e.value_off + e.value_len",
                "let start = 4 + i * ENTRY_SIZE;",
            ][..],
        ),
        (
            "Set",
            include_str!("../src/assoc/set.rs"),
            &[
                "base + e.key_off as usize",
                "e.key_off + e.key_len",
                "let start = 4 + i * ENTRY_SIZE;",
            ][..],
        ),
    ] {
        assert!(
            source.contains("fn blob_range_validated"),
            "{name} should route owned validated key/value access through a checked range helper"
        );
        assert!(
            source.contains(".get(0..4)"),
            "{name} payload count decode should avoid direct payload[0..4] slicing"
        );
        assert!(
            source.contains(".get_mut(0..4)"),
            "{name} count update should avoid direct payload[0..4] slicing"
        );
        assert!(
            source.contains("let offset = u32_to_usize::<Self>(offset, \"blob offset\")?;")
                && source.contains("let len = u32_to_usize::<Self>(len, \"blob length\")?;")
                && source.contains(".checked_add(offset)")
                && source.contains(".checked_add(len)"),
            "{name} checked range helper should guard start/end arithmetic"
        );
        assert!(
            source.contains(".get(start..end)"),
            "{name} checked range helper should bounds-check slicing through get(start..end)"
        );
        assert!(
            source.contains(".checked_sub(blob_offset)"),
            "{name} validation should compute blob length with checked subtraction"
        );
        assert!(
            source.contains("checked_mul(ENTRY_SIZE)")
                && source.contains("and_then(|offset| 4usize.checked_add(offset))"),
            "{name} entry reads should guard table offset arithmetic"
        );
        assert!(
            source.contains("payload.get(absolute_start..absolute_end)"),
            "{name} validation blob helper should bounds-check absolute payload slicing"
        );
        assert!(
            source.contains(".get_mut(entries_start..blob_start)"),
            "{name} entry table write should use checked table range"
        );
        let view_validation =
            format!("{name}::validate_wire_parts(&{name}Header {{}}, self.data)?");
        assert!(
            source.contains(&view_validation),
            "{name} borrowed view size helpers should validate the full payload before trusting count"
        );
        assert!(
            source.contains("let snapshot_count = old_count")
                && source.contains(".checked_add(1)")
                && source.contains(".checked_sub(1)")
                && source.contains("Self::try_reserve_vec(&mut"),
            "{name} mutation snapshots should reserve checked insert/remove counts"
        );
        for needle in [
            "let len = self.try_size()?;",
            "let n = self.try_size()?;",
            "hi.checked_sub(lo)",
            "lo.checked_add(span / 2)",
            "mid.checked_add(1)",
            "let old_count = self.try_count_usize()?;",
            "fn try_count_usize(&self) -> Result<usize, WireError>",
            "fn u32_to_usize<P: 'static>(value: u32, field: &'static str) -> Result<usize, WireError>",
            "u32_to_usize::<Self>(self.try_count()?, \"count\")",
            "let count_usize = u32_to_usize::<Self>(count, \"entry count\")?;",
            "u32_to_usize::<",
        ] {
            assert!(
                source.contains(needle),
                "{name} fallible owned access/search/mutation path should use {needle:?}"
            );
        }
        assert!(
            source.contains("let entries_end = entries_start.checked_add(table_len)")
                && source.contains("new_data.resize(entries_end, 0);"),
            "{name} entry table resize should guard entries_start + table_len overflow"
        );
        for needle in forbidden {
            assert!(
                !source.contains(needle),
                "{name} owned validated accessors should not retain unchecked range arithmetic {needle:?}"
            );
        }
        for needle in [
            "self.data[0..4].copy_from_slice",
            "new_data[entries_start..blob_start].copy_from_slice",
            "new_data.resize(entries_start + table_len, 0)",
            "old_count + 1",
            "old_count.saturating_sub(1)",
            "if i >= self.size()",
            "let n = self.size()",
            "let old_count = self.count() as usize",
            "let mid = (lo + hi) / 2",
            "lo = mid + 1",
            "as usize",
        ] {
            assert!(
                !source.contains(needle),
                "{name} internal writes should not retain direct indexing pattern {needle:?}"
            );
        }
    }
}

#[test]
fn bytes_dpstr_and_bitvec_helpers_use_checked_ranges() {
    for (name, source, required, forbidden) in [
        (
            "Bytes",
            include_str!("../src/seq/bytes.rs"),
            &[
                "let Some(hay) = self.data.get(from..) else",
                "self.data.len().checked_sub(1)",
                "pos",
                ".checked_add(count)",
                ".unwrap_or(self.data.len())",
                ".get(pos..end)",
                ".get(i)",
            ][..],
            &[
                "self.data[from..]",
                "self.data[i]",
                "&self.data[pos..end]",
                ".unwrap_or(&[])",
                "saturating_sub(1)",
                "saturating_add(count)",
            ][..],
        ),
        (
            "DpStr",
            include_str!("../src/seq/string.rs"),
            &["let Some(hay) = self.data.get(from..) else"][..],
            &["&self.data[from..]", ".unwrap_or(&[])"][..],
        ),
        (
            "BitVec",
            include_str!("../src/seq/bitvec.rs"),
            &[
                ".get(i >> 3)",
                ".get_mut(i >> 3)",
                ".get(..full_bytes)",
                ".get(full_bytes)",
                ".get(index >> 3)",
                "payload.len().checked_sub(1)",
                "payload.get(tail_index)",
                "pub fn try_size(&self) -> Result<usize, WireError>",
                "self.try_size().unwrap_or(0)",
                "self.try_empty().unwrap_or(true)",
                "Ok(self.try_size()? == 0)",
                "let bits_u64 = u64::try_from(bits)",
                "let len = self.try_size()?;",
                "usize::try_from(self.bits)",
                "usize::try_from(self.header.bits)",
                "self.bits.checked_sub(1)",
                "let new_bits = usize::try_from(self.bits)",
                "self.data.len().checked_sub(1)",
                "let full_bytes = len / 8",
                "let tail_bits = len % 8",
            ][..],
            &[
                "bits as u64",
                "if i >= self.size()",
                "if index >= self.header.bits as usize",
                "self.bits -= 1",
                "self.bits as usize & 7",
                "(self.bits as usize) >> 3",
                "self.bits as usize % 8",
                "(self.data.len()) - 1",
                "(self.bits / 8) as usize",
                "(self.header.bits / 8) as usize",
                "self.data[i >> 3]",
                "self.data[(self.bits as usize) >> 3]",
                "self.data[..full_bytes]",
                "self.data[full_bytes]",
                "self.data[index >> 3]",
                "payload[payload.len() - 1]",
            ][..],
        ),
    ] {
        for needle in required {
            assert!(
                source.contains(needle),
                "{name} helpers should use checked range/index helper {needle:?}"
            );
        }
        for needle in forbidden {
            assert!(
                !source.contains(needle),
                "{name} helpers should not retain direct indexing pattern {needle:?}"
            );
        }
    }
}

#[test]
fn bytes_and_dpstr_expose_fallible_owned_allocation_paths() {
    for (name, source, needles) in [
        (
            "Bytes",
            include_str!("../src/seq/bytes.rs"),
            &[
                "pub fn try_from_slice(slice: &[u8]) -> Result<Self, WireError>",
                "data.try_reserve_exact(slice.len())",
                "pub fn try_append(&mut self, slice: &[u8]) -> Result<(), WireError>",
                "failed to reserve {} appended bytes",
                "pub fn try_substr(&self, pos: usize, count: usize) -> Result<Vec<u8>, WireError>",
                "out.try_reserve_exact(slice.len())",
            ][..],
        ),
        (
            "DpStr",
            include_str!("../src/seq/string.rs"),
            &[
                "pub fn try_from_str(s: &str) -> Result<Self, WireError>",
                "data.try_reserve_exact(bytes.len())",
                "pub fn try_append(&mut self, s: &str) -> Result<(), WireError>",
                "failed to reserve {} appended UTF-8 bytes",
                "pub fn try_push(&mut self, ch: char) -> Result<(), WireError>",
                "self.data.try_reserve_exact(s.len())",
                "pub fn try_as_str(&self) -> Result<&str, WireError>",
                "<Self as DataPodValidate>::validate_wire_parts(&DpStrHeader {}, &self.data)?",
                "pub fn try_to_string_lossy(&self) -> Result<String, WireError>",
                "output.try_reserve_exact(output_len)",
                "push_lossy_utf8::<Self>(&mut output, &self.data)?",
            ][..],
        ),
        (
            "DpString",
            include_str!("../src/id/string.rs"),
            &[
                "pub fn try_from_str(s: &str) -> Result<Self, WireError>",
                "bytes.try_reserve_exact(source.len())",
                "failed to reserve {} UTF-8 string bytes",
                "bytes.extend_from_slice(source);",
                "pub fn try_as_str(&self) -> Result<&str, WireError>",
                "<Self as DataPodValidate>::validate_wire_parts(&DpStringHeader::default(), &self.bytes)?",
            ][..],
        ),
    ] {
        for needle in needles {
            assert!(
                source.contains(needle),
                "{name} should expose fallible allocation path via {needle:?}"
            );
        }
        if name == "DpStr" {
            assert!(
                !source.contains("String::from_utf8_lossy"),
                "DpStr lossy conversion should avoid infallible from_utf8_lossy allocation"
            );
        }
    }
}

#[test]
fn python_wire_split_uses_borrowed_validation_and_fallible_copy() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "fn copy_bytes_for_python(label: &str, bytes: &[u8]) -> PyResult<Vec<u8>>",
        "out.try_reserve_exact(bytes.len())",
        "failed to reserve {} {label} bytes",
        "out.extend_from_slice(bytes);",
        "crate::validate_registered_wire_v1(type_hash, &wire)",
        "let header_range = wire",
        ".get(..header_size)",
        ".get(header_size..)",
        "copy_bytes_for_python(\"wire header\", header_range)",
        "copy_bytes_for_python(\"wire payload\", payload_range)",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python wire split helpers should avoid cloning before validation and copy fallibly via {needle:?}"
        );
    }

    for forbidden in [
        "validate_wire_message_v1(type_hash, wire.clone())",
        "copy_bytes_for_python(\"wire header\", &wire[..header_size])",
        "copy_bytes_for_python(\"wire payload\", &wire[header_size..])",
        "wire[header_size..].to_vec()",
        "wire[..header_size].to_vec()",
    ] {
        assert!(
            !PYTHON.contains(forbidden),
            "Python wire split helpers should not contain infallible/duplicate copy pattern {forbidden:?}"
        );
    }
}

#[test]
fn python_heap_split_wire_uses_fallible_header_payload_copy() {
    const HEAP_PY: &str = include_str!("../src/python/heap.rs");

    for needle in [
        "fn zeroed_bytes_for_python(label: &str, len: usize) -> PyResult<Vec<u8>>",
        "zeroed_bytes_for_python(\"heap header\", crate::bind::header_size::<T>())",
        "out.try_reserve_exact(len)",
        "out.resize(len, 0);",
        "fn copy_bytes_for_python(label: &str, bytes: &[u8]) -> PyResult<Vec<u8>>",
        "out.try_reserve_exact(bytes.len())",
        "failed to reserve {} {label} bytes",
        "let header = data",
        ".get(..header_size)",
        ".get(header_size..)",
        "copy_bytes_for_python(\"wire header\", header)",
        "copy_bytes_for_python(\"wire payload\", payload)",
    ] {
        assert!(
            HEAP_PY.contains(needle),
            "Python heap split_wire should copy header/payload fallibly via {needle:?}"
        );
    }

    assert!(
        !HEAP_PY.contains("Ok((data[..header_size].to_vec(), data[header_size..].to_vec()))"),
        "Python heap split_wire should not copy header/payload with infallible to_vec"
    );
    assert!(
        !HEAP_PY.contains("copy_bytes_for_python(\"wire header\", &data[..header_size])")
            && !HEAP_PY.contains("copy_bytes_for_python(\"wire payload\", &data[header_size..])"),
        "Python heap split_wire should not copy header/payload through direct indexing"
    );
    assert!(
        !HEAP_PY.contains("let mut out = vec![0_u8; crate::bind::header_size::<T>()]"),
        "Python heap header_bytes should not allocate with infallible vec! macro"
    );
    assert!(
        !HEAP_PY.contains("self.inner.payload_bytes().to_vec()"),
        "Python heap payload_bytes helpers should copy payloads through fallible copy_bytes_for_python"
    );
    assert!(
        HEAP_PY.contains("copy_bytes_for_python(\"payload\", self.inner.payload_bytes())")
            && HEAP_PY.contains("fn payload_bytes(&self) -> PyResult<Vec<u8>>"),
        "Python heap payload_bytes helpers should return PyResult and reserve before copying"
    );
}

#[test]
fn python_fixed_and_geometry_helpers_reserve_before_copying_bytes() {
    for (name, source, needles, forbidden) in [
        (
            "fixed",
            include_str!("../src/python/fixed.rs"),
            &[
                "fn zeroed_bytes_for_python(label: &str, len: usize) -> PyResult<Vec<u8>>",
                "out.try_reserve_exact(len)",
                "out.resize(len, 0);",
                "fn pod_bytes_for_python<T: bytemuck::Pod>(label: &str, value: &T) -> PyResult<Vec<u8>>",
                "out.try_reserve_exact(bytes.len())",
                "out.extend_from_slice(bytes);",
                "pod_bytes_for_python(\"MapEntry\", &value)",
                "pod_bytes_for_python(\"SetEntry\", &value)",
                ".get(..size)",
            ][..],
            &[
                "let mut out = vec![0_u8; crate::bind::header_size::<T>()]",
                "bytemuck::bytes_of(&value).to_vec()",
                "pod_read_unaligned::<MapEntry>(&data[..size])",
                "pod_read_unaligned::<SetEntry>(&data[..size])",
            ][..],
        ),
        (
            "geometry",
            include_str!("../src/python/geometry.rs"),
            &[
                "fn zeroed_bytes_for_python(label: &str, len: usize) -> PyResult<Vec<u8>>",
                "fn copy_bytes_for_python(label: &str, bytes: &[u8]) -> PyResult<Vec<u8>>",
                "out.try_reserve_exact(len)",
                "out.try_reserve_exact(bytes.len())",
                "copy_bytes_for_python(\"Polygon payload\", self.inner.payload_bytes())",
            ][..],
            &[
                "let mut out = vec![0_u8;",
                "self.inner.payload_bytes().to_vec()",
            ][..],
        ),
    ] {
        for needle in needles {
            assert!(
                source.contains(needle),
                "Python {name} helpers should reserve before copying via {needle:?}"
            );
        }
        for needle in forbidden {
            assert!(
                !source.contains(needle),
                "Python {name} helpers should not retain infallible byte-copy pattern {needle:?}"
            );
        }
    }
}

#[test]
fn python_binding_decode_parts_reserve_before_joining_payloads() {
    for (name, source) in [
        ("fixed", include_str!("../src/python/fixed.rs")),
        ("geometry", include_str!("../src/python/geometry.rs")),
        ("heap", include_str!("../src/python/heap.rs")),
    ] {
        for needle in [
            "PyMemoryError",
            "bytes.try_reserve_exact(payload.len())",
            "failed to reserve {} decoded payload bytes",
            "bytes.extend_from_slice(&payload);",
        ] {
            assert!(
                source.contains(needle),
                "Python {name} decode_parts should reserve before joining payload bytes via {needle:?}"
            );
        }
    }
}

#[test]
fn python_binding_allocation_failures_use_memory_error() {
    for (name, source) in [
        ("fixed", include_str!("../src/python/fixed.rs")),
        ("geometry", include_str!("../src/python/geometry.rs")),
        ("heap", include_str!("../src/python/heap.rs")),
        ("generic", include_str!("../src/python/mod.rs")),
    ] {
        assert!(
            source.contains("PyMemoryError") || source.contains("pyo3::exceptions::PyMemoryError"),
            "Python {name} module should map allocation failures to PyMemoryError"
        );
    }
}

#[test]
fn ragged_wire_validation_uses_checked_offset_table_arithmetic() {
    for (name, source) in [
        ("Vecvec", include_str!("../src/seq/vecvec.rs")),
        ("seq::mod", include_str!("../src/seq/mod.rs")),
    ] {
        for needle in [
            ".checked_sub(header_bytes)",
            ".checked_mul(4)",
            "4usize.checked_add(offset)",
            ".checked_sub(start)",
        ] {
            assert!(
                source.contains(needle),
                "{name} ragged validation should use checked offset-table arithmetic via {needle:?}"
            );
        }
        for forbidden in [
            "payload.len() - header_bytes",
            "4 + index * 4",
            "(end - start)",
        ] {
            assert!(
                !source.contains(forbidden),
                "{name} ragged validation should not retain unchecked pattern {forbidden:?}"
            );
        }
    }
}

#[test]
fn ragged_container_mutation_builds_rewritten_payload_fallibly_before_swap() {
    for (name, source) in [
        ("Vecvec", include_str!("../src/seq/vecvec.rs")),
        ("PagedVecvec", include_str!("../src/seq/paged_vecvec.rs")),
    ] {
        let start = source
            .find("pub fn try_push_bucket_bytes(&mut self, bytes: &[u8]) -> Result<(), WireError>")
            .unwrap_or_else(|| panic!("{name} try_push_bucket_bytes should exist"));
        let body = &source[start..];
        let end = body
            .find("\n    }\n}")
            .unwrap_or_else(|| panic!("{name} try_push_bucket_bytes body should be findable"));
        let body = &body[..end];

        for needle in [
            "let Some(payload) = self.data.get(old_header..) else",
            "let mut new_data = Vec::new();",
            "new_data.try_reserve_exact(new_len)",
            "new_data.extend_from_slice(payload);",
            "new_data.extend_from_slice(bytes);",
            "self.data = new_data;",
        ] {
            assert!(
                body.contains(needle),
                "{name} try_push_bucket_bytes should stage a fully reserved replacement before mutating via {needle:?}"
            );
        }

        for forbidden in [".to_vec()", "self.data.clear()"] {
            assert!(
                !body.contains(forbidden),
                "{name} try_push_bucket_bytes should not use infallible copy or mutate before staging replacement via {forbidden:?}"
            );
        }
    }
}

#[test]
fn ragged_container_size_helpers_validate_full_payloads() {
    for (name, source, header) in [
        (
            "Vecvec",
            include_str!("../src/seq/vecvec.rs"),
            "VecvecHeader",
        ),
        (
            "PagedVecvec",
            include_str!("../src/seq/paged_vecvec.rs"),
            "PagedVecvecHeader",
        ),
    ] {
        let start = source
            .find("pub fn try_size(&self) -> Result<usize, WireError>")
            .unwrap_or_else(|| panic!("{name} try_size should exist"));
        let body = &source[start..];
        let end = body
            .find("\n    }\n\n    pub fn empty")
            .unwrap_or_else(|| panic!("{name} try_size body should be findable"));
        let body = &body[..end];

        assert!(
            body.contains("<Self as DataPodValidate>::validate_wire_parts(")
                && body.contains(&format!("&{header} {{"))
                && body.contains("&self.data"),
            "{name} owned try_size should validate the full ragged payload before trusting bucket_count"
        );
    }

    let vecvec = include_str!("../src/seq/vecvec.rs");
    let view_start = vecvec
        .find("impl<'a> VecvecView<'a>")
        .expect("VecvecView impl should exist");
    let start = vecvec[view_start..]
        .find("pub fn try_bucket_count(&self) -> Result<u32, WireError>")
        .expect("VecvecView::try_bucket_count should exist")
        + view_start;
    let body = &vecvec[start..];
    let end = body
        .find("\n    }\n\n    pub fn size")
        .expect("VecvecView::try_bucket_count body should be findable");
    let body = &body[..end];
    assert!(
        body.contains("Vecvec::validate_wire_parts(&self.header, self.data)?"),
        "VecvecView::try_bucket_count should validate full borrowed payload shape before returning count"
    );
}

#[test]
fn ragged_container_bucket_views_use_checked_byte_ranges() {
    for (name, source) in [
        ("Vecvec", include_str!("../src/seq/vecvec.rs")),
        ("PagedVecvec", include_str!("../src/seq/paged_vecvec.rs")),
    ] {
        let start = source
            .find("pub fn try_bucket<T: bytemuck::Pod>(&self, i: usize) -> Result<&[T], WireError>")
            .unwrap_or_else(|| panic!("{name} try_bucket should exist"));
        let body = &source[start..];
        let end = body
            .find("\n\n    pub fn bucket_size")
            .unwrap_or_else(|| panic!("{name} try_bucket body should be findable"));
        let body = &body[..end];

        for needle in [
            ".get(start..end)",
            "bucket range is out of bounds",
            "bytemuck::try_cast_slice(bytes)",
            "checked_add(self.try_offset_usize(i)?)",
        ] {
            assert!(
                body.contains(needle),
                "{name} try_bucket should use checked bucket range via {needle:?}"
            );
        }
        assert!(
            !body.contains("&self.data[start..end]"),
            "{name} try_bucket should not pass a direct range-indexed slice to bytemuck"
        );
        for forbidden in [
            "self.try_offset(i)? as usize",
            "self.try_offset(i + 1)? as usize",
        ] {
            assert!(
                !body.contains(forbidden),
                "{name} try_bucket should not cast wire offsets through {forbidden:?}"
            );
        }
    }
}

#[test]
fn ragged_container_bucket_sizes_use_checked_offset_spans() {
    for (name, source) in [
        ("Vecvec", include_str!("../src/seq/vecvec.rs")),
        ("PagedVecvec", include_str!("../src/seq/paged_vecvec.rs")),
    ] {
        let start = source
            .find("pub fn try_bucket_size(&self, i: usize) -> Result<usize, WireError>")
            .unwrap_or_else(|| panic!("{name} try_bucket_size should exist"));
        let body = &source[start..];
        let end = body
            .find("\n\n    pub fn push_bucket")
            .or_else(|| body.find("\n\n    /// Append"))
            .unwrap_or_else(|| panic!("{name} try_bucket_size body should be findable"));
        let body = &body[..end];

        for needle in [
            "i\n            .checked_add(1)",
            "bucket next index overflowed",
            "let start = self.try_offset(i)?;",
            "let end = self.try_offset(next)?;",
            ".checked_sub(start)",
            "bucket span underflowed",
            "let element_size = self.try_element_size()?;",
            "super::checked_u32_to_usize::<Self>(span, \"bucket span\")?",
            "Ok(span / element_size)",
        ] {
            assert!(
                body.contains(needle),
                "{name} try_bucket_size should use checked offset-span arithmetic via {needle:?}"
            );
        }
        for forbidden in [
            "try_offset(i + 1)? - self.try_offset(i)?",
            "end - start",
            "span as usize",
            "self.element_size as usize",
        ] {
            assert!(
                !body.contains(forbidden),
                "{name} try_bucket_size should not retain unchecked span arithmetic {forbidden:?}"
            );
        }
    }
}

#[test]
fn ragged_container_owned_helpers_use_checked_header_conversions() {
    for (name, source) in [
        ("Vecvec", include_str!("../src/seq/vecvec.rs")),
        ("PagedVecvec", include_str!("../src/seq/paged_vecvec.rs")),
    ] {
        for needle in [
            "fn try_bucket_count_usize(&self) -> Result<usize, WireError>",
            "fn try_element_size(&self) -> Result<usize, WireError>",
            "fn try_offset_usize(&self, i: usize) -> Result<usize, WireError>",
            "super::checked_u32_to_usize::<Self>(self.try_bucket_count()?, \"bucket_count\")",
            "super::checked_u32_to_usize::<Self>(self.element_size, \"element_size\")",
            "let count_usize = super::checked_u32_to_usize::<Self>(count, \"bucket_count\")?;",
            "for i in 0..=count_usize",
            "super::checked_u32_to_usize::<P>(stored, \"element_size\")",
        ] {
            assert!(
                source.contains(needle),
                "{name} owned ragged helpers should use checked header conversions via {needle:?}"
            );
        }
        for forbidden in [
            "try_bucket_count()? as usize",
            "self.try_offset(i)? as usize",
            "self.try_offset(i + 1)? as usize",
            "count as usize",
            "self.element_size as usize",
            "stored as usize",
        ] {
            assert!(
                !source.contains(forbidden),
                "{name} owned ragged helpers should not retain unchecked conversion {forbidden:?}"
            );
        }
    }
}

#[test]
fn indexed_heap_index_build_uses_fallible_hashmap_reservation() {
    const INDEXED_HEAP: &str = include_str!("../src/seq/indexed_heap.rs");

    let start = INDEXED_HEAP
        .find("fn build_index_validated(&self) -> Result<HashMap<u64, usize>, WireError>")
        .expect("IndexedHeap::build_index_validated should be fallible");
    let body = &INDEXED_HEAP[start..];
    let end = body
        .find("\n\n    fn update_at")
        .expect("IndexedHeap::build_index_validated body should be findable");
    let body = &body[..end];

    for needle in [
        "let mut index = HashMap::new();",
        "index.try_reserve(size)",
        "failed to reserve {size} indexed heap index entries",
        "index.insert(self.try_key_at(i)?, i);",
        "Ok(index)",
    ] {
        assert!(
            body.contains(needle),
            "IndexedHeap::build_index_validated should allocate the side index fallibly via {needle:?}"
        );
    }

    assert!(
        !body.contains(".collect()"),
        "IndexedHeap::build_index_validated should not use infallible collect allocation"
    );
    assert!(
        !body.contains("self.key_at(i)"),
        "IndexedHeap::build_index_validated should not use fallback-zero key reads"
    );
}

#[test]
fn indexed_heap_wire_validation_reserves_duplicate_key_set_fallibly() {
    const SEQ_MOD: &str = include_str!("../src/seq/mod.rs");

    let start = SEQ_MOD
        .find("impl DataPodValidate for IndexedHeap")
        .expect("IndexedHeap validation impl should exist");
    let body = &SEQ_MOD[start..];
    let end = body
        .find("\n}\n\nimpl_byte_payload_access!(IndexedHeap")
        .expect("IndexedHeap validation impl body should be findable");
    let body = &body[..end];

    for needle in [
        "let entry_count = payload.len() / entry_size;",
        "let mut keys = HashSet::new();",
        "keys.try_reserve(entry_count)",
        "failed to reserve {entry_count} indexed heap key slots",
        "let key = read_checked_u64_le::<Self>(entry, 0)?;",
        "if !keys.insert(key)",
        "let priority_size = checked_u32_to_usize::<Self>(header.priority_size, \"priority_size\")?;",
        ".checked_add(priority_size)",
    ] {
        assert!(
            body.contains(needle),
            "IndexedHeap wire validation should reserve duplicate-key tracking fallibly via {needle:?}"
        );
    }
    assert!(
        SEQ_MOD.contains("fn read_checked_u64_le<T: 'static>"),
        "IndexedHeap wire validation should have a checked u64 reader"
    );
    assert!(
        !body.contains("read_u64_le(entry"),
        "IndexedHeap wire validation should not use fallback-zero u64 reads"
    );
    assert!(
        !body.contains("header.priority_size as usize"),
        "IndexedHeap wire validation should not truncate priority_size through unchecked casts"
    );
}

#[test]
fn linked_list_wire_validation_allocates_seen_bitmaps_fallibly() {
    const SEQ_MOD: &str = include_str!("../src/seq/mod.rs");

    for needle in [
        "let mut seen = bool_scratch::<T>(slots, \"active linked-list slots\")?;",
        "let mut free_seen = bool_scratch::<T>(slots, \"free linked-list slots\")?;",
        "if count.checked_add(free_count) != Some(slots)",
        "fn bool_scratch<T: 'static>(slots: usize, label: &'static str) -> Result<Vec<bool>, WireError>",
        "scratch.try_reserve_exact(slots)",
        "failed to reserve {slots} {label}",
        "scratch.resize(slots, false);",
    ] {
        assert!(
            SEQ_MOD.contains(needle),
            "linked-list wire validation should avoid infallible seen-bitmap allocation via {needle:?}"
        );
    }

    let start = SEQ_MOD
        .find("fn validate_linked_list_payload")
        .expect("linked-list validation helper should exist");
    let body = &SEQ_MOD[start..];
    let end = body
        .find("\n\nfn validate_index_or_nil")
        .expect("linked-list validation helper body should be findable");
    let body = &body[..end];
    assert!(
        !body.contains("vec![false; slots]"),
        "linked-list validation should not allocate seen bitmaps with infallible vec! macro"
    );
}

#[test]
fn linked_list_wire_validation_uses_checked_node_offsets() {
    const SEQ_MOD: &str = include_str!("../src/seq/mod.rs");

    for needle in [
        "fn node_offset<T: 'static>",
        "index\n        .checked_mul(node_size)",
        ".and_then(|base| base.checked_add(element_size))",
        "fn node_link_offset<T: 'static>",
        ".checked_add(link_offset)",
        "fn read_checked_u32_le<T: 'static>",
        "bytes.get(offset..end)",
        "let element_size = checked_u32_to_usize::<T>(element_size, \"element_size\")?;",
        "node_offset::<T>(index, node_size, element_size)?",
        "node_link_offset::<T>(",
        "read_checked_u32_le::<T>(",
        "let index = checked_u32_to_usize::<T>(cursor, \"node index\")?;",
        "let index = checked_u32_to_usize::<T>(cursor, \"free node index\")?;",
        "let value = checked_u32_to_usize::<T>(value, field)?;",
    ] {
        assert!(
            SEQ_MOD.contains(needle),
            "linked-list validation should fail closed through checked node offsets/readers via {needle:?}"
        );
    }

    let start = SEQ_MOD
        .find("fn validate_linked_list_payload")
        .expect("linked-list validation helper should exist");
    let body = &SEQ_MOD[start..];
    let end = body
        .find("\n\nfn validate_index_or_nil")
        .expect("linked-list validation helper body should be findable");
    let body = &body[..end];
    assert!(
        !body.contains("node_offset(index, node_size, element_size as usize) +"),
        "linked-list validation should not use unchecked addition after node_offset"
    );
    assert!(
        !body.contains("element_size as usize"),
        "linked-list validation should not truncate element_size through unchecked casts"
    );
    assert!(
        !body.contains("cursor as usize"),
        "linked-list validation should not truncate node cursors through unchecked casts"
    );
}

#[test]
fn vecvec_wire_validation_uses_checked_offset_reads() {
    const SEQ_MOD: &str = include_str!("../src/seq/mod.rs");
    const VECVEC: &str = include_str!("../src/seq/vecvec.rs");

    let shared_start = SEQ_MOD
        .find("fn validate_vecvec_payload")
        .expect("shared vecvec validation helper should exist");
    let shared_body = &SEQ_MOD[shared_start..];
    for needle in [
        "read_checked_u32_le::<T>(payload, 0)?",
        "read_checked_u32_le::<T>(payload, offset_start)?",
        "read_checked_u32_le::<T>(payload, start_offset)?",
        "read_checked_u32_le::<T>(payload, end_offset)?",
        "checked_u32_to_usize::<T>(read_checked_u32_le::<T>(payload, 0)?, \"bucket_count\")?",
        "checked_u32_to_usize::<T>(read_checked_u32_le::<T>(payload, offset_start)?, \"offset\")?",
        "let element_size = checked_u32_to_usize::<T>(element_size, \"element_size\")?;",
        "checked_u32_to_usize::<T>(read_checked_u32_le::<T>(payload, start_offset)?, \"start\")?",
        "checked_u32_to_usize::<T>(read_checked_u32_le::<T>(payload, end_offset)?, \"end\")?",
    ] {
        assert!(
            shared_body.contains(needle),
            "shared Vecvec/PagedVecvec validation should use checked offset reads via {needle:?}"
        );
    }
    assert!(
        !shared_body.contains("read_u32_le(payload"),
        "shared Vecvec/PagedVecvec validation should not use fallback-zero u32 reads"
    );
    assert!(
        !shared_body.contains("as usize"),
        "shared Vecvec/PagedVecvec validation should not use unchecked usize casts"
    );

    let vecvec_start = VECVEC
        .find("impl DataPodValidate for Vecvec")
        .expect("Vecvec validation impl should exist");
    let vecvec_body = &VECVEC[vecvec_start..];
    let vecvec_end = vecvec_body
        .find("\n\nimpl DataPodAccess for Vecvec")
        .or_else(|| vecvec_body.find("\n\nimpl_byte_payload_access!(Vecvec"))
        .expect("Vecvec validation impl body should be findable");
    let vecvec_body = &vecvec_body[..vecvec_end];
    for needle in [
        "read_u32_le::<Self>(payload, 0)?",
        "read_u32_le::<Self>(payload, offset_start)?",
        "read_u32_le::<Self>(payload, start_offset)?",
        "read_u32_le::<Self>(payload, end_offset)?",
        "super::checked_u32_to_usize::<Self>(read_u32_le::<Self>(payload, 0)?, \"bucket_count\")?",
        "super::checked_u32_to_usize::<Self>(",
        "let element_size =\n            super::checked_u32_to_usize::<Self>(header.element_size, \"element_size\")?;",
    ] {
        assert!(
            vecvec_body.contains(needle),
            "Vecvec validation should use checked offset reads via {needle:?}"
        );
    }
    assert!(
        !vecvec_body.contains("read_u32(payload"),
        "Vecvec validation should not use fallback-zero u32 reads"
    );
    assert!(
        !vecvec_body.contains("as usize"),
        "Vecvec validation should not use unchecked usize casts"
    );
}

#[test]
fn vecvec_unaligned_bucket_decode_uses_fallible_allocation() {
    const VECVEC: &str = include_str!("../src/seq/vecvec.rs");

    let start = VECVEC
        .find("pub fn bucket_unaligned<T: bytemuck::Pod + Copy>")
        .expect("VecvecView::bucket_unaligned should exist");
    let body = &VECVEC[start..];
    let end = body
        .find("\n\n    fn try_offset")
        .expect("VecvecView::bucket_unaligned body should be findable");
    let body = &body[..end];

    for needle in [
        "let count = bytes.len() / elem_size;",
        "let mut values = Vec::new();",
        "values.try_reserve_exact(count)",
        "failed to reserve {count} unaligned bucket elements",
        "values.extend(",
        ".chunks_exact(elem_size)",
        ".map(bytemuck::pod_read_unaligned::<T>)",
    ] {
        assert!(
            body.contains(needle),
            "VecvecView::bucket_unaligned should reserve fallibly before decoding via {needle:?}"
        );
    }

    assert!(
        !body.contains(".collect()"),
        "VecvecView::bucket_unaligned should not use infallible collect allocation"
    );
}

#[test]
fn python_error_paths_do_not_depend_on_user_controlled_class_lookup() {
    const PYTHON: &str = include_str!("../src/python/mod.rs");

    for needle in [
        "def _safe_datapod_label(obj):",
        "is_type = isinstance(obj, type)",
        "except Exception:",
        "is_type = False",
        "return type(obj).__name__",
        "is_numeric_target = isinstance(target, (int, bool, float))",
        "is_type_target = isinstance(target, type)",
        "cls = target if is_type_target else type(target)",
        "datapod schema target metadata lookup failed",
        "raise ValueError(f\"{_safe_datapod_label(obj)} is not a datapod object\")",
        "raise ValueError(f\"{_safe_datapod_label(obj)}.archive is not callable\")",
        "raise ValueError(f\"{_safe_datapod_label(cls)} is not a registered datapod type\")",
        "type_hash_value = _metadata_attr(type_or_hash, \"TYPE_HASH\")",
        "type_hash = _u64_type_hash(_metadata_attr(cls, \"TYPE_HASH\"), \"TYPE_HASH\")",
        "type_hash_value = _metadata_attr(inner_cls, \"TYPE_HASH\")",
        "return name if isinstance(name, str) else _safe_type_name(obj)",
        "if not isinstance(label, str):",
    ] {
        assert!(
            PYTHON.contains(needle),
            "Python public error paths should avoid user-controlled __class__/__name__ lookup via {needle:?}"
        );
    }

    for forbidden in [
        "cls = target if isinstance(target, type) else target.__class__",
        "raise ValueError(f\"{obj.__class__.__name__} is not a datapod object\")",
        "raise ValueError(f\"{obj.__class__.__name__}.archive is not callable\")",
        "hasattr(type_or_hash, \"TYPE_HASH\")",
        "getattr(type_or_hash, \"TYPE_HASH\")",
        "getattr(inner_cls, \"TYPE_HASH\", None)",
        "inner_cls.TYPE_HASH",
        "cls.TYPE_HASH",
    ] {
        assert!(
            !PYTHON.contains(forbidden),
            "Python public error paths should not contain hostile metadata lookup pattern {forbidden:?}"
        );
    }
}
