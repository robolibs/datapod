use std::hint::black_box;
use std::time::{Duration, Instant};

use datapod::{
    Bytes, DataPod, DataPodAccess, DataPodDecode, DataPodValidate, Encoding, Grid, LeWireHeader,
    Matrix, Pose, archive, from_archive, segmented_archive, to_wire_message, view_archive,
};

const PAYLOAD_SIZES: &[usize] = &[64, 4 * 1024, 1024 * 1024, 64 * 1024 * 1024];

#[datapod::datapod(name = "robolibs.bench.camera_frame.v1")]
pub struct BenchCameraFrame {
    pub width: u32,
    pub height: u32,
    pub encoding: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

#[datapod::datapod(name = "robolibs.bench.sectioned_frame.v1")]
pub struct BenchSectionedFrame {
    pub id: u32,
    #[dp(bytes, section = "left")]
    pub left: Vec<u8>,
    #[dp(bytes, section = "right")]
    pub right: Vec<u8>,
}

fn payload(payload_size: usize) -> Vec<u8> {
    (0..payload_size).map(|i| (i % 251) as u8).collect()
}

fn sample_bytes(payload_size: usize) -> Bytes {
    Bytes {
        data: payload(payload_size),
    }
}

fn sample_matrix(payload_size: usize) -> Matrix {
    Matrix::from_bytes::<u8>(1, payload_size as u32, payload(payload_size))
}

fn sample_grid(payload_size: usize) -> Grid {
    Grid::new(
        1,
        payload_size as u32,
        Encoding::U8,
        1.0,
        false,
        Pose::default(),
        payload(payload_size),
    )
}

fn sample_camera(payload_size: usize) -> BenchCameraFrame {
    BenchCameraFrame {
        width: payload_size as u32,
        height: 1,
        encoding: 1,
        data: payload(payload_size),
    }
}

fn sample_sectioned(payload_size: usize) -> BenchSectionedFrame {
    let left_len = payload_size / 2;
    BenchSectionedFrame {
        id: 1,
        left: payload(left_len),
        right: payload(payload_size - left_len),
    }
}

fn iterations_for(payload_size: usize) -> usize {
    if let Ok(value) = std::env::var("DATAPOD_BENCH_ITERS") {
        return value
            .parse()
            .expect("DATAPOD_BENCH_ITERS must be a positive integer");
    }
    match payload_size {
        0..=64 => 50_000,
        65..=4096 => 20_000,
        4097..=1_048_576 => 500,
        _ => 5,
    }
}

fn time_it(mut f: impl FnMut(), iterations: usize) -> Duration {
    let start = Instant::now();
    for _ in 0..iterations {
        f();
    }
    start.elapsed()
}

fn ns_per_op(duration: Duration, iterations: usize) -> u128 {
    duration.as_nanos() / iterations as u128
}

fn report(
    language: &str,
    type_name: &str,
    payload_size: usize,
    operation: &str,
    duration: Duration,
    iterations: usize,
    copied_bytes_per_op: usize,
) {
    println!(
        "language={language} type={type_name} payload_bytes={payload_size} op={operation} \
         ns_per_op={} iterations={iterations} copied_bytes_per_op={copied_bytes_per_op}",
        ns_per_op(duration, iterations)
    );
}

fn bench_archive_value<T>(type_name: &str, payload_size: usize, value: &T)
where
    T: DataPod + DataPodValidate + DataPodAccess + DataPodDecode,
    T::Header: LeWireHeader,
{
    let iterations = iterations_for(payload_size);
    let owned_encode = time_it(
        || {
            let msg = to_wire_message(black_box(value));
            black_box(msg.bytes.len());
        },
        iterations,
    );
    let msg = to_wire_message(value);
    report(
        "rust",
        type_name,
        payload_size,
        "owned_encode_to_wire_message",
        owned_encode,
        iterations,
        msg.bytes.len(),
    );

    let owned_decode = time_it(
        || {
            archive(value, |archive| {
                let owned = from_archive::<T>(black_box(archive)).expect("archive decode");
                black_box(owned.payload_len());
            })
            .expect("archive");
        },
        iterations,
    );
    report(
        "rust",
        type_name,
        payload_size,
        "owned_decode_from_archive",
        owned_decode,
        iterations,
        payload_size,
    );

    let archive_view = time_it(
        || {
            archive(black_box(value), |archive| {
                let view = view_archive::<T>(archive).expect("archive view");
                black_box(view);
            })
            .expect("archive");
        },
        iterations,
    );
    report(
        "rust",
        type_name,
        payload_size,
        "archive_view",
        archive_view,
        iterations,
        0,
    );
}

fn bench_sectioned(payload_size: usize) {
    let value = sample_sectioned(payload_size);
    let iterations = iterations_for(payload_size);
    let right_bytes: &[u8] = &value.right;
    let expected_payload_len = value.left.len() + right_bytes.len();

    let segmented_view = time_it(
        || {
            segmented_archive(black_box(&value), |archive| {
                let payload_len = archive
                    .payload_segments()
                    .iter()
                    .map(|segment| segment.len())
                    .sum::<usize>();
                black_box(payload_len);
            })
            .expect("segmented archive");
        },
        iterations,
    );
    report(
        "rust",
        "sectioned_frame",
        payload_size,
        "segmented_archive_view",
        segmented_view,
        iterations,
        0,
    );

    let owned_message = time_it(
        || {
            let msg = to_wire_message(black_box(&value));
            black_box(msg.bytes.len());
        },
        iterations,
    );
    report(
        "rust",
        "sectioned_frame",
        payload_size,
        "owned_join_sectioned_message",
        owned_message,
        iterations,
        BenchSectionedFrameHeader::LE_WIRE_SIZE + expected_payload_len,
    );
}

fn bench_payload(payload_size: usize) {
    bench_archive_value("bytes", payload_size, &sample_bytes(payload_size));
    bench_archive_value("matrix", payload_size, &sample_matrix(payload_size));
    bench_archive_value("grid", payload_size, &sample_grid(payload_size));
    bench_archive_value("camera_frame", payload_size, &sample_camera(payload_size));
    bench_sectioned(payload_size);
}

fn main() {
    println!(
        "datapod wire benchmark: archive/view paths report copied_bytes_per_op=0; \
         owned/message paths report the bytes copied into owned storage"
    );
    for &payload_size in PAYLOAD_SIZES {
        bench_payload(payload_size);
    }
}
