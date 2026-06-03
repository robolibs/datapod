use datapod::{
    DataPod, LeWireHeader, Point, access_wire_bytes_v1, from_wire_message_v1, to_wire_message_v1,
    to_wire_message_v1_named, validate_wire_bytes_v1,
};

#[datapod::datapod]
struct PaddedFixed {
    pub tag: u8,
    pub value: u32,
}

#[test]
fn v1_emits_canonical_hash_and_little_endian_fixed_header() {
    let point = Point::new(1.0, 2.0, 3.0);
    let message = to_wire_message_v1(&point).expect("Point has a canonical datapod name");

    assert_eq!(
        message.type_hash,
        datapod::bind::type_hash_canonical::<Point>().unwrap()
    );
    assert_eq!(message.bytes.len(), <Point as LeWireHeader>::LE_WIRE_SIZE);

    let mut expected = Vec::new();
    expected.extend_from_slice(&1.0f64.to_bits().to_le_bytes());
    expected.extend_from_slice(&2.0f64.to_bits().to_le_bytes());
    expected.extend_from_slice(&3.0f64.to_bits().to_le_bytes());
    assert_eq!(message.bytes, expected);

    validate_wire_bytes_v1::<Point>(message.type_hash, &message.bytes).unwrap();
    let decoded = from_wire_message_v1::<Point>(&message).unwrap();
    assert_eq!(decoded, point);
}

#[test]
fn v1_header_codec_drops_native_struct_padding() {
    let value = PaddedFixed {
        tag: 7,
        value: 0x1122_3344,
    };
    let stable_hash = datapod::bind::type_hash_name("test.padded_fixed.v1");
    let stable = to_wire_message_v1_named(stable_hash, &value);

    assert!(core::mem::size_of::<PaddedFixed>() > stable.bytes.len());
    assert_eq!(stable.type_hash, stable_hash);
    assert_eq!(stable.bytes, [7, 0x44, 0x33, 0x22, 0x11]);

    let decoded_header = <PaddedFixed as LeWireHeader>::read_le(&stable.bytes).unwrap();
    assert_eq!(decoded_header, value);
}

#[test]
fn v1_heap_message_uses_le_header_and_borrowed_access() {
    let matrix = datapod::Matrix::from_bytes::<u16>(2, 2, vec![1, 0, 2, 0, 3, 0, 4, 0]);
    let message = to_wire_message_v1(&matrix).expect("Matrix has a canonical datapod name");

    assert_eq!(
        message.type_hash,
        datapod::bind::type_hash_canonical::<datapod::Matrix>().unwrap()
    );
    assert_eq!(
        message.bytes.len(),
        <datapod::MatrixHeader as LeWireHeader>::LE_WIRE_SIZE + matrix.payload_len()
    );
    assert_eq!(
        &message.bytes[..16],
        &[2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0]
    );
    assert_eq!(&message.bytes[16..], matrix.payload_bytes());

    validate_wire_bytes_v1::<datapod::Matrix>(message.type_hash, &message.bytes).unwrap();
    let view = access_wire_bytes_v1::<datapod::Matrix>(message.type_hash, &message.bytes).unwrap();
    assert_eq!(view.rows(), 2);
    assert_eq!(view.cols(), 2);
    assert_eq!(view.payload_bytes(), matrix.payload_bytes());
}

#[test]
fn v1_requires_canonical_hash_for_builtins() {
    let point = Point::new(1.0, 2.0, 3.0);
    let stable = to_wire_message_v1(&point).unwrap();
    let rust_path_hash = datapod::bind::rust_type_hash::<Point>();

    assert_ne!(rust_path_hash, stable.type_hash);
    assert!(matches!(
        validate_wire_bytes_v1::<Point>(rust_path_hash, &stable.bytes),
        Err(datapod::WireError::WrongTypeHash { .. })
    ));
}
