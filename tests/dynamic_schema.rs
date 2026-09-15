use datapod::dynamic::{self, DynamicValue};
use datapod::{Encoding, Grid, Matrix, Point, Pose, registry, to_wire_message};

#[test]
fn every_builtin_datapod_has_schema_fields() {
    let infos = registry::all_type_infos();
    let builtins: Vec<_> = infos
        .iter()
        .filter(|info| info.validator == registry::ValidatorKind::BuiltIn)
        .collect();
    assert!(builtins.len() >= 80);

    for info in builtins {
        let schema = registry::find_schema(info.type_hash)
            .unwrap_or_else(|| panic!("missing schema for {}", info.canonical_name));
        assert_eq!(schema.type_hash, info.type_hash);
        assert_eq!(schema.canonical_name, info.canonical_name);
        assert_eq!(schema.header_size, info.header_size);
        assert_eq!(schema.payload_kind, info.payload_kind);
        assert!(
            !schema.fields.is_empty(),
            "{} must expose dynamic schema fields",
            info.canonical_name
        );
    }
}

#[test]
fn point_dynamic_view_reads_header_fields_without_decoding_owned_point() {
    let msg = to_wire_message(&Point::new(1.0, 2.0, 3.0));
    let view = dynamic::view_message(msg.type_hash, &msg.bytes).expect("dynamic point view");

    assert_eq!(view.schema().canonical_name, "datapod.point.v1");
    assert_eq!(view.get_f64("x").unwrap(), 1.0);
    assert_eq!(view.get_f64("y").unwrap(), 2.0);
    assert_eq!(view.get_f64("z").unwrap(), 3.0);
    assert!(view.payload().is_empty());
}

#[test]
fn matrix_dynamic_view_borrows_payload() {
    let matrix =
        Matrix::from_bytes::<u16>(2, 3, bytemuck::cast_slice(&[1_u16, 2, 3, 4, 5, 6]).to_vec());
    let msg = to_wire_message(&matrix);
    let payload_start = datapod::registry::v1_header_size(msg.type_hash).unwrap();
    let original_payload = &msg.bytes[payload_start..];

    let view = dynamic::view_message(msg.type_hash, &msg.bytes).expect("dynamic matrix view");

    assert_eq!(view.schema().canonical_name, "datapod.matrix.v1");
    assert_eq!(view.get_u32("rows").unwrap(), 2);
    assert_eq!(view.get_u32("cols").unwrap(), 3);
    assert_eq!(view.get_u32("element_size").unwrap(), 2);
    assert_eq!(view.payload(), original_payload);
    assert_eq!(view.payload().as_ptr(), original_payload.as_ptr());
}

#[test]
fn grid_dynamic_view_reads_nested_encoding_and_borrows_payload() {
    let grid = Grid::new(
        2,
        2,
        Encoding::Rgba8,
        0.5,
        false,
        Pose::default(),
        (0_u8..16).collect(),
    );
    let msg = to_wire_message(&grid);
    let view = dynamic::view_message(msg.type_hash, &msg.bytes).expect("dynamic grid view");

    assert_eq!(view.schema().canonical_name, "datapod.grid.v1");
    assert_eq!(view.get_u32("rows").unwrap(), 2);
    assert_eq!(view.get_u32("cols").unwrap(), 2);
    assert_eq!(view.get_f64("resolution").unwrap(), 0.5);
    match view.field("encoding").unwrap() {
        DynamicValue::Nested(encoding) => {
            assert_eq!(encoding.get_u32("value").unwrap(), Encoding::Rgba8.0);
        }
        other => panic!("encoding should be nested dynamic value, got {other:?}"),
    }
    assert_eq!(view.payload(), &(0_u8..16).collect::<Vec<_>>());
}

#[test]
fn schema_field_type_wire_size_reports_overflow_instead_of_saturating() {
    let huge_array = datapod::schema::FieldType::Array {
        element: datapod::schema::ScalarType::U128,
        len: usize::MAX,
    };
    assert_eq!(huge_array.wire_size(), None);

    let huge_nested = datapod::schema::FieldType::NestedArray {
        type_hash: datapod::bind::type_hash::<Pose>(),
        len: usize::MAX,
    };
    assert_eq!(huge_nested.wire_size(), None);
}
