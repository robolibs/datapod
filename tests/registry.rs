#[test]
fn registry_exposes_core_metadata_for_exported_types() {
    let grid_hash = datapod::bind::type_hash::<datapod::Grid>();
    let grid = datapod::registry::find_type_info(grid_hash).expect("Grid must be registered");
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
    assert!(datapod::registry::type_exists(hash));
}
