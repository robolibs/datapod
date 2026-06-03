#include "include/datapod.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const size_t PAYLOAD_SIZES[] = {64, 4 * 1024, 1024 * 1024, 64 * 1024 * 1024};

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

static void report(
    size_t payload_size,
    const char *operation,
    uint64_t elapsed_ns,
    size_t iterations,
    size_t copied_bytes_per_op) {
    printf(
        "language=c type=matrix payload_bytes=%zu op=%s ns_per_op=%llu "
        "iterations=%zu copied_bytes_per_op=%zu\n",
        payload_size,
        operation,
        (unsigned long long)(elapsed_ns / iterations),
        iterations,
        copied_bytes_per_op);
}

static int bench_payload(size_t payload_size) {
    uint8_t *payload = (uint8_t *)malloc(payload_size);
    if (!payload) {
        return 10;
    }
    for (size_t i = 0; i < payload_size; ++i) {
        payload[i] = (uint8_t)(i % 251);
    }

    DatapodMatrix *matrix =
        datapod_matrix_from_bytes(1, (uint32_t)payload_size, 1, payload, payload_size);
    if (!matrix) {
        free(payload);
        return 11;
    }

    DatapodOwnedBytes wire = {0};
    if (!datapod_matrix_to_wire(matrix, &wire)) {
        datapod_matrix_free(matrix);
        free(payload);
        return 12;
    }

    uint64_t matrix_hash = datapod_emitted_type_hash(datapod_matrix_type_hash());
    DatapodWireMessage message = datapod_wire_message_borrow(matrix_hash, wire.ptr, wire.len);
    DatapodWireFrame frame = {0};
    if (!datapod_wire_frame_from_message(message, &frame)) {
        datapod_owned_bytes_free(wire);
        datapod_matrix_free(matrix);
        free(payload);
        return 13;
    }

    size_t iterations = iterations_for(payload_size);
    uint64_t start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodMatrix *owned = datapod_matrix_from_wire(wire.ptr, wire.len);
        if (!owned) {
            datapod_owned_bytes_free(wire);
            datapod_matrix_free(matrix);
            free(payload);
            return 14;
        }
        datapod_matrix_free(owned);
    }
    report(
        payload_size,
        "owned_decode_from_wire_message",
        monotonic_ns() - start,
        iterations,
        payload_size);

    start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodMatrixView view = {0};
        if (!datapod_matrix_view_from_wire(wire.ptr, wire.len, &view)) {
            datapod_owned_bytes_free(wire);
            datapod_matrix_free(matrix);
            free(payload);
            return 15;
        }
    }
    report(
        payload_size,
        "joined_wire_view",
        monotonic_ns() - start,
        iterations,
        0);

    start = monotonic_ns();
    for (size_t i = 0; i < iterations; ++i) {
        DatapodMatrixView view = {0};
        if (!datapod_matrix_view_from_frame(frame, &view)) {
            datapod_owned_bytes_free(wire);
            datapod_matrix_free(matrix);
            free(payload);
            return 16;
        }
    }
    report(
        payload_size,
        "borrowed_frame_view",
        monotonic_ns() - start,
        iterations,
        0);

    datapod_owned_bytes_free(wire);
    datapod_matrix_free(matrix);
    free(payload);
    return 0;
}

int main(void) {
    puts("datapod C wire benchmark: frame views borrow payload pointers and copy 0 payload bytes");
    for (size_t i = 0; i < sizeof(PAYLOAD_SIZES) / sizeof(PAYLOAD_SIZES[0]); ++i) {
        int rc = bench_payload(PAYLOAD_SIZES[i]);
        if (rc != 0) {
            return rc;
        }
    }
    return 0;
}
