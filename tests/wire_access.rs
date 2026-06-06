use datapod::{
    BitVec, Bytes, DataPod, DataPodValidate, Deque, DpStr, DpString, Encoding, ForwardList,
    Geometry, GeometryKind, Grid, GridView, Heap, HeapOrder, IndexedHeap, Ip, Joint, JointType,
    Layer, LayerView, Link, List, MacAddr, Map, MapEntry, Matrix, MatrixView, PagedVecvec, Point,
    Polygon, PolygonView, Queue, Set, SetEntry, Stack, Tensor, TensorView, TransmissionJoint,
    Vector, Vecvec, WireError, access_wire, access_wire_frame, access_wire_unchecked,
    from_wire_frame, from_wire_message, split_wire_frame, split_wire_parts, to_wire_message,
    try_wire_frame_to_message, try_wire_segmented_frame_to_message, validate_registered_wire,
    validate_wire, validate_wire_frame, wire_frame_to_message, wire_segmented_frame_to_message,
    with_segmented_wire_frame, with_wire_frame, with_wire_frame_slices,
    with_wire_segmented_frame_slices,
};

#[repr(C)]
#[derive(Debug, Clone, Copy, Default, bytemuck::Pod, bytemuck::Zeroable)]
struct BadSegmentedHeader {
    reported_len: u32,
}

unsafe impl datapod::ZeroCopySend for BadSegmentedHeader {}

#[derive(Debug)]
struct BadSegmentedPayloadReport;

impl DataPod for BadSegmentedPayloadReport {
    type Header = BadSegmentedHeader;
    type Payload = [u8];

    fn header(&self) -> Self::Header {
        BadSegmentedHeader { reported_len: 4 }
    }

    fn payload_bytes(&self) -> &[u8] {
        &[]
    }

    fn payload_len(&self) -> usize {
        4
    }

    fn with_payload_segments<R>(&self, f: impl FnOnce(&[&[u8]]) -> R) -> R
    where
        Self: Sized,
    {
        let payloads: [&[u8]; 2] = [b"a", b"b"];
        f(&payloads)
    }
}

impl DataPodValidate for BadSegmentedPayloadReport {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        if payload.len() == header.reported_len as usize {
            Ok(())
        } else {
            Err(WireError::InvalidPayloadSize {
                type_name: core::any::type_name::<Self>(),
                message: format!(
                    "payload has {} bytes, expected {}",
                    payload.len(),
                    header.reported_len
                ),
            })
        }
    }
}

#[test]
fn zero_sized_pod_elements_are_rejected_before_mutating_byte_counted_containers() {
    assert_eq!(Vector::try_new::<u32>().unwrap().element_size, 4);
    assert_eq!(Stack::try_new::<u32>().unwrap().element_size, 4);
    assert_eq!(Queue::try_new::<u32>().unwrap().element_size, 4);
    assert_eq!(Deque::try_new::<u32>().unwrap().element_size, 4);
    assert_eq!(Heap::try_new::<u32>().unwrap().element_size, 4);
    assert_eq!(Heap::try_new_min::<u32>().unwrap().element_size, 4);
    assert_eq!(IndexedHeap::try_new::<u32>().unwrap().priority_size, 4);
    assert_eq!(IndexedHeap::try_new_min::<u32>().unwrap().priority_size, 4);
    assert_eq!(List::try_new::<u32>().unwrap().element_size, 4);
    assert_eq!(ForwardList::try_new::<u32>().unwrap().element_size, 4);
    assert_eq!(Vecvec::try_new::<u32>().unwrap().element_size, 4);
    assert_eq!(PagedVecvec::try_new::<u32>().unwrap().element_size, 4);
    assert_eq!(Matrix::try_new::<u32>(1, 1).unwrap().element_size, 4);
    assert_eq!(
        Matrix::try_from_bytes::<u32>(1, 1, vec![0; 4])
            .unwrap()
            .element_size,
        4
    );
    assert_eq!(Tensor::try_new::<u32>(1, 1, 1).unwrap().element_size, 4);
    assert_eq!(
        Tensor::try_from_bytes::<u32>(1, 1, 1, vec![0; 4])
            .unwrap()
            .element_size,
        4
    );

    let mut vector = Vector::new::<()>();
    assert!(vector.try_push(()).is_err());
    assert_eq!(vector.size(), 0);

    let mut stack = Stack::new::<()>();
    assert!(stack.try_push(()).is_err());
    assert_eq!(stack.size(), 0);

    let mut queue = Queue::new::<()>();
    assert!(queue.try_push_back(()).is_err());
    assert_eq!(queue.size(), 0);

    let mut deque = Deque::new::<()>();
    assert!(deque.try_push_front(()).is_err());
    assert!(deque.try_push_back(()).is_err());
    assert_eq!(deque.size(), 0);

    let mut heap = Heap::new::<()>();
    assert!(heap.try_push(()).is_err());
    assert_eq!(heap.size(), 0);

    let mut indexed_heap = IndexedHeap::new::<()>();
    assert!(indexed_heap.try_push(7, ()).is_err());
    assert_eq!(indexed_heap.size(), 0);

    let mut list = List::new::<()>();
    assert!(list.try_push_back(()).is_err());
    assert!(list.try_push_front(()).is_err());
    assert_eq!(list.size(), 0);

    let mut forward_list = ForwardList::new::<()>();
    assert!(forward_list.try_push_front(()).is_err());
    assert_eq!(forward_list.size(), 0);

    let mut vecvec = Vecvec::new::<()>();
    assert!(vecvec.try_push_bucket(&[()]).is_err());
    assert_eq!(vecvec.size(), 0);

    let mut paged_vecvec = PagedVecvec::new::<()>();
    assert!(paged_vecvec.try_push_bucket(&[()]).is_err());
    assert_eq!(paged_vecvec.size(), 0);

    assert!(Matrix::try_new::<()>(1, 1).is_err());
    assert!(Tensor::try_new::<()>(1, 1, 1).is_err());
}

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
    assert_eq!(
        parts
            .try_joined_len()
            .expect("registered matrix parts have exact joined length"),
        msg.bytes.len()
    );
}

#[test]
fn split_wire_parts_rejects_payload_bytes_for_fixed_datapods() {
    let point = Point::new(1.0, 2.0, 3.0);
    let msg = to_wire_message(&point);
    let mut fixed_with_payload = msg.bytes.clone();
    fixed_with_payload.push(0xaa);

    assert!(matches!(
        split_wire_parts(msg.type_hash, &fixed_with_payload),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        split_wire_frame(msg.type_hash, &fixed_with_payload),
        Err(WireError::InvalidPayloadSize { .. })
    ));
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
    assert_eq!(
        frame
            .try_joined_len()
            .expect("registered matrix frame has exact joined length"),
        msg.bytes.len()
    );
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

        let msg = try_wire_frame_to_message(frame).expect("fallible frame collection succeeds");
        assert_eq!(msg, wire_frame_to_message(frame));
        let from_msg: Matrix = from_wire_message(&msg).expect("collected frame still decodes");
        assert_eq!(from_msg, matrix);
    })
    .expect("contiguous matrix exposes zero-copy frame");
}

#[test]
fn wire_frame_fast_path_validates_owned_value_before_exposing_archive() {
    let bad_matrix = Matrix {
        rows: 2,
        cols: 2,
        element_size: 1,
        _pad: 0,
        data: vec![1, 2, 3],
    };
    assert!(matches!(
        with_wire_frame(&bad_matrix, |_| ()),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let bad_ip = Ip {
        family: 4,
        _pad: 0,
        bytes: [1; 16],
    };
    assert!(matches!(
        with_wire_frame(&bad_ip, |_| ()),
        Err(WireError::InvalidHeader { .. })
    ));
}

#[test]
fn segmented_wire_frame_rejects_invalid_single_payload_value_before_exposing_archive() {
    let bad_matrix = Matrix {
        rows: 2,
        cols: 2,
        element_size: 1,
        _pad: 0,
        data: vec![1, 2, 3],
    };

    assert!(matches!(
        with_segmented_wire_frame(&bad_matrix, |_| ()),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}

#[test]
fn segmented_wire_frame_rejects_payload_segment_total_mismatch() {
    let bad = BadSegmentedPayloadReport;

    let err = with_segmented_wire_frame(&bad, |_| ()).expect_err("mismatched segments reject");
    assert!(matches!(err, WireError::InvalidPayloadSize { .. }));
    assert!(
        err.to_string()
            .contains("exposes 2 segmented payload bytes but reports 4 payload bytes")
    );
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
                frame
                    .try_joined_len()
                    .expect("scatter/gather frame length is exact"),
                frame.joined_len()
            );
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

        let msg = try_wire_segmented_frame_to_message(frame)
            .expect("fallible segmented frame collection succeeds");
        assert_eq!(msg, wire_segmented_frame_to_message(frame));
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
fn bytes_and_dpstr_search_edges_do_not_panic() {
    let empty = Bytes::new();
    assert_eq!(empty.find(1, 0), usize::MAX);
    assert_eq!(empty.rfind(1, usize::MAX), usize::MAX);

    let bytes = Bytes::from_slice(&[1, 2, 3]);
    assert_eq!(bytes.find(2, usize::MAX), usize::MAX);
    assert_eq!(bytes.find(2, 2), usize::MAX);
    assert_eq!(bytes.rfind(2, usize::MAX), 1);
    assert_eq!(bytes.rfind(1, 0), 0);
    assert_eq!(bytes.try_substr(3, usize::MAX).unwrap(), Vec::<u8>::new());
    assert_eq!(bytes.try_substr(usize::MAX, 1).unwrap(), Vec::<u8>::new());
    assert_eq!(bytes.try_substr(1, 0).unwrap(), Vec::<u8>::new());
    assert_eq!(bytes.substr(1, usize::MAX), vec![2, 3]);
    assert_eq!(Bytes::try_from_slice(&[4, 5]).unwrap().as_slice(), &[4, 5]);
    let mut appended = Bytes::new();
    appended.try_append(&[1, 2]).unwrap();
    appended.append(&[3]);
    assert_eq!(appended.as_slice(), &[1, 2, 3]);
    assert_eq!(appended.try_substr(1, usize::MAX).unwrap(), vec![2, 3]);

    let empty_str = DpStr::new();
    assert_eq!(empty_str.find("", 0), 0);
    assert_eq!(empty_str.find("x", 0), usize::MAX);

    let text = DpStr::from("abc");
    assert_eq!(DpStr::try_from_str("abc").unwrap().as_str(), Some("abc"));
    assert_eq!(text.find("", 0), 0);
    assert_eq!(text.find("", 3), 3);
    assert_eq!(text.find("", 4), usize::MAX);
    assert_eq!(text.find("", usize::MAX), usize::MAX);
    assert_eq!(text.find("b", 0), 1);
    assert_eq!(text.find("b", 2), usize::MAX);
    assert_eq!(text.find("b", usize::MAX), usize::MAX);
    let mut text = DpStr::new();
    text.try_append("ab").unwrap();
    text.try_push('ç').unwrap();
    text.append("d");
    text.push('é');
    assert_eq!(text.as_str(), Some("abçdé"));
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
    assert_eq!(matrix.try_size().unwrap(), 4);
    assert!(!matrix.try_is_empty().unwrap());
    assert_eq!(matrix.try_flat_index(1, 1).unwrap(), 3);
    assert_eq!(matrix.try_get::<u16>(1, 1).unwrap(), 4);
    assert!(matches!(
        matrix.try_get::<u16>(2, 0),
        Err(WireError::InvalidHeader { .. })
    ));
    let mut matrix_mut = matrix.clone();
    matrix_mut.try_set::<u16>(0, 1, 9).unwrap();
    assert_eq!(matrix_mut.try_get::<u16>(0, 1).unwrap(), 9);
    assert!(matches!(
        Matrix::try_from_bytes::<u16>(2, 2, vec![0xaa]),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let invalid_matrix_from_bytes = Matrix::from_bytes::<u16>(2, 2, vec![0xaa]);
    assert_eq!(invalid_matrix_from_bytes.size(), 0);
    assert!(!invalid_matrix_from_bytes.is_valid());
    let overflowing_matrix = Matrix::new::<u16>(u32::MAX, u32::MAX);
    assert_eq!(overflowing_matrix.size(), 0);
    assert!(!overflowing_matrix.is_valid());
    let mut malformed_matrix = Matrix {
        rows: 1,
        cols: 2,
        element_size: 2,
        _pad: 0,
        data: vec![0xaa],
    };
    let before = malformed_matrix.data.clone();
    assert!(matches!(
        malformed_matrix.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_matrix.try_is_empty(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(
        malformed_matrix.is_empty(),
        "infallible matrix is_empty should fail closed for malformed owned state"
    );
    assert!(matches!(
        malformed_matrix.try_flat_index(0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        malformed_matrix.flat_index(0, 0),
        usize::MAX,
        "infallible matrix flat_index should fail closed for malformed owned state"
    );
    assert!(matches!(
        malformed_matrix.try_get::<u16>(0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_matrix.try_set::<u16>(0, 0, 7),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(malformed_matrix.get::<u16>(0, 0), 0);
    malformed_matrix.set::<u16>(0, 0, 7);
    assert!(malformed_matrix.as_slice::<u16>().is_empty());
    assert!(malformed_matrix.as_mut_slice::<u16>().is_empty());
    assert!(malformed_matrix.row::<u16>(0).is_empty());
    assert_eq!(
        malformed_matrix.data, before,
        "failed matrix set must not mutate malformed owned data"
    );
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
    assert_eq!(tensor.try_size().unwrap(), 8);
    assert!(!tensor.try_is_empty().unwrap());
    assert_eq!(tensor.try_flat_index(1, 1, 1).unwrap(), 7);
    assert_eq!(tensor.try_layer::<u16>(1).unwrap(), &[5_u16, 6, 7, 8]);
    assert_eq!(tensor.try_get::<u16>(1, 1, 1).unwrap(), 8);
    assert!(matches!(
        tensor.try_get::<u16>(0, 0, 2),
        Err(WireError::InvalidHeader { .. })
    ));
    let mut tensor_mut = tensor.clone();
    tensor_mut.try_set::<u16>(0, 1, 1, 9).unwrap();
    assert_eq!(tensor_mut.try_get::<u16>(0, 1, 1).unwrap(), 9);
    assert!(matches!(
        Tensor::try_from_bytes::<u16>(2, 2, 2, vec![0xaa]),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let invalid_tensor_from_bytes = Tensor::from_bytes::<u16>(2, 2, 2, vec![0xaa]);
    assert_eq!(invalid_tensor_from_bytes.size(), 0);
    assert!(!invalid_tensor_from_bytes.is_valid());
    let overflowing_tensor = Tensor::new::<u16>(u32::MAX, u32::MAX, 2);
    assert_eq!(overflowing_tensor.size(), 0);
    assert!(!overflowing_tensor.is_valid());
    let mut malformed_tensor = Tensor {
        rows: 1,
        cols: 1,
        layers: 2,
        element_size: 2,
        data: vec![0xaa],
    };
    let before = malformed_tensor.data.clone();
    assert!(matches!(
        malformed_tensor.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_tensor.try_is_empty(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(
        malformed_tensor.is_empty(),
        "infallible tensor is_empty should fail closed for malformed owned state"
    );
    assert!(matches!(
        malformed_tensor.try_flat_index(0, 0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        malformed_tensor.flat_index(0, 0, 0),
        usize::MAX,
        "infallible tensor flat_index should fail closed for malformed owned state"
    );
    assert!(matches!(
        malformed_tensor.try_layer::<u16>(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_tensor.try_get::<u16>(0, 0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_tensor.try_set::<u16>(0, 0, 0, 7),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(malformed_tensor.get::<u16>(0, 0, 0), 0);
    malformed_tensor.set::<u16>(0, 0, 0, 7);
    assert!(malformed_tensor.as_slice::<u16>().is_empty());
    assert!(malformed_tensor.as_mut_slice::<u16>().is_empty());
    assert!(malformed_tensor.layer::<u16>(0).is_empty());
    assert_eq!(
        malformed_tensor.data, before,
        "failed tensor set must not mutate malformed owned data"
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
    let invalid_owned_dpstr = DpStr { data: vec![0xff] };
    assert!(matches!(
        invalid_owned_dpstr.try_as_str(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(invalid_owned_dpstr.try_to_string_lossy().unwrap(), "�");
    assert_eq!(invalid_owned_dpstr.to_string_lossy(), "�");
    let mixed_owned_dpstr = DpStr {
        data: vec![b'a', 0xff, b'b', 0xe2, 0x82],
    };
    assert_eq!(mixed_owned_dpstr.try_to_string_lossy().unwrap(), "a�b�");
    assert_eq!(mixed_owned_dpstr.to_string_lossy(), "a�b�");
    assert_eq!(
        invalid_owned_dpstr.as_str(),
        None,
        "owned DpStr::as_str should fail closed for malformed UTF-8"
    );

    let msg = datapod::WireMessage {
        type_hash: datapod::bind::type_hash::<DpString>(),
        bytes: vec![0xff, 0xfe],
    };
    assert!(matches!(
        validate_registered_wire(msg.type_hash, &msg.bytes),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        access_wire::<DpString>(&msg),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let invalid_owned_dpstring = DpString::new(vec![0xff]);
    assert!(matches!(
        invalid_owned_dpstring.try_as_str(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        invalid_owned_dpstring.as_str(),
        None,
        "owned DpString::as_str should fail closed for malformed UTF-8"
    );

    let invalid_dpstr_view = datapod::DpStrView { data: &[0xff] };
    assert!(matches!(
        invalid_dpstr_view.try_as_str(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(invalid_dpstr_view.as_str(), "");

    let valid_dpstr_view = datapod::DpStrView {
        data: "hello".as_bytes(),
    };
    assert_eq!(valid_dpstr_view.try_as_str().unwrap(), "hello");
    assert_eq!(valid_dpstr_view.as_str(), "hello");

    let invalid_dpstring_view = datapod::DpStringView {
        header: datapod::DpStringHeader::default(),
        payload: &[0xff],
    };
    assert!(matches!(
        invalid_dpstring_view.try_as_str(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(invalid_dpstring_view.as_str(), "");

    let valid_dpstring_view = datapod::DpStringView {
        header: datapod::DpStringHeader::default(),
        payload: "world".as_bytes(),
    };
    assert_eq!(valid_dpstring_view.try_as_str().unwrap(), "world");
    assert_eq!(valid_dpstring_view.as_str(), "world");
    assert_eq!(
        DpString::try_from_str("fallible-owned").unwrap().as_str(),
        Some("fallible-owned")
    );
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
    assert_eq!(grid.try_size().unwrap(), 4);
    assert!(grid.try_is_valid(Encoding::Rgba8.byte_width()).unwrap());
    assert!(!grid.try_is_valid(Encoding::Mono8.byte_width()).unwrap());
    assert_eq!(grid.try_flat_index(1, 1).unwrap(), 3);
    assert_eq!(view.try_size().unwrap(), 4);
    assert_eq!(view.try_flat_index(1, 1).unwrap(), 3);
    assert!(matches!(
        Grid::try_new(
            2,
            2,
            Encoding::Rgba8,
            0.5,
            false,
            datapod::Pose::default(),
            vec![0_u8; 15],
        ),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        Grid::new(
            2,
            2,
            Encoding::Rgba8,
            0.5,
            false,
            datapod::Pose::default(),
            vec![0_u8; 15],
        )
        .size(),
        0,
        "infallible Grid::new should fail closed for invalid owned data"
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

    let malformed_grid = Grid {
        rows: 2,
        cols: 2,
        encoding: Encoding::Rgba8,
        centered: 0,
        resolution: 1.0,
        pose: datapod::Pose::default(),
        data: vec![0_u8; 15],
    };
    assert!(matches!(
        malformed_grid.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_grid.try_is_valid(Encoding::Rgba8.byte_width()),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_grid.try_flat_index(0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        malformed_grid.flat_index(0, 0),
        usize::MAX,
        "infallible grid flat_index should fail closed for malformed owned state"
    );
    assert!(matches!(
        malformed_grid.try_world_to_grid(Point::new(0.0, 0.0, 0.0)),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        grid.try_world_to_grid(Point::new(f64::NAN, 0.0, 0.0)),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        grid.try_world_to_grid(Point::new(-1_000.0, -1_000.0, 0.0))
            .unwrap(),
        (0, 0),
        "negative world points clamp to the first grid cell"
    );
    assert_eq!(
        grid.try_world_to_grid(Point::new(1_000.0, 1_000.0, 0.0))
            .unwrap(),
        (1, 1),
        "large world points clamp to the final grid cell"
    );

    let invalid_encoding = Grid {
        rows: 1,
        cols: 1,
        encoding: Encoding(999),
        centered: 0,
        resolution: 1.0,
        pose: datapod::Pose::default(),
        data: Vec::new(),
    };
    let invalid_encoding = to_wire_message(&invalid_encoding);
    assert!(matches!(
        validate_registered_wire(invalid_encoding.type_hash, &invalid_encoding.bytes),
        Err(WireError::InvalidHeader { .. })
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
    assert_eq!(layer.try_size().unwrap(), 8);
    assert_eq!(layer.try_layer_count().unwrap(), 2);
    assert_eq!(layer.layer_count(), 2);
    assert!(layer.try_is_valid(Encoding::Mono16.byte_width()).unwrap());
    assert!(!layer.try_is_valid(Encoding::Mono8.byte_width()).unwrap());
    assert_eq!(layer.try_flat_index(1, 1, 1).unwrap(), 7);
    assert_eq!(view.try_size().unwrap(), 8);
    assert_eq!(view.try_flat_index(1, 1, 1).unwrap(), 7);
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

    let malformed_layer = Layer {
        rows: 2,
        cols: 2,
        layers: 2,
        encoding: Encoding::Mono16,
        centered: 0,
        _pad: 0,
        resolution: 0.5,
        layer_height: 0.25,
        pose: datapod::Pose::default(),
        data: vec![0_u8; 15],
    };
    assert!(matches!(
        malformed_layer.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_layer.try_layer_count(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        malformed_layer.layer_count(),
        0,
        "infallible layer_count should fail closed for malformed owned state"
    );
    assert!(matches!(
        malformed_layer.try_is_valid(Encoding::Mono16.byte_width()),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_layer.try_flat_index(0, 0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        malformed_layer.flat_index(0, 0, 0),
        usize::MAX,
        "infallible layer flat_index should fail closed for malformed owned state"
    );
    assert!(matches!(
        malformed_layer.try_world_to_voxel(Point::new(0.0, 0.0, 0.0)),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        layer.try_world_to_voxel(Point::new(0.0, f64::NAN, 0.0)),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        layer
            .try_world_to_voxel(Point::new(-1_000.0, -1_000.0, -1_000.0))
            .unwrap(),
        (0, 0, 0),
        "negative world points clamp to the first voxel"
    );
    assert_eq!(
        layer
            .try_world_to_voxel(Point::new(1_000.0, 1_000.0, 1_000.0))
            .unwrap(),
        (1, 1, 1),
        "large world points clamp to the final voxel"
    );

    let invalid_encoding = Layer {
        rows: 1,
        cols: 1,
        layers: 1,
        encoding: Encoding(999),
        centered: 0,
        _pad: 0,
        resolution: 1.0,
        layer_height: 1.0,
        pose: datapod::Pose::default(),
        data: Vec::new(),
    };
    let invalid_encoding = to_wire_message(&invalid_encoding);
    assert!(matches!(
        validate_registered_wire(invalid_encoding.type_hash, &invalid_encoding.bytes),
        Err(WireError::InvalidHeader { .. })
    ));
}

#[test]
fn malformed_shaped_views_return_errors_instead_of_panicking() {
    let short_matrix = MatrixView {
        header: datapod::MatrixHeader {
            rows: 1,
            cols: 1,
            element_size: 2,
            _pad: 0,
        },
        data: &[0_u8],
    };
    assert_eq!(short_matrix.size(), 0);
    assert_eq!(short_matrix.flat_index(0, 0), usize::MAX);
    assert!(matches!(
        short_matrix.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_matrix.try_flat_index(0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_matrix.get_unaligned::<u16>(0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_matrix.as_aligned_slice::<u16>(),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let short_tensor = TensorView {
        header: datapod::TensorHeader {
            rows: 1,
            cols: 1,
            layers: 1,
            element_size: 2,
        },
        data: &[0_u8],
    };
    assert_eq!(short_tensor.flat_index(0, 0, 0), usize::MAX);
    assert!(matches!(
        short_tensor.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_tensor.try_flat_index(0, 0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_tensor.get_unaligned::<u16>(0, 0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_tensor.as_aligned_slice::<u16>(),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let short_grid = GridView {
        header: datapod::GridHeader {
            rows: 1,
            cols: 1,
            encoding: Encoding::Mono16,
            centered: 0,
            resolution: 1.0,
            pose: datapod::Pose::default(),
        },
        data: &[0_u8],
    };
    assert_eq!(short_grid.size(), 0);
    assert_eq!(short_grid.flat_index(0, 0), usize::MAX);
    assert!(matches!(
        short_grid.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_grid.try_flat_index(0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let short_layer = LayerView {
        header: datapod::LayerHeader {
            rows: 1,
            cols: 1,
            layers: 1,
            encoding: Encoding::Mono16,
            centered: 0,
            _pad: 0,
            resolution: 1.0,
            layer_height: 1.0,
            pose: datapod::Pose::default(),
        },
        data: &[0_u8],
    };
    assert_eq!(short_layer.size(), 0);
    assert_eq!(short_layer.flat_index(0, 0, 0), usize::MAX);
    assert!(matches!(
        short_layer.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_layer.try_flat_index(0, 0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}

#[test]
fn overflowing_shaped_views_return_errors_instead_of_panicking() {
    let huge_grid = GridView {
        header: datapod::GridHeader {
            rows: u32::MAX,
            cols: u32::MAX,
            encoding: Encoding::Mono8,
            centered: 0,
            resolution: 1.0,
            pose: datapod::Pose::default(),
        },
        data: &[],
    };
    assert_eq!(huge_grid.size(), 0);
    assert_eq!(huge_grid.flat_index(0, 0), usize::MAX);
    assert!(matches!(
        huge_grid.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        huge_grid.try_flat_index(0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let huge_matrix = MatrixView {
        header: datapod::MatrixHeader {
            rows: u32::MAX,
            cols: u32::MAX,
            element_size: 2,
            _pad: 0,
        },
        data: &[],
    };
    assert_eq!(huge_matrix.size(), 0);
    assert_eq!(huge_matrix.flat_index(0, 0), usize::MAX);
    assert!(matches!(
        huge_matrix.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        huge_matrix.try_flat_index(0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let huge_tensor = TensorView {
        header: datapod::TensorHeader {
            rows: u32::MAX,
            cols: u32::MAX,
            layers: u32::MAX,
            element_size: 1,
        },
        data: &[],
    };
    assert_eq!(huge_tensor.flat_index(0, 0, 0), usize::MAX);
    assert!(matches!(
        huge_tensor.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        huge_tensor.try_flat_index(0, 0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let huge_layer = LayerView {
        header: datapod::LayerHeader {
            rows: u32::MAX,
            cols: u32::MAX,
            layers: u32::MAX,
            encoding: Encoding::Mono8,
            centered: 0,
            _pad: 0,
            resolution: 1.0,
            layer_height: 1.0,
            pose: datapod::Pose::default(),
        },
        data: &[],
    };
    assert_eq!(huge_layer.size(), 0);
    assert_eq!(huge_layer.flat_index(0, 0, 0), usize::MAX);
    assert!(matches!(
        huge_layer.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        huge_layer.try_flat_index(0, 0, 0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}

#[test]
fn map_and_set_views_validate_offsets_and_sorting() {
    let mut map = Map::new();
    map.insert_str("alpha", "one");
    map.insert_str("bravo", "two");
    assert_eq!(map.try_size().unwrap(), 2);
    assert!(!map.try_empty().unwrap());
    assert!(map.try_contains_key(b"alpha").unwrap());
    assert_eq!(map.try_key_at(0).unwrap(), b"alpha");
    assert_eq!(map.try_value_at(1).unwrap(), b"two");
    assert_eq!(map.try_get(b"alpha").unwrap(), Some(&b"one"[..]));
    assert!(matches!(
        map.try_key_at(2),
        Err(WireError::InvalidHeader { .. })
    ));
    assert_eq!(map.try_insert_str("bravo", "dos").unwrap().unwrap(), b"two");
    assert_eq!(map.get_str("bravo"), Some("dos"));
    assert_eq!(map.try_get_str("bravo").unwrap(), Some("dos"));
    let pod_key = 250_u8;
    map.try_insert(bytemuck::bytes_of(&pod_key), &[0xaa])
        .unwrap();
    assert!(matches!(
        map.try_get_pod::<u8, u16>(&pod_key),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let msg = to_wire_message(&map);
    let view = access_wire::<Map>(&msg).expect("map view");
    assert_eq!(view.size(), 3);
    assert_eq!(view.key_at(0).expect("map key view"), b"alpha");
    assert_eq!(view.value_at(1).expect("map value view"), b"dos");
    assert!(matches!(
        view.key_at(3),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(matches!(
        view.value_at(3),
        Err(WireError::InvalidHeader { .. })
    ));
    assert_eq!(view.payload_bytes().as_ptr(), msg.bytes.as_ptr());

    let mut set = Set::new();
    assert!(set.insert(b"alpha"));
    assert!(set.insert(b"bravo"));
    assert_eq!(set.try_size().unwrap(), 2);
    assert!(!set.try_empty().unwrap());
    assert!(set.try_contains(b"alpha").unwrap());
    assert!(set.try_contains_str("bravo").unwrap());
    assert!(!set.try_contains_pod(&7_u16).unwrap());
    assert_eq!(set.try_key_at(0).unwrap(), b"alpha");
    assert!(matches!(
        set.try_key_at(2),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(!set.try_insert(b"bravo").unwrap());
    let msg = to_wire_message(&set);
    let view = access_wire::<Set>(&msg).expect("set view");
    assert_eq!(view.size(), 2);
    assert_eq!(view.key_at(1).expect("set key view"), b"bravo");
    assert!(matches!(
        view.key_at(2),
        Err(WireError::InvalidHeader { .. })
    ));

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
    assert!(matches!(
        validate_wire::<Map>(&datapod::WireMessage {
            type_hash: datapod::bind::type_hash::<Map>(),
            bytes: malformed_map_with_trailing_blob_bytes(),
        }),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        validate_wire::<Set>(&datapod::WireMessage {
            type_hash: datapod::bind::type_hash::<Set>(),
            bytes: malformed_set_with_noncanonical_offset(),
        }),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        validate_wire::<Map>(&datapod::WireMessage {
            type_hash: datapod::bind::type_hash::<Map>(),
            bytes: malformed_map_with_overflowing_entry_range(),
        }),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        validate_wire::<Set>(&datapod::WireMessage {
            type_hash: datapod::bind::type_hash::<Set>(),
            bytes: malformed_set_with_overflowing_entry_range(),
        }),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let mut malformed_map = Map {
        data: vec![0, 0, 0],
    };
    let before = malformed_map.data.clone();
    assert_eq!(malformed_map.size(), 0);
    assert!(malformed_map.empty());
    assert!(!malformed_map.contains_key(b"k"));
    assert_eq!(malformed_map.get(b"k"), None);
    assert_eq!(malformed_map.get_pod::<u8, u16>(&1), None);
    assert_eq!(malformed_map.key_at(0), b"");
    assert_eq!(malformed_map.value_at(0), b"");
    assert_eq!(malformed_map.iter().count(), 0);
    assert!(matches!(
        malformed_map.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_map.try_empty(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_map.try_contains_key(b"k"),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_map.try_get(b"k"),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_map.try_get_pod::<u8, u16>(&1),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_map.try_insert(b"k", b"v"),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_map.try_remove(b"k"),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(malformed_map.insert(b"k", b"v"), None);
    assert_eq!(malformed_map.insert_str("k", "v"), None);
    assert_eq!(malformed_map.insert_pod(&1_u8, &2_u16), None);
    assert_eq!(malformed_map.remove(b"k"), None);
    assert_eq!(
        malformed_map.data, before,
        "failed map mutators must not mutate malformed owned data"
    );

    let mut count_only_map = Map {
        data: vec![1, 0, 0, 0],
    };
    let before = count_only_map.data.clone();
    assert!(matches!(
        count_only_map.try_key_at(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        count_only_map.try_value_at(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        count_only_map.try_insert(b"k", b"v"),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        count_only_map.try_remove(b"k"),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        count_only_map.data, before,
        "count-only map payload must reject access/mutation without mutation"
    );

    let mut malformed_set = Set {
        data: vec![0, 0, 0],
    };
    let before = malformed_set.data.clone();
    assert_eq!(malformed_set.size(), 0);
    assert!(malformed_set.empty());
    assert!(!malformed_set.contains(b"k"));
    assert!(!malformed_set.contains_str("k"));
    assert!(!malformed_set.contains_pod(&1_u8));
    assert_eq!(malformed_set.key_at(0), b"");
    assert_eq!(malformed_set.iter().count(), 0);
    assert!(matches!(
        malformed_set.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_set.try_empty(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_set.try_contains(b"k"),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_set.try_contains_str("k"),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_set.try_contains_pod(&1_u8),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_set.try_insert(b"k"),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_set.try_remove(b"k"),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(!malformed_set.insert(b"k"));
    assert!(!malformed_set.insert_str("k"));
    assert!(!malformed_set.insert_pod(&1_u8));
    assert!(!malformed_set.remove(b"k"));
    assert_eq!(
        malformed_set.data, before,
        "failed set mutators must not mutate malformed owned data"
    );

    let mut count_only_set = Set {
        data: vec![1, 0, 0, 0],
    };
    let before = count_only_set.data.clone();
    assert!(matches!(
        count_only_set.try_key_at(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        count_only_set.try_insert(b"k"),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        count_only_set.try_remove(b"k"),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        count_only_set.data, before,
        "count-only set payload must reject access/mutation without mutation"
    );

    let short_map_view = datapod::MapView { data: &[0, 0, 0] };
    assert_eq!(short_map_view.size(), 0);
    assert!(short_map_view.is_empty());
    assert!(matches!(
        short_map_view.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_map_view.key_at(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let truncated_map_view = datapod::MapView {
        data: &[1, 0, 0, 0],
    };
    assert_eq!(
        truncated_map_view.size(),
        0,
        "infallible MapView size should fail closed for truncated borrowed state"
    );
    assert!(truncated_map_view.is_empty());
    assert!(matches!(
        truncated_map_view.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_map_view.value_at(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let short_set_view = datapod::SetView { data: &[0, 0, 0] };
    assert_eq!(short_set_view.size(), 0);
    assert!(short_set_view.is_empty());
    assert!(matches!(
        short_set_view.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_set_view.key_at(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let truncated_set_view = datapod::SetView {
        data: &[1, 0, 0, 0],
    };
    assert_eq!(
        truncated_set_view.size(),
        0,
        "infallible SetView size should fail closed for truncated borrowed state"
    );
    assert!(truncated_set_view.is_empty());
    assert!(matches!(
        truncated_set_view.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_set_view.key_at(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let overflowing_map_bytes = malformed_map_with_overflowing_entry_range();
    let overflowing_map_view = datapod::MapView {
        data: &overflowing_map_bytes,
    };
    assert!(matches!(
        overflowing_map_view.key_at(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        overflowing_map_view.value_at(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let mut overflowing_map = Map {
        data: overflowing_map_bytes.clone(),
    };
    let before = overflowing_map.data.clone();
    assert!(matches!(
        overflowing_map.try_key_at(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        overflowing_map.try_insert(b"z", b"v"),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        overflowing_map.data, before,
        "map access/mutation must reject overflowing entry ranges without mutation"
    );

    let overflowing_set_bytes = malformed_set_with_overflowing_entry_range();
    let overflowing_set_view = datapod::SetView {
        data: &overflowing_set_bytes,
    };
    assert!(matches!(
        overflowing_set_view.key_at(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let mut overflowing_set = Set {
        data: overflowing_set_bytes.clone(),
    };
    let before = overflowing_set.data.clone();
    assert!(matches!(
        overflowing_set.try_key_at(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        overflowing_set.try_insert(b"z"),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        overflowing_set.data, before,
        "set access/mutation must reject overflowing entry ranges without mutation"
    );
}

#[test]
fn vector_bitvec_and_vecvec_views_validate_semantics() {
    let vector = Vector::from_bytes::<u16>(bytemuck::cast_slice(&[10_u16, 20, 30]).to_vec());
    assert_eq!(vector.try_get::<u16>(1).unwrap(), 20);
    assert_eq!(vector.try_size().unwrap(), 3);
    assert!(!vector.try_empty().unwrap());
    assert!(matches!(
        vector.try_get::<u16>(3),
        Err(WireError::InvalidHeader { .. })
    ));
    let mut vector_mut = vector.clone();
    vector_mut.try_set::<u16>(1, 99).unwrap();
    assert_eq!(vector_mut.try_get::<u16>(1).unwrap(), 99);
    assert!(matches!(
        Vector::try_from_bytes::<u16>(vec![0xaa]),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let invalid_vector_from_bytes = Vector::from_bytes::<u16>(vec![0xaa]);
    assert_eq!(invalid_vector_from_bytes.size(), 0);
    assert!(invalid_vector_from_bytes.empty());
    let overflowing_vector = Vector::with_capacity::<u16>(usize::MAX);
    assert_eq!(overflowing_vector.size(), 0);
    assert!(overflowing_vector.empty());
    let mut malformed_vector = Vector {
        element_size: 2,
        _pad: 0,
        data: vec![0xaa],
    };
    let before = malformed_vector.data.clone();
    assert!(matches!(
        malformed_vector.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_vector.try_empty(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        malformed_vector.size(),
        0,
        "infallible vector size should fail closed for malformed owned state"
    );
    assert!(
        malformed_vector.empty(),
        "infallible vector empty should fail closed for malformed owned state"
    );
    assert!(matches!(
        malformed_vector.try_get::<u16>(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_vector.try_set::<u16>(0, 7),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_vector.try_push::<u16>(7),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_vector.try_pop::<u16>(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(malformed_vector.get::<u16>(0), 0);
    malformed_vector.set::<u16>(0, 7);
    malformed_vector.push::<u16>(7);
    assert_eq!(malformed_vector.pop::<u16>(), None);
    assert!(malformed_vector.as_slice::<u16>().is_empty());
    assert!(malformed_vector.as_mut_slice::<u16>().is_empty());
    assert_eq!(
        malformed_vector.data, before,
        "failed vector push must not mutate malformed owned data"
    );
    let msg = to_wire_message(&vector);
    let view = access_wire::<Vector>(&msg).expect("vector view");
    assert_eq!(view.size(), 3);
    assert_eq!(view.try_size().unwrap(), 3);
    assert_eq!(view.get_unaligned::<u16>(1).unwrap(), 20);
    assert!(matches!(
        view.get_unaligned::<u32>(0),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(matches!(
        view.as_aligned_slice::<u32>(),
        Err(WireError::InvalidHeader { .. })
    ));

    let short_vector_view = datapod::VectorView {
        header: datapod::VectorHeader {
            element_size: 2,
            _pad: 0,
        },
        data: &[0xaa],
    };
    assert_eq!(short_vector_view.size(), 0);
    assert!(matches!(
        short_vector_view.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_vector_view.get_unaligned::<u16>(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_vector_view.as_aligned_slice::<u16>(),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let mut bad = msg;
    bad.bytes.push(0xff);
    assert!(matches!(
        validate_wire::<Vector>(&bad),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let mut bits = BitVec::with_len(3);
    bits.set_bit(0, true);
    bits.set_bit(2, true);
    assert!(bits.try_test(0).unwrap());
    assert!(matches!(
        bits.try_test(3),
        Err(WireError::InvalidHeader { .. })
    ));
    let before = bits.data.clone();
    assert!(matches!(
        bits.try_set_bit(3, true),
        Err(WireError::InvalidHeader { .. })
    ));
    assert_eq!(bits.data, before, "failed bit set must not mutate BitVec");
    bits.try_flip(1).unwrap();
    assert!(bits.try_test(1).unwrap());
    bits.try_push(false).unwrap();
    assert_eq!(bits.size(), 4);
    assert!(!bits.try_pop().unwrap().unwrap());
    let msg = to_wire_message(&bits);
    let view = access_wire::<BitVec>(&msg).expect("bitvec view");
    assert_eq!(view.bits(), 3);
    assert!(view.test(0).unwrap());
    assert!(view.test(1).unwrap());
    assert!(view.test(2).unwrap());
    assert!(matches!(view.test(3), Err(WireError::InvalidHeader { .. })));
    assert_eq!(view.try_count_ones().unwrap(), 3);
    assert_eq!(view.count_ones(), 3);

    let mut slack = msg;
    *slack.bytes.last_mut().unwrap() |= 0b1111_1000;
    assert!(matches!(
        validate_wire::<BitVec>(&slack),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let overflowing_bits = BitVec::with_len(usize::MAX);
    assert_eq!(overflowing_bits.size(), 0);
    assert!(overflowing_bits.empty());

    let mut malformed_bits = BitVec {
        bits: 9,
        data: vec![0b1010_1010],
    };
    let before = malformed_bits.data.clone();
    assert!(matches!(
        malformed_bits.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_bits.try_empty(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        malformed_bits.size(),
        0,
        "infallible BitVec size should fail closed for malformed owned state"
    );
    assert!(
        malformed_bits.empty(),
        "infallible BitVec empty should fail closed for malformed owned state"
    );
    assert!(matches!(
        malformed_bits.try_test(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_bits.try_set_bit(0, true),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_bits.try_flip(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_bits.try_push(true),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_bits.try_pop(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_bits.try_count_ones(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_bits.try_one_out(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(!malformed_bits.test(0));
    malformed_bits.set_bit(0, true);
    malformed_bits.flip(0);
    malformed_bits.push(true);
    assert_eq!(malformed_bits.pop(), None);
    assert_eq!(malformed_bits.count_ones(), 0);
    assert!(!malformed_bits.any());
    assert!(malformed_bits.none());
    malformed_bits.zero_out();
    malformed_bits.one_out();
    assert_eq!(
        malformed_bits.data, before,
        "failed bit mutators must not mutate malformed BitVec"
    );

    let short_bit_view = datapod::BitVecView {
        header: datapod::BitVecHeader { bits: 9 },
        data: &[0xff],
    };
    assert!(matches!(
        short_bit_view.test(8),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_bit_view.try_count_ones(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_bit_view.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        short_bit_view.size(),
        0,
        "infallible BitVecView size should fail closed for malformed borrowed state"
    );
    assert_eq!(short_bit_view.count_ones(), 0);

    let slack_bit_view = datapod::BitVecView {
        header: datapod::BitVecHeader { bits: 3 },
        data: &[0xff],
    };
    assert!(matches!(
        slack_bit_view.test(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        slack_bit_view.try_count_ones(),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    const BITVEC_RS: &str = include_str!("../src/seq/bitvec.rs");
    for needle in [
        "fn bit_tail_bits<T: 'static>(bits: u64) -> Result<usize, WireError>",
        "usize::try_from(bits % 8)",
        "fn byte_count_ones<T: 'static>(byte: u8) -> Result<usize, WireError>",
        "usize::try_from(byte.count_ones())",
        "fn count_bit_ones<T: 'static>(bytes: &[u8]) -> Result<usize, WireError>",
        ".checked_add(byte_count_ones::<T>(*byte)?)",
    ] {
        assert!(
            BITVEC_RS.contains(needle),
            "BitVec should keep checked bit-count conversion path {needle:?}"
        );
    }
    for forbidden in [
        "b.count_ones() as usize",
        "(*tail & mask).count_ones() as usize",
        "(header.bits % 8) as usize",
    ] {
        assert!(
            !BITVEC_RS.contains(forbidden),
            "BitVec should not retain unchecked count/tail cast {forbidden:?}"
        );
    }

    let mut ragged = Vecvec::new::<u16>();
    ragged.push_bucket::<u16>(&[1, 2]);
    ragged.push_bucket::<u16>(&[3]);
    let msg = to_wire_message(&ragged);
    let view = access_wire::<Vecvec>(&msg).expect("vecvec view");
    assert_eq!(view.size(), 2);
    assert_eq!(view.bucket_unaligned::<u16>(0).unwrap(), vec![1, 2]);
    assert_eq!(view.bucket_unaligned::<u16>(1).unwrap(), vec![3]);
    assert!(matches!(
        view.bucket_unaligned::<u32>(0),
        Err(WireError::InvalidHeader { .. })
    ));
    assert_eq!(ragged.try_bucket_size(0).unwrap(), 2);
    assert!(matches!(
        ragged.try_bucket_size(2),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        ragged.try_bucket::<u32>(0),
        Err(WireError::InvalidHeader { .. })
    ));

    let before = ragged.data.clone();
    assert!(matches!(
        ragged.try_push_bucket_bytes(&[0xaa]),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        ragged.data, before,
        "failed byte append must not mutate Vecvec"
    );

    let short_vecvec_view = datapod::VecvecView {
        header: datapod::VecvecHeader {
            element_size: 2,
            _pad: 0,
        },
        data: &[0, 0, 0],
    };
    assert_eq!(short_vecvec_view.bucket_count(), 0);
    assert_eq!(short_vecvec_view.size(), 0);
    assert_eq!(short_vecvec_view.header_bytes(), 0);
    assert!(matches!(
        short_vecvec_view.try_bucket_count(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_vecvec_view.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_vecvec_view.bucket_bytes(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        short_vecvec_view.bucket_unaligned::<u16>(0),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let count_only_vecvec_view = datapod::VecvecView {
        header: datapod::VecvecHeader {
            element_size: 2,
            _pad: 0,
        },
        data: &[1, 0, 0, 0],
    };
    assert_eq!(count_only_vecvec_view.bucket_count(), 0);
    assert_eq!(
        count_only_vecvec_view.size(),
        0,
        "infallible VecvecView size should fail closed for count-only borrowed state"
    );
    assert!(matches!(
        count_only_vecvec_view.try_bucket_count(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        count_only_vecvec_view.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let mut malformed_owned = Vecvec {
        element_size: 2,
        _pad: 0,
        data: vec![0, 0, 0],
    };
    let before = malformed_owned.data.clone();
    assert!(matches!(
        malformed_owned.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_owned.try_empty(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_owned.try_push_bucket::<u16>(&[9]),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(malformed_owned.size(), 0);
    assert!(malformed_owned.empty());
    assert!(malformed_owned.bucket::<u16>(0).is_empty());
    assert_eq!(malformed_owned.bucket_size(0), 0);
    malformed_owned.push_bucket::<u16>(&[9]);
    malformed_owned.push_bucket_bytes(&[0, 9]);
    assert_eq!(
        malformed_owned.data, before,
        "failed vecvec convenience appends must not mutate malformed owned data"
    );
    let count_only_owned = Vecvec {
        element_size: 2,
        _pad: 0,
        data: vec![1, 0, 0, 0],
    };
    assert!(matches!(
        count_only_owned.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        count_only_owned.size(),
        0,
        "infallible Vecvec size should fail closed for count-only owned state"
    );
    assert!(
        count_only_owned.empty(),
        "infallible Vecvec empty should fail closed for count-only owned state"
    );

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
    assert_eq!(stack.try_size().unwrap(), 2);
    assert!(!stack.try_empty().unwrap());
    assert_eq!(stack.try_top::<u16>().unwrap(), Some(2));
    assert_eq!(stack.try_pop::<u16>().unwrap(), Some(2));
    stack.try_push(3_u16).unwrap();
    assert_eq!(stack.try_top::<u16>().unwrap(), Some(3));
    let msg = to_wire_message(&stack);
    assert_eq!(access_wire::<Stack>(&msg).unwrap().payload.len(), 4);

    let mut bad = msg.clone();
    bad.bytes.push(0xff);
    assert!(matches!(
        validate_wire::<Stack>(&bad),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let mut malformed_stack = Stack {
        element_size: 2,
        _pad: 0,
        data: vec![0xaa],
    };
    let before = malformed_stack.data.clone();
    assert!(matches!(
        malformed_stack.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_stack.try_empty(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        malformed_stack.size(),
        0,
        "infallible stack size should fail closed for malformed owned state"
    );
    assert!(
        malformed_stack.empty(),
        "infallible stack empty should fail closed for malformed owned state"
    );
    assert!(matches!(
        malformed_stack.try_push::<u16>(7),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(malformed_stack.top::<u16>(), None);
    assert_eq!(malformed_stack.pop::<u16>(), None);
    malformed_stack.push(7_u16);
    assert_eq!(
        malformed_stack.data, before,
        "failed stack push must not mutate malformed owned data"
    );

    let mut queue = Queue::new::<u16>();
    queue.push_back(1_u16);
    queue.push_back(2_u16);
    assert_eq!(queue.try_size().unwrap(), 2);
    assert!(!queue.try_empty().unwrap());
    assert_eq!(queue.try_front_elem::<u16>().unwrap(), Some(1));
    assert_eq!(queue.try_back_elem::<u16>().unwrap(), Some(2));
    assert_eq!(queue.try_pop_front::<u16>().unwrap(), Some(1));
    queue.try_push_back(3_u16).unwrap();
    assert_eq!(queue.try_front_elem::<u16>().unwrap(), Some(2));
    assert_eq!(queue.try_back_elem::<u16>().unwrap(), Some(3));
    let msg = to_wire_message(&queue);
    access_wire::<Queue>(&msg).expect("queue view");

    let mut compacting_queue = Queue::new::<u16>();
    for value in [1_u16, 2, 3, 4] {
        compacting_queue.try_push_back(value).unwrap();
    }
    assert_eq!(compacting_queue.try_pop_front::<u16>().unwrap(), Some(1));
    assert_eq!(compacting_queue.front, 1);
    assert_eq!(compacting_queue.try_pop_front::<u16>().unwrap(), Some(2));
    assert_eq!(
        compacting_queue.front, 0,
        "queue should compact and reset front once popped slots reach half the raw buffer"
    );
    assert_eq!(
        compacting_queue.data.len(),
        2 * core::mem::size_of::<u16>(),
        "queue compaction should drain only the popped prefix"
    );
    assert_eq!(compacting_queue.try_front_elem::<u16>().unwrap(), Some(3));
    assert_eq!(compacting_queue.try_back_elem::<u16>().unwrap(), Some(4));

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
    let mut malformed_queue = Queue {
        element_size: 2,
        front: 99,
        data: vec![0, 1],
    };
    let before = malformed_queue.data.clone();
    assert!(matches!(
        malformed_queue.try_size(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(matches!(
        malformed_queue.try_empty(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert_eq!(
        malformed_queue.size(),
        0,
        "infallible queue size should fail closed for malformed owned state"
    );
    assert!(
        malformed_queue.empty(),
        "infallible queue empty should fail closed for malformed owned state"
    );
    assert!(matches!(
        malformed_queue.try_push_back::<u16>(7),
        Err(WireError::InvalidHeader { .. })
    ));
    assert_eq!(malformed_queue.front_elem::<u16>(), None);
    assert_eq!(malformed_queue.back_elem::<u16>(), None);
    assert_eq!(malformed_queue.pop_front::<u16>(), None);
    malformed_queue.push_back(7_u16);
    assert_eq!(
        malformed_queue.data, before,
        "failed queue push must not mutate malformed owned data"
    );

    let mut deque = Deque::new::<u16>();
    deque.push_front(1_u16);
    deque.push_back(2_u16);
    deque.try_push_front(0_u16).unwrap();
    deque.try_push_back(3_u16).unwrap();
    assert_eq!(deque.try_pop_front::<u16>().unwrap(), Some(0));
    assert_eq!(deque.try_pop_back::<u16>().unwrap(), Some(3));
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
    let misaligned_deque = Deque {
        element_size: 2,
        split_byte: 1,
        data: vec![1, 0, 2, 0],
    };
    assert!(matches!(
        validate_wire::<Deque>(&to_wire_message(&misaligned_deque)),
        Err(WireError::InvalidHeader { .. })
    ));
    let mut malformed_deque = Deque {
        element_size: 2,
        split_byte: 1,
        data: vec![1, 0, 2, 0],
    };
    let before = malformed_deque.data.clone();
    let before_split = malformed_deque.split_byte;
    assert!(matches!(
        malformed_deque.try_size(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(matches!(
        malformed_deque.try_empty(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert_eq!(
        malformed_deque.size(),
        0,
        "infallible deque size should fail closed for malformed owned state"
    );
    assert!(
        malformed_deque.empty(),
        "infallible deque empty should fail closed for malformed owned state"
    );
    assert!(matches!(
        malformed_deque.try_push_front::<u16>(7),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(matches!(
        malformed_deque.try_pop_front::<u16>(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(matches!(
        malformed_deque.try_pop_back::<u16>(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert_eq!(malformed_deque.pop_front::<u16>(), None);
    assert_eq!(malformed_deque.pop_back::<u16>(), None);
    malformed_deque.push_front(7_u16);
    malformed_deque.push_back(8_u16);
    assert_eq!(
        malformed_deque.data, before,
        "failed deque push must not mutate malformed owned data"
    );
    assert_eq!(malformed_deque.split_byte, before_split);

    let mut heap = Heap::new::<u16>();
    heap.push(4_u16);
    heap.push(9_u16);
    assert_eq!(heap.try_top::<u16>().unwrap(), Some(9));
    heap.try_push(7_u16).unwrap();
    assert_eq!(heap.try_pop::<u16>().unwrap(), Some(9));
    assert_eq!(heap.try_top::<u16>().unwrap(), Some(7));
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
    assert!(matches!(
        bad_heap.try_size(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(matches!(
        bad_heap.try_empty(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(matches!(
        bad_heap.try_top::<u16>(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert_eq!(
        bad_heap.size(),
        0,
        "infallible heap size should fail closed for invalid header state"
    );
    assert!(
        bad_heap.empty(),
        "infallible heap empty should fail closed for invalid header state"
    );
    let mut truncated_heap = Heap {
        element_size: 2,
        order: HeapOrder::Max,
        _pad: [0; 3],
        data: vec![0xaa],
    };
    let before_truncated_heap = truncated_heap.data.clone();
    assert!(matches!(
        validate_wire::<Heap>(&to_wire_message(&truncated_heap)),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_heap.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_heap.try_empty(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        truncated_heap.size(),
        0,
        "infallible heap size should fail closed for truncated owned state"
    );
    assert!(
        truncated_heap.empty(),
        "infallible heap empty should fail closed for truncated owned state"
    );
    assert!(matches!(
        truncated_heap.try_top::<u16>(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_heap.try_pop::<u16>(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_heap.try_push::<u16>(7),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(truncated_heap.top::<u16>(), None);
    assert_eq!(truncated_heap.pop::<u16>(), None);
    truncated_heap.push(7_u16);
    assert_eq!(
        truncated_heap.data, before_truncated_heap,
        "failed heap access/mutation must not mutate truncated owned data"
    );
    let mut malformed_heap = Heap {
        element_size: 2,
        order: HeapOrder::Max,
        _pad: [0; 3],
        data: bytemuck::cast_slice(&[1_u16, 9]).to_vec(),
    };
    let before = malformed_heap.data.clone();
    assert!(matches!(
        malformed_heap.try_push::<u16>(7),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(malformed_heap.top::<u16>(), Some(1));
    assert_eq!(malformed_heap.pop::<u16>(), None);
    malformed_heap.push(7_u16);
    assert_eq!(
        malformed_heap.data, before,
        "failed heap push must not mutate malformed owned data"
    );

    let mut indexed = IndexedHeap::new::<u16>();
    indexed.push(7, 1_u16);
    indexed.push(8, 2_u16);
    assert_eq!(indexed.try_top::<u16>().unwrap(), Some((8, 2)));
    indexed.try_push(7, 3_u16).unwrap();
    assert_eq!(indexed.try_top::<u16>().unwrap(), Some((7, 3)));
    assert_eq!(indexed.try_position_of(8).unwrap(), Some(1));
    assert_eq!(indexed.try_build_index().unwrap().len(), 2);
    assert_eq!(indexed.try_pop::<u16>().unwrap(), Some((7, 3)));
    let msg = to_wire_message(&indexed);
    access_wire::<IndexedHeap>(&msg).expect("indexed heap view");

    let bad_indexed_header = IndexedHeap {
        priority_size: 2,
        order: HeapOrder(9),
        _pad: [0; 3],
        data: indexed.data.clone(),
    };
    assert!(matches!(
        bad_indexed_header.try_size(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(matches!(
        bad_indexed_header.try_top::<u16>(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(matches!(
        bad_indexed_header.try_position_of(7),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(matches!(
        bad_indexed_header.try_build_index(),
        Err(WireError::InvalidHeader { .. })
    ));

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
    let mut duplicate_indexed = IndexedHeap {
        priority_size: 2,
        order: HeapOrder::Max,
        _pad: [0; 3],
        data: duplicate_indexed_heap_payload(),
    };
    let before_duplicate = duplicate_indexed.data.clone();
    assert!(matches!(
        duplicate_indexed.try_push::<u16>(9, 7),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        duplicate_indexed.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        duplicate_indexed.try_empty(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(duplicate_indexed.size(), 0);
    assert!(duplicate_indexed.empty());
    assert_eq!(duplicate_indexed.top::<u16>(), None);
    assert_eq!(duplicate_indexed.pop::<u16>(), None);
    assert_eq!(duplicate_indexed.position_of(7), None);
    assert!(duplicate_indexed.build_index().is_empty());
    duplicate_indexed.push(9, 7_u16);
    assert_eq!(
        duplicate_indexed.data, before_duplicate,
        "failed indexed heap convenience calls must not mutate structurally malformed owned data"
    );

    let truncated_indexed = IndexedHeap {
        priority_size: 2,
        order: HeapOrder::Max,
        _pad: [0; 3],
        data: truncated_indexed_heap_payload(),
    };
    assert!(matches!(
        validate_wire::<IndexedHeap>(&to_wire_message(&truncated_indexed)),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let mut truncated_indexed = truncated_indexed;
    let before_truncated = truncated_indexed.data.clone();
    assert!(matches!(
        truncated_indexed.try_top::<u16>(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_indexed.try_pop::<u16>(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_indexed.try_position_of(7),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_indexed.try_build_index(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_indexed.try_push::<u16>(9, 7),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_indexed.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_indexed.try_empty(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(truncated_indexed.size(), 0);
    assert!(truncated_indexed.empty());
    assert_eq!(truncated_indexed.top::<u16>(), None);
    assert_eq!(truncated_indexed.pop::<u16>(), None);
    assert_eq!(truncated_indexed.position_of(7), None);
    assert!(truncated_indexed.build_index().is_empty());
    truncated_indexed.push(9, 7_u16);
    assert_eq!(
        truncated_indexed.data, before_truncated,
        "failed indexed heap access/mutation must not mutate truncated owned data"
    );

    let mut malformed_indexed = IndexedHeap {
        priority_size: 2,
        order: HeapOrder::Max,
        _pad: [0; 3],
        data: indexed_heap_order_violation_payload(),
    };
    let before = malformed_indexed.data.clone();
    assert!(matches!(
        malformed_indexed.try_push::<u16>(9, 7),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(malformed_indexed.top::<u16>(), Some((7, 1)));
    assert_eq!(malformed_indexed.pop::<u16>(), None);
    assert_eq!(malformed_indexed.position_of(7), Some(0));
    assert_eq!(malformed_indexed.build_index().len(), 2);
    malformed_indexed.push(9, 7_u16);
    assert_eq!(
        malformed_indexed.data, before,
        "failed indexed heap push must not mutate malformed owned data"
    );

    let mut list = List::new::<u16>();
    list.push_back(1_u16);
    list.push_back(2_u16);
    assert_eq!(list.try_size().unwrap(), 2);
    assert!(!list.try_empty().unwrap());
    assert_eq!(list.try_capacity().unwrap(), 2);
    assert_eq!(list.try_front::<u16>().unwrap(), Some(1));
    assert_eq!(list.try_back::<u16>().unwrap(), Some(2));
    list.try_push_front(0_u16).unwrap();
    assert_eq!(list.try_pop_front::<u16>().unwrap(), Some(0));
    assert_eq!(list.try_pop_back::<u16>().unwrap(), Some(2));
    list.try_push_back(3_u16).unwrap();
    assert_eq!(list.try_back::<u16>().unwrap(), Some(3));
    assert_eq!(
        list.try_iter::<u16>().unwrap().collect::<Vec<_>>(),
        vec![1, 3]
    );
    assert!(matches!(
        list.try_iter::<u32>(),
        Err(WireError::InvalidHeader { .. })
    ));
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
    assert_eq!(
        bad_list.size(),
        0,
        "infallible list size should fail closed for invalid header state"
    );
    assert!(
        bad_list.empty(),
        "infallible list empty should fail closed for invalid header state"
    );
    assert_eq!(
        bad_list.capacity(),
        0,
        "infallible list capacity should fail closed for invalid header state"
    );
    let mut truncated_list = List {
        head: 0,
        tail: 0,
        free_head: u32::MAX,
        size_: 1,
        element_size: 2,
        _pad: 0,
        data: vec![0xaa],
    };
    let before_truncated_list = truncated_list.data.clone();
    let before_truncated_list_size = truncated_list.size_;
    assert!(matches!(
        validate_wire::<List>(&to_wire_message(&truncated_list)),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_list.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_list.try_empty(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_list.try_capacity(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        truncated_list.size(),
        0,
        "infallible list size should fail closed for truncated owned state"
    );
    assert!(
        truncated_list.empty(),
        "infallible list empty should fail closed for truncated owned state"
    );
    assert_eq!(
        truncated_list.capacity(),
        0,
        "infallible list capacity should fail closed for truncated owned state"
    );
    assert!(matches!(
        truncated_list.try_front::<u16>(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_list.try_pop_front::<u16>(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_list.try_push_back::<u16>(7),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(truncated_list.front::<u16>(), None);
    assert_eq!(truncated_list.pop_front::<u16>(), None);
    truncated_list.push_back(7_u16);
    assert_eq!(
        truncated_list.data, before_truncated_list,
        "failed list access/mutation must not mutate truncated owned data"
    );
    assert_eq!(truncated_list.size_, before_truncated_list_size);
    let mut malformed_list = bad_list.clone();
    let before = malformed_list.data.clone();
    let before_size = malformed_list.size_;
    assert!(matches!(
        malformed_list.try_size(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(matches!(
        malformed_list.try_empty(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(matches!(
        malformed_list.try_capacity(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert_eq!(
        malformed_list.size(),
        0,
        "infallible list size should fail closed for malformed owned state"
    );
    assert!(
        malformed_list.empty(),
        "infallible list empty should fail closed for malformed owned state"
    );
    assert_eq!(
        malformed_list.capacity(),
        0,
        "infallible list capacity should fail closed for malformed owned state"
    );
    assert!(matches!(
        malformed_list.try_iter::<u16>(),
        Err(WireError::InvalidHeader { .. })
    ));
    assert!(matches!(
        malformed_list.try_push_back::<u16>(7),
        Err(WireError::InvalidHeader { .. })
    ));
    assert_eq!(malformed_list.front::<u16>(), None);
    assert_eq!(malformed_list.back::<u16>(), None);
    assert_eq!(malformed_list.pop_front::<u16>(), None);
    assert_eq!(malformed_list.pop_back::<u16>(), None);
    assert_eq!(
        malformed_list.iter::<u16>().collect::<Vec<_>>(),
        Vec::<u16>::new()
    );
    malformed_list.push_front(6_u16);
    malformed_list.push_back(7_u16);
    assert_eq!(
        malformed_list.data, before,
        "failed list push must not mutate malformed owned data"
    );
    assert_eq!(malformed_list.size_, before_size);
    let cyclic_list = List {
        head: 0,
        tail: 1,
        free_head: u32::MAX,
        size_: 2,
        element_size: 1,
        _pad: 0,
        data: linked_list_payload_with_cycle(),
    };
    assert!(matches!(
        validate_wire::<List>(&to_wire_message(&cyclic_list)),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let overlapping_free_list = List {
        head: 0,
        tail: 0,
        free_head: 0,
        size_: 1,
        element_size: 1,
        _pad: 0,
        data: one_node_list_payload(),
    };
    assert!(matches!(
        validate_wire::<List>(&to_wire_message(&overlapping_free_list)),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let orphan_free_list_slot = List {
        head: u32::MAX,
        tail: u32::MAX,
        free_head: u32::MAX,
        size_: 0,
        element_size: 1,
        _pad: 0,
        data: one_free_list_node_payload(),
    };
    assert!(matches!(
        validate_wire::<List>(&to_wire_message(&orphan_free_list_slot)),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let reserved_prev_free_list_slot = List {
        head: u32::MAX,
        tail: u32::MAX,
        free_head: 0,
        size_: 0,
        element_size: 1,
        _pad: 0,
        data: free_list_node_with_reserved_prev_payload(),
    };
    assert!(matches!(
        validate_wire::<List>(&to_wire_message(&reserved_prev_free_list_slot)),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let mut forward = ForwardList::new::<u16>();
    forward.push_front(1_u16);
    forward.push_front(2_u16);
    assert_eq!(forward.try_size().unwrap(), 2);
    assert!(!forward.try_empty().unwrap());
    assert_eq!(forward.try_capacity().unwrap(), 2);
    assert_eq!(forward.try_front::<u16>().unwrap(), Some(2));
    forward.try_push_front(3_u16).unwrap();
    assert_eq!(forward.try_pop_front::<u16>().unwrap(), Some(3));
    assert_eq!(
        forward.try_iter::<u16>().unwrap().collect::<Vec<_>>(),
        vec![2, 1]
    );
    assert!(matches!(
        forward.try_iter::<u32>(),
        Err(WireError::InvalidHeader { .. })
    ));
    let msg = to_wire_message(&forward);
    access_wire::<ForwardList>(&msg).expect("forward list view");
    let overlapping_forward_free = ForwardList {
        head: 0,
        free_head: 0,
        size_: 1,
        element_size: 1,
        data: one_forward_node_payload(),
    };
    assert!(matches!(
        validate_wire::<ForwardList>(&to_wire_message(&overlapping_forward_free)),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        overlapping_forward_free.size(),
        0,
        "infallible forward-list size should fail closed for overlapping free-list state"
    );
    assert!(
        overlapping_forward_free.empty(),
        "infallible forward-list empty should fail closed for overlapping free-list state"
    );
    assert_eq!(
        overlapping_forward_free.capacity(),
        0,
        "infallible forward-list capacity should fail closed for overlapping free-list state"
    );
    let mut truncated_forward = ForwardList {
        head: 0,
        free_head: u32::MAX,
        size_: 1,
        element_size: 2,
        data: vec![0xaa],
    };
    let before_truncated_forward = truncated_forward.data.clone();
    let before_truncated_forward_size = truncated_forward.size_;
    assert!(matches!(
        validate_wire::<ForwardList>(&to_wire_message(&truncated_forward)),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_forward.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_forward.try_empty(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_forward.try_capacity(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        truncated_forward.size(),
        0,
        "infallible forward-list size should fail closed for truncated owned state"
    );
    assert!(
        truncated_forward.empty(),
        "infallible forward-list empty should fail closed for truncated owned state"
    );
    assert_eq!(
        truncated_forward.capacity(),
        0,
        "infallible forward-list capacity should fail closed for truncated owned state"
    );
    assert!(matches!(
        truncated_forward.try_front::<u16>(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_forward.try_pop_front::<u16>(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        truncated_forward.try_push_front::<u16>(7),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(truncated_forward.front::<u16>(), None);
    assert_eq!(truncated_forward.pop_front::<u16>(), None);
    truncated_forward.push_front(7_u16);
    assert_eq!(
        truncated_forward.data, before_truncated_forward,
        "failed forward-list access/mutation must not mutate truncated owned data"
    );
    assert_eq!(truncated_forward.size_, before_truncated_forward_size);
    let mut malformed_forward = overlapping_forward_free.clone();
    let before = malformed_forward.data.clone();
    let before_size = malformed_forward.size_;
    assert!(matches!(
        malformed_forward.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_forward.try_empty(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_forward.try_capacity(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        malformed_forward.size(),
        0,
        "infallible forward-list size should fail closed for malformed owned state"
    );
    assert!(
        malformed_forward.empty(),
        "infallible forward-list empty should fail closed for malformed owned state"
    );
    assert_eq!(
        malformed_forward.capacity(),
        0,
        "infallible forward-list capacity should fail closed for malformed owned state"
    );
    assert!(matches!(
        malformed_forward.try_iter::<u8>(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        malformed_forward.try_push_front::<u8>(7),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(malformed_forward.front::<u8>(), None);
    assert_eq!(malformed_forward.pop_front::<u8>(), None);
    assert_eq!(
        malformed_forward.iter::<u8>().collect::<Vec<_>>(),
        Vec::<u8>::new()
    );
    malformed_forward.push_front(7_u8);
    assert_eq!(
        malformed_forward.data, before,
        "failed forward-list push must not mutate malformed owned data"
    );
    assert_eq!(malformed_forward.size_, before_size);
    let orphan_forward_slot = ForwardList {
        head: u32::MAX,
        free_head: u32::MAX,
        size_: 0,
        element_size: 1,
        data: one_forward_node_payload(),
    };
    assert!(matches!(
        validate_wire::<ForwardList>(&to_wire_message(&orphan_forward_slot)),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let reserved_forward_pad = ForwardList {
        head: 0,
        free_head: u32::MAX,
        size_: 1,
        element_size: 1,
        data: forward_node_with_reserved_pad_payload(),
    };
    assert!(matches!(
        validate_wire::<ForwardList>(&to_wire_message(&reserved_forward_pad)),
        Err(WireError::InvalidPayloadSize { .. })
    ));

    let mut paged = PagedVecvec::new::<u16>();
    assert!(paged.try_empty().unwrap());
    paged.push_bucket::<u16>(&[1, 2]);
    paged.push_bucket::<u16>(&[3]);
    assert_eq!(paged.try_size().unwrap(), 2);
    let msg = to_wire_message(&paged);
    access_wire::<PagedVecvec>(&msg).expect("paged vecvec view");
    assert_eq!(paged.try_bucket_size(1).unwrap(), 1);
    assert!(matches!(
        paged.try_bucket_size(2),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert!(matches!(
        paged.try_bucket::<u32>(0),
        Err(WireError::InvalidHeader { .. })
    ));
    let before = paged.data.clone();
    assert!(matches!(
        paged.try_push_bucket_bytes(&[0xaa]),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        paged.data, before,
        "failed byte append must not mutate PagedVecvec"
    );
    let mut malformed_paged_owned = PagedVecvec {
        element_size: 2,
        _pad: 0,
        data: vec![0, 0, 0],
    };
    assert!(matches!(
        malformed_paged_owned.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    let before = malformed_paged_owned.data.clone();
    assert!(matches!(
        malformed_paged_owned.try_push_bucket::<u16>(&[9]),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(malformed_paged_owned.size(), 0);
    assert!(malformed_paged_owned.empty());
    assert!(malformed_paged_owned.bucket::<u16>(0).is_empty());
    assert_eq!(malformed_paged_owned.bucket_size(0), 0);
    malformed_paged_owned.push_bucket::<u16>(&[9]);
    malformed_paged_owned.push_bucket_bytes(&[0, 9]);
    assert_eq!(
        malformed_paged_owned.data, before,
        "failed paged append must not mutate malformed owned data"
    );
    let count_only_paged_owned = PagedVecvec {
        element_size: 2,
        _pad: 0,
        data: vec![1, 0, 0, 0],
    };
    assert!(matches!(
        count_only_paged_owned.try_size(),
        Err(WireError::InvalidPayloadSize { .. })
    ));
    assert_eq!(
        count_only_paged_owned.size(),
        0,
        "infallible PagedVecvec size should fail closed for count-only owned state"
    );
    assert!(
        count_only_paged_owned.empty(),
        "infallible PagedVecvec empty should fail closed for count-only owned state"
    );
    let non_monotonic_paged = PagedVecvec {
        element_size: 1,
        _pad: 0,
        data: malformed_vecvec_non_monotonic_offsets(),
    };
    assert!(matches!(
        validate_wire::<PagedVecvec>(&to_wire_message(&non_monotonic_paged)),
        Err(WireError::InvalidPayloadSize { .. })
    ));
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

#[test]
fn network_identifier_datapods_validate_reserved_fields_and_tags() {
    let invalid_family = datapod::WireMessage {
        type_hash: datapod::bind::type_hash::<Ip>(),
        bytes: bytemuck::bytes_of(&Ip {
            family: 5,
            _pad: 0,
            bytes: [0; 16],
        })
        .to_vec(),
    };
    assert!(matches!(
        validate_registered_wire(invalid_family.type_hash, &invalid_family.bytes),
        Err(WireError::InvalidHeader { .. })
    ));

    let mut noncanonical_v4_bytes = [0_u8; 16];
    noncanonical_v4_bytes[0..4].copy_from_slice(&[192, 168, 0, 1]);
    noncanonical_v4_bytes[15] = 1;
    let noncanonical_v4 = datapod::WireMessage {
        type_hash: datapod::bind::type_hash::<Ip>(),
        bytes: bytemuck::bytes_of(&Ip {
            family: 4,
            _pad: 0,
            bytes: noncanonical_v4_bytes,
        })
        .to_vec(),
    };
    assert!(matches!(
        validate_registered_wire(noncanonical_v4.type_hash, &noncanonical_v4.bytes),
        Err(WireError::InvalidHeader { .. })
    ));

    let invalid_mac_pad = datapod::WireMessage {
        type_hash: datapod::bind::type_hash::<MacAddr>(),
        bytes: bytemuck::bytes_of(&MacAddr {
            bytes: [0xde, 0xad, 0xbe, 0xef, 0x12, 0x34],
            _pad: [0, 1],
        })
        .to_vec(),
    };
    assert!(matches!(
        validate_registered_wire(invalid_mac_pad.type_hash, &invalid_mac_pad.bytes),
        Err(WireError::InvalidHeader { .. })
    ));
}

#[test]
fn generated_fixed_datapods_reject_nonzero_reserved_padding() {
    let bad_link = Link {
        _pad: 1,
        ..Link::default()
    };
    let bad_link = datapod::WireMessage {
        type_hash: datapod::bind::type_hash::<Link>(),
        bytes: bytemuck::bytes_of(&bad_link).to_vec(),
    };
    assert!(matches!(
        validate_registered_wire(bad_link.type_hash, &bad_link.bytes),
        Err(WireError::InvalidHeader { .. })
    ));

    let bad_joint = TransmissionJoint {
        _pad: 1,
        ..TransmissionJoint::default()
    };
    let bad_joint = datapod::WireMessage {
        type_hash: datapod::bind::type_hash::<TransmissionJoint>(),
        bytes: bytemuck::bytes_of(&bad_joint).to_vec(),
    };
    assert!(matches!(
        validate_registered_wire(bad_joint.type_hash, &bad_joint.bytes),
        Err(WireError::InvalidHeader { .. })
    ));
}

#[test]
fn current_registered_validator_rejects_fixed_payload_bytes() {
    let point = Point::new(1.0, 2.0, 3.0);
    let mut message = to_wire_message(&point);
    message.bytes.push(0xff);

    assert!(matches!(
        validate_registered_wire(message.type_hash, &message.bytes),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}

fn bytes_0_to_15() -> Vec<u8> {
    (0_u8..16).collect()
}

fn one_node_list_payload() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.push(42);
    bytes.extend_from_slice(&u32::MAX.to_le_bytes());
    bytes.extend_from_slice(&u32::MAX.to_le_bytes());
    bytes
}

fn one_free_list_node_payload() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.push(0);
    bytes.extend_from_slice(&0_u32.to_le_bytes());
    bytes.extend_from_slice(&u32::MAX.to_le_bytes());
    bytes
}

fn free_list_node_with_reserved_prev_payload() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.push(0);
    bytes.extend_from_slice(&7_u32.to_le_bytes());
    bytes.extend_from_slice(&u32::MAX.to_le_bytes());
    bytes
}

fn linked_list_payload_with_cycle() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.push(1);
    bytes.extend_from_slice(&u32::MAX.to_le_bytes());
    bytes.extend_from_slice(&1_u32.to_le_bytes());
    bytes.push(2);
    bytes.extend_from_slice(&0_u32.to_le_bytes());
    bytes.extend_from_slice(&0_u32.to_le_bytes());
    bytes
}

fn one_forward_node_payload() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.push(42);
    bytes.extend_from_slice(&u32::MAX.to_le_bytes());
    bytes.extend_from_slice(&0_u32.to_le_bytes());
    bytes
}

fn forward_node_with_reserved_pad_payload() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.push(42);
    bytes.extend_from_slice(&u32::MAX.to_le_bytes());
    bytes.extend_from_slice(&7_u32.to_le_bytes());
    bytes
}

fn malformed_vecvec_non_monotonic_offsets() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&2_u32.to_le_bytes());
    bytes.extend_from_slice(&0_u32.to_le_bytes());
    bytes.extend_from_slice(&4_u32.to_le_bytes());
    bytes.extend_from_slice(&2_u32.to_le_bytes());
    bytes.extend_from_slice(&[1, 2, 3, 4]);
    bytes
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

fn malformed_map_with_trailing_blob_bytes() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&1_u32.to_le_bytes());
    bytes.extend_from_slice(bytemuck::bytes_of(&MapEntry {
        key_off: 0,
        key_len: 1,
        value_off: 1,
        value_len: 1,
    }));
    bytes.extend_from_slice(b"kv!");
    bytes
}

fn malformed_map_with_overflowing_entry_range() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&1_u32.to_le_bytes());
    bytes.extend_from_slice(bytemuck::bytes_of(&MapEntry {
        key_off: 0,
        key_len: u32::MAX,
        value_off: u32::MAX,
        value_len: u32::MAX,
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

fn malformed_set_with_overflowing_entry_range() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&1_u32.to_le_bytes());
    bytes.extend_from_slice(bytemuck::bytes_of(&SetEntry {
        key_off: 0,
        key_len: u32::MAX,
    }));
    bytes.extend_from_slice(b"x");
    bytes
}

fn malformed_set_with_noncanonical_offset() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&1_u32.to_le_bytes());
    bytes.extend_from_slice(bytemuck::bytes_of(&SetEntry {
        key_off: 1,
        key_len: 1,
    }));
    bytes.extend_from_slice(b"!k");
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

fn truncated_indexed_heap_payload() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&7_u64.to_le_bytes());
    bytes.push(0xaa);
    bytes
}

fn indexed_heap_order_violation_payload() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&7_u64.to_le_bytes());
    bytes.extend_from_slice(&1_u16.to_ne_bytes());
    bytes.extend_from_slice(&8_u64.to_le_bytes());
    bytes.extend_from_slice(&9_u16.to_ne_bytes());
    bytes
}
