use datapod::{
    DataPod, DataPodDecode, DataPodValidate, Grid, LeWireHeader, Matrix, Point, WireError,
    from_wire_message, to_wire_message, to_wire_message_v1,
};

fn assert_decode<T>(value: T)
where
    T: DataPod + DataPodDecode + DataPodValidate + PartialEq + std::fmt::Debug,
    T::Header: LeWireHeader,
{
    let msg = to_wire_message(&value);
    let decoded: T = from_wire_message(&msg).expect("wire decode should succeed");
    assert_eq!(decoded, value);
}

#[test]
fn generic_wire_message_round_trips_fixed_datapod() {
    assert_decode(Point::new(1.0, 2.0, 3.0));
}

#[test]
fn generic_wire_message_round_trips_heap_datapods() {
    let grid = Grid::new(
        2,
        2,
        datapod::Encoding::Rgba8,
        0.05,
        false,
        datapod::Pose::default(),
        vec![
            255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255,
        ],
    );
    assert_decode(grid);

    let matrix =
        Matrix::from_bytes::<u16>(2, 3, bytemuck::cast_slice(&[1_u16, 2, 3, 4, 5, 6]).to_vec());
    assert_decode(matrix);
}

#[test]
fn generic_wire_message_rejects_wrong_type_hash_and_short_headers() {
    let point = Point::new(1.0, 2.0, 3.0);
    let mut msg = to_wire_message(&point);
    msg.type_hash = datapod::bind::type_hash::<Grid>();
    assert!(matches!(
        from_wire_message::<Point>(&msg),
        Err(WireError::WrongTypeHash { .. })
    ));

    let mut msg = to_wire_message(&point);
    msg.bytes.truncate(1);
    assert!(matches!(
        from_wire_message::<Point>(&msg),
        Err(WireError::ShortHeader { .. })
    ));
}

#[test]
fn generic_wire_message_decode_and_v1_encode_reject_invalid_owned_payloads() {
    let invalid_matrix = Matrix {
        rows: 2,
        cols: 3,
        element_size: 2,
        _pad: 0,
        data: vec![1, 0, 2, 0],
    };
    let msg = to_wire_message(&invalid_matrix);

    assert!(matches!(
        from_wire_message::<Matrix>(&msg),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        to_wire_message_v1(&invalid_matrix),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}
