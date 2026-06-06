#[test]
fn registry_exposes_core_metadata_for_exported_types() {
    let grid_hash = datapod::bind::type_hash::<datapod::Grid>();
    let grid = datapod::registry::find_type_info(grid_hash).expect("Grid must be registered");
    assert_eq!(
        datapod::registry::try_find_type_info(grid_hash)
            .expect("fallible Grid lookup should not hit registry lock errors")
            .expect("Grid must be registered")
            .canonical_name,
        "datapod.grid.v1"
    );
    assert_eq!(grid.canonical_name, "datapod.grid.v1");
    assert_eq!(
        grid.canonical_type_hash,
        datapod::registry::type_hash_name("datapod.grid.v1")
    );
    assert_eq!(
        grid.header_size,
        datapod::registry::v1_header_size(grid_hash).unwrap()
    );
    assert_eq!(grid.payload_kind, datapod::registry::PayloadKind::Bytes);
    assert_eq!(grid.format_version, 1);
    assert_eq!(
        grid.wire_format,
        datapod::registry::WireFormat::DatapodWireV1Little
    );
    assert_eq!(grid.endian, datapod::registry::Endian::Little);
    assert_eq!(
        grid.alignment,
        datapod::registry::AlignmentPolicy::UnalignedWire
    );
    assert_eq!(grid.validator, datapod::registry::ValidatorKind::BuiltIn);
    assert_eq!(grid.emitted_hash, grid.canonical_type_hash);
    assert_eq!(
        grid.emitted_hash_kind,
        datapod::registry::HashKind::CanonicalName
    );
    assert!(grid.has_archive);
    assert!(grid.has_view);
    assert!(grid.has_owned_decode);
    assert_eq!(
        grid.archive_shape,
        datapod::registry::ArchiveShape::SinglePayload
    );
    assert_eq!(
        datapod::bind::emitted_type_hash::<datapod::Grid>(),
        grid.canonical_type_hash
    );
    assert_eq!(
        datapod::registry::WireFormat::from_name("datapod-wire-v1/le"),
        Some(datapod::registry::WireFormat::DatapodWireV1Little)
    );
    assert_eq!(
        datapod::registry::current_wire_format_name(),
        "datapod-wire-v1/le"
    );
    assert!(datapod::registry::builtin_hash_policy().contains("canonical-name"));
    assert!(datapod::registry::type_exists(grid_hash));
    assert_eq!(grid.type_hash, grid.canonical_type_hash);
    assert_eq!(
        datapod::registry::find_type_info_by_name("datapod.grid.v1")
            .expect("Grid canonical name lookup must resolve")
            .type_hash,
        grid_hash
    );
    assert_eq!(
        datapod::registry::try_find_type_info_by_name("datapod.grid.v1")
            .expect("fallible Grid canonical lookup should not hit registry lock errors")
            .expect("Grid canonical name lookup must resolve")
            .type_hash,
        grid_hash
    );
    assert!(
        datapod::registry::try_find_type_info(u64::MAX)
            .expect("unknown lookup should not hit registry lock errors")
            .is_none()
    );

    let matrix_hash = datapod::bind::type_hash::<datapod::Matrix>();
    let matrix = datapod::registry::find_type_info(matrix_hash).expect("Matrix must be registered");
    assert_eq!(matrix.canonical_name, "datapod.matrix.v1");
    assert_eq!(
        matrix.header_size,
        std::mem::size_of::<datapod::MatrixHeader>()
    );
    assert_eq!(matrix.payload_kind, datapod::registry::PayloadKind::Bytes);

    let point_hash = datapod::bind::type_hash::<datapod::Point>();
    let point = datapod::registry::find_type_info(point_hash).expect("Point must be registered");
    assert_eq!(point.canonical_name, "datapod.point.v1");
    assert_eq!(point.payload_kind, datapod::registry::PayloadKind::Fixed);
    assert_eq!(point.archive_shape, datapod::registry::ArchiveShape::Fixed);

    assert!(datapod::registry::type_count() >= 80);
    assert!(
        datapod::registry::all_type_infos()
            .iter()
            .any(|info| info.canonical_name == "datapod.point.v1")
    );
}

#[test]
fn type_hash_snapshots_catch_accidental_wire_id_changes() {
    assert_eq!(
        datapod::bind::type_hash::<datapod::Point>(),
        16752848715516365559
    );
    assert_eq!(
        datapod::bind::type_hash::<datapod::Grid>(),
        13945582450867521529
    );
    assert_eq!(
        datapod::bind::type_hash::<datapod::Matrix>(),
        3387574957426414502
    );
    assert_eq!(
        datapod::bind::type_hash::<datapod::Bytes>(),
        776550463611190628
    );
    assert_eq!(
        datapod::bind::type_hash::<datapod::Pose>(),
        9005792090823025934
    );
    assert_eq!(
        datapod::bind::type_hash::<datapod::Model>(),
        9031910666800182102
    );

    assert_eq!(
        datapod::bind::type_hash_canonical::<datapod::Point>(),
        Some(16752848715516365559)
    );
    assert_eq!(
        datapod::bind::type_hash_canonical::<datapod::Grid>(),
        Some(13945582450867521529)
    );
    assert_eq!(
        datapod::bind::type_hash_canonical::<datapod::Matrix>(),
        Some(3387574957426414502)
    );
    assert_eq!(
        datapod::bind::type_hash_canonical::<datapod::Bytes>(),
        Some(776550463611190628)
    );
    assert_eq!(
        datapod::bind::type_hash_canonical::<datapod::Pose>(),
        Some(9005792090823025934)
    );
    assert_eq!(
        datapod::bind::type_hash_canonical::<datapod::Model>(),
        Some(9031910666800182102)
    );
}

#[test]
fn canonical_hash_decodes_current_wire_message() {
    let point = datapod::Point::new(1.0, 2.0, 3.0);
    let canonical_hash =
        datapod::bind::type_hash_canonical::<datapod::Point>().expect("Point has canonical hash");

    let canonical_wire = datapod::to_wire_message(&point);
    assert_eq!(canonical_wire.type_hash, canonical_hash);

    let canonical = datapod::WireMessage {
        type_hash: canonical_hash,
        bytes: canonical_wire.bytes.clone(),
    };
    let decoded =
        datapod::from_wire_message::<datapod::Point>(&canonical).expect("canonical hash decodes");
    assert_eq!(decoded, point);
    datapod::validate_wire::<datapod::Point>(&canonical).expect("canonical hash validates");
    let view = datapod::access_wire::<datapod::Point>(&canonical).expect("canonical hash accesses");
    assert_eq!(view.value, point);
}

#[test]
fn registry_canonical_names_and_hashes_are_unique() {
    let infos = datapod::registry::all_type_infos();
    let mut names = std::collections::BTreeSet::new();
    let mut hashes = std::collections::BTreeSet::new();
    for info in &infos {
        assert!(
            names.insert(info.canonical_name.clone()),
            "duplicate name {}",
            info.canonical_name
        );
        assert!(
            hashes.insert(info.type_hash),
            "duplicate hash {}",
            info.type_hash
        );
    }
    // Runtime C/Python schemas are process-global and can be registered by
    // another concurrently running test in this binary. `all_type_infos()` is
    // internally consistent, but a later `type_count()` call may observe one
    // additional runtime registration.
    assert!(datapod::registry::type_count() >= infos.len());
}

#[test]
fn every_builtin_registry_entry_advertises_archive_view_owned_surface() {
    let infos = datapod::registry::all_type_infos();
    let builtins: Vec<_> = infos
        .iter()
        .filter(|info| info.validator == datapod::registry::ValidatorKind::BuiltIn)
        .collect();
    assert!(builtins.len() >= 80);

    for info in builtins {
        assert!(
            info.has_archive,
            "{} must expose archive",
            info.canonical_name
        );
        assert!(info.has_view, "{} must expose view", info.canonical_name);
        assert!(
            info.has_owned_decode,
            "{} must expose owned decode",
            info.canonical_name
        );
        match info.payload_kind {
            datapod::registry::PayloadKind::Fixed => assert_eq!(
                info.archive_shape,
                datapod::registry::ArchiveShape::Fixed,
                "{} fixed type must advertise fixed archive shape",
                info.canonical_name
            ),
            datapod::registry::PayloadKind::Bytes => assert_eq!(
                info.archive_shape,
                datapod::registry::ArchiveShape::SinglePayload,
                "{} built-in byte-payload type must advertise single-payload archive shape",
                info.canonical_name
            ),
        }
    }
}

#[test]
fn registered_validator_rejects_unknown_short_and_fixed_payload_messages() {
    assert!(matches!(
        datapod::validate_registered_wire(u64::MAX, &[]),
        Err(datapod::WireError::UnknownTypeHash { .. })
    ));

    for info in datapod::registry::all_type_infos()
        .into_iter()
        .filter(|info| info.validator == datapod::registry::ValidatorKind::BuiltIn)
    {
        if info.header_size > 0 {
            let short_header = vec![0_u8; info.header_size - 1];
            assert!(
                matches!(
                    datapod::validate_registered_wire(info.type_hash, &short_header),
                    Err(datapod::WireError::ShortHeader { .. })
                ),
                "{} should reject short registered headers",
                info.canonical_name
            );
        }

        if info.payload_kind == datapod::registry::PayloadKind::Fixed {
            let mut bytes = vec![0_u8; info.header_size];
            bytes.push(0xff);
            assert!(
                datapod::validate_registered_wire(info.type_hash, &bytes).is_err(),
                "{} should reject payload bytes for fixed registered messages",
                info.canonical_name
            );
        }
    }
}

#[test]
fn registered_frame_validator_uses_builtin_semantics_for_all_static_types() {
    let polygon_hash = datapod::bind::type_hash::<datapod::Polygon>();
    let valid = datapod::Polygon::new(vec![
        datapod::Point::new(0.0, 0.0, 0.0),
        datapod::Point::new(1.0, 0.0, 0.0),
        datapod::Point::new(0.0, 1.0, 0.0),
    ]);
    let valid = datapod::to_wire_message(&valid);
    let valid_frame =
        datapod::split_wire_frame(valid.type_hash, &valid.bytes).expect("polygon frame splits");
    datapod::validate_registered_wire_frame(valid_frame)
        .expect("registered frame validator accepts valid built-in polygon");

    let malformed_payload = [0_u8; 1];
    let malformed_frame = datapod::WireFrame {
        type_hash: polygon_hash,
        header: &[],
        payload: &malformed_payload,
    };
    assert!(
        matches!(
            datapod::validate_registered_wire_frame(malformed_frame),
            Err(datapod::WireError::InvalidPayloadSize { .. })
        ),
        "registered frame validation must dispatch to Polygon's element-size validator"
    );
}

#[test]
fn runtime_registry_accepts_custom_c_python_type_metadata() {
    let canonical = "acme.depth_image.v1";
    let hash = datapod::registry::type_hash_name(canonical);
    datapod::registry::register_type(hash, canonical, 12, datapod::registry::PayloadKind::Bytes)
        .expect("custom runtime type registration should succeed");

    let info = datapod::registry::find_type_info(hash).expect("custom type should be registered");
    assert_eq!(info.canonical_name, canonical);
    assert_eq!(info.canonical_type_hash, hash);
    assert_eq!(info.header_size, 12);
    assert_eq!(info.payload_kind, datapod::registry::PayloadKind::Bytes);
    assert_eq!(info.format_version, 1);
    assert_eq!(
        info.wire_format,
        datapod::registry::WireFormat::DatapodWireV1Little
    );
    assert_eq!(info.endian, datapod::registry::Endian::Little);
    assert_eq!(
        info.alignment,
        datapod::registry::AlignmentPolicy::UnalignedWire
    );
    assert_eq!(
        info.validator,
        datapod::registry::ValidatorKind::RuntimeSchema
    );
    assert_eq!(info.emitted_hash, hash);
    assert_eq!(
        info.emitted_hash_kind,
        datapod::registry::HashKind::CanonicalName
    );
    assert!(info.has_archive);
    assert!(info.has_view);
    assert!(!info.has_owned_decode);
    assert_eq!(
        info.archive_shape,
        datapod::registry::ArchiveShape::RuntimeSchema
    );
    assert!(datapod::registry::type_exists(hash));
}

#[test]
fn runtime_registry_rejects_noncanonical_type_hashes() {
    let canonical = "acme.bad_hash.v1";
    let expected = datapod::registry::type_hash_name(canonical);
    let wrong = expected ^ 0x55aa_55aa_55aa_55aa;

    let error = datapod::registry::register_type(
        wrong,
        canonical,
        4,
        datapod::registry::PayloadKind::Bytes,
    )
    .expect_err("runtime schema registration should reject mismatched hashes");

    assert!(matches!(
        error,
        datapod::registry::RegistryError::TypeHashCanonicalNameMismatch {
            expected_hash,
            provided_hash,
            ..
        } if expected_hash == expected && provided_hash == wrong
    ));
    assert!(
        !datapod::registry::type_exists(wrong),
        "mismatched runtime type hash must not be registered"
    );
    assert!(
        datapod::registry::find_type_info_by_name(canonical).is_none(),
        "mismatched runtime canonical name must not be registered"
    );
}

#[test]
fn runtime_registry_rejects_zero_type_hashes() {
    let canonical = "acme.zero_hash.v1";
    let error =
        datapod::registry::register_type(0, canonical, 4, datapod::registry::PayloadKind::Bytes)
            .expect_err("runtime schema registration should reject zero hashes");

    assert!(matches!(
        error,
        datapod::registry::RegistryError::InvalidTypeHash
    ));
    assert!(
        datapod::registry::find_type_info_by_name(canonical).is_none(),
        "zero-hash runtime canonical name must not be registered"
    );
}

#[test]
fn runtime_registry_rejects_nul_canonical_names_before_hash_registration() {
    let canonical = "acme.bad\0name.v1";
    let hash = datapod::registry::type_hash_name(canonical);

    let error =
        datapod::registry::register_type(hash, canonical, 4, datapod::registry::PayloadKind::Bytes)
            .expect_err("runtime schema registration should reject NUL-bearing canonical names");

    assert!(matches!(
        error,
        datapod::registry::RegistryError::InvalidCanonicalName { reason, .. }
            if reason.contains("NUL")
    ));
    assert!(
        !datapod::registry::type_exists(hash),
        "NUL-bearing runtime type name must not be registered by hash"
    );
    assert!(
        datapod::registry::find_type_info_by_name(canonical).is_none(),
        "NUL-bearing runtime type name must not be registered by name"
    );
}

#[test]
fn runtime_registry_rejects_malformed_canonical_names_before_hash_registration() {
    for canonical in [
        ".acme.bad.v1",
        "acme..bad.v1",
        "acme.bad.v1.",
        "Acme.bad.v1",
        "acme.bad-name.v1",
        "acme.bad name.v1",
        "acme.bad/name.v1",
    ] {
        let hash = datapod::registry::type_hash_name(canonical);
        let error = datapod::registry::register_type(
            hash,
            canonical,
            4,
            datapod::registry::PayloadKind::Bytes,
        )
        .expect_err("runtime schema registration should reject malformed canonical names");

        assert!(
            matches!(
                error,
                datapod::registry::RegistryError::InvalidCanonicalName { .. }
            ),
            "malformed canonical name {canonical:?} should be rejected, got {error:?}"
        );
        assert!(
            !datapod::registry::type_exists(hash),
            "malformed runtime type name {canonical:?} must not be registered by hash"
        );
        assert!(
            datapod::registry::find_type_info_by_name(canonical).is_none(),
            "malformed runtime type name {canonical:?} must not be registered by name"
        );
    }
}

#[test]
fn runtime_registry_rejects_impossible_header_sizes() {
    let canonical = "acme.huge_header.v1";
    let hash = datapod::registry::type_hash_name(canonical);

    let error = datapod::registry::register_type(
        hash,
        canonical,
        isize::MAX as usize + 1,
        datapod::registry::PayloadKind::Bytes,
    )
    .expect_err("runtime schema registration should reject impossible header sizes");

    assert!(matches!(
        error,
        datapod::registry::RegistryError::HeaderSizeTooLarge { header_size }
            if header_size == isize::MAX as usize + 1
    ));
    assert!(
        !datapod::registry::type_exists(hash),
        "impossible runtime header size must not be registered"
    );
    assert!(
        datapod::registry::find_type_info_by_name(canonical).is_none(),
        "impossible runtime header name must not be registered"
    );
}
