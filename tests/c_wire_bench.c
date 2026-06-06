#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "include/datapod.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const size_t PAYLOAD_SIZES[] = {64, 4 * 1024, 1024 * 1024, 64 * 1024 * 1024};

typedef struct CameraHeader {
    uint32_t width;
    uint32_t height;
    uint32_t encoding;
} CameraHeader;

typedef struct Section {
    uint32_t offset;
    uint32_t len;
} Section;

typedef struct SectionedHeader {
    uint32_t id;
    Section left;
    Section right;
} SectionedHeader;

static size_t iterations_for(size_t payload_size) {
    const char *env = getenv("DATAPOD_BENCH_ITERS");
    if (env && env[0]) {
        return (size_t)strtoull(env, 0, 10);
    }
    if (payload_size <= 64) {
        return 50000;
    }
    if (payload_size <= 4096) {
        return 20000;
    }
    if (payload_size <= 1024 * 1024) {
        return 500;
    }
    return 5;
}

static uint64_t monotonic_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void fill_payload(uint8_t *payload, size_t payload_size) {
    for (size_t i = 0; i < payload_size; ++i) {
        payload[i] = (uint8_t)(i % 251);
    }
}

static void report(
    const char *type_name,
    size_t payload_size,
    const char *operation,
    uint64_t elapsed_ns,
    size_t iterations,
    size_t copied_bytes_per_op) {
    printf(
        "language=c type=%s payload_bytes=%zu op=%s ns_per_op=%llu "
        "iterations=%zu copied_bytes_per_op=%zu\n",
        type_name,
        payload_size,
        operation,
        (unsigned long long)(elapsed_ns / iterations),
        iterations,
        copied_bytes_per_op);
}

static int bench_matrix(size_t payload_size, const uint8_t *payload, size_t iterations) {
    DatapodMatrix *matrix =
        datapod_matrix_from_bytes(1, (uint32_t)payload_size, 1, payload, payload_size);
    if (!matrix) {
        return 10;
    }

    uint64_t start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodOwnedBytes owned = {0};
        if (!datapod_matrix_to_wire(matrix, &owned)) {
            datapod_matrix_free(matrix);
            return 11;
        }
        datapod_owned_bytes_free(owned);
    }
    report(
        "matrix",
        payload_size,
        "owned_encode_to_wire_message",
        monotonic_ns() - start,
        iterations,
        datapod_matrix_header_size() + payload_size);

    DatapodArchiveFrame archive = {0};
    if (!datapod_matrix_archive(matrix, &archive)) {
        datapod_matrix_free(matrix);
        return 12;
    }

    start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodMatrix *owned = datapod_matrix_from_archive(archive);
        if (!owned) {
            datapod_matrix_free(matrix);
            return 13;
        }
        datapod_matrix_free(owned);
    }
    report(
        "matrix",
        payload_size,
        "owned_decode_from_archive",
        monotonic_ns() - start,
        iterations,
        payload_size);

    start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodMatrixView view = {0};
        if (!datapod_matrix_view_from_archive(archive, &view)) {
            datapod_matrix_free(matrix);
            return 14;
        }
        if (view.payload.ptr != archive.payload || view.payload.len != archive.payload_len) {
            datapod_matrix_free(matrix);
            return 15;
        }
    }
    report("matrix", payload_size, "archive_view", monotonic_ns() - start, iterations, 0);

    datapod_matrix_free(matrix);
    return 0;
}

static int bench_bytes(size_t payload_size, const uint8_t *payload, size_t iterations) {
    DatapodBytesValue *bytes = datapod_bytes_value_new(payload, payload_size);
    if (!bytes) {
        return 20;
    }

    uint64_t start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodOwnedBytes owned = {0};
        if (!datapod_bytes_value_to_wire(bytes, &owned)) {
            datapod_bytes_value_free(bytes);
            return 21;
        }
        datapod_owned_bytes_free(owned);
    }
    report(
        "bytes",
        payload_size,
        "owned_encode_to_wire_message",
        monotonic_ns() - start,
        iterations,
        datapod_bytes_value_header_size() + payload_size);

    DatapodArchiveFrame archive = {0};
    if (!datapod_bytes_value_archive(bytes, &archive)) {
        datapod_bytes_value_free(bytes);
        return 22;
    }

    start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodBytesValue *owned = datapod_bytes_value_from_archive(archive);
        if (!owned) {
            datapod_bytes_value_free(bytes);
            return 23;
        }
        datapod_bytes_value_free(owned);
    }
    report(
        "bytes",
        payload_size,
        "owned_decode_from_archive",
        monotonic_ns() - start,
        iterations,
        payload_size);

    start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodBytesView view = {0};
        if (!datapod_bytes_value_view_from_archive(archive, &view)) {
            datapod_bytes_value_free(bytes);
            return 24;
        }
        if (view.payload.ptr != archive.payload || view.payload.len != archive.payload_len) {
            datapod_bytes_value_free(bytes);
            return 25;
        }
    }
    report("bytes", payload_size, "archive_view", monotonic_ns() - start, iterations, 0);

    datapod_bytes_value_free(bytes);
    return 0;
}

static int bench_grid(size_t payload_size, const uint8_t *payload, size_t iterations) {
    DatapodPose pose = datapod_pose_new(datapod_point_new(0.0, 0.0, 0.0), datapod_quaternion_identity());
    DatapodGrid *grid =
        datapod_grid_new(1, (uint32_t)payload_size, 0, false, 1.0, pose, payload, payload_size);
    if (!grid) {
        return 30;
    }

    uint64_t start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodOwnedBytes owned = {0};
        if (!datapod_grid_to_wire(grid, &owned)) {
            datapod_grid_free(grid);
            return 31;
        }
        datapod_owned_bytes_free(owned);
    }
    report(
        "grid",
        payload_size,
        "owned_encode_to_wire_message",
        monotonic_ns() - start,
        iterations,
        datapod_grid_header_size() + payload_size);

    DatapodArchiveFrame archive = {0};
    if (!datapod_grid_archive(grid, &archive)) {
        datapod_grid_free(grid);
        return 32;
    }

    start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodGrid *owned = datapod_grid_from_archive(archive);
        if (!owned) {
            datapod_grid_free(grid);
            return 33;
        }
        datapod_grid_free(owned);
    }
    report(
        "grid",
        payload_size,
        "owned_decode_from_archive",
        monotonic_ns() - start,
        iterations,
        payload_size);

    start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodGridView view = {0};
        if (!datapod_grid_view_from_archive(archive, &view)) {
            datapod_grid_free(grid);
            return 34;
        }
        if (view.data.ptr != archive.payload || view.data.len != archive.payload_len) {
            datapod_grid_free(grid);
            return 35;
        }
    }
    report("grid", payload_size, "archive_view", monotonic_ns() - start, iterations, 0);

    datapod_grid_free(grid);
    return 0;
}

static int bench_runtime_camera(size_t payload_size, const uint8_t *payload, size_t iterations) {
    uint64_t type_hash = datapod_register_type_name(
        "robolibs.bench.c_camera_frame.v1", sizeof(CameraHeader), 1);
    if (type_hash == 0) {
        return 40;
    }
    CameraHeader header = {
        .width = (uint32_t)payload_size,
        .height = 1,
        .encoding = 1,
    };
    DatapodArchiveFrame archive =
        datapod_archive_frame_borrow(type_hash, (const uint8_t *)&header, sizeof(header), payload, payload_size);

    uint64_t start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodOwnedBytes owned = {0};
        if (!datapod_wire_message_join(
                type_hash,
                (const uint8_t *)&header,
                sizeof(header),
                payload,
                payload_size,
                &owned)) {
            return 41;
        }
        datapod_owned_bytes_free(owned);
    }
    report(
        "camera_frame",
        payload_size,
        "owned_encode_to_wire_message",
        monotonic_ns() - start,
        iterations,
        sizeof(header) + payload_size);

    start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodOwnedBytes owned = {0};
        if (!datapod_archive_to_message(archive, &owned)) {
            return 42;
        }
        datapod_owned_bytes_free(owned);
    }
    report(
        "camera_frame",
        payload_size,
        "owned_decode_from_archive",
        monotonic_ns() - start,
        iterations,
        payload_size);

    start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        if (!datapod_archive_validate(archive)) {
            return 43;
        }
        if (archive.payload != payload || archive.payload_len != payload_size) {
            return 44;
        }
    }
    report("camera_frame", payload_size, "archive_view", monotonic_ns() - start, iterations, 0);

    return 0;
}

static int bench_runtime_sectioned(size_t payload_size, const uint8_t *payload, size_t iterations) {
    uint64_t type_hash = datapod_register_type_name(
        "robolibs.bench.c_sectioned_frame.v1", sizeof(SectionedHeader), 1);
    if (type_hash == 0) {
        return 50;
    }
    size_t left_len = payload_size / 2;
    SectionedHeader header = {
        .id = 1,
        .left = {.offset = 0, .len = (uint32_t)left_len},
        .right = {.offset = (uint32_t)left_len, .len = (uint32_t)(payload_size - left_len)},
    };
    DatapodArchiveFrame archive =
        datapod_archive_frame_borrow(type_hash, (const uint8_t *)&header, sizeof(header), payload, payload_size);

    uint64_t start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodOwnedBytes owned = {0};
        if (!datapod_wire_message_join(
                type_hash,
                (const uint8_t *)&header,
                sizeof(header),
                payload,
                payload_size,
                &owned)) {
            return 51;
        }
        datapod_owned_bytes_free(owned);
    }
    report(
        "sectioned_frame",
        payload_size,
        "owned_join_sectioned_message",
        monotonic_ns() - start,
        iterations,
        sizeof(header) + payload_size);

    start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        if (!datapod_archive_validate(archive)) {
            return 52;
        }
        if (archive.payload != payload || archive.payload_len != payload_size) {
            return 53;
        }
    }
    report("sectioned_frame", payload_size, "archive_view", monotonic_ns() - start, iterations, 0);

    return 0;
}

static int bench_payload(size_t payload_size) {
    uint8_t *payload = (uint8_t *)malloc(payload_size);
    if (!payload) {
        return 1;
    }
    fill_payload(payload, payload_size);

    size_t iterations = iterations_for(payload_size);
    int rc = bench_bytes(payload_size, payload, iterations);
    if (rc == 0) {
        rc = bench_matrix(payload_size, payload, iterations);
    }
    if (rc == 0) {
        rc = bench_grid(payload_size, payload, iterations);
    }
    if (rc == 0) {
        rc = bench_runtime_camera(payload_size, payload, iterations);
    }
    if (rc == 0) {
        rc = bench_runtime_sectioned(payload_size, payload, iterations);
    }
    free(payload);
    return rc;
}

int main(void) {
    puts("datapod C wire benchmark: archive/view paths borrow payload pointers and copy 0 payload bytes");
    for (size_t i = 0; i < sizeof(PAYLOAD_SIZES) / sizeof(PAYLOAD_SIZES[0]); ++i) {
        int rc = bench_payload(PAYLOAD_SIZES[i]);
        if (rc != 0) {
            return rc;
        }
    }
    return 0;
}
