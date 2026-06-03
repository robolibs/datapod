use datapod::{
    BitVec, Bytes, DataPod, Deque, DpStr, Encoding, ForwardList, Geometry, GeometryKind, Grid,
    Heap, HeapOrder, IndexedHeap, Joint, JointType, Layer, List, Map, MapEntry, Matrix,
    PagedVecvec, Point, Polygon, PolygonView, Queue, Set, SetEntry, Stack, Tensor, Vector, Vecvec,
    WireError, access_wire, access_wire_frame, access_wire_unchecked, from_wire_frame,
    from_wire_message, split_wire_frame, split_wire_parts, to_wire_message,
    validate_registered_wire, validate_wire, validate_wire_frame, wire_frame_to_message,
    wire_segmented_frame_to_message, with_segmented_wire_frame, with_wire_frame,
    with_wire_frame_slices, with_wire_segmented_frame_slices,
};

#[test]
fn split_wire_parts_borrows_registered_header_and_payload() {
    let matrix = Matrix::from_bytes::<u16>(2, 2, bytemuck::cast_slice(&[1_u16, 2, 3, 4]).to_vec());
    let msg = to_wire_message(&matrix);

    let parts = split_wire_parts(msg.type_hash, &msg.bytes).expect("registered matrix splits");
    let header_len = core::mem::size_of::<datapod::MatrixHeader>();

    assert_eq!(parts.type_hash, msg.type_hash);
    assert_eq!(parts.header.len(), header_len);
    assert_eq!(parts.payload, &msg.bytes[header_len..]);
    assert_eq!(parts.joined_len(), msg.bytes.len());
}

#[test]
fn split_wire_frame_borrows_joined_header_and_payload() {
    let matrix = Matrix::from_bytes::<u16>(2, 2, bytemuck::cast_slice(&[1_u16, 2, 3, 4]).to_vec());
    let msg = to_wire_message(&matrix);
    let header_len = core::mem::size_of::<datapod::MatrixHeader>();

    let frame = split_wire_frame(msg.type_hash, &msg.bytes).expect("registered matrix frame");

    assert_eq!(frame.type_hash, msg.type_hash);
    assert_eq!(frame.header, &msg.bytes[..header_len]);
    assert_eq!(frame.payload, &msg.bytes[header_len..]);
    assert_eq!(frame.joined_len(), msg.bytes.len());
}

#[test]
fn wire_frame_fast_path_borrows_original_payload_without_joining() {
    let matrix =
        Matrix::from_bytes::<u16>(2, 3, bytemuck::cast_slice(&[1_u16, 2, 3, 4, 5, 6]).to_vec());
    let original_payload_ptr = matrix.data.as_ptr();

    with_wire_frame(&matrix, |frame| {
        assert_eq!(frame.type_hash, datapod::bind::type_hash::<Matrix>());
        assert_eq!(
            frame.header.len(),
            core::mem::size_of::<datapod::MatrixHeader>()
        );
        assert_eq!(frame.payload.as_ptr(), original_payload_ptr);
        assert_eq!(frame.payload.len(), matrix.data.len());

        validate_wire_frame::<Matrix>(frame).expect("frame validates");
        let view = access_wire_frame::<Matrix>(frame).expect("frame view");
        assert_eq!(view.payload_bytes().as_ptr(), original_payload_ptr);
        assert_eq!(view.get_unaligned::<u16>(1, 2).unwrap(), 6);

        let owned: Matrix = from_wire_frame(frame).expect("owned decode from frame");
        assert_eq!(owned, matrix);

        let msg = wire_frame_to_message(frame);
        let from_msg: Matrix = from_wire_message(&msg).expect("collected frame still decodes");
        assert_eq!(from_msg, matrix);
    })
    .expect("contiguous matrix exposes zero-copy frame");
}

#[test]
fn wire_frame_slices_expose_transport_scatter_gather_without_joining() {
    let matrix = Matrix::from_bytes::<u16>(2, 2, bytemuck::cast_slice(&[1_u16, 2, 3, 4]).to_vec());
    let payload_ptr = matrix.data.as_ptr();

    with_wire_frame(&matrix, |frame| {
        with_wire_frame_slices(frame, |slices| {
            assert_eq!(slices[0], &frame.type_hash.to_le_bytes());
            assert_eq!(slices[1], frame.header);
            assert_eq!(slices[2].as_ptr(), payload_ptr);
            assert_eq!(
                slices.iter().map(|slice| slice.len()).sum::<usize>(),
                8 + frame.joined_len()
            );
        });
    })
    .expect("matrix exposes frame slices");
}

#[test]
fn wire_frame_rejects_multi_section_payload_instead_of_hiding_copy() {
    #[datapod::datapod]
    #[derive(Default)]
    struct SplitPayloadForFrame {
        #[dp(bytes, section = "left")]
        left: Vec<u8>,
        #[dp(bytes, section = "right")]
        right: Vec<u8>,
    }

    let value = SplitPayloadForFrame {
        left: b"left".to_vec(),
        right: b"right".to_vec(),
    };

    assert!(matches!(
        with_wire_frame(&value, |_| ()),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}

#[test]
fn segmented_wire_frame_borrows_each_multi_vec_payload_without_joining() {
    #[datapod::datapod]
    #[derive(Default)]
    struct SplitPayloadForSegments {
        #[dp(bytes, section = "left")]
        left: Vec<u8>,
        #[dp(bytes, section = "right")]
        right: Vec<u16>,
    }

    let value = SplitPayloadForSegments {
        left: b"left".to_vec(),
        right: vec![10_u16, 11],
    };
    let left_ptr = value.left.as_ptr();
    let right_ptr = value.right.as_ptr().cast::<u8>();

    with_segmented_wire_frame(&value, |frame| {
        assert_eq!(frame.payload_segment_count(), 2);
        assert_eq!(frame.payloads[0].as_ptr(), left_ptr);
        assert_eq!(frame.payloads[0], b"left");
        assert_eq!(frame.payloads[1].as_ptr(), right_ptr);
        let expected_right: &[u8] = bytemuck::cast_slice(&[10_u16, 11]);
        assert_eq!(frame.payloads[1], expected_right);

        with_wire_segmented_frame_slices(frame, |prefix, payloads| {
            assert_eq!(prefix[0], &frame.type_hash.to_le_bytes());
            assert_eq!(prefix[1], frame.header);
            assert_eq!(payloads[0].as_ptr(), left_ptr);
            assert_eq!(payloads[1].as_ptr(), right_ptr);
        });

        let msg = wire_segmented_frame_to_message(frame);
        validate_wire::<SplitPayloadForSegments>(&msg).expect("segmented frame collects validly");
        let decoded: SplitPayloadForSegments =
            from_wire_message(&msg).expect("segmented frame owned decode");
        assert_eq!(decoded.left, b"left");
        assert_eq!(decoded.right, vec![10_u16, 11]);
    })
    .expect("sectioned datapod exposes segmented zero-copy frame");
}

#[test]
fn fixed_datapod_access_returns_copied_fixed_view() {
    let point = Point::new(1.0, 2.0, 3.0);
    let msg = to_wire_message(&point);

    let view = access_wire::<Point>(&msg).expect("point view");
    assert_eq!(view.value, point);
}

#[test]
fn macro_generated_view_is_public_for_existing_heap_type() {
    let polygon = Polygon::new(vec![
        Point::new(0.0, 0.0, 0.0),
        Point::new(1.0, 0.0, 0.0),
        Point::new(1.0, 1.0, 0.0),
    ]);
    let msg = to_wire_message(&polygon);

    let view = access_wire::<Polygon>(&msg).expect("polygon view");
    let _: PolygonView<'_> = view;
    assert_eq!(
        view.payload_bytes().len(),
        3 * core::mem::size_of::<Point>()
    );
    assert_eq!(view.payload_bytes().as_ptr(), msg.bytes.as_ptr());
}

#[test]
fn bytes_view_borrows_payload_without_owned_decode() {
    let bytes = Bytes::from_slice(&[9, 8, 7, 6]);
    let msg = to_wire_message(&bytes);

    let view = access_wire::<Bytes>(&msg).expect("bytes view");
    assert_eq!(view.as_slice(), &[9, 8, 7, 6]);
    assert_eq!(view.as_slice().as_ptr(), msg.bytes.as_ptr());
}

#[test]
fn matrix_view_validates_and_borrows_payload() {
    let matrix =
        Matrix::from_bytes::<u16>(2, 3, bytemuck::cast_slice(&[1_u16, 2, 3, 4, 5, 6]).to_vec());
    let msg = to_wire_message(&matrix);
    let header_len = core::mem::size_of::<datapod::MatrixHeader>();

    validate_wire::<Matrix>(&msg).expect("valid matrix wire");
    let view = access_wire::<Matrix>(&msg).expect("matrix view");
    assert_eq!((view.rows(), view.cols(), view.element_size()), (2, 3, 2));
    assert_eq!(
        view.payload_bytes().as_ptr(),
        msg.bytes[header_len..].as_ptr()
    );
    assert_eq!(view.get_unaligned::<u16>(1, 2).unwrap(), 6);

    let decoded: Matrix = from_wire_message(&msg).expect("owned decode still works");
    assert_eq!(decoded, matrix);
}

#[test]
fn unchecked_matrix_access_is_available_for_trusted_bytes() {
    let matrix = Matrix::from_bytes::<u16>(1, 2, bytemuck::cast_slice(&[10_u16, 11]).to_vec());
    let msg = to_wire_message(&matrix);

    let view = unsafe { access_wire_unchecked::<Matrix>(&msg) };
    assert_eq!(view.get_unaligned::<u16>(0, 1).unwrap(), 11);
}

#[test]
fn validation_rejects_bad_matrix_and_tensor_lengths() {
    let matrix = Matrix::from_bytes::<u16>(2, 2, bytemuck::cast_slice(&[1_u16, 2, 3, 4]).to_vec());
    let mut msg = to_wire_message(&matrix);
    msg.bytes.pop();

    assert!(matches!(
        validate_wire::<Matrix>(&msg),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        access_wire::<Matrix>(&msg),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let tensor = Tensor::from_bytes::<u16>(
        2,
        2,
        2,
        bytemuck::cast_slice(&[1_u16, 2, 3, 4, 5, 6, 7, 8]).to_vec(),
    );
    let mut msg = to_wire_message(&tensor);
    msg.bytes.pop();
    assert!(matches!(
        validate_wire::<Tensor>(&msg),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}

#[test]
fn validation_rejects_invalid_dpstr_utf8() {
    let msg = datapod::WireMessage {
        type_hash: datapod::bind::type_hash::<DpStr>(),
        bytes: vec![0xff, 0xfe],
    };

    assert!(matches!(
        validate_wire::<DpStr>(&msg),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}

#[test]
fn grid_view_validates_payload_size_from_encoding() {
    let grid = Grid::new(
        2,
        2,
        Encoding::Rgba8,
        0.5,
        false,
        datapod::Pose::default(),
        bytes_0_to_15(),
    );
    let msg = to_wire_message(&grid);
    let header_len = core::mem::size_of::<datapod::GridHeader>();

    let view = access_wire::<Grid>(&msg).expect("grid view");
    assert_eq!(
        (view.rows(), view.cols(), view.encoding()),
        (2, 2, Encoding::Rgba8)
    );
    assert_eq!(
        view.payload_bytes().as_ptr(),
        msg.bytes[header_len..].as_ptr()
    );
    assert_eq!(view.payload_bytes(), bytes_0_to_15());

    let mut bad = msg;
    bad.bytes.pop();
    assert!(matches!(
        validate_wire::<Grid>(&bad),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}

#[test]
fn layer_view_validates_3d_payload_size_from_encoding() {
    let layer = Layer {
        rows: 2,
        cols: 2,
        layers: 2,
        encoding: Encoding::Mono16,
        centered: 0,
        _pad: 0,
        resolution: 0.5,
        layer_height: 0.25,
        pose: datapod::Pose::default(),
        data: vec![0_u8; 16],
    };
    let msg = to_wire_message(&layer);
    let header_len = core::mem::size_of::<datapod::LayerHeader>();

    let view = access_wire::<Layer>(&msg).expect("layer view");
    assert_eq!(
        (view.rows(), view.cols(), view.layers(), view.encoding()),
        (2, 2, 2, Encoding::Mono16)
    );
    assert_eq!(
        view.payload_bytes().as_ptr(),
        msg.bytes[header_len..].as_ptr()
    );

    let mut bad = msg;
    bad.bytes.pop();
    assert!(matches!(
        validate_wire::<Layer>(&bad),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}

#[test]
fn map_and_set_views_validate_offsets_and_sorting() {
    let mut map = Map::new();
    map.insert_str("alpha", "one");
    map.insert_str("bravo", "two");
    let msg = to_wire_message(&map);
    let view = access_wire::<Map>(&msg).expect("map view");
    assert_eq!(view.size(), 2);
    assert_eq!(view.key_at(0), b"alpha");
    assert_eq!(view.value_at(1), b"two");
    assert_eq!(view.payload_bytes().as_ptr(), msg.bytes.as_ptr());

    let mut set = Set::new();
    assert!(set.insert(b"alpha"));
    assert!(set.insert(b"bravo"));
    let msg = to_wire_message(&set);
    let view = access_wire::<Set>(&msg).expect("set view");
    assert_eq!(view.size(), 2);
    assert_eq!(view.key_at(1), b"bravo");

    assert!(matches!(
        validate_wire::<Map>(&datapod::WireMessage {
            type_hash: datapod::bind::type_hash::<Map>(),
            bytes: malformed_map_out_of_bounds(),
        }),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        validate_wire::<Set>(&datapod::WireMessage {
            type_hash: datapod::bind::type_hash::<Set>(),
            bytes: malformed_set_unsorted(),
        }),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}

#[test]
fn vector_bitvec_and_vecvec_views_validate_semantics() {
    let vector = Vector::from_bytes::<u16>(bytemuck::cast_slice(&[10_u16, 20, 30]).to_vec());
    let msg = to_wire_message(&vector);
    let view = access_wire::<Vector>(&msg).expect("vector view");
    assert_eq!(view.size(), 3);
    assert_eq!(view.get_unaligned::<u16>(1).unwrap(), 20);

    let mut bad = msg;
    bad.bytes.push(0xff);
    assert!(matches!(
        validate_wire::<Vector>(&bad),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let mut bits = BitVec::with_len(3);
    bits.set_bit(0, true);
    bits.set_bit(2, true);
    let msg = to_wire_message(&bits);
    let view = access_wire::<BitVec>(&msg).expect("bitvec view");
    assert_eq!(view.bits(), 3);
    assert!(view.test(0).unwrap());
    assert!(!view.test(1).unwrap());
    assert!(view.test(2).unwrap());

    let mut slack = msg;
    *slack.bytes.last_mut().unwrap() |= 0b1111_1000;
    assert!(matches!(
        validate_wire::<BitVec>(&slack),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let mut ragged = Vecvec::new::<u16>();
    ragged.push_bucket::<u16>(&[1, 2]);
    ragged.push_bucket::<u16>(&[3]);
    let msg = to_wire_message(&ragged);
    let view = access_wire::<Vecvec>(&msg).expect("vecvec view");
    assert_eq!(view.size(), 2);
    assert_eq!(view.bucket_unaligned::<u16>(0).unwrap(), vec![1, 2]);
    assert_eq!(view.bucket_unaligned::<u16>(1).unwrap(), vec![3]);

    assert!(matches!(
        validate_wire::<Vecvec>(&datapod::WireMessage {
            type_hash: datapod::bind::type_hash::<Vecvec>(),
            bytes: malformed_vecvec_bucket_width(),
        }),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}

#[test]
fn remaining_sequence_views_validate_semantics() {
    let mut stack = Stack::new::<u16>();
    stack.push(1_u16);
    stack.push(2_u16);
    let msg = to_wire_message(&stack);
    assert_eq!(access_wire::<Stack>(&msg).unwrap().payload.len(), 4);

    let mut bad = msg.clone();
    bad.bytes.push(0xff);
    assert!(matches!(
        validate_wire::<Stack>(&bad),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let mut queue = Queue::new::<u16>();
    queue.push_back(1_u16);
    queue.push_back(2_u16);
    let msg = to_wire_message(&queue);
    access_wire::<Queue>(&msg).expect("queue view");

    let mut bad_queue = msg.clone();
    let bad_header = datapod::QueueHeader {
        element_size: 2,
        front: 99,
    };
    bad_queue.bytes[..core::mem::size_of::<datapod::QueueHeader>()]
        .copy_from_slice(bytemuck::bytes_of(&bad_header));
    assert!(matches!(
        validate_wire::<Queue>(&bad_queue),
        Err(WireError::InvalidHeader { .. })
    ));

    let mut deque = Deque::new::<u16>();
    deque.push_front(1_u16);
    deque.push_back(2_u16);
    let msg = to_wire_message(&deque);
    access_wire::<Deque>(&msg).expect("deque view");

    let mut bad_deque = msg.clone();
    let bad_deque_header = datapod::DequeHeader {
        element_size: 2,
        split_byte: 99,
    };
    bad_deque.bytes[..core::mem::size_of::<datapod::DequeHeader>()]
        .copy_from_slice(bytemuck::bytes_of(&bad_deque_header));
    assert!(matches!(
        validate_wire::<Deque>(&bad_deque),
        Err(WireError::InvalidHeader { .. })
    ));

    let mut heap = Heap::new::<u16>();
    heap.push(4_u16);
    heap.push(9_u16);
    let msg = to_wire_message(&heap);
    validate_registered_wire(msg.type_hash, &msg.bytes).expect("registered heap validates");

    let bad_heap = Heap {
        element_size: 2,
        order: HeapOrder(9),
        _pad: [0; 3],
        data: vec![0, 1],
    };
    assert!(matches!(
        validate_wire::<Heap>(&to_wire_message(&bad_heap)),
        Err(WireError::InvalidHeader { .. })
    ));

    let mut indexed = IndexedHeap::new::<u16>();
    indexed.push(7, 1_u16);
    indexed.push(8, 2_u16);
    let msg = to_wire_message(&indexed);
    access_wire::<IndexedHeap>(&msg).expect("indexed heap view");

    let duplicate_keys = IndexedHeap {
        priority_size: 2,
        order: HeapOrder::Max,
        _pad: [0; 3],
        data: duplicate_indexed_heap_payload(),
    };
    assert!(matches!(
        validate_wire::<IndexedHeap>(&to_wire_message(&duplicate_keys)),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let mut list = List::new::<u16>();
    list.push_back(1_u16);
    list.push_back(2_u16);
    let msg = to_wire_message(&list);
    access_wire::<List>(&msg).expect("list view");

    let bad_list = List {
        head: 99,
        tail: u32::MAX,
        free_head: u32::MAX,
        size_: 1,
        element_size: 2,
        _pad: 0,
        data: Vec::new(),
    };
    assert!(matches!(
        validate_wire::<List>(&to_wire_message(&bad_list)),
        Err(WireError::InvalidHeader { .. })
    ));

    let mut forward = ForwardList::new::<u16>();
    forward.push_front(1_u16);
    forward.push_front(2_u16);
    let msg = to_wire_message(&forward);
    access_wire::<ForwardList>(&msg).expect("forward list view");

    let mut paged = PagedVecvec::new::<u16>();
    paged.push_bucket::<u16>(&[1, 2]);
    paged.push_bucket::<u16>(&[3]);
    let msg = to_wire_message(&paged);
    access_wire::<PagedVecvec>(&msg).expect("paged vecvec view");
}

#[test]
fn fixed_tag_datapods_validate_known_discriminants() {
    let bad_encoding = datapod::WireMessage {
        type_hash: datapod::bind::type_hash::<Encoding>(),
        bytes: bytemuck::bytes_of(&Encoding(999)).to_vec(),
    };
    assert!(matches!(
        validate_registered_wire(bad_encoding.type_hash, &bad_encoding.bytes),
        Err(WireError::InvalidHeader { .. })
    ));

    let bad_kind = datapod::WireMessage {
        type_hash: datapod::bind::type_hash::<GeometryKind>(),
        bytes: bytemuck::bytes_of(&GeometryKind(99)).to_vec(),
    };
    assert!(matches!(
        validate_registered_wire(bad_kind.type_hash, &bad_kind.bytes),
        Err(WireError::InvalidHeader { .. })
    ));

    let bad_geometry = Geometry {
        kind: GeometryKind(99),
        ..Geometry::default()
    };
    let bad_geometry = to_wire_message(&bad_geometry);
    assert!(matches!(
        validate_registered_wire(bad_geometry.type_hash, &bad_geometry.bytes),
        Err(WireError::InvalidHeader { .. })
    ));

    let bad_joint = Joint {
        joint_type: JointType(99),
        ..Joint::default()
    };
    let bad_joint = to_wire_message(&bad_joint);
    assert!(matches!(
        validate_registered_wire(bad_joint.type_hash, &bad_joint.bytes),
        Err(WireError::InvalidHeader { .. })
    ));
}

fn bytes_0_to_15() -> Vec<u8> {
    (0_u8..16).collect()
}

fn malformed_map_out_of_bounds() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&1_u32.to_le_bytes());
    bytes.extend_from_slice(bytemuck::bytes_of(&MapEntry {
        key_off: 0,
        key_len: 99,
        value_off: 0,
        value_len: 1,
    }));
    bytes.extend_from_slice(b"x");
    bytes
}

fn malformed_set_unsorted() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&2_u32.to_le_bytes());
    bytes.extend_from_slice(bytemuck::bytes_of(&SetEntry {
        key_off: 0,
        key_len: 1,
    }));
    bytes.extend_from_slice(bytemuck::bytes_of(&SetEntry {
        key_off: 1,
        key_len: 1,
    }));
    bytes.extend_from_slice(b"ba");
    bytes
}

fn malformed_vecvec_bucket_width() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(bytemuck::bytes_of(&datapod::VecvecHeader {
        element_size: 2,
        _pad: 0,
    }));
    bytes.extend_from_slice(&1_u32.to_le_bytes());
    bytes.extend_from_slice(&0_u32.to_le_bytes());
    bytes.extend_from_slice(&3_u32.to_le_bytes());
    bytes.extend_from_slice(&[1, 2, 3]);
    bytes
}

fn duplicate_indexed_heap_payload() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&7_u64.to_le_bytes());
    bytes.extend_from_slice(&1_u16.to_ne_bytes());
    bytes.extend_from_slice(&7_u64.to_le_bytes());
    bytes.extend_from_slice(&2_u16.to_ne_bytes());
    bytes
}
