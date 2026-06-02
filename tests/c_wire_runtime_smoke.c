#include "include/datapod.h"

#include <stdint.h>
#include <string.h>

int main(void) {
    uint8_t payload[4] = {1, 2, 3, 4};

    DatapodPoint point = datapod_point_new(1.0, 2.0, 3.0);
    DatapodOwnedBytes point_wire = {0};
    if (!datapod_point_to_wire(point, &point_wire)) {
        return 1;
    }
    DatapodPoint point_out = {0};
    if (!datapod_point_from_wire(point_wire.ptr, point_wire.len, &point_out)) {
        return 2;
    }
    if (point_out.x != 1.0 || point_out.y != 2.0 || point_out.z != 3.0) {
        return 3;
    }
    datapod_owned_bytes_free(point_wire);

    DatapodPose pose = datapod_pose_new(point, datapod_quaternion_identity());
    DatapodOwnedBytes pose_wire = {0};
    if (!datapod_fixed_value_to_wire(
            datapod_pose_type_hash(),
            (const uint8_t *)&pose,
            sizeof(pose),
            &pose_wire)) {
        return 22;
    }
    DatapodPose pose_out = {0};
    if (!datapod_fixed_value_from_wire(
            datapod_pose_type_hash(),
            pose_wire.ptr,
            pose_wire.len,
            (uint8_t *)&pose_out,
            sizeof(pose_out))) {
        return 23;
    }
    if (pose_out.point.x != 1.0 || pose_out.point.y != 2.0 || pose_out.point.z != 3.0) {
        return 24;
    }
    datapod_owned_bytes_free(pose_wire);

    DatapodMatrix *matrix = datapod_matrix_from_bytes(2, 2, 1, payload, sizeof(payload));
    if (!matrix) {
        return 4;
    }
    DatapodOwnedBytes matrix_wire = {0};
    if (!datapod_matrix_to_wire(matrix, &matrix_wire)) {
        return 5;
    }
    DatapodMatrix *matrix_out = datapod_matrix_from_wire(matrix_wire.ptr, matrix_wire.len);
    if (!matrix_out) {
        return 6;
    }
    if (datapod_matrix_payload(matrix_out).len != sizeof(payload)) {
        return 7;
    }
    datapod_matrix_free(matrix);
    datapod_matrix_free(matrix_out);
    datapod_owned_bytes_free(matrix_wire);

    DatapodVector *vector = datapod_vector_from_bytes(1, payload, sizeof(payload));
    if (!vector) {
        return 13;
    }
    DatapodOwnedBytes vector_wire = {0};
    if (!datapod_vector_to_wire(vector, &vector_wire)) {
        return 14;
    }
    DatapodVector *vector_out = datapod_vector_from_wire(vector_wire.ptr, vector_wire.len);
    if (!vector_out) {
        return 15;
    }
    if (datapod_vector_payload(vector_out).len != sizeof(payload)) {
        return 16;
    }
    datapod_vector_free(vector);
    datapod_vector_free(vector_out);
    datapod_owned_bytes_free(vector_wire);

    uint8_t header[256];
    if (!datapod_point_to_header_bytes(point, header, datapod_point_header_size())) {
        return 17;
    }
    DatapodOwnedBytes joined = {0};
    if (!datapod_wire_message_join(
            datapod_point_type_hash(),
            header,
            datapod_point_header_size(),
            0,
            0,
            &joined)) {
        return 18;
    }
    DatapodWireMessage message =
        datapod_wire_message_borrow(datapod_point_type_hash(), joined.ptr, joined.len);
    if (!datapod_wire_message_is_valid(message)) {
        return 19;
    }
    if (datapod_wire_message_header(message).len != datapod_point_header_size()) {
        return 20;
    }
    if (datapod_wire_message_payload(message).len != 0) {
        return 21;
    }
    datapod_owned_bytes_free(joined);

    const char *custom_name = "acme.c_packet.v1";
    uint64_t custom_hash =
        datapod_register_type_name(custom_name, 4, datapod_payload_kind_bytes());
    if (custom_hash == 0) {
        return 25;
    }
    if (custom_hash != datapod_type_hash_name(custom_name)) {
        return 26;
    }
    uint8_t custom_header[4] = {9, 8, 7, 6};
    uint8_t custom_payload[3] = {5, 4, 3};
    DatapodOwnedBytes custom_wire = {0};
    if (!datapod_wire_message_join(
            custom_hash,
            custom_header,
            sizeof(custom_header),
            custom_payload,
            sizeof(custom_payload),
            &custom_wire)) {
        return 27;
    }
    DatapodWireMessage custom_msg =
        datapod_wire_message_borrow(custom_hash, custom_wire.ptr, custom_wire.len);
    if (!datapod_wire_message_is_valid(custom_msg)) {
        return 28;
    }
    DatapodBytes custom_header_view = datapod_wire_message_header(custom_msg);
    DatapodBytes custom_payload_view = datapod_wire_message_payload(custom_msg);
    if (custom_header_view.len != sizeof(custom_header) ||
        memcmp(custom_header_view.ptr, custom_header, sizeof(custom_header)) != 0) {
        return 29;
    }
    if (custom_payload_view.len != sizeof(custom_payload) ||
        memcmp(custom_payload_view.ptr, custom_payload, sizeof(custom_payload)) != 0) {
        return 30;
    }
    datapod_owned_bytes_free(custom_wire);

    return 0;
}
