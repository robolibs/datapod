use std::hint::black_box;
use std::time::{Duration, Instant};

use datapod::{
    DataPod, Matrix, WireMessage, access_wire_frame, from_wire_message, to_wire_message,
    with_wire_frame, with_wire_frame_slices,
};

const PAYLOAD_SIZES: &[usize] = &[64, 4 * 1024, 1024 * 1024, 64 * 1024 * 1024];

fn sample_matrix(payload_size: usize) -> Matrix {
    let data = (0..payload_size).map(|i| (i % 251) as u8).collect();
    Matrix::from_bytes::<u8>(1, payload_size as u32, data)
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
    payload_size: usize,
    operation: &str,
    duration: Duration,
    iterations: usize,
    copied_bytes_per_op: usize,
) {
    println!(
        "language={language} type=matrix payload_bytes={payload_size} op={operation} \
         ns_per_op={} iterations={iterations} copied_bytes_per_op={copied_bytes_per_op}",
        ns_per_op(duration, iterations)
    );
}

fn bench_payload(payload_size: usize) {
    let matrix = sample_matrix(payload_size);
    let header_bytes = core::mem::size_of::<datapod::MatrixHeader>();
    let joined_bytes = header_bytes + payload_size;
    let iterations = iterations_for(payload_size);

    let owned_encode = time_it(
        || {
            let msg = to_wire_message(black_box(&matrix));
            black_box(msg.bytes.len());
        },
        iterations,
    );
    report(
        "rust",
        payload_size,
        "owned_encode_to_wire_message",
        owned_encode,
        iterations,
        joined_bytes,
    );

    let wire: WireMessage = to_wire_message(&matrix);
    let owned_decode = time_it(
        || {
            let value: Matrix = from_wire_message(black_box(&wire)).expect("matrix decode");
            black_box(value.payload_bytes());
        },
        iterations,
    );
    report(
        "rust",
        payload_size,
        "owned_decode_from_wire_message",
        owned_decode,
        iterations,
        payload_size,
    );

    let frame_access = time_it(
        || {
            with_wire_frame(black_box(&matrix), |frame| {
                let view = access_wire_frame::<Matrix>(frame).expect("matrix frame access");
                black_box(view.payload_bytes().as_ptr());
                black_box(view.payload_bytes().len());
            })
            .expect("matrix frame");
        },
        iterations,
    );
    report(
        "rust",
        payload_size,
        "borrowed_frame_access",
        frame_access,
        iterations,
        0,
    );

    let scatter_gather = time_it(
        || {
            with_wire_frame(black_box(&matrix), |frame| {
                with_wire_frame_slices(frame, |slices| {
                    let total = slices.iter().map(|slice| slice.len()).sum::<usize>();
                    black_box(total);
                });
            })
            .expect("matrix frame slices");
        },
        iterations,
    );
    report(
        "rust",
        payload_size,
        "borrowed_scatter_gather_slices",
        scatter_gather,
        iterations,
        0,
    );
}

fn main() {
    println!(
        "datapod wire benchmark: owned paths report copied payload/header bytes; \
         borrowed frame paths report copied_bytes_per_op=0"
    );
    for &payload_size in PAYLOAD_SIZES {
        bench_payload(payload_size);
    }
}
