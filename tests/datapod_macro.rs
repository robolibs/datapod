//! Smoke tests for `#[datapod]` and `#[derive(DataPod)]`.

use datapod::{
    DataPod, Encoding, Envelope, PayloadLayoutBuilder, PayloadSection, WireError, access_wire,
    access_wire_frame, bind, datapod, from_wire_message, to_wire_message, validate_wire,
};

// ---------------------------------------------------------------------------
// #[datapod] — fixed-Pod case (no #[dp(bytes)])
// ---------------------------------------------------------------------------

#[datapod]
pub struct Pose2 {
    pub x: f32,
    pub y: f32,
    pub yaw: f32,
}

#[test]
fn fixed_datapod_attr_emits_pod_and_trait() {
    let p = Pose2 {
        x: 1.0,
        y: 2.0,
        yaw: 0.5,
    };
    let bytes = bytemuck::bytes_of(&p);
    let q: &Pose2 = bytemuck::from_bytes(bytes);
    assert_eq!(*q, p);

    // Header = Self for fixed-Pod types.
    fn _assert<T: DataPod<Header = T, Payload = ()>>() {}
    _assert::<Pose2>();
    assert_eq!(p.header(), p);
    assert_eq!(p.payload_bytes().len(), 0);
}

// ---------------------------------------------------------------------------
// #[datapod] — heap-bearing case (data INSIDE; macro generates sibling header)
// ---------------------------------------------------------------------------

#[datapod]
#[derive(Default)]
pub struct DepthImage {
    pub width: u32,
    pub height: u32,
    pub encoding: Encoding,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

#[datapod]
#[derive(Default)]
pub struct Samples {
    pub count: u32,
    #[dp(bytes)]
    pub values: Vec<u16>,
}

#[datapod]
#[derive(Default)]
pub struct ImageWithMeta {
    pub width: u32,
    pub height: u32,
    pub pixels: PayloadSection,
    pub metadata: PayloadSection,
    #[dp(bytes)]
    pub payload_blob: Vec<u8>,
}

#[datapod]
#[derive(Default)]
pub struct SplitImage {
    pub width: u32,
    pub height: u32,
    #[dp(bytes, section = "pixels")]
    pub pixels: Vec<u8>,
    #[dp(bytes, section = "metadata")]
    pub metadata: Vec<u16>,
}

#[datapod(name = "robolibs.camera_frame.v1")]
#[derive(Default)]
pub struct CameraFrame {
    pub width: u32,
    pub height: u32,
    pub encoding: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

#[test]
fn variable_datapod_attr_keeps_data_inside_and_generates_sibling_header() {
    let img = DepthImage {
        width: 640,
        height: 480,
        encoding: Encoding::Mono16,
        data: vec![0u8; 640 * 480 * 2],
    };
    // Data is inside.
    assert_eq!(img.data.len(), 640 * 480 * 2);
    assert_eq!(img.width, 640);

    // Wire shipping: header is the generated sibling Pod struct.
    let h: DepthImageHeader = img.header();
    assert_eq!(h.width, 640);
    assert_eq!(h.height, 480);
    assert_eq!(h.encoding, Encoding::Mono16);
    // sibling IS Pod
    let _: &[u8] = bytemuck::bytes_of(&h);

    // Payload is the cast bytes of the inside Vec.
    assert_eq!(img.payload_bytes().len(), 640 * 480 * 2);

    fn _assert<T: DataPod<Payload = [u8]>>() {}
    _assert::<DepthImage>();
}

#[test]
fn variable_datapod_attr_generates_borrowed_view_and_default_validation() {
    let img = DepthImage {
        width: 2,
        height: 2,
        encoding: Encoding::Mono8,
        data: vec![1, 2, 3, 4],
    };
    let msg = to_wire_message(&img);

    validate_wire::<DepthImage>(&msg).expect("u8 payload validates at any byte length");
    let view = access_wire::<DepthImage>(&msg).expect("generated view");
    let _: DepthImageView<'_> = view;
    assert_eq!(view.header.width, 2);
    assert_eq!(view.header.height, 2);
    assert_eq!(view.payload_bytes(), &[1, 2, 3, 4]);

    let samples = Samples {
        count: 2,
        values: vec![10, 11],
    };
    let mut msg = to_wire_message(&samples);
    validate_wire::<Samples>(&msg).expect("u16 payload validates when byte length is even");
    let view = access_wire::<Samples>(&msg).expect("generated u16 view");
    let _: SamplesView<'_> = view;
    assert_eq!(view.header.count, 2);
    let expected_values = [10_u16, 11];
    let expected_bytes: &[u8] = bytemuck::cast_slice(&expected_values);
    assert_eq!(view.payload_bytes(), expected_bytes);

    msg.bytes.pop();
    assert!(matches!(
        validate_wire::<Samples>(&msg),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}

#[test]
fn variable_datapod_attr_validates_payload_section_fields() {
    let mut builder = PayloadLayoutBuilder::new();
    let pixels = builder.push_section(b"pixels").unwrap();
    let metadata = builder.push_section(b"meta").unwrap();
    let (_, payload_blob) = builder.finish();

    let image = ImageWithMeta {
        width: 2,
        height: 1,
        pixels,
        metadata,
        payload_blob,
    };
    let msg = to_wire_message(&image);
    validate_wire::<ImageWithMeta>(&msg).expect("payload sections validate");
    let view = access_wire::<ImageWithMeta>(&msg).expect("sectioned view");
    let _: ImageWithMetaView<'_> = view;
    assert_eq!(view.header.width, 2);
    assert_eq!(view.section_bytes(view.header.pixels).unwrap(), b"pixels");
    assert_eq!(view.section_bytes(view.header.metadata).unwrap(), b"meta");

    let bad = ImageWithMeta {
        width: 2,
        height: 1,
        pixels: PayloadSection::new(2, 4),
        metadata: PayloadSection::new(0, 2),
        payload_blob: b"abcdef".to_vec(),
    };
    let bad = to_wire_message(&bad);
    assert!(matches!(
        validate_wire::<ImageWithMeta>(&bad),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}

#[test]
fn variable_datapod_attr_supports_direct_multi_vec_sections() {
    let image = SplitImage {
        width: 2,
        height: 1,
        pixels: b"px".to_vec(),
        metadata: vec![10_u16, 11],
    };
    let msg = to_wire_message(&image);
    let header_len = core::mem::size_of::<SplitImageHeader>();

    assert_eq!(msg.bytes.len(), header_len + 2 + 4);
    validate_wire::<SplitImage>(&msg).expect("sectioned multi-vec payload validates");
    let view = access_wire::<SplitImage>(&msg).expect("sectioned view");
    let _: SplitImageView<'_> = view;
    assert_eq!(view.header.width, 2);
    assert_eq!(view.header.height, 1);
    assert_eq!(view.header.pixels, PayloadSection::new(0, 2));
    assert_eq!(view.header.metadata, PayloadSection::new(2, 4));
    assert_eq!(view.section_bytes(view.header.pixels).unwrap(), b"px");
    let expected_metadata: &[u8] = bytemuck::cast_slice(&[10_u16, 11]);
    assert_eq!(
        view.section_bytes(view.header.metadata).unwrap(),
        expected_metadata
    );

    let decoded: SplitImage = from_wire_message(&msg).expect("sectioned decode");
    assert_eq!(decoded.width, 2);
    assert_eq!(decoded.height, 1);
    assert_eq!(decoded.pixels, b"px");
    assert_eq!(decoded.metadata, vec![10_u16, 11]);

    let mut bad = msg;
    bad.bytes.pop();
    assert!(matches!(
        validate_wire::<SplitImage>(&bad),
        Err(WireError::InvalidPayloadSize { .. })
    ));
}

#[test]
fn datapod_attr_name_sets_custom_rust_canonical_wire_hash() {
    let image = CameraFrame {
        width: 640,
        height: 480,
        encoding: 11,
        data: vec![1, 2, 3, 4],
    };
    let expected_hash = bind::type_hash_name("robolibs.camera_frame.v1");

    assert_eq!(CameraFrame::CANONICAL_NAME, "robolibs.camera_frame.v1");
    assert_eq!(CameraFrame::TYPE_HASH, expected_hash);
    assert_eq!(bind::type_hash::<CameraFrame>(), expected_hash);
    assert_eq!(
        bind::type_hash_canonical::<CameraFrame>(),
        Some(expected_hash)
    );
    assert_eq!(
        datapod::registry::canonical_name::<CameraFrame>(),
        Some("robolibs.camera_frame.v1")
    );
    CameraFrame::register_schema().expect("custom Rust schema registers");
    let info = datapod::registry::find_type_info(expected_hash).expect("schema is discoverable");
    assert_eq!(info.canonical_name, "robolibs.camera_frame.v1");
    assert_eq!(info.rust_type_name, core::any::type_name::<CameraFrame>());
    assert_eq!(info.header_size, core::mem::size_of::<CameraFrameHeader>());
    assert_eq!(info.payload_kind, datapod::registry::PayloadKind::Bytes);
    assert_eq!(
        info.validator,
        datapod::registry::ValidatorKind::RegistryOnly
    );

    let msg = image.to_wire_message();
    assert_eq!(msg.type_hash, expected_hash);
    let msg_from_free_fn = to_wire_message(&image);
    assert_eq!(msg_from_free_fn.type_hash, expected_hash);
    datapod::validate_registered_wire(msg.type_hash, &msg.bytes)
        .expect("registered custom Rust schema validates generically");
    let split = datapod::split_wire_frame(msg.type_hash, &msg.bytes)
        .expect("registered custom Rust schema splits generically");
    assert_eq!(split.type_hash, expected_hash);
    assert_eq!(
        split.header.len(),
        core::mem::size_of::<CameraFrameHeader>()
    );
    assert_eq!(split.payload, &[1, 2, 3, 4]);

    image
        .with_wire_frame(|frame| {
            assert_eq!(frame.type_hash, expected_hash);
            CameraFrame::validate_wire_frame(frame).expect("canonical frame validates");
            let view = CameraFrame::view_from_wire_frame(frame).expect("canonical frame views");
            let _: CameraFrameView<'_> = view;
            assert_eq!(view.header.width, 640);
            assert_eq!(view.header.height, 480);
            assert_eq!(view.header.encoding, 11);
            assert_eq!(view.payload_bytes(), &[1, 2, 3, 4]);

            let view_from_free_fn =
                access_wire_frame::<CameraFrame>(frame).expect("free function accepts canonical");
            assert_eq!(view_from_free_fn.payload_bytes(), &[1, 2, 3, 4]);

            let owned = CameraFrame::from_wire_frame(frame).expect("frame decodes");
            assert_eq!(owned.width, 640);
            assert_eq!(owned.height, 480);
            assert_eq!(owned.encoding, 11);
            assert_eq!(owned.data, vec![1, 2, 3, 4]);
        })
        .expect("named custom Rust datapod exposes canonical frame");
}

// ---------------------------------------------------------------------------
// #[derive(DataPod)] — explicit form (fixed-Pod only)
// ---------------------------------------------------------------------------

use datapod::ZeroCopySend;

#[repr(C)]
#[derive(
    Debug, Clone, Copy, PartialEq, bytemuck::Pod, bytemuck::Zeroable, ZeroCopySend, datapod::DataPod,
)]
pub struct ManualPoint {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

#[test]
fn explicit_derive_form_emits_only_impl() {
    let p = ManualPoint {
        x: 1.0,
        y: 2.0,
        z: 3.0,
    };
    let bytes = bytemuck::bytes_of(&p);
    let back: &ManualPoint = bytemuck::from_bytes(bytes);
    assert_eq!(*back, p);

    fn _assert<T: DataPod<Header = T, Payload = ()>>() {}
    _assert::<ManualPoint>();
}

// ---------------------------------------------------------------------------
// Envelope round-trip
// ---------------------------------------------------------------------------

#[test]
fn envelope_round_trip() {
    let e = Envelope {
        seq: 42,
        stamp_ns: 1_700_000_000_000_000_000,
        source_id: [7u8; 32],
        topic_hash: 0xDEADBEEF,
        kind: 0,
        flags: 0,
    };
    let b = bytemuck::bytes_of(&e);
    let back: &Envelope = bytemuck::from_bytes(b);
    assert_eq!(back.seq, 42);
    assert_eq!(back.source_id, [7u8; 32]);
}
