import os
import time

import datapod


PAYLOAD_SIZES = (64, 4 * 1024, 1024 * 1024, 64 * 1024 * 1024)


def iterations_for(payload_size):
    override = os.environ.get("DATAPOD_BENCH_ITERS")
    if override:
        return int(override)
    if payload_size <= 64:
        return 50_000
    if payload_size <= 4 * 1024:
        return 20_000
    if payload_size <= 1024 * 1024:
        return 500
    return 5


def sample_matrix(payload_size):
    return datapod.Matrix(1, payload_size, 1, bytes((i % 251 for i in range(payload_size))))


def time_it(fn, iterations):
    start = time.perf_counter_ns()
    for _ in range(iterations):
        fn()
    return time.perf_counter_ns() - start


def report(payload_size, operation, elapsed_ns, iterations, copied_bytes_per_op):
    print(
        "language=python type=matrix "
        f"payload_bytes={payload_size} op={operation} "
        f"ns_per_op={elapsed_ns // iterations} iterations={iterations} "
        f"copied_bytes_per_op={copied_bytes_per_op}"
    )


def bench_payload(payload_size):
    matrix = sample_matrix(payload_size)
    iterations = iterations_for(payload_size)

    elapsed = time_it(lambda: datapod.to_wire_message(matrix), iterations)
    _, wire = datapod.to_wire_message(matrix)
    report(
        payload_size,
        "owned_encode_to_wire_message",
        elapsed,
        iterations,
        len(wire),
    )

    matrix_hash = datapod.Matrix.TYPE_HASH
    elapsed = time_it(lambda: datapod.from_wire_message(matrix_hash, wire), iterations)
    report(
        payload_size,
        "owned_decode_from_wire_message",
        elapsed,
        iterations,
        payload_size,
    )

    frame = matrix.to_wire_frame()
    view = datapod.Matrix.view_from_wire_frame(frame)
    assert isinstance(view.payload, memoryview)
    elapsed = time_it(lambda: len(view.payload), iterations)
    report(
        payload_size,
        "memoryview_payload_access",
        elapsed,
        iterations,
        0,
    )


def main():
    print(
        "datapod Python wire benchmark: memoryview frame paths report "
        "copied_bytes_per_op=0"
    )
    for payload_size in PAYLOAD_SIZES:
        bench_payload(payload_size)


if __name__ == "__main__":
    main()
