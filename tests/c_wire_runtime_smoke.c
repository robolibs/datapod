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
    uint64_t pose_wire_hash = datapod_emitted_type_hash(datapod_pose_type_hash());
    DatapodOwnedBytes pose_wire = {0};
    if (!datapod_fixed_value_to_wire(
            pose_wire_hash,
            (const uint8_t *)&pose,
            sizeof(pose),
            &pose_wire)) {
        return 22;
    }
    DatapodPose pose_out = {0};
    if (!datapod_fixed_value_from_wire(
            pose_wire_hash,
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

    DatapodBytesValue *bytes_value = datapod_bytes_value_new(payload, sizeof(payload));
    if (!bytes_value) {
        return 37;
    }
    DatapodOwnedBytes bytes_wire = {0};
    if (!datapod_bytes_value_to_wire(bytes_value, &bytes_wire)) {
        return 38;
    }
    DatapodBytesView bytes_view = {0};
    if (!datapod_bytes_value_view_from_wire(bytes_wire.ptr, bytes_wire.len, &bytes_view)) {
        return 39;
    }
    if (bytes_view.payload.len != sizeof(payload)) {
        return 40;
    }
    DatapodWireMessage bytes_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_bytes_value_type_hash()),
        bytes_wire.ptr,
        bytes_wire.len);
    DatapodWireFrame bytes_frame = {0};
    if (!datapod_wire_frame_from_message(bytes_msg, &bytes_frame)) {
        return 130;
    }
    DatapodBytesView bytes_frame_view = {0};
    if (!datapod_bytes_value_view_from_frame(bytes_frame, &bytes_frame_view)) {
        return 131;
    }
    if (bytes_frame_view.payload.ptr != bytes_frame.payload ||
        bytes_frame_view.payload.len != bytes_frame.payload_len) {
        return 132;
    }
    datapod_bytes_value_free(bytes_value);
    datapod_owned_bytes_free(bytes_wire);

    const uint8_t text_payload[5] = {'h', 'e', 'l', 'l', 'o'};
    DatapodDpStr *text = datapod_dpstr_new(text_payload, sizeof(text_payload));
    if (!text) {
        return 53;
    }
    DatapodOwnedBytes text_wire = {0};
    if (!datapod_dpstr_to_wire(text, &text_wire)) {
        return 54;
    }
    DatapodDpStrView text_view = {0};
    if (!datapod_dpstr_view_from_wire(text_wire.ptr, text_wire.len, &text_view)) {
        return 55;
    }
    if (text_view.utf8.len != sizeof(text_payload) ||
        memcmp(text_view.utf8.ptr, text_payload, sizeof(text_payload)) != 0) {
        return 56;
    }
    DatapodWireMessage text_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_dpstr_type_hash()),
        text_wire.ptr,
        text_wire.len);
    DatapodWireFrame text_frame = {0};
    if (!datapod_wire_frame_from_message(text_msg, &text_frame)) {
        return 133;
    }
    DatapodDpStrView text_frame_view = {0};
    if (!datapod_dpstr_view_from_frame(text_frame, &text_frame_view)) {
        return 134;
    }
    if (text_frame_view.utf8.ptr != text_frame.payload ||
        text_frame_view.utf8.len != text_frame.payload_len) {
        return 135;
    }
    datapod_dpstr_free(text);
    datapod_owned_bytes_free(text_wire);

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
    DatapodWireMessage matrix_msg =
        datapod_wire_message_borrow(
            datapod_emitted_type_hash(datapod_matrix_type_hash()),
            matrix_wire.ptr,
            matrix_wire.len);
    if (!datapod_wire_message_validate(matrix_msg)) {
        return 31;
    }
    DatapodMatrixView matrix_view = {0};
    if (!datapod_matrix_view_from_wire(matrix_wire.ptr, matrix_wire.len, &matrix_view)) {
        return 33;
    }
    if (matrix_view.rows != 2 || matrix_view.cols != 2 ||
        matrix_view.element_size != 1 || matrix_view.payload.len != sizeof(payload)) {
        return 34;
    }
    DatapodWireFrame matrix_frame = {0};
    if (!datapod_wire_frame_from_message(matrix_msg, &matrix_frame)) {
        return 111;
    }
    if (!datapod_wire_frame_validate(matrix_frame)) {
        return 112;
    }
    if (!datapod_wire_frame_validate_as(matrix_msg.type_hash, matrix_frame)) {
        return 210;
    }
    if (datapod_wire_frame_validate_as(matrix_msg.type_hash ^ 1, matrix_frame)) {
        return 211;
    }
    if (matrix_frame.header_len != datapod_matrix_header_size() ||
        matrix_frame.payload_len != sizeof(payload)) {
        return 113;
    }
    DatapodMatrixView matrix_frame_view = {0};
    if (!datapod_matrix_view_from_frame(matrix_frame, &matrix_frame_view)) {
        return 114;
    }
    if (matrix_frame_view.payload.ptr != matrix_frame.payload ||
        matrix_frame_view.payload.len != matrix_frame.payload_len) {
        return 115;
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
    DatapodVectorView vector_view = {0};
    if (!datapod_vector_view_from_wire(vector_wire.ptr, vector_wire.len, &vector_view)) {
        return 35;
    }
    if (vector_view.element_size != 1 || vector_view.payload.len != sizeof(payload)) {
        return 36;
    }
    DatapodWireMessage vector_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_vector_type_hash()),
        vector_wire.ptr,
        vector_wire.len);
    DatapodWireFrame vector_frame = {0};
    if (!datapod_wire_frame_from_message(vector_msg, &vector_frame)) {
        return 136;
    }
    DatapodVectorView vector_frame_view = {0};
    if (!datapod_vector_view_from_frame(vector_frame, &vector_frame_view)) {
        return 137;
    }
    if (vector_frame_view.payload.ptr != vector_frame.payload ||
        vector_frame_view.payload.len != vector_frame.payload_len) {
        return 138;
    }
    if (datapod_vector_payload(vector_out).len != sizeof(payload)) {
        return 16;
    }
    datapod_vector_free(vector);
    datapod_vector_free(vector_out);
    datapod_owned_bytes_free(vector_wire);

    DatapodStack *stack = datapod_stack_from_bytes(1, payload, sizeof(payload));
    if (!stack) {
        return 85;
    }
    DatapodOwnedBytes stack_wire = {0};
    if (!datapod_stack_to_wire(stack, &stack_wire)) {
        return 86;
    }
    DatapodStackView stack_view = {0};
    if (!datapod_stack_view_from_wire(stack_wire.ptr, stack_wire.len, &stack_view)) {
        return 87;
    }
    if (stack_view.element_size != 1 || stack_view.element_count != sizeof(payload) ||
        stack_view.payload.len != sizeof(payload)) {
        return 88;
    }
    DatapodWireMessage stack_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_stack_type_hash()),
        stack_wire.ptr,
        stack_wire.len);
    DatapodWireFrame stack_frame = {0};
    if (!datapod_wire_frame_from_message(stack_msg, &stack_frame)) {
        return 145;
    }
    DatapodStackView stack_frame_view = {0};
    if (!datapod_stack_view_from_frame(stack_frame, &stack_frame_view)) {
        return 146;
    }
    if (stack_frame_view.payload.ptr != stack_frame.payload ||
        stack_frame_view.payload.len != stack_frame.payload_len) {
        return 147;
    }
    datapod_stack_free(stack);
    datapod_owned_bytes_free(stack_wire);

    DatapodQueue *queue = datapod_queue_from_bytes(1, 1, payload, sizeof(payload));
    if (!queue) {
        return 89;
    }
    DatapodOwnedBytes queue_wire = {0};
    if (!datapod_queue_to_wire(queue, &queue_wire)) {
        return 90;
    }
    DatapodQueueView queue_view = {0};
    if (!datapod_queue_view_from_wire(queue_wire.ptr, queue_wire.len, &queue_view)) {
        return 91;
    }
    if (queue_view.element_size != 1 || queue_view.front != 1 ||
        queue_view.raw_count != sizeof(payload) || queue_view.logical_count != 3) {
        return 92;
    }
    DatapodWireMessage queue_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_queue_type_hash()),
        queue_wire.ptr,
        queue_wire.len);
    DatapodWireFrame queue_frame = {0};
    if (!datapod_wire_frame_from_message(queue_msg, &queue_frame)) {
        return 148;
    }
    DatapodQueueView queue_frame_view = {0};
    if (!datapod_queue_view_from_frame(queue_frame, &queue_frame_view)) {
        return 149;
    }
    if (queue_frame_view.payload.ptr != queue_frame.payload ||
        queue_frame_view.payload.len != queue_frame.payload_len) {
        return 150;
    }
    datapod_queue_free(queue);
    datapod_owned_bytes_free(queue_wire);

    DatapodDeque *deque = datapod_deque_from_bytes(1, 2, payload, sizeof(payload));
    if (!deque) {
        return 93;
    }
    DatapodOwnedBytes deque_wire = {0};
    if (!datapod_deque_to_wire(deque, &deque_wire)) {
        return 94;
    }
    DatapodDequeView deque_view = {0};
    if (!datapod_deque_view_from_wire(deque_wire.ptr, deque_wire.len, &deque_view)) {
        return 95;
    }
    if (deque_view.element_size != 1 || deque_view.split_byte != 2 ||
        deque_view.element_count != sizeof(payload) || deque_view.front.len != 2 ||
        deque_view.back.len != 2) {
        return 96;
    }
    DatapodWireMessage deque_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_deque_type_hash()),
        deque_wire.ptr,
        deque_wire.len);
    DatapodWireFrame deque_frame = {0};
    if (!datapod_wire_frame_from_message(deque_msg, &deque_frame)) {
        return 151;
    }
    DatapodDequeView deque_frame_view = {0};
    if (!datapod_deque_view_from_frame(deque_frame, &deque_frame_view)) {
        return 152;
    }
    if (deque_frame_view.payload.ptr != deque_frame.payload ||
        deque_frame_view.payload.len != deque_frame.payload_len ||
        deque_frame_view.front.ptr != deque_frame.payload ||
        deque_frame_view.back.ptr != deque_frame.payload + 2) {
        return 153;
    }
    datapod_deque_free(deque);
    datapod_owned_bytes_free(deque_wire);

    DatapodHeap *heap = datapod_heap_from_bytes(1, false, payload, sizeof(payload));
    if (!heap) {
        return 97;
    }
    DatapodOwnedBytes heap_wire = {0};
    if (!datapod_heap_to_wire(heap, &heap_wire)) {
        return 98;
    }
    DatapodHeapView heap_view = {0};
    if (!datapod_heap_view_from_wire(heap_wire.ptr, heap_wire.len, &heap_view)) {
        return 99;
    }
    if (heap_view.element_size != 1 || heap_view.order != 0 ||
        heap_view.element_count != sizeof(payload)) {
        return 100;
    }
    DatapodWireMessage heap_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_heap_type_hash()),
        heap_wire.ptr,
        heap_wire.len);
    DatapodWireFrame heap_frame = {0};
    if (!datapod_wire_frame_from_message(heap_msg, &heap_frame)) {
        return 154;
    }
    DatapodHeapView heap_frame_view = {0};
    if (!datapod_heap_view_from_frame(heap_frame, &heap_frame_view)) {
        return 155;
    }
    if (heap_frame_view.payload.ptr != heap_frame.payload ||
        heap_frame_view.payload.len != heap_frame.payload_len) {
        return 156;
    }
    datapod_heap_free(heap);
    datapod_owned_bytes_free(heap_wire);

    uint8_t indexed_payload[18] = {
        1, 0, 0, 0, 0, 0, 0, 0, 7,
        2, 0, 0, 0, 0, 0, 0, 0, 8,
    };
    DatapodIndexedHeap *indexed_heap =
        datapod_indexed_heap_from_bytes(1, false, indexed_payload, sizeof(indexed_payload));
    if (!indexed_heap) {
        return 101;
    }
    DatapodOwnedBytes indexed_heap_wire = {0};
    if (!datapod_indexed_heap_to_wire(indexed_heap, &indexed_heap_wire)) {
        return 102;
    }
    DatapodIndexedHeapView indexed_heap_view = {0};
    if (!datapod_indexed_heap_view_from_wire(
            indexed_heap_wire.ptr, indexed_heap_wire.len, &indexed_heap_view)) {
        return 103;
    }
    if (indexed_heap_view.priority_size != 1 || indexed_heap_view.order != 0 ||
        indexed_heap_view.entry_size != 9 || indexed_heap_view.entry_count != 2) {
        return 104;
    }
    DatapodWireMessage indexed_heap_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_indexed_heap_type_hash()),
        indexed_heap_wire.ptr,
        indexed_heap_wire.len);
    DatapodWireFrame indexed_heap_frame = {0};
    if (!datapod_wire_frame_from_message(indexed_heap_msg, &indexed_heap_frame)) {
        return 157;
    }
    DatapodIndexedHeapView indexed_heap_frame_view = {0};
    if (!datapod_indexed_heap_view_from_frame(indexed_heap_frame, &indexed_heap_frame_view)) {
        return 158;
    }
    if (indexed_heap_frame_view.payload.ptr != indexed_heap_frame.payload ||
        indexed_heap_frame_view.payload.len != indexed_heap_frame.payload_len) {
        return 159;
    }
    datapod_indexed_heap_free(indexed_heap);
    datapod_owned_bytes_free(indexed_heap_wire);

    DatapodList *list = datapod_list_empty(1);
    if (!list) {
        return 105;
    }
    DatapodOwnedBytes list_wire = {0};
    if (!datapod_list_to_wire(list, &list_wire)) {
        return 106;
    }
    DatapodListView list_view = {0};
    if (!datapod_list_view_from_wire(list_wire.ptr, list_wire.len, &list_view)) {
        return 107;
    }
    if (list_view.element_size != 1 || list_view.size != 0 ||
        list_view.node_size != 9 || list_view.slot_count != 0) {
        return 108;
    }
    DatapodWireMessage list_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_list_type_hash()),
        list_wire.ptr,
        list_wire.len);
    DatapodWireFrame list_frame = {0};
    if (!datapod_wire_frame_from_message(list_msg, &list_frame)) {
        return 160;
    }
    DatapodListView list_frame_view = {0};
    if (!datapod_list_view_from_frame(list_frame, &list_frame_view)) {
        return 161;
    }
    if (list_frame_view.payload.ptr != list_frame.payload ||
        list_frame_view.payload.len != list_frame.payload_len) {
        return 162;
    }
    datapod_list_free(list);
    datapod_owned_bytes_free(list_wire);

    DatapodForwardList *forward_list = datapod_forward_list_empty(1);
    if (!forward_list) {
        return 109;
    }
    DatapodOwnedBytes forward_list_wire = {0};
    if (!datapod_forward_list_to_wire(forward_list, &forward_list_wire)) {
        return 110;
    }
    DatapodForwardListView forward_list_view = {0};
    if (!datapod_forward_list_view_from_wire(
            forward_list_wire.ptr, forward_list_wire.len, &forward_list_view)) {
        return 111;
    }
    if (forward_list_view.element_size != 1 || forward_list_view.size != 0 ||
        forward_list_view.node_size != 9 || forward_list_view.slot_count != 0) {
        return 112;
    }
    DatapodWireMessage forward_list_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_forward_list_type_hash()),
        forward_list_wire.ptr,
        forward_list_wire.len);
    DatapodWireFrame forward_list_frame = {0};
    if (!datapod_wire_frame_from_message(forward_list_msg, &forward_list_frame)) {
        return 163;
    }
    DatapodForwardListView forward_list_frame_view = {0};
    if (!datapod_forward_list_view_from_frame(forward_list_frame, &forward_list_frame_view)) {
        return 164;
    }
    if (forward_list_frame_view.payload.ptr != forward_list_frame.payload ||
        forward_list_frame_view.payload.len != forward_list_frame.payload_len) {
        return 165;
    }
    datapod_forward_list_free(forward_list);
    datapod_owned_bytes_free(forward_list_wire);

    uint8_t empty_vecvec_payload[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    DatapodPagedVecvec *paged_vecvec =
        datapod_paged_vecvec_from_bytes(1, empty_vecvec_payload, sizeof(empty_vecvec_payload));
    if (!paged_vecvec) {
        return 113;
    }
    DatapodOwnedBytes paged_vecvec_wire = {0};
    if (!datapod_paged_vecvec_to_wire(paged_vecvec, &paged_vecvec_wire)) {
        return 114;
    }
    DatapodPagedVecvecView paged_vecvec_view = {0};
    if (!datapod_paged_vecvec_view_from_wire(
            paged_vecvec_wire.ptr, paged_vecvec_wire.len, &paged_vecvec_view)) {
        return 115;
    }
    if (paged_vecvec_view.element_size != 1 || paged_vecvec_view.bucket_count != 0 ||
        paged_vecvec_view.payload.len != sizeof(empty_vecvec_payload)) {
        return 116;
    }
    DatapodWireMessage paged_vecvec_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_paged_vecvec_type_hash()),
        paged_vecvec_wire.ptr,
        paged_vecvec_wire.len);
    DatapodWireFrame paged_vecvec_frame = {0};
    if (!datapod_wire_frame_from_message(paged_vecvec_msg, &paged_vecvec_frame)) {
        return 166;
    }
    DatapodPagedVecvecView paged_vecvec_frame_view = {0};
    if (!datapod_paged_vecvec_view_from_frame(paged_vecvec_frame, &paged_vecvec_frame_view)) {
        return 167;
    }
    if (paged_vecvec_frame_view.payload.ptr != paged_vecvec_frame.payload ||
        paged_vecvec_frame_view.payload.len != paged_vecvec_frame.payload_len) {
        return 168;
    }
    datapod_paged_vecvec_free(paged_vecvec);
    datapod_owned_bytes_free(paged_vecvec_wire);

    DatapodMap *map = datapod_map_new();
    if (!map) {
        return 187;
    }
    const uint8_t map_key[1] = {9};
    const uint8_t map_value[2] = {8, 7};
    if (!datapod_map_insert(map, map_key, sizeof(map_key), map_value, sizeof(map_value))) {
        return 188;
    }
    DatapodOwnedBytes map_wire = {0};
    if (!datapod_map_to_wire(map, &map_wire)) {
        return 189;
    }
    DatapodMapView map_view = {0};
    if (!datapod_map_view_from_wire(map_wire.ptr, map_wire.len, &map_view)) {
        return 190;
    }
    if (map_view.count != 1 || map_view.payload.len == 0) {
        return 191;
    }
    DatapodWireMessage map_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_map_type_hash()),
        map_wire.ptr,
        map_wire.len);
    DatapodWireFrame map_frame = {0};
    if (!datapod_wire_frame_from_message(map_msg, &map_frame)) {
        return 192;
    }
    DatapodMapView map_frame_view = {0};
    if (!datapod_map_view_from_frame(map_frame, &map_frame_view)) {
        return 193;
    }
    if (map_frame_view.payload.ptr != map_frame.payload ||
        map_frame_view.payload.len != map_frame.payload_len) {
        return 194;
    }
    datapod_map_free(map);
    datapod_owned_bytes_free(map_wire);

    DatapodSet *set = datapod_set_new();
    if (!set) {
        return 195;
    }
    if (!datapod_set_insert(set, map_key, sizeof(map_key))) {
        return 196;
    }
    DatapodOwnedBytes set_wire = {0};
    if (!datapod_set_to_wire(set, &set_wire)) {
        return 197;
    }
    DatapodSetView set_view = {0};
    if (!datapod_set_view_from_wire(set_wire.ptr, set_wire.len, &set_view)) {
        return 198;
    }
    if (set_view.count != 1 || set_view.payload.len == 0) {
        return 199;
    }
    DatapodWireMessage set_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_set_type_hash()),
        set_wire.ptr,
        set_wire.len);
    DatapodWireFrame set_frame = {0};
    if (!datapod_wire_frame_from_message(set_msg, &set_frame)) {
        return 200;
    }
    DatapodSetView set_frame_view = {0};
    if (!datapod_set_view_from_frame(set_frame, &set_frame_view)) {
        return 201;
    }
    if (set_frame_view.payload.ptr != set_frame.payload ||
        set_frame_view.payload.len != set_frame.payload_len) {
        return 202;
    }
    datapod_set_free(set);
    datapod_owned_bytes_free(set_wire);

    DatapodVecvec *vecvec =
        datapod_vecvec_from_bytes(1, empty_vecvec_payload, sizeof(empty_vecvec_payload));
    if (!vecvec) {
        return 203;
    }
    DatapodOwnedBytes vecvec_wire = {0};
    if (!datapod_vecvec_to_wire(vecvec, &vecvec_wire)) {
        return 204;
    }
    DatapodVecvecView vecvec_view = {0};
    if (!datapod_vecvec_view_from_wire(vecvec_wire.ptr, vecvec_wire.len, &vecvec_view)) {
        return 205;
    }
    if (vecvec_view.element_size != 1 || vecvec_view.bucket_count != 0 ||
        vecvec_view.payload.len != sizeof(empty_vecvec_payload)) {
        return 206;
    }
    DatapodWireMessage vecvec_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_vecvec_type_hash()),
        vecvec_wire.ptr,
        vecvec_wire.len);
    DatapodWireFrame vecvec_frame = {0};
    if (!datapod_wire_frame_from_message(vecvec_msg, &vecvec_frame)) {
        return 207;
    }
    DatapodVecvecView vecvec_frame_view = {0};
    if (!datapod_vecvec_view_from_frame(vecvec_frame, &vecvec_frame_view)) {
        return 208;
    }
    if (vecvec_frame_view.payload.ptr != vecvec_frame.payload ||
        vecvec_frame_view.payload.len != vecvec_frame.payload_len) {
        return 209;
    }
    datapod_vecvec_free(vecvec);
    datapod_owned_bytes_free(vecvec_wire);

    DatapodTensor *tensor = datapod_tensor_from_bytes(1, 2, 2, 1, payload, sizeof(payload));
    if (!tensor) {
        return 41;
    }
    DatapodOwnedBytes tensor_wire = {0};
    if (!datapod_tensor_to_wire(tensor, &tensor_wire)) {
        return 42;
    }
    DatapodTensorView tensor_view = {0};
    if (!datapod_tensor_view_from_wire(tensor_wire.ptr, tensor_wire.len, &tensor_view)) {
        return 43;
    }
    if (tensor_view.rows != 1 || tensor_view.cols != 2 || tensor_view.layers != 2 ||
        tensor_view.element_size != 1 || tensor_view.payload.len != sizeof(payload)) {
        return 44;
    }
    DatapodWireMessage tensor_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_tensor_type_hash()),
        tensor_wire.ptr,
        tensor_wire.len);
    DatapodWireFrame tensor_frame = {0};
    if (!datapod_wire_frame_from_message(tensor_msg, &tensor_frame)) {
        return 139;
    }
    DatapodTensorView tensor_frame_view = {0};
    if (!datapod_tensor_view_from_frame(tensor_frame, &tensor_frame_view)) {
        return 140;
    }
    if (tensor_frame_view.payload.ptr != tensor_frame.payload ||
        tensor_frame_view.payload.len != tensor_frame.payload_len) {
        return 141;
    }
    datapod_tensor_free(tensor);
    datapod_owned_bytes_free(tensor_wire);

    DatapodLayer *layer = datapod_layer_new(
        1,
        2,
        2,
        0,
        false,
        1.0,
        0.5,
        pose,
        payload,
        sizeof(payload));
    if (!layer) {
        return 57;
    }
    DatapodOwnedBytes layer_wire = {0};
    if (!datapod_layer_to_wire(layer, &layer_wire)) {
        return 58;
    }
    DatapodLayerView layer_view = {0};
    if (!datapod_layer_view_from_wire(layer_wire.ptr, layer_wire.len, &layer_view)) {
        return 59;
    }
    if (layer_view.rows != 1 || layer_view.cols != 2 || layer_view.layers != 2 ||
        layer_view.encoding != 0 || layer_view.centered ||
        layer_view.data.len != sizeof(payload)) {
        return 60;
    }
    DatapodWireMessage layer_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_layer_type_hash()),
        layer_wire.ptr,
        layer_wire.len);
    DatapodWireFrame layer_frame = {0};
    if (!datapod_wire_frame_from_message(layer_msg, &layer_frame)) {
        return 142;
    }
    DatapodLayerView layer_frame_view = {0};
    if (!datapod_layer_view_from_frame(layer_frame, &layer_frame_view)) {
        return 143;
    }
    if (layer_frame_view.data.ptr != layer_frame.payload ||
        layer_frame_view.data.len != layer_frame.payload_len) {
        return 144;
    }
    datapod_layer_free(layer);
    datapod_owned_bytes_free(layer_wire);

    DatapodPoint points[3] = {
        datapod_point_new(0.0, 0.0, 0.0),
        datapod_point_new(1.0, 0.0, 0.0),
        datapod_point_new(0.0, 1.0, 0.0),
    };
    DatapodPolygon *polygon = datapod_polygon_new(points, 3);
    if (!polygon) {
        return 61;
    }
    DatapodOwnedBytes polygon_wire = {0};
    if (!datapod_polygon_to_wire(polygon, &polygon_wire)) {
        return 62;
    }
    DatapodPolygonView polygon_view = {0};
    if (!datapod_polygon_view_from_wire(polygon_wire.ptr, polygon_wire.len, &polygon_view)) {
        return 63;
    }
    if (polygon_view.vertex_count != 3 || polygon_view.point_size != sizeof(DatapodPoint) ||
        polygon_view.vertices.len != 3 * sizeof(DatapodPoint)) {
        return 64;
    }
    DatapodWireMessage polygon_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_polygon_type_hash()),
        polygon_wire.ptr,
        polygon_wire.len);
    DatapodWireFrame polygon_frame = {0};
    if (!datapod_wire_frame_from_message(polygon_msg, &polygon_frame)) {
        return 169;
    }
    DatapodPolygonView polygon_frame_view = {0};
    if (!datapod_polygon_view_from_frame(polygon_frame, &polygon_frame_view)) {
        return 170;
    }
    if (polygon_frame_view.vertices.ptr != polygon_frame.payload ||
        polygon_frame_view.vertices.len != polygon_frame.payload_len) {
        return 171;
    }
    datapod_polygon_free(polygon);
    datapod_owned_bytes_free(polygon_wire);

    DatapodLinestring *linestring = datapod_linestring_new(points, 3);
    if (!linestring) {
        return 65;
    }
    DatapodOwnedBytes linestring_wire = {0};
    if (!datapod_linestring_to_wire(linestring, &linestring_wire)) {
        return 66;
    }
    DatapodLinestringView linestring_view = {0};
    if (!datapod_linestring_view_from_wire(
            linestring_wire.ptr, linestring_wire.len, &linestring_view)) {
        return 67;
    }
    if (linestring_view.point_count != 3 ||
        linestring_view.points.len != 3 * sizeof(DatapodPoint)) {
        return 68;
    }
    DatapodWireMessage linestring_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_linestring_type_hash()),
        linestring_wire.ptr,
        linestring_wire.len);
    DatapodWireFrame linestring_frame = {0};
    if (!datapod_wire_frame_from_message(linestring_msg, &linestring_frame)) {
        return 172;
    }
    DatapodLinestringView linestring_frame_view = {0};
    if (!datapod_linestring_view_from_frame(linestring_frame, &linestring_frame_view)) {
        return 173;
    }
    if (linestring_frame_view.points.ptr != linestring_frame.payload ||
        linestring_frame_view.points.len != linestring_frame.payload_len) {
        return 174;
    }
    datapod_linestring_free(linestring);
    datapod_owned_bytes_free(linestring_wire);

    DatapodMultiPoint *multi_point = datapod_multi_point_new(points, 3);
    if (!multi_point) {
        return 69;
    }
    DatapodOwnedBytes multi_point_wire = {0};
    if (!datapod_multi_point_to_wire(multi_point, &multi_point_wire)) {
        return 70;
    }
    DatapodMultiPointView multi_point_view = {0};
    if (!datapod_multi_point_view_from_wire(
            multi_point_wire.ptr, multi_point_wire.len, &multi_point_view)) {
        return 71;
    }
    if (multi_point_view.point_count != 3 ||
        multi_point_view.points.len != 3 * sizeof(DatapodPoint)) {
        return 72;
    }
    DatapodWireMessage multi_point_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_multi_point_type_hash()),
        multi_point_wire.ptr,
        multi_point_wire.len);
    DatapodWireFrame multi_point_frame = {0};
    if (!datapod_wire_frame_from_message(multi_point_msg, &multi_point_frame)) {
        return 175;
    }
    DatapodMultiPointView multi_point_frame_view = {0};
    if (!datapod_multi_point_view_from_frame(multi_point_frame, &multi_point_frame_view)) {
        return 176;
    }
    if (multi_point_frame_view.points.ptr != multi_point_frame.payload ||
        multi_point_frame_view.points.len != multi_point_frame.payload_len) {
        return 177;
    }
    datapod_multi_point_free(multi_point);
    datapod_owned_bytes_free(multi_point_wire);

    DatapodRing *ring = datapod_ring_new(points, 3);
    if (!ring) {
        return 73;
    }
    DatapodOwnedBytes ring_wire = {0};
    if (!datapod_ring_to_wire(ring, &ring_wire)) {
        return 74;
    }
    DatapodRingView ring_view = {0};
    if (!datapod_ring_view_from_wire(ring_wire.ptr, ring_wire.len, &ring_view)) {
        return 75;
    }
    if (ring_view.point_count != 3 || ring_view.points.len != 3 * sizeof(DatapodPoint)) {
        return 76;
    }
    DatapodWireMessage ring_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_ring_type_hash()),
        ring_wire.ptr,
        ring_wire.len);
    DatapodWireFrame ring_frame = {0};
    if (!datapod_wire_frame_from_message(ring_msg, &ring_frame)) {
        return 178;
    }
    DatapodRingView ring_frame_view = {0};
    if (!datapod_ring_view_from_frame(ring_frame, &ring_frame_view)) {
        return 179;
    }
    if (ring_frame_view.points.ptr != ring_frame.payload ||
        ring_frame_view.points.len != ring_frame.payload_len) {
        return 180;
    }
    datapod_ring_free(ring);
    datapod_owned_bytes_free(ring_wire);

    DatapodPose waypoints[2] = {pose, pose};
    DatapodPath *path = datapod_path_new(waypoints, 2);
    if (!path) {
        return 77;
    }
    DatapodOwnedBytes path_wire = {0};
    if (!datapod_path_to_wire(path, &path_wire)) {
        return 78;
    }
    DatapodPathView path_view = {0};
    if (!datapod_path_view_from_wire(path_wire.ptr, path_wire.len, &path_view)) {
        return 79;
    }
    if (path_view.waypoint_count != 2 || path_view.pose_size != sizeof(DatapodPose) ||
        path_view.waypoints.len != 2 * sizeof(DatapodPose)) {
        return 80;
    }
    DatapodWireMessage path_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_path_type_hash()),
        path_wire.ptr,
        path_wire.len);
    DatapodWireFrame path_frame = {0};
    if (!datapod_wire_frame_from_message(path_msg, &path_frame)) {
        return 181;
    }
    DatapodPathView path_frame_view = {0};
    if (!datapod_path_view_from_frame(path_frame, &path_frame_view)) {
        return 182;
    }
    if (path_frame_view.waypoints.ptr != path_frame.payload ||
        path_frame_view.waypoints.len != path_frame.payload_len) {
        return 183;
    }
    datapod_path_free(path);
    datapod_owned_bytes_free(path_wire);

    DatapodState states[2] = {{pose, {1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}},
                              {pose, {7.0, 8.0, 9.0}, {10.0, 11.0, 12.0}}};
    DatapodTrajectory *trajectory = datapod_trajectory_new(states, 2);
    if (!trajectory) {
        return 81;
    }
    DatapodOwnedBytes trajectory_wire = {0};
    if (!datapod_trajectory_to_wire(trajectory, &trajectory_wire)) {
        return 82;
    }
    DatapodTrajectoryView trajectory_view = {0};
    if (!datapod_trajectory_view_from_wire(
            trajectory_wire.ptr, trajectory_wire.len, &trajectory_view)) {
        return 83;
    }
    if (trajectory_view.state_count != 2 ||
        trajectory_view.state_size != sizeof(DatapodState) ||
        trajectory_view.states.len != 2 * sizeof(DatapodState)) {
        return 84;
    }
    DatapodWireMessage trajectory_msg = datapod_wire_message_borrow(
        datapod_emitted_type_hash(datapod_trajectory_type_hash()),
        trajectory_wire.ptr,
        trajectory_wire.len);
    DatapodWireFrame trajectory_frame = {0};
    if (!datapod_wire_frame_from_message(trajectory_msg, &trajectory_frame)) {
        return 184;
    }
    DatapodTrajectoryView trajectory_frame_view = {0};
    if (!datapod_trajectory_view_from_frame(trajectory_frame, &trajectory_frame_view)) {
        return 185;
    }
    if (trajectory_frame_view.states.ptr != trajectory_frame.payload ||
        trajectory_frame_view.states.len != trajectory_frame.payload_len) {
        return 186;
    }
    datapod_trajectory_free(trajectory);
    datapod_owned_bytes_free(trajectory_wire);

    uint8_t header[256];
    if (!datapod_point_to_header_bytes(point, header, datapod_point_header_size())) {
        return 17;
    }
    DatapodOwnedBytes joined = {0};
    uint64_t point_wire_hash = datapod_emitted_type_hash(datapod_point_type_hash());
    if (!datapod_wire_message_join(
            point_wire_hash,
            header,
            datapod_point_header_size(),
            0,
            0,
            &joined)) {
        return 18;
    }
    DatapodWireMessage message =
        datapod_wire_message_borrow(point_wire_hash, joined.ptr, joined.len);
    if (!datapod_wire_message_is_valid(message)) {
        return 19;
    }
    if (datapod_wire_message_header(message).len != datapod_point_header_size()) {
        return 20;
    }
    if (datapod_wire_message_payload(message).len != 0) {
        return 21;
    }
    uint64_t point_v1_hash = datapod_type_hash_name("datapod.point.v1");
    if (datapod_header_size_v1(point_v1_hash) != datapod_point_header_size()) {
        return 49;
    }
    DatapodOwnedBytes joined_v1 = {0};
    if (!datapod_wire_message_join_v1(
            point_v1_hash,
            header,
            datapod_point_header_size(),
            0,
            0,
            &joined_v1)) {
        return 50;
    }
    DatapodWireMessage message_v1 =
        datapod_wire_message_borrow(point_v1_hash, joined_v1.ptr, joined_v1.len);
    if (!datapod_wire_message_validate_v1(message_v1)) {
        return 51;
    }
    if (!datapod_wire_message_validate_v1(message)) {
        return 52;
    }
    datapod_owned_bytes_free(joined);
    datapod_owned_bytes_free(joined_v1);

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
    if (!datapod_wire_message_validate(custom_msg)) {
        return 32;
    }
    if (datapod_header_size_v1(custom_hash) != sizeof(custom_header)) {
        return 45;
    }
    if (!datapod_wire_message_validate_v1(custom_msg)) {
        return 46;
    }
    if (datapod_wire_message_header_v1(custom_msg).len != sizeof(custom_header)) {
        return 47;
    }
    if (datapod_wire_message_payload_v1(custom_msg).len != sizeof(custom_payload)) {
        return 48;
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
