#[test]
fn registry_exposes_core_metadata_for_exported_types() {
    let grid_hash = datapod::bind::type_hash::<datapod::Grid>();
    let grid = datapod::registry::find_type_info(grid_hash).expect("Grid must be registered");
    assert_eq!(grid.canonical_name, "datapod.grid.v1");
    assert_eq!(grid.header_size, std::mem::size_of::<datapod::GridHeader>());
    assert_eq!(grid.payload_kind, datapod::registry::PayloadKind::Bytes);
    assert!(datapod::registry::type_exists(grid_hash));

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
        2930091146569838768
    );
    assert_eq!(
        datapod::bind::type_hash::<datapod::Grid>(),
        10346755810369800133
    );
    assert_eq!(
        datapod::bind::type_hash::<datapod::Matrix>(),
        7091211201439449723
    );
    assert_eq!(
        datapod::bind::type_hash::<datapod::Bytes>(),
        5344038153897440205
    );
    assert_eq!(
        datapod::bind::type_hash::<datapod::Pose>(),
        9896036077706990354
    );
    assert_eq!(
        datapod::bind::type_hash::<datapod::Model>(),
        9676918748838539918
    );
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
    assert_eq!(infos.len(), datapod::registry::type_count());
}

#[test]
fn runtime_registry_accepts_custom_c_python_type_metadata() {
    let canonical = "acme.depth_image.v1";
    let hash = datapod::registry::type_hash_name(canonical);
    datapod::registry::register_type(hash, canonical, 12, datapod::registry::PayloadKind::Bytes)
        .expect("custom runtime type registration should succeed");

    let info = datapod::registry::find_type_info(hash).expect("custom type should be registered");
    assert_eq!(info.canonical_name, canonical);
    assert_eq!(info.header_size, 12);
    assert_eq!(info.payload_kind, datapod::registry::PayloadKind::Bytes);
    assert!(datapod::registry::type_exists(hash));
}
