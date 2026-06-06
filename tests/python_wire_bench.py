import os
import time

import datapod


PAYLOAD_SIZES = (64, 4 * 1024, 1024 * 1024, 64 * 1024 * 1024)


@datapod.datapod_type("robolibs.bench.python_camera_frame.v1", payload=True, dataclass=True)
class CameraFrame:
    width: datapod.u32
    height: datapod.u32
    encoding: datapod.u32
    data: bytes


@datapod.datapod_type("robolibs.bench.python_sectioned_frame.v1", payload=True, dataclass=True)
class SectionedFrame:
    id: datapod.u32
    left_off: datapod.u32
    left_len: datapod.u32
    right_off: datapod.u32
    right_len: datapod.u32
    payload: bytes


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


def payload(payload_size):
    return bytes((i % 251 for i in range(payload_size)))


def sample_values(payload_size):
    data = payload(payload_size)
    left_len = payload_size // 2
    return (
        ("bytes", datapod.Bytes(data), datapod.Bytes.TYPE_HASH),
        ("matrix", datapod.Matrix(1, payload_size, 1, data), datapod.Matrix.TYPE_HASH),
        (
            "grid",
            datapod.Grid(1, payload_size, 0, False, 1.0, [0, 0, 0, 1, 0, 0, 0], data),
            datapod.Grid.TYPE_HASH,
        ),
        (
            "camera_frame",
            CameraFrame(width=payload_size, height=1, encoding=1, data=data),
            CameraFrame.TYPE_HASH,
        ),
        (
            "sectioned_frame",
            SectionedFrame(
                id=1,
                left_off=0,
                left_len=left_len,
                right_off=left_len,
                right_len=payload_size - left_len,
                payload=data,
            ),
            SectionedFrame.TYPE_HASH,
        ),
    )


def time_it(fn, iterations):
    start = time.perf_counter_ns()
    for _ in range(iterations):
        fn()
    return time.perf_counter_ns() - start


def report(type_name, payload_size, operation, elapsed_ns, iterations, copied_bytes_per_op):
    print(
        f"language=python type={type_name} "
        f"payload_bytes={payload_size} op={operation} "
        f"ns_per_op={elapsed_ns // iterations} iterations={iterations} "
        f"copied_bytes_per_op={copied_bytes_per_op}"
    )


def bench_value(type_name, value, type_hash, payload_size):
    iterations = iterations_for(payload_size)

    elapsed = time_it(lambda: value.to_wire_message(), iterations)
    _, wire = value.to_wire_message()
    report(
        type_name,
        payload_size,
        "owned_encode_to_wire_message",
        elapsed,
        iterations,
        len(wire),
    )

    elapsed = time_it(lambda: datapod.from_wire_message(type_hash, wire), iterations)
    report(
        type_name,
        payload_size,
        "owned_decode_from_wire_message",
        elapsed,
        iterations,
        payload_size,
    )

    archive = value.archive()
    assert isinstance(archive.payload, memoryview)
    elapsed = time_it(lambda: len(archive.payload), iterations)
    report(
        type_name,
        payload_size,
        "archive_payload_memoryview_access",
        elapsed,
        iterations,
        0,
    )

    if hasattr(type(value), "view_archive"):
        view = type(value).view_archive(archive)
        payload_view = getattr(view, "payload", getattr(view, "data", archive.payload))
        assert isinstance(payload_view, memoryview)
        elapsed = time_it(lambda: len(payload_view), iterations)
        report(
            type_name,
            payload_size,
            "typed_view_archive_memoryview_access",
            elapsed,
            iterations,
            0,
        )


def bench_payload(payload_size):
    for type_name, value, type_hash in sample_values(payload_size):
        bench_value(type_name, value, type_hash, payload_size)


def main():
    print(
        "datapod Python wire benchmark: archive/view memoryview paths report "
        "copied_bytes_per_op=0"
    )
    for payload_size in PAYLOAD_SIZES:
        bench_payload(payload_size)


if __name__ == "__main__":
    main()
