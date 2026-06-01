//! Smoke tests for `#[datapod]` and `#[derive(DataPod)]`.

use datapod::{DataPod, Encoding, Envelope, datapod};

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
