#ifndef DATAPOD_H
#define DATAPOD_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Sentinel for "no string assigned" — use this value when a `*_id: u32`
 * field is unset.
 */
#define STRING_NONE UINT32_MAX

#define INVALID_ID UINT32_MAX

/**
 * Max number of `(key, value)` URDF property pairs per joint.
 */
#define JOINT_PROP_CAP 8

/**
 * Max number of visual IDs per link.
 */
#define LINK_VISUAL_CAP 8

/**
 * Max number of collision IDs per link.
 */
#define LINK_COLLISION_CAP 8

/**
 * Max number of `(key, value)` URDF property pairs per link.
 */
#define LINK_PROP_CAP 8

/**
 * Max number of `(key, value)` URDF property pairs on a robot.
 */
#define ROBOT_PROP_CAP 16

/**
 * Max number of `(key, value)` URDF property pairs per sensor.
 */
#define SENSOR_PROP_CAP 8

/**
 * Max number of joints attached to a single transmission.
 */
#define TRANSMISSION_JOINT_CAP 8

/**
 * Max number of actuators attached to a single transmission.
 */
#define TRANSMISSION_ACTUATOR_CAP 8



#define FORWARD_LIST_NIL UINT32_MAX

/**
 * Sentinel for "no node" — used for `head`/`tail`/`free_head`/`prev`/`next`.
 */
#define LIST_NIL UINT32_MAX



/**
 * Opaque fixed-value handle for Accel.
 */
typedef struct DatapodAccelHandle DatapodAccelHandle;

/**
 * Opaque fixed-value handle for Actuator.
 */
typedef struct DatapodActuatorHandle DatapodActuatorHandle;

/**
 * Opaque bit-vector handle.
 */
typedef struct DatapodBitVec DatapodBitVec;

/**
 * Opaque fixed-value handle for crate::Box.
 */
typedef struct DatapodBoxHandle DatapodBoxHandle;

/**
 * Opaque fixed-value handle for BoxShape.
 */
typedef struct DatapodBoxShapeHandle DatapodBoxShapeHandle;

/**
 * Opaque byte-buffer handle.
 */
typedef struct DatapodBytesValue DatapodBytesValue;

/**
 * Opaque fixed-value handle for Collision.
 */
typedef struct DatapodCollisionHandle DatapodCollisionHandle;

/**
 * Opaque fixed-value handle for CylinderShape.
 */
typedef struct DatapodCylinderShapeHandle DatapodCylinderShapeHandle;

/**
 * Opaque deque handle.
 */
typedef struct DatapodDeque DatapodDeque;

/**
 * Opaque UTF-8-ish datapod string handle (`seq::DpStr`).
 */
typedef struct DatapodDpStr DatapodDpStr;

/**
 * Opaque identifier string handle (`id::DpString`).
 */
typedef struct DatapodDpString DatapodDpString;

/**
 * Opaque fixed-value handle for Encoding.
 */
typedef struct DatapodEncodingHandle DatapodEncodingHandle;

/**
 * Opaque fixed-value handle for Envelope.
 */
typedef struct DatapodEnvelopeHandle DatapodEnvelopeHandle;

/**
 * Opaque forward-list handle.
 */
typedef struct DatapodForwardList DatapodForwardList;

/**
 * Opaque fixed-value handle for GaussianBox.
 */
typedef struct DatapodGaussianBoxHandle DatapodGaussianBoxHandle;

/**
 * Opaque fixed-value handle for GaussianCircle.
 */
typedef struct DatapodGaussianCircleHandle DatapodGaussianCircleHandle;

/**
 * Opaque fixed-value handle for GaussianPoint.
 */
typedef struct DatapodGaussianPointHandle DatapodGaussianPointHandle;

/**
 * Opaque fixed-value handle for GaussianRectangle.
 */
typedef struct DatapodGaussianRectangleHandle DatapodGaussianRectangleHandle;

/**
 * Opaque fixed-value handle for Geometry.
 */
typedef struct DatapodGeometryHandle DatapodGeometryHandle;

/**
 * Opaque fixed-value handle for GeometryKind.
 */
typedef struct DatapodGeometryKindHandle DatapodGeometryKindHandle;

/**
 * Opaque raster grid handle.
 */
typedef struct DatapodGrid DatapodGrid;

/**
 * Opaque heap handle.
 */
typedef struct DatapodHeap DatapodHeap;

/**
 * Opaque fixed-value handle for Identity.
 */
typedef struct DatapodIdentityHandle DatapodIdentityHandle;

/**
 * Opaque indexed-heap handle.
 */
typedef struct DatapodIndexedHeap DatapodIndexedHeap;

/**
 * Opaque fixed-value handle for JointCalibration.
 */
typedef struct DatapodJointCalibrationHandle DatapodJointCalibrationHandle;

/**
 * Opaque fixed-value handle for JointDynamics.
 */
typedef struct DatapodJointDynamicsHandle DatapodJointDynamicsHandle;

/**
 * Opaque fixed-value handle for Joint.
 */
typedef struct DatapodJointHandle DatapodJointHandle;

/**
 * Opaque fixed-value handle for JointMimic.
 */
typedef struct DatapodJointMimicHandle DatapodJointMimicHandle;

/**
 * Opaque fixed-value handle for JointSafetyController.
 */
typedef struct DatapodJointSafetyControllerHandle DatapodJointSafetyControllerHandle;

/**
 * Opaque fixed-value handle for JointType.
 */
typedef struct DatapodJointTypeHandle DatapodJointTypeHandle;

/**
 * Opaque fixed-value handle for KV.
 */
typedef struct DatapodKvHandle DatapodKvHandle;

/**
 * Opaque raster layer handle.
 */
typedef struct DatapodLayer DatapodLayer;

/**
 * Opaque linestring handle.
 */
typedef struct DatapodLinestring DatapodLinestring;

/**
 * Opaque fixed-value handle for Link.
 */
typedef struct DatapodLinkHandle DatapodLinkHandle;

/**
 * Opaque list handle.
 */
typedef struct DatapodList DatapodList;

/**
 * Opaque map handle.
 */
typedef struct DatapodMap DatapodMap;

/**
 * Opaque fixed-value handle for Material.
 */
typedef struct DatapodMaterialHandle DatapodMaterialHandle;

/**
 * Opaque matrix handle.
 */
typedef struct DatapodMatrix DatapodMatrix;

/**
 * Opaque fixed-value handle for MeshShape.
 */
typedef struct DatapodMeshShapeHandle DatapodMeshShapeHandle;

/**
 * Opaque fixed-value handle for Model.
 */
typedef struct DatapodModelHandle DatapodModelHandle;

/**
 * Opaque multi-point handle.
 */
typedef struct DatapodMultiPoint DatapodMultiPoint;

/**
 * Opaque fixed-value handle for Obb.
 */
typedef struct DatapodObbHandle DatapodObbHandle;

/**
 * Opaque paged vec-of-vec handle.
 */
typedef struct DatapodPagedVecvec DatapodPagedVecvec;

/**
 * Opaque path handle.
 */
typedef struct DatapodPath DatapodPath;

/**
 * Opaque fixed-value handle for PointKey.
 */
typedef struct DatapodPointKeyHandle DatapodPointKeyHandle;

/**
 * Opaque polygon handle.
 */
typedef struct DatapodPolygon DatapodPolygon;

/**
 * Opaque queue handle.
 */
typedef struct DatapodQueue DatapodQueue;

/**
 * Opaque ring handle.
 */
typedef struct DatapodRing DatapodRing;

/**
 * Opaque fixed-value handle for Robot.
 */
typedef struct DatapodRobotHandle DatapodRobotHandle;

/**
 * Opaque fixed-value handle for Sensor.
 */
typedef struct DatapodSensorHandle DatapodSensorHandle;

/**
 * Opaque set handle.
 */
typedef struct DatapodSet DatapodSet;

/**
 * Opaque fixed-value handle for Size.
 */
typedef struct DatapodSizeHandle DatapodSizeHandle;

/**
 * Opaque fixed-value handle for SphereShape.
 */
typedef struct DatapodSphereShapeHandle DatapodSphereShapeHandle;

/**
 * Opaque fixed-value handle for Square.
 */
typedef struct DatapodSquareHandle DatapodSquareHandle;

/**
 * Opaque stack handle.
 */
typedef struct DatapodStack DatapodStack;

/**
 * Opaque tensor handle.
 */
typedef struct DatapodTensor DatapodTensor;

/**
 * Opaque trajectory handle.
 */
typedef struct DatapodTrajectory DatapodTrajectory;

/**
 * Opaque fixed-value handle for Transmission.
 */
typedef struct DatapodTransmissionHandle DatapodTransmissionHandle;

/**
 * Opaque fixed-value handle for TransmissionJoint.
 */
typedef struct DatapodTransmissionJointHandle DatapodTransmissionJointHandle;

/**
 * Opaque vector handle.
 */
typedef struct DatapodVector DatapodVector;

/**
 * Opaque vec-of-vec handle.
 */
typedef struct DatapodVecvec DatapodVecvec;

/**
 * Opaque fixed-value handle for Visual.
 */
typedef struct DatapodVisualHandle DatapodVisualHandle;

typedef struct HashKind HashKind;

typedef struct WireFormat WireFormat;

/**
 * Owned bytes returned by Rust to C. Free with `datapod_owned_bytes_free`.
 */
typedef struct {
  uint8_t *ptr;
  uintptr_t len;
  uintptr_t capacity;
} DatapodOwnedBytes;

/**
 * Generic borrowed datapod wire message: `data = header || payload`.
 */
typedef struct {
  uint64_t type_hash;
  const uint8_t *data;
  uintptr_t len;
} DatapodWireMessage;

/**
 * Generic borrowed zero-copy datapod wire frame.
 *
 * This is the C ABI fast path: `header` and `payload` are separate borrowed
 * buffers and are not joined or copied by datapod.
 */
typedef struct {
  uint64_t type_hash;
  const uint8_t *header;
  uintptr_t header_len;
  const uint8_t *payload;
  uintptr_t payload_len;
} DatapodWireFrame;

/**
 * A borrowed byte view.
 */
typedef struct {
  const uint8_t *ptr;
  uintptr_t len;
} DatapodBytes;

/**
 * FFI-safe point value.
 */
typedef struct {
  double x;
  double y;
  double z;
} DatapodPoint;

/**
 * FFI-safe geodetic value.
 */
typedef struct {
  double latitude;
  double longitude;
  double altitude;
} DatapodGeo;

/**
 * FFI-safe segment value.
 */
typedef struct {
  DatapodPoint start;
  DatapodPoint end;
} DatapodSegment;

/**
 * FFI-safe Euler angle value.
 */
typedef struct {
  double roll;
  double pitch;
  double yaw;
} DatapodEuler;

/**
 * FFI-safe quaternion value.
 */
typedef struct {
  double w;
  double x;
  double y;
  double z;
} DatapodQuaternion;

/**
 * FFI-safe pose value.
 */
typedef struct {
  DatapodPoint point;
  DatapodQuaternion rotation;
} DatapodPose;

/**
 * FFI-safe velocity value.
 */
typedef struct {
  double vx;
  double vy;
  double vz;
} DatapodVelocity;

/**
 * FFI-safe acceleration value.
 */
typedef struct {
  double ax;
  double ay;
  double az;
} DatapodAcceleration;

/**
 * FFI-safe transform value.
 */
typedef struct {
  double rw;
  double rx;
  double ry;
  double rz;
  double dw;
  double dx;
  double dy;
  double dz;
} DatapodTransform;

/**
 * FFI-safe state value.
 */
typedef struct {
  DatapodPose pose;
  DatapodVelocity linear_velocity;
  DatapodVelocity angular_velocity;
} DatapodState;

/**
 * FFI-safe local/world value.
 */
typedef struct {
  DatapodPoint local;
  DatapodGeo origin;
} DatapodLoc;

/**
 * FFI-safe UTM value.
 */
typedef struct {
  int32_t zone;
  uint32_t band;
  double easting;
  double northing;
  double altitude;
} DatapodUtm;

/**
 * FFI-safe line value.
 */
typedef struct {
  DatapodPoint origin;
  DatapodPoint direction;
} DatapodLine;

/**
 * FFI-safe rectangle value.
 */
typedef struct {
  DatapodPoint top_left;
  DatapodPoint top_right;
  DatapodPoint bottom_left;
  DatapodPoint bottom_right;
} DatapodRectangle;

/**
 * FFI-safe AABB value.
 */
typedef struct {
  DatapodPoint min_point;
  DatapodPoint max_point;
} DatapodAabb;

/**
 * FFI-safe bounding sphere value.
 */
typedef struct {
  DatapodPoint center;
  double radius;
} DatapodBoundingSphere;

/**
 * FFI-safe circle value.
 */
typedef struct {
  DatapodPoint center;
  double radius;
} DatapodCircle;

/**
 * FFI-safe triangle value.
 */
typedef struct {
  DatapodPoint a;
  DatapodPoint b;
  DatapodPoint c;
} DatapodTriangle;

/**
 * FFI-safe size value.
 */
typedef struct {
  double x;
  double y;
  double z;
} DatapodSize;

/**
 * FFI-safe oriented box value.
 */
typedef struct {
  DatapodPose pose;
  DatapodSize size;
} DatapodBox3;

/**
 * FFI-safe twist value.
 */
typedef struct {
  DatapodVelocity linear;
  DatapodVelocity angular;
} DatapodTwist;

/**
 * FFI-safe wrench value.
 */
typedef struct {
  DatapodPoint force;
  DatapodPoint torque;
} DatapodWrench;

/**
 * FFI-safe odometry value.
 */
typedef struct {
  DatapodPose pose;
  DatapodTwist twist;
} DatapodOdom;

/**
 * FFI-safe joint limits value.
 */
typedef struct {
  double lower;
  double upper;
  double effort;
  double velocity;
} DatapodJointLimits;

/**
 * FFI-safe inertial value.
 */
typedef struct {
  DatapodPose origin;
  double mass;
  double ixx;
  double ixy;
  double ixz;
  double iyy;
  double iyz;
  double izz;
} DatapodInertial;

/**
 * FFI-safe UUID value.
 */
typedef struct {
  uint8_t bytes[16];
} DatapodUuid;

/**
 * FFI-safe IP value.
 */
typedef struct {
  uint32_t family;
  uint32_t _pad;
  uint8_t bytes[16];
} DatapodIp;

/**
 * FFI-safe MAC address value.
 */
typedef struct {
  uint8_t bytes[6];
  uint8_t _pad[2];
} DatapodMacAddr;

/**
 * FFI-safe map entry value used inside the raw Map payload table.
 */
typedef struct {
  uint32_t key_off;
  uint32_t key_len;
  uint32_t value_off;
  uint32_t value_len;
} DatapodMapEntry;

/**
 * FFI-safe set entry value used inside the raw Set payload table.
 */
typedef struct {
  uint32_t key_off;
  uint32_t key_len;
} DatapodSetEntry;

/**
 * Borrowed C view over a validated polygon wire message.
 */
typedef struct {
  uintptr_t vertex_count;
  uintptr_t point_size;
  DatapodBytes vertices;
} DatapodPolygonView;

/**
 * Borrowed C view over a validated Bytes wire message.
 */
typedef struct {
  DatapodBytes payload;
} DatapodBytesView;

/**
 * Borrowed C view over a validated Grid wire message.
 *
 * `data` points into the caller-owned wire bytes passed to
 * `datapod_grid_view_from_wire`; keep those bytes alive while using the view.
 */
typedef struct {
  uint32_t rows;
  uint32_t cols;
  uint32_t encoding;
  bool centered;
  double resolution;
  DatapodPose pose;
  DatapodBytes data;
} DatapodGridView;

/**
 * Borrowed C view over a validated Matrix wire message.
 *
 * `payload` points into the caller-owned wire bytes passed to
 * `datapod_matrix_view_from_wire`; keep those bytes alive while using the
 * view.
 */
typedef struct {
  uint32_t rows;
  uint32_t cols;
  uint32_t element_size;
  DatapodBytes payload;
} DatapodMatrixView;

/**
 * Borrowed C view over a validated UTF-8 DpStr wire message.
 */
typedef struct {
  DatapodBytes utf8;
} DatapodDpStrView;

/**
 * Borrowed C view over a validated point-sequence wire message.
 */
typedef struct {
  uintptr_t point_count;
  uintptr_t point_size;
  DatapodBytes points;
} DatapodLinestringView;

/**
 * Borrowed C view over a validated multi-point wire message.
 */
typedef struct {
  uintptr_t point_count;
  uintptr_t point_size;
  DatapodBytes points;
} DatapodMultiPointView;

/**
 * Borrowed C view over a validated ring wire message.
 */
typedef struct {
  uintptr_t point_count;
  uintptr_t point_size;
  DatapodBytes points;
} DatapodRingView;

/**
 * Borrowed C view over a validated path wire message.
 */
typedef struct {
  uintptr_t waypoint_count;
  uintptr_t pose_size;
  DatapodBytes waypoints;
} DatapodPathView;

/**
 * Borrowed C view over a validated trajectory wire message.
 */
typedef struct {
  uintptr_t state_count;
  uintptr_t state_size;
  DatapodBytes states;
} DatapodTrajectoryView;

/**
 * Borrowed C view over a validated Layer wire message.
 */
typedef struct {
  uint32_t rows;
  uint32_t cols;
  uint32_t layers;
  uint32_t encoding;
  bool centered;
  double resolution;
  double layer_height;
  DatapodPose pose;
  DatapodBytes data;
} DatapodLayerView;

/**
 * Borrowed C view over a validated Map wire message.
 */
typedef struct {
  uint32_t count;
  DatapodBytes entries;
  DatapodBytes blob;
  DatapodBytes payload;
} DatapodMapView;

/**
 * Borrowed C view over a validated Set wire message.
 */
typedef struct {
  uint32_t count;
  DatapodBytes entries;
  DatapodBytes blob;
  DatapodBytes payload;
} DatapodSetView;

/**
 * Borrowed C view over a validated Vector wire message.
 */
typedef struct {
  uint32_t element_size;
  DatapodBytes payload;
} DatapodVectorView;

/**
 * Borrowed C view over a validated Tensor wire message.
 */
typedef struct {
  uint32_t rows;
  uint32_t cols;
  uint32_t layers;
  uint32_t element_size;
  DatapodBytes payload;
} DatapodTensorView;

/**
 * Borrowed C view over a validated bit-packed BitVec wire message.
 */
typedef struct {
  uint64_t bits;
  DatapodBytes data;
} DatapodBitVecView;

/**
 * Borrowed C view over a validated Deque wire message.
 */
typedef struct {
  uint32_t element_size;
  uint32_t split_byte;
  uintptr_t element_count;
  DatapodBytes front;
  DatapodBytes back;
  DatapodBytes payload;
} DatapodDequeView;

/**
 * Borrowed C view over a validated Queue wire message.
 */
typedef struct {
  uint32_t element_size;
  uint32_t front;
  uintptr_t raw_count;
  uintptr_t logical_count;
  DatapodBytes payload;
} DatapodQueueView;

/**
 * Borrowed C view over a validated Stack wire message.
 */
typedef struct {
  uint32_t element_size;
  uintptr_t element_count;
  DatapodBytes payload;
} DatapodStackView;

/**
 * Borrowed C view over a validated List wire message.
 */
typedef struct {
  uint32_t head;
  uint32_t tail;
  uint32_t free_head;
  uint32_t size;
  uint32_t element_size;
  uintptr_t node_size;
  uintptr_t slot_count;
  DatapodBytes payload;
} DatapodListView;

/**
 * Borrowed C view over a validated ForwardList wire message.
 */
typedef struct {
  uint32_t head;
  uint32_t free_head;
  uint32_t size;
  uint32_t element_size;
  uintptr_t node_size;
  uintptr_t slot_count;
  DatapodBytes payload;
} DatapodForwardListView;

/**
 * Borrowed C view over a validated Heap wire message.
 */
typedef struct {
  uint32_t element_size;
  uint8_t order;
  uintptr_t element_count;
  DatapodBytes payload;
} DatapodHeapView;

/**
 * Borrowed C view over a validated IndexedHeap wire message.
 */
typedef struct {
  uint32_t priority_size;
  uint8_t order;
  uintptr_t entry_size;
  uintptr_t entry_count;
  DatapodBytes payload;
} DatapodIndexedHeapView;

/**
 * Borrowed C view over a validated ragged Vecvec wire message.
 */
typedef struct {
  uint32_t element_size;
  uint32_t bucket_count;
  DatapodBytes payload;
} DatapodVecvecView;

/**
 * Borrowed C view over a validated PagedVecvec wire message.
 */
typedef struct {
  uint32_t element_size;
  uint32_t bucket_count;
  DatapodBytes payload;
} DatapodPagedVecvecView;





#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

const char *datapod_last_error_message(void);

void datapod_owned_bytes_free(DatapodOwnedBytes bytes);

DatapodWireMessage datapod_wire_message_borrow(uint64_t type_hash,
                                               const uint8_t *data,
                                               uintptr_t len);

DatapodWireFrame datapod_wire_frame_borrow(uint64_t type_hash,
                                           const uint8_t *header,
                                           uintptr_t header_len,
                                           const uint8_t *payload,
                                           uintptr_t payload_len);

bool datapod_wire_frame_from_message_v1(DatapodWireMessage message, DatapodWireFrame *out);

bool datapod_wire_frame_from_message(DatapodWireMessage message, DatapodWireFrame *out);

bool datapod_wire_message_copy(DatapodWireMessage message, DatapodOwnedBytes *out);

bool datapod_wire_message_join(uint64_t type_hash,
                               const uint8_t *header,
                               uintptr_t header_len,
                               const uint8_t *payload,
                               uintptr_t payload_len,
                               DatapodOwnedBytes *out);

bool datapod_wire_message_join_v1(uint64_t type_hash,
                                  const uint8_t *header,
                                  uintptr_t header_len,
                                  const uint8_t *payload,
                                  uintptr_t payload_len,
                                  DatapodOwnedBytes *out);

bool datapod_wire_message_is_valid(DatapodWireMessage message);

bool datapod_wire_message_validate(DatapodWireMessage message);

bool datapod_wire_message_validate_v1(DatapodWireMessage message);

bool datapod_wire_message_is_valid_v1(DatapodWireMessage message);

bool datapod_wire_frame_validate_v1(DatapodWireFrame frame);

bool datapod_wire_frame_validate_as_v1(uint64_t expected_hash, DatapodWireFrame frame);

bool datapod_wire_frame_validate_as(uint64_t expected_hash, DatapodWireFrame frame);

bool datapod_wire_frame_validate(DatapodWireFrame frame);

bool datapod_wire_frame_is_valid_v1(DatapodWireFrame frame);

bool datapod_wire_frame_is_valid(DatapodWireFrame frame);

DatapodBytes datapod_wire_message_header(DatapodWireMessage message);

DatapodBytes datapod_wire_message_payload(DatapodWireMessage message);

DatapodBytes datapod_wire_message_header_v1(DatapodWireMessage message);

DatapodBytes datapod_wire_message_payload_v1(DatapodWireMessage message);

/**
 * Copy any fixed-size datapod C value into canonical wire bytes.
 *
 * `value` must point at the concrete C struct bytes for a registered fixed
 * datapod whose type hash is `type_hash`. The returned owned bytes are the
 * complete datapod wire body for fixed values: just the header bytes.
 */
bool datapod_fixed_value_to_wire(uint64_t type_hash,
                                 const uint8_t *value,
                                 uintptr_t value_len,
                                 DatapodOwnedBytes *out);

/**
 * Decode canonical fixed-size datapod wire bytes into a caller-owned C value.
 *
 * `out` must point at writable storage for the concrete C struct identified by
 * `type_hash`; `out_len` must be at least that type's header size.
 */
bool datapod_fixed_value_from_wire(uint64_t type_hash,
                                   const uint8_t *data,
                                   uintptr_t data_len,
                                   uint8_t *out,
                                   uintptr_t out_len);

bool datapod_type_exists(uint64_t type_hash);

uint64_t datapod_type_hash_name(const char *name);

uint64_t datapod_canonical_type_hash(uint64_t type_hash);

uint64_t datapod_emitted_type_hash(uint64_t type_hash);

uint32_t datapod_hash_kind_canonical_name(void);

const char *datapod_current_wire_format_name(void);

const char *datapod_builtin_hash_policy(void);

uint32_t datapod_payload_kind_fixed(void);

uint32_t datapod_payload_kind_bytes(void);

uint32_t datapod_endian_little(void);

uint32_t datapod_alignment_unaligned_wire(void);

uint32_t datapod_validator_registry_only(void);

uint32_t datapod_validator_builtin(void);

uint32_t datapod_validator_runtime_schema(void);

bool datapod_register_type(uint64_t type_hash,
                           const char *canonical_name,
                           uintptr_t header_size,
                           uint32_t payload_kind);

uint64_t datapod_register_type_name(const char *canonical_name,
                                    uintptr_t header_size,
                                    uint32_t payload_kind);

const char *datapod_type_name(uint64_t type_hash);

bool datapod_type_exists_name(const char *canonical_name);

uintptr_t datapod_header_size(uint64_t type_hash);

uintptr_t datapod_header_size_v1(uint64_t type_hash);

uint32_t datapod_payload_kind(uint64_t type_hash);

uint32_t datapod_format_version(uint64_t type_hash);

uint32_t datapod_wire_format(uint64_t type_hash);

uint32_t datapod_emitted_hash_kind(uint64_t type_hash);

uint32_t datapod_endian(uint64_t type_hash);

uint32_t datapod_alignment_policy(uint64_t type_hash);

uint32_t datapod_validator_kind(uint64_t type_hash);

bool datapod_point_to_wire(DatapodPoint value, DatapodOwnedBytes *out);

bool datapod_point_from_wire(const uint8_t *ptr, uintptr_t len, DatapodPoint *out);

DatapodPoint datapod_point_new(double x, double y, double z);

double datapod_point_magnitude(DatapodPoint point);

double datapod_point_distance_to(DatapodPoint a, DatapodPoint b);

double datapod_point_distance_to_2d(DatapodPoint a, DatapodPoint b);

DatapodGeo datapod_geo_new(double latitude, double longitude, double altitude);

bool datapod_geo_is_valid(DatapodGeo geo);

double datapod_geo_distance_to(DatapodGeo a, DatapodGeo b);

double datapod_geo_bearing_to(DatapodGeo a, DatapodGeo b);

DatapodSegment datapod_segment_new(DatapodPoint start, DatapodPoint end);

double datapod_segment_length(DatapodSegment segment);

DatapodPoint datapod_segment_midpoint(DatapodSegment segment);

DatapodPoint datapod_segment_closest_point(DatapodSegment segment, DatapodPoint point);

double datapod_segment_distance_to(DatapodSegment segment, DatapodPoint point);

DatapodEuler datapod_euler_new(double roll, double pitch, double yaw);

DatapodQuaternion datapod_quaternion_new(double w, double x, double y, double z);

DatapodQuaternion datapod_quaternion_identity(void);

DatapodPose datapod_pose_new(DatapodPoint point, DatapodQuaternion rotation);

DatapodVelocity datapod_velocity_new(double vx, double vy, double vz);

double datapod_velocity_speed(DatapodVelocity velocity);

DatapodAcceleration datapod_acceleration_new(double ax, double ay, double az);

double datapod_acceleration_magnitude(DatapodAcceleration acceleration);

DatapodTransform datapod_transform_new(double rw,
                                       double rx,
                                       double ry,
                                       double rz,
                                       double dw,
                                       double dx,
                                       double dy,
                                       double dz);

DatapodState datapod_state_new(DatapodPose pose,
                               DatapodVelocity linear_velocity,
                               DatapodVelocity angular_velocity);

DatapodLoc datapod_loc_new(DatapodPoint local, DatapodGeo origin);

DatapodUtm datapod_utm_new(int32_t zone,
                           uint32_t band,
                           double easting,
                           double northing,
                           double altitude);

DatapodLine datapod_line_new(DatapodPoint origin, DatapodPoint direction);

DatapodRectangle datapod_rectangle_new(DatapodPoint top_left,
                                       DatapodPoint top_right,
                                       DatapodPoint bottom_left,
                                       DatapodPoint bottom_right);

double datapod_rectangle_area(DatapodRectangle rectangle);

DatapodAabb datapod_aabb_new(DatapodPoint min_point, DatapodPoint max_point);

DatapodPoint datapod_aabb_center(DatapodAabb aabb);

DatapodBoundingSphere datapod_bounding_sphere_new(DatapodPoint center, double radius);

DatapodCircle datapod_circle_new(DatapodPoint center, double radius);

double datapod_circle_area(DatapodCircle circle);

DatapodTriangle datapod_triangle_new(DatapodPoint a, DatapodPoint b, DatapodPoint c);

double datapod_triangle_area(DatapodTriangle triangle);

DatapodSize datapod_size_new(double x, double y, double z);

DatapodBox3 datapod_box3_new(DatapodPose pose, DatapodSize size);

uint64_t datapod_size_value_type_hash(void);

uintptr_t datapod_size_value_header_size(void);

bool datapod_size_value_to_header_bytes(DatapodSize value, uint8_t *out, uintptr_t out_len);

bool datapod_size_value_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodSize *out);

uint64_t datapod_box3_type_hash(void);

uintptr_t datapod_box3_header_size(void);

bool datapod_box3_to_header_bytes(DatapodBox3 value, uint8_t *out, uintptr_t out_len);

bool datapod_box3_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodBox3 *out);

DatapodTwist datapod_twist_new(DatapodVelocity linear, DatapodVelocity angular);

DatapodWrench datapod_wrench_new(DatapodPoint force, DatapodPoint torque);

DatapodOdom datapod_odom_new(DatapodPose pose, DatapodTwist twist);

DatapodJointLimits datapod_joint_limits_new(double lower,
                                            double upper,
                                            double effort,
                                            double velocity);

DatapodInertial datapod_inertial_new(DatapodPose origin,
                                     double mass,
                                     double ixx,
                                     double ixy,
                                     double ixz,
                                     double iyy,
                                     double iyz,
                                     double izz);

DatapodUuid datapod_uuid_nil(void);

DatapodIp datapod_ip_v4(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

DatapodMacAddr datapod_mac_addr_new(const uint8_t *bytes, uintptr_t len);

uint64_t datapod_point_type_hash(void);

uintptr_t datapod_point_header_size(void);

bool datapod_point_to_header_bytes(DatapodPoint value, uint8_t *out, uintptr_t out_len);

bool datapod_point_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodPoint *out);

uint64_t datapod_geo_type_hash(void);

uintptr_t datapod_geo_header_size(void);

bool datapod_geo_to_header_bytes(DatapodGeo value, uint8_t *out, uintptr_t out_len);

bool datapod_geo_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodGeo *out);

uint64_t datapod_segment_type_hash(void);

uintptr_t datapod_segment_header_size(void);

bool datapod_segment_to_header_bytes(DatapodSegment value, uint8_t *out, uintptr_t out_len);

bool datapod_segment_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodSegment *out);

uint64_t datapod_euler_type_hash(void);

uintptr_t datapod_euler_header_size(void);

bool datapod_euler_to_header_bytes(DatapodEuler value, uint8_t *out, uintptr_t out_len);

bool datapod_euler_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodEuler *out);

uint64_t datapod_quaternion_type_hash(void);

uintptr_t datapod_quaternion_header_size(void);

bool datapod_quaternion_to_header_bytes(DatapodQuaternion value, uint8_t *out, uintptr_t out_len);

bool datapod_quaternion_from_header_bytes(const uint8_t *ptr,
                                          uintptr_t len,
                                          DatapodQuaternion *out);

uint64_t datapod_pose_type_hash(void);

uintptr_t datapod_pose_header_size(void);

bool datapod_pose_to_header_bytes(DatapodPose value, uint8_t *out, uintptr_t out_len);

bool datapod_pose_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodPose *out);

uint64_t datapod_velocity_type_hash(void);

uintptr_t datapod_velocity_header_size(void);

bool datapod_velocity_to_header_bytes(DatapodVelocity value, uint8_t *out, uintptr_t out_len);

bool datapod_velocity_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodVelocity *out);

uint64_t datapod_acceleration_type_hash(void);

uintptr_t datapod_acceleration_header_size(void);

bool datapod_acceleration_to_header_bytes(DatapodAcceleration value,
                                          uint8_t *out,
                                          uintptr_t out_len);

bool datapod_acceleration_from_header_bytes(const uint8_t *ptr,
                                            uintptr_t len,
                                            DatapodAcceleration *out);

uint64_t datapod_transform_type_hash(void);

uintptr_t datapod_transform_header_size(void);

bool datapod_transform_to_header_bytes(DatapodTransform value, uint8_t *out, uintptr_t out_len);

bool datapod_transform_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodTransform *out);

uint64_t datapod_state_type_hash(void);

uintptr_t datapod_state_header_size(void);

bool datapod_state_to_header_bytes(DatapodState value, uint8_t *out, uintptr_t out_len);

bool datapod_state_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodState *out);

uint64_t datapod_loc_type_hash(void);

uintptr_t datapod_loc_header_size(void);

bool datapod_loc_to_header_bytes(DatapodLoc value, uint8_t *out, uintptr_t out_len);

bool datapod_loc_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodLoc *out);

uint64_t datapod_utm_type_hash(void);

uintptr_t datapod_utm_header_size(void);

bool datapod_utm_to_header_bytes(DatapodUtm value, uint8_t *out, uintptr_t out_len);

bool datapod_utm_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodUtm *out);

uint64_t datapod_line_type_hash(void);

uintptr_t datapod_line_header_size(void);

bool datapod_line_to_header_bytes(DatapodLine value, uint8_t *out, uintptr_t out_len);

bool datapod_line_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodLine *out);

uint64_t datapod_rectangle_type_hash(void);

uintptr_t datapod_rectangle_header_size(void);

bool datapod_rectangle_to_header_bytes(DatapodRectangle value, uint8_t *out, uintptr_t out_len);

bool datapod_rectangle_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodRectangle *out);

uint64_t datapod_aabb_type_hash(void);

uintptr_t datapod_aabb_header_size(void);

bool datapod_aabb_to_header_bytes(DatapodAabb value, uint8_t *out, uintptr_t out_len);

bool datapod_aabb_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodAabb *out);

uint64_t datapod_bounding_sphere_type_hash(void);

uintptr_t datapod_bounding_sphere_header_size(void);

bool datapod_bounding_sphere_to_header_bytes(DatapodBoundingSphere value,
                                             uint8_t *out,
                                             uintptr_t out_len);

bool datapod_bounding_sphere_from_header_bytes(const uint8_t *ptr,
                                               uintptr_t len,
                                               DatapodBoundingSphere *out);

uint64_t datapod_circle_type_hash(void);

uintptr_t datapod_circle_header_size(void);

bool datapod_circle_to_header_bytes(DatapodCircle value, uint8_t *out, uintptr_t out_len);

bool datapod_circle_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodCircle *out);

uint64_t datapod_triangle_type_hash(void);

uintptr_t datapod_triangle_header_size(void);

bool datapod_triangle_to_header_bytes(DatapodTriangle value, uint8_t *out, uintptr_t out_len);

bool datapod_triangle_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodTriangle *out);

uint64_t datapod_twist_type_hash(void);

uintptr_t datapod_twist_header_size(void);

bool datapod_twist_to_header_bytes(DatapodTwist value, uint8_t *out, uintptr_t out_len);

bool datapod_twist_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodTwist *out);

uint64_t datapod_wrench_type_hash(void);

uintptr_t datapod_wrench_header_size(void);

bool datapod_wrench_to_header_bytes(DatapodWrench value, uint8_t *out, uintptr_t out_len);

bool datapod_wrench_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodWrench *out);

uint64_t datapod_odom_type_hash(void);

uintptr_t datapod_odom_header_size(void);

bool datapod_odom_to_header_bytes(DatapodOdom value, uint8_t *out, uintptr_t out_len);

bool datapod_odom_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodOdom *out);

uint64_t datapod_joint_limits_type_hash(void);

uintptr_t datapod_joint_limits_header_size(void);

bool datapod_joint_limits_to_header_bytes(DatapodJointLimits value,
                                          uint8_t *out,
                                          uintptr_t out_len);

bool datapod_joint_limits_from_header_bytes(const uint8_t *ptr,
                                            uintptr_t len,
                                            DatapodJointLimits *out);

uint64_t datapod_inertial_type_hash(void);

uintptr_t datapod_inertial_header_size(void);

bool datapod_inertial_to_header_bytes(DatapodInertial value, uint8_t *out, uintptr_t out_len);

bool datapod_inertial_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodInertial *out);

uint64_t datapod_uuid_type_hash(void);

uintptr_t datapod_uuid_header_size(void);

bool datapod_uuid_to_header_bytes(DatapodUuid value, uint8_t *out, uintptr_t out_len);

bool datapod_uuid_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodUuid *out);

uint64_t datapod_ip_type_hash(void);

uintptr_t datapod_ip_header_size(void);

bool datapod_ip_to_header_bytes(DatapodIp value, uint8_t *out, uintptr_t out_len);

bool datapod_ip_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodIp *out);

uint64_t datapod_mac_addr_type_hash(void);

uintptr_t datapod_mac_addr_header_size(void);

bool datapod_mac_addr_to_header_bytes(DatapodMacAddr value, uint8_t *out, uintptr_t out_len);

bool datapod_mac_addr_from_header_bytes(const uint8_t *ptr, uintptr_t len, DatapodMacAddr *out);

DatapodMapEntry datapod_map_entry_new(uint32_t key_off,
                                      uint32_t key_len,
                                      uint32_t value_off,
                                      uint32_t value_len);

uint64_t datapod_map_entry_type_hash(void);

uintptr_t datapod_map_entry_byte_size(void);

bool datapod_map_entry_to_bytes(DatapodMapEntry value, uint8_t *out, uintptr_t out_len);

bool datapod_map_entry_from_bytes(const uint8_t *ptr, uintptr_t len, DatapodMapEntry *out);

DatapodSetEntry datapod_set_entry_new(uint32_t key_off, uint32_t key_len);

uint64_t datapod_set_entry_type_hash(void);

uintptr_t datapod_set_entry_byte_size(void);

bool datapod_set_entry_to_bytes(DatapodSetEntry value, uint8_t *out, uintptr_t out_len);

bool datapod_set_entry_from_bytes(const uint8_t *ptr, uintptr_t len, DatapodSetEntry *out);

DatapodPolygon *datapod_polygon_new(const DatapodPoint *vertices, uintptr_t len);

void datapod_polygon_free(DatapodPolygon *polygon);

uintptr_t datapod_polygon_len(const DatapodPolygon *polygon);

double datapod_polygon_area(const DatapodPolygon *polygon);

double datapod_polygon_perimeter(const DatapodPolygon *polygon);

bool datapod_polygon_contains(const DatapodPolygon *polygon, DatapodPoint point);

bool datapod_polygon_vertex(const DatapodPolygon *polygon, uintptr_t index, DatapodPoint *out);

DatapodBytes datapod_polygon_vertices(const DatapodPolygon *polygon);

uint64_t datapod_polygon_type_hash(void);

uintptr_t datapod_polygon_header_size(void);

bool datapod_polygon_to_header_bytes(const DatapodPolygon *polygon,
                                     uint8_t *out,
                                     uintptr_t out_len);

DatapodBytes datapod_polygon_payload(const DatapodPolygon *polygon);

bool datapod_polygon_to_wire(const DatapodPolygon *handle, DatapodOwnedBytes *out);

DatapodPolygon *datapod_polygon_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_polygon_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodPolygonView *out);

bool datapod_polygon_view_from_frame(DatapodWireFrame frame, DatapodPolygonView *out);

DatapodBytesValue *datapod_bytes_value_new(const uint8_t *ptr, uintptr_t len);

DatapodDpStr *datapod_dpstr_new(const uint8_t *ptr, uintptr_t len);

DatapodDpString *datapod_dpstring_new(const uint8_t *ptr, uintptr_t len);

DatapodLinestring *datapod_linestring_new(const DatapodPoint *points, uintptr_t len);

DatapodMultiPoint *datapod_multi_point_new(const DatapodPoint *points, uintptr_t len);

DatapodRing *datapod_ring_new(const DatapodPoint *points, uintptr_t len);

DatapodPath *datapod_path_new(const DatapodPose *waypoints, uintptr_t len);

DatapodTrajectory *datapod_trajectory_new(const DatapodState *states, uintptr_t len);

DatapodGrid *datapod_grid_new(uint32_t rows,
                              uint32_t cols,
                              uint32_t encoding,
                              bool centered,
                              double resolution,
                              DatapodPose pose,
                              const uint8_t *data,
                              uintptr_t data_len);

DatapodLayer *datapod_layer_new(uint32_t rows,
                                uint32_t cols,
                                uint32_t layers,
                                uint32_t encoding,
                                bool centered,
                                double resolution,
                                double layer_height,
                                DatapodPose pose,
                                const uint8_t *data,
                                uintptr_t data_len);

DatapodMap *datapod_map_new(void);

bool datapod_map_insert(DatapodMap *map,
                        const uint8_t *key,
                        uintptr_t key_len,
                        const uint8_t *value,
                        uintptr_t value_len);

DatapodSet *datapod_set_new(void);

bool datapod_set_insert(DatapodSet *set, const uint8_t *key, uintptr_t key_len);

DatapodVector *datapod_vector_from_bytes(uint32_t element_size,
                                         const uint8_t *data,
                                         uintptr_t data_len);

DatapodMatrix *datapod_matrix_from_bytes(uint32_t rows,
                                         uint32_t cols,
                                         uint32_t element_size,
                                         const uint8_t *data,
                                         uintptr_t data_len);

DatapodTensor *datapod_tensor_from_bytes(uint32_t rows,
                                         uint32_t cols,
                                         uint32_t layers,
                                         uint32_t element_size,
                                         const uint8_t *data,
                                         uintptr_t data_len);

DatapodBitVec *datapod_bitvec_from_bytes(uint64_t bits, const uint8_t *data, uintptr_t data_len);

DatapodDeque *datapod_deque_from_bytes(uint32_t element_size,
                                       uint32_t split_byte,
                                       const uint8_t *data,
                                       uintptr_t data_len);

DatapodQueue *datapod_queue_from_bytes(uint32_t element_size,
                                       uint32_t front,
                                       const uint8_t *data,
                                       uintptr_t data_len);

DatapodStack *datapod_stack_from_bytes(uint32_t element_size,
                                       const uint8_t *data,
                                       uintptr_t data_len);

DatapodList *datapod_list_empty(uint32_t element_size);

DatapodForwardList *datapod_forward_list_empty(uint32_t element_size);

DatapodHeap *datapod_heap_from_bytes(uint32_t element_size,
                                     bool min_order,
                                     const uint8_t *data,
                                     uintptr_t data_len);

DatapodIndexedHeap *datapod_indexed_heap_from_bytes(uint32_t priority_size,
                                                    bool min_order,
                                                    const uint8_t *data,
                                                    uintptr_t data_len);

DatapodVecvec *datapod_vecvec_from_bytes(uint32_t element_size,
                                         const uint8_t *data,
                                         uintptr_t data_len);

DatapodPagedVecvec *datapod_paged_vecvec_from_bytes(uint32_t element_size,
                                                    const uint8_t *data,
                                                    uintptr_t data_len);

void datapod_bytes_value_free(DatapodBytesValue *handle);

uint64_t datapod_bytes_value_type_hash(void);

uintptr_t datapod_bytes_value_header_size(void);

bool datapod_bytes_value_to_header_bytes(const DatapodBytesValue *handle,
                                         uint8_t *out,
                                         uintptr_t out_len);

DatapodBytes datapod_bytes_value_payload(const DatapodBytesValue *handle);

bool datapod_bytes_value_to_wire(const DatapodBytesValue *handle, DatapodOwnedBytes *out);

DatapodBytesValue *datapod_bytes_value_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_bytes_value_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodBytesView *out);

bool datapod_bytes_value_view_from_frame(DatapodWireFrame frame, DatapodBytesView *out);

void datapod_dpstr_free(DatapodDpStr *handle);

uint64_t datapod_dpstr_type_hash(void);

uintptr_t datapod_dpstr_header_size(void);

bool datapod_dpstr_to_header_bytes(const DatapodDpStr *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_dpstr_payload(const DatapodDpStr *handle);

void datapod_dpstring_free(DatapodDpString *handle);

uint64_t datapod_dpstring_type_hash(void);

uintptr_t datapod_dpstring_header_size(void);

bool datapod_dpstring_to_header_bytes(const DatapodDpString *handle,
                                      uint8_t *out,
                                      uintptr_t out_len);

DatapodBytes datapod_dpstring_payload(const DatapodDpString *handle);

void datapod_linestring_free(DatapodLinestring *handle);

uint64_t datapod_linestring_type_hash(void);

uintptr_t datapod_linestring_header_size(void);

bool datapod_linestring_to_header_bytes(const DatapodLinestring *handle,
                                        uint8_t *out,
                                        uintptr_t out_len);

DatapodBytes datapod_linestring_payload(const DatapodLinestring *handle);

void datapod_multi_point_free(DatapodMultiPoint *handle);

uint64_t datapod_multi_point_type_hash(void);

uintptr_t datapod_multi_point_header_size(void);

bool datapod_multi_point_to_header_bytes(const DatapodMultiPoint *handle,
                                         uint8_t *out,
                                         uintptr_t out_len);

DatapodBytes datapod_multi_point_payload(const DatapodMultiPoint *handle);

void datapod_ring_free(DatapodRing *handle);

uint64_t datapod_ring_type_hash(void);

uintptr_t datapod_ring_header_size(void);

bool datapod_ring_to_header_bytes(const DatapodRing *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_ring_payload(const DatapodRing *handle);

void datapod_path_free(DatapodPath *handle);

uint64_t datapod_path_type_hash(void);

uintptr_t datapod_path_header_size(void);

bool datapod_path_to_header_bytes(const DatapodPath *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_path_payload(const DatapodPath *handle);

void datapod_trajectory_free(DatapodTrajectory *handle);

uint64_t datapod_trajectory_type_hash(void);

uintptr_t datapod_trajectory_header_size(void);

bool datapod_trajectory_to_header_bytes(const DatapodTrajectory *handle,
                                        uint8_t *out,
                                        uintptr_t out_len);

DatapodBytes datapod_trajectory_payload(const DatapodTrajectory *handle);

void datapod_grid_free(DatapodGrid *handle);

uint64_t datapod_grid_type_hash(void);

uintptr_t datapod_grid_header_size(void);

bool datapod_grid_to_header_bytes(const DatapodGrid *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_grid_payload(const DatapodGrid *handle);

bool datapod_grid_to_wire(const DatapodGrid *handle, DatapodOwnedBytes *out);

DatapodGrid *datapod_grid_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_grid_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodGridView *out);

bool datapod_grid_view_from_frame(DatapodWireFrame frame, DatapodGridView *out);

void datapod_layer_free(DatapodLayer *handle);

uint64_t datapod_layer_type_hash(void);

uintptr_t datapod_layer_header_size(void);

bool datapod_layer_to_header_bytes(const DatapodLayer *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_layer_payload(const DatapodLayer *handle);

void datapod_map_free(DatapodMap *handle);

uint64_t datapod_map_type_hash(void);

uintptr_t datapod_map_header_size(void);

bool datapod_map_to_header_bytes(const DatapodMap *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_map_payload(const DatapodMap *handle);

void datapod_set_free(DatapodSet *handle);

uint64_t datapod_set_type_hash(void);

uintptr_t datapod_set_header_size(void);

bool datapod_set_to_header_bytes(const DatapodSet *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_set_payload(const DatapodSet *handle);

void datapod_vector_free(DatapodVector *handle);

uint64_t datapod_vector_type_hash(void);

uintptr_t datapod_vector_header_size(void);

bool datapod_vector_to_header_bytes(const DatapodVector *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_vector_payload(const DatapodVector *handle);

void datapod_matrix_free(DatapodMatrix *handle);

uint64_t datapod_matrix_type_hash(void);

uintptr_t datapod_matrix_header_size(void);

bool datapod_matrix_to_header_bytes(const DatapodMatrix *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_matrix_payload(const DatapodMatrix *handle);

bool datapod_matrix_to_wire(const DatapodMatrix *handle, DatapodOwnedBytes *out);

DatapodMatrix *datapod_matrix_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_matrix_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodMatrixView *out);

bool datapod_matrix_view_from_frame(DatapodWireFrame frame, DatapodMatrixView *out);

void datapod_tensor_free(DatapodTensor *handle);

uint64_t datapod_tensor_type_hash(void);

uintptr_t datapod_tensor_header_size(void);

bool datapod_tensor_to_header_bytes(const DatapodTensor *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_tensor_payload(const DatapodTensor *handle);

void datapod_bitvec_free(DatapodBitVec *handle);

uint64_t datapod_bitvec_type_hash(void);

uintptr_t datapod_bitvec_header_size(void);

bool datapod_bitvec_to_header_bytes(const DatapodBitVec *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_bitvec_payload(const DatapodBitVec *handle);

void datapod_deque_free(DatapodDeque *handle);

uint64_t datapod_deque_type_hash(void);

uintptr_t datapod_deque_header_size(void);

bool datapod_deque_to_header_bytes(const DatapodDeque *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_deque_payload(const DatapodDeque *handle);

void datapod_queue_free(DatapodQueue *handle);

uint64_t datapod_queue_type_hash(void);

uintptr_t datapod_queue_header_size(void);

bool datapod_queue_to_header_bytes(const DatapodQueue *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_queue_payload(const DatapodQueue *handle);

void datapod_stack_free(DatapodStack *handle);

uint64_t datapod_stack_type_hash(void);

uintptr_t datapod_stack_header_size(void);

bool datapod_stack_to_header_bytes(const DatapodStack *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_stack_payload(const DatapodStack *handle);

void datapod_list_free(DatapodList *handle);

uint64_t datapod_list_type_hash(void);

uintptr_t datapod_list_header_size(void);

bool datapod_list_to_header_bytes(const DatapodList *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_list_payload(const DatapodList *handle);

void datapod_forward_list_free(DatapodForwardList *handle);

uint64_t datapod_forward_list_type_hash(void);

uintptr_t datapod_forward_list_header_size(void);

bool datapod_forward_list_to_header_bytes(const DatapodForwardList *handle,
                                          uint8_t *out,
                                          uintptr_t out_len);

DatapodBytes datapod_forward_list_payload(const DatapodForwardList *handle);

void datapod_heap_free(DatapodHeap *handle);

uint64_t datapod_heap_type_hash(void);

uintptr_t datapod_heap_header_size(void);

bool datapod_heap_to_header_bytes(const DatapodHeap *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_heap_payload(const DatapodHeap *handle);

void datapod_indexed_heap_free(DatapodIndexedHeap *handle);

uint64_t datapod_indexed_heap_type_hash(void);

uintptr_t datapod_indexed_heap_header_size(void);

bool datapod_indexed_heap_to_header_bytes(const DatapodIndexedHeap *handle,
                                          uint8_t *out,
                                          uintptr_t out_len);

DatapodBytes datapod_indexed_heap_payload(const DatapodIndexedHeap *handle);

void datapod_vecvec_free(DatapodVecvec *handle);

uint64_t datapod_vecvec_type_hash(void);

uintptr_t datapod_vecvec_header_size(void);

bool datapod_vecvec_to_header_bytes(const DatapodVecvec *handle, uint8_t *out, uintptr_t out_len);

DatapodBytes datapod_vecvec_payload(const DatapodVecvec *handle);

void datapod_paged_vecvec_free(DatapodPagedVecvec *handle);

uint64_t datapod_paged_vecvec_type_hash(void);

uintptr_t datapod_paged_vecvec_header_size(void);

bool datapod_paged_vecvec_to_header_bytes(const DatapodPagedVecvec *handle,
                                          uint8_t *out,
                                          uintptr_t out_len);

DatapodBytes datapod_paged_vecvec_payload(const DatapodPagedVecvec *handle);

bool datapod_dpstr_to_wire(const DatapodDpStr *handle, DatapodOwnedBytes *out);

DatapodDpStr *datapod_dpstr_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_dpstr_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodDpStrView *out);

bool datapod_dpstr_view_from_frame(DatapodWireFrame frame, DatapodDpStrView *out);

bool datapod_dpstring_to_wire(const DatapodDpString *handle, DatapodOwnedBytes *out);

DatapodDpString *datapod_dpstring_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_linestring_to_wire(const DatapodLinestring *handle, DatapodOwnedBytes *out);

DatapodLinestring *datapod_linestring_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_linestring_view_from_wire(const uint8_t *ptr,
                                       uintptr_t len,
                                       DatapodLinestringView *out);

bool datapod_linestring_view_from_frame(DatapodWireFrame frame, DatapodLinestringView *out);

bool datapod_multi_point_to_wire(const DatapodMultiPoint *handle, DatapodOwnedBytes *out);

DatapodMultiPoint *datapod_multi_point_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_multi_point_view_from_wire(const uint8_t *ptr,
                                        uintptr_t len,
                                        DatapodMultiPointView *out);

bool datapod_multi_point_view_from_frame(DatapodWireFrame frame, DatapodMultiPointView *out);

bool datapod_ring_to_wire(const DatapodRing *handle, DatapodOwnedBytes *out);

DatapodRing *datapod_ring_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_ring_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodRingView *out);

bool datapod_ring_view_from_frame(DatapodWireFrame frame, DatapodRingView *out);

bool datapod_path_to_wire(const DatapodPath *handle, DatapodOwnedBytes *out);

DatapodPath *datapod_path_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_path_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodPathView *out);

bool datapod_path_view_from_frame(DatapodWireFrame frame, DatapodPathView *out);

bool datapod_trajectory_to_wire(const DatapodTrajectory *handle, DatapodOwnedBytes *out);

DatapodTrajectory *datapod_trajectory_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_trajectory_view_from_wire(const uint8_t *ptr,
                                       uintptr_t len,
                                       DatapodTrajectoryView *out);

bool datapod_trajectory_view_from_frame(DatapodWireFrame frame, DatapodTrajectoryView *out);

bool datapod_layer_to_wire(const DatapodLayer *handle, DatapodOwnedBytes *out);

DatapodLayer *datapod_layer_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_layer_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodLayerView *out);

bool datapod_layer_view_from_frame(DatapodWireFrame frame, DatapodLayerView *out);

bool datapod_map_to_wire(const DatapodMap *handle, DatapodOwnedBytes *out);

DatapodMap *datapod_map_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_map_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodMapView *out);

bool datapod_map_view_from_frame(DatapodWireFrame frame, DatapodMapView *out);

bool datapod_set_to_wire(const DatapodSet *handle, DatapodOwnedBytes *out);

DatapodSet *datapod_set_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_set_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodSetView *out);

bool datapod_set_view_from_frame(DatapodWireFrame frame, DatapodSetView *out);

bool datapod_vector_to_wire(const DatapodVector *handle, DatapodOwnedBytes *out);

DatapodVector *datapod_vector_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_vector_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodVectorView *out);

bool datapod_vector_view_from_frame(DatapodWireFrame frame, DatapodVectorView *out);

bool datapod_tensor_to_wire(const DatapodTensor *handle, DatapodOwnedBytes *out);

DatapodTensor *datapod_tensor_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_tensor_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodTensorView *out);

bool datapod_tensor_view_from_frame(DatapodWireFrame frame, DatapodTensorView *out);

bool datapod_bitvec_to_wire(const DatapodBitVec *handle, DatapodOwnedBytes *out);

DatapodBitVec *datapod_bitvec_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_bitvec_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodBitVecView *out);

bool datapod_bitvec_view_from_frame(DatapodWireFrame frame, DatapodBitVecView *out);

bool datapod_deque_to_wire(const DatapodDeque *handle, DatapodOwnedBytes *out);

DatapodDeque *datapod_deque_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_deque_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodDequeView *out);

bool datapod_deque_view_from_frame(DatapodWireFrame frame, DatapodDequeView *out);

bool datapod_queue_to_wire(const DatapodQueue *handle, DatapodOwnedBytes *out);

DatapodQueue *datapod_queue_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_queue_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodQueueView *out);

bool datapod_queue_view_from_frame(DatapodWireFrame frame, DatapodQueueView *out);

bool datapod_stack_to_wire(const DatapodStack *handle, DatapodOwnedBytes *out);

DatapodStack *datapod_stack_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_stack_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodStackView *out);

bool datapod_stack_view_from_frame(DatapodWireFrame frame, DatapodStackView *out);

bool datapod_list_to_wire(const DatapodList *handle, DatapodOwnedBytes *out);

DatapodList *datapod_list_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_list_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodListView *out);

bool datapod_list_view_from_frame(DatapodWireFrame frame, DatapodListView *out);

bool datapod_forward_list_to_wire(const DatapodForwardList *handle, DatapodOwnedBytes *out);

DatapodForwardList *datapod_forward_list_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_forward_list_view_from_wire(const uint8_t *ptr,
                                         uintptr_t len,
                                         DatapodForwardListView *out);

bool datapod_forward_list_view_from_frame(DatapodWireFrame frame, DatapodForwardListView *out);

bool datapod_heap_to_wire(const DatapodHeap *handle, DatapodOwnedBytes *out);

DatapodHeap *datapod_heap_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_heap_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodHeapView *out);

bool datapod_heap_view_from_frame(DatapodWireFrame frame, DatapodHeapView *out);

bool datapod_indexed_heap_to_wire(const DatapodIndexedHeap *handle, DatapodOwnedBytes *out);

DatapodIndexedHeap *datapod_indexed_heap_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_indexed_heap_view_from_wire(const uint8_t *ptr,
                                         uintptr_t len,
                                         DatapodIndexedHeapView *out);

bool datapod_indexed_heap_view_from_frame(DatapodWireFrame frame, DatapodIndexedHeapView *out);

bool datapod_vecvec_to_wire(const DatapodVecvec *handle, DatapodOwnedBytes *out);

DatapodVecvec *datapod_vecvec_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_vecvec_view_from_wire(const uint8_t *ptr, uintptr_t len, DatapodVecvecView *out);

bool datapod_vecvec_view_from_frame(DatapodWireFrame frame, DatapodVecvecView *out);

bool datapod_paged_vecvec_to_wire(const DatapodPagedVecvec *handle, DatapodOwnedBytes *out);

DatapodPagedVecvec *datapod_paged_vecvec_from_wire(const uint8_t *ptr, uintptr_t len);

bool datapod_paged_vecvec_view_from_wire(const uint8_t *ptr,
                                         uintptr_t len,
                                         DatapodPagedVecvecView *out);

bool datapod_paged_vecvec_view_from_frame(DatapodWireFrame frame, DatapodPagedVecvecView *out);

DatapodPointKeyHandle *datapod_point_key_new_default(void);

void datapod_point_key_free(DatapodPointKeyHandle *handle);

uint64_t datapod_point_key_type_hash(void);

uintptr_t datapod_point_key_header_size(void);

bool datapod_point_key_to_header_bytes(const DatapodPointKeyHandle *handle,
                                       uint8_t *out,
                                       uintptr_t out_len);

DatapodPointKeyHandle *datapod_point_key_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodSizeHandle *datapod_size_new_default(void);

void datapod_size_free(DatapodSizeHandle *handle);

uint64_t datapod_size_type_hash(void);

uintptr_t datapod_size_header_size(void);

bool datapod_size_to_header_bytes(const DatapodSizeHandle *handle, uint8_t *out, uintptr_t out_len);

DatapodSizeHandle *datapod_size_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodSquareHandle *datapod_square_new_default(void);

void datapod_square_free(DatapodSquareHandle *handle);

uint64_t datapod_square_type_hash(void);

uintptr_t datapod_square_header_size(void);

bool datapod_square_to_header_bytes(const DatapodSquareHandle *handle,
                                    uint8_t *out,
                                    uintptr_t out_len);

DatapodSquareHandle *datapod_square_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodObbHandle *datapod_obb_new_default(void);

void datapod_obb_free(DatapodObbHandle *handle);

uint64_t datapod_obb_type_hash(void);

uintptr_t datapod_obb_header_size(void);

bool datapod_obb_to_header_bytes(const DatapodObbHandle *handle, uint8_t *out, uintptr_t out_len);

DatapodObbHandle *datapod_obb_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodBoxHandle *datapod_box_new_default(void);

void datapod_box_free(DatapodBoxHandle *handle);

uint64_t datapod_box_type_hash(void);

uintptr_t datapod_box_header_size(void);

bool datapod_box_to_header_bytes(const DatapodBoxHandle *handle, uint8_t *out, uintptr_t out_len);

DatapodBoxHandle *datapod_box_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodGaussianPointHandle *datapod_gaussian_point_new_default(void);

void datapod_gaussian_point_free(DatapodGaussianPointHandle *handle);

uint64_t datapod_gaussian_point_type_hash(void);

uintptr_t datapod_gaussian_point_header_size(void);

bool datapod_gaussian_point_to_header_bytes(const DatapodGaussianPointHandle *handle,
                                            uint8_t *out,
                                            uintptr_t out_len);

DatapodGaussianPointHandle *datapod_gaussian_point_from_header_bytes(const uint8_t *ptr,
                                                                     uintptr_t len);

DatapodGaussianCircleHandle *datapod_gaussian_circle_new_default(void);

void datapod_gaussian_circle_free(DatapodGaussianCircleHandle *handle);

uint64_t datapod_gaussian_circle_type_hash(void);

uintptr_t datapod_gaussian_circle_header_size(void);

bool datapod_gaussian_circle_to_header_bytes(const DatapodGaussianCircleHandle *handle,
                                             uint8_t *out,
                                             uintptr_t out_len);

DatapodGaussianCircleHandle *datapod_gaussian_circle_from_header_bytes(const uint8_t *ptr,
                                                                       uintptr_t len);

DatapodGaussianRectangleHandle *datapod_gaussian_rectangle_new_default(void);

void datapod_gaussian_rectangle_free(DatapodGaussianRectangleHandle *handle);

uint64_t datapod_gaussian_rectangle_type_hash(void);

uintptr_t datapod_gaussian_rectangle_header_size(void);

bool datapod_gaussian_rectangle_to_header_bytes(const DatapodGaussianRectangleHandle *handle,
                                                uint8_t *out,
                                                uintptr_t out_len);

DatapodGaussianRectangleHandle *datapod_gaussian_rectangle_from_header_bytes(const uint8_t *ptr,
                                                                             uintptr_t len);

DatapodGaussianBoxHandle *datapod_gaussian_box_new_default(void);

void datapod_gaussian_box_free(DatapodGaussianBoxHandle *handle);

uint64_t datapod_gaussian_box_type_hash(void);

uintptr_t datapod_gaussian_box_header_size(void);

bool datapod_gaussian_box_to_header_bytes(const DatapodGaussianBoxHandle *handle,
                                          uint8_t *out,
                                          uintptr_t out_len);

DatapodGaussianBoxHandle *datapod_gaussian_box_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodAccelHandle *datapod_accel_new_default(void);

void datapod_accel_free(DatapodAccelHandle *handle);

uint64_t datapod_accel_type_hash(void);

uintptr_t datapod_accel_header_size(void);

bool datapod_accel_to_header_bytes(const DatapodAccelHandle *handle,
                                   uint8_t *out,
                                   uintptr_t out_len);

DatapodAccelHandle *datapod_accel_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodJointDynamicsHandle *datapod_joint_dynamics_new_default(void);

void datapod_joint_dynamics_free(DatapodJointDynamicsHandle *handle);

uint64_t datapod_joint_dynamics_type_hash(void);

uintptr_t datapod_joint_dynamics_header_size(void);

bool datapod_joint_dynamics_to_header_bytes(const DatapodJointDynamicsHandle *handle,
                                            uint8_t *out,
                                            uintptr_t out_len);

DatapodJointDynamicsHandle *datapod_joint_dynamics_from_header_bytes(const uint8_t *ptr,
                                                                     uintptr_t len);

DatapodJointMimicHandle *datapod_joint_mimic_new_default(void);

void datapod_joint_mimic_free(DatapodJointMimicHandle *handle);

uint64_t datapod_joint_mimic_type_hash(void);

uintptr_t datapod_joint_mimic_header_size(void);

bool datapod_joint_mimic_to_header_bytes(const DatapodJointMimicHandle *handle,
                                         uint8_t *out,
                                         uintptr_t out_len);

DatapodJointMimicHandle *datapod_joint_mimic_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodJointSafetyControllerHandle *datapod_joint_safety_controller_new_default(void);

void datapod_joint_safety_controller_free(DatapodJointSafetyControllerHandle *handle);

uint64_t datapod_joint_safety_controller_type_hash(void);

uintptr_t datapod_joint_safety_controller_header_size(void);

bool datapod_joint_safety_controller_to_header_bytes(const DatapodJointSafetyControllerHandle *handle,
                                                     uint8_t *out,
                                                     uintptr_t out_len);

DatapodJointSafetyControllerHandle *datapod_joint_safety_controller_from_header_bytes(const uint8_t *ptr,
                                                                                      uintptr_t len);

DatapodJointCalibrationHandle *datapod_joint_calibration_new_default(void);

void datapod_joint_calibration_free(DatapodJointCalibrationHandle *handle);

uint64_t datapod_joint_calibration_type_hash(void);

uintptr_t datapod_joint_calibration_header_size(void);

bool datapod_joint_calibration_to_header_bytes(const DatapodJointCalibrationHandle *handle,
                                               uint8_t *out,
                                               uintptr_t out_len);

DatapodJointCalibrationHandle *datapod_joint_calibration_from_header_bytes(const uint8_t *ptr,
                                                                           uintptr_t len);

DatapodKvHandle *datapod_kv_new_default(void);

void datapod_kv_free(DatapodKvHandle *handle);

uint64_t datapod_kv_type_hash(void);

uintptr_t datapod_kv_header_size(void);

bool datapod_kv_to_header_bytes(const DatapodKvHandle *handle, uint8_t *out, uintptr_t out_len);

DatapodKvHandle *datapod_kv_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodBoxShapeHandle *datapod_box_shape_new_default(void);

void datapod_box_shape_free(DatapodBoxShapeHandle *handle);

uint64_t datapod_box_shape_type_hash(void);

uintptr_t datapod_box_shape_header_size(void);

bool datapod_box_shape_to_header_bytes(const DatapodBoxShapeHandle *handle,
                                       uint8_t *out,
                                       uintptr_t out_len);

DatapodBoxShapeHandle *datapod_box_shape_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodSphereShapeHandle *datapod_sphere_shape_new_default(void);

void datapod_sphere_shape_free(DatapodSphereShapeHandle *handle);

uint64_t datapod_sphere_shape_type_hash(void);

uintptr_t datapod_sphere_shape_header_size(void);

bool datapod_sphere_shape_to_header_bytes(const DatapodSphereShapeHandle *handle,
                                          uint8_t *out,
                                          uintptr_t out_len);

DatapodSphereShapeHandle *datapod_sphere_shape_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodCylinderShapeHandle *datapod_cylinder_shape_new_default(void);

void datapod_cylinder_shape_free(DatapodCylinderShapeHandle *handle);

uint64_t datapod_cylinder_shape_type_hash(void);

uintptr_t datapod_cylinder_shape_header_size(void);

bool datapod_cylinder_shape_to_header_bytes(const DatapodCylinderShapeHandle *handle,
                                            uint8_t *out,
                                            uintptr_t out_len);

DatapodCylinderShapeHandle *datapod_cylinder_shape_from_header_bytes(const uint8_t *ptr,
                                                                     uintptr_t len);

DatapodMeshShapeHandle *datapod_mesh_shape_new_default(void);

void datapod_mesh_shape_free(DatapodMeshShapeHandle *handle);

uint64_t datapod_mesh_shape_type_hash(void);

uintptr_t datapod_mesh_shape_header_size(void);

bool datapod_mesh_shape_to_header_bytes(const DatapodMeshShapeHandle *handle,
                                        uint8_t *out,
                                        uintptr_t out_len);

DatapodMeshShapeHandle *datapod_mesh_shape_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodGeometryKindHandle *datapod_geometry_kind_new_default(void);

void datapod_geometry_kind_free(DatapodGeometryKindHandle *handle);

uint64_t datapod_geometry_kind_type_hash(void);

uintptr_t datapod_geometry_kind_header_size(void);

bool datapod_geometry_kind_to_header_bytes(const DatapodGeometryKindHandle *handle,
                                           uint8_t *out,
                                           uintptr_t out_len);

DatapodGeometryKindHandle *datapod_geometry_kind_from_header_bytes(const uint8_t *ptr,
                                                                   uintptr_t len);

DatapodGeometryHandle *datapod_geometry_new_default(void);

void datapod_geometry_free(DatapodGeometryHandle *handle);

uint64_t datapod_geometry_type_hash(void);

uintptr_t datapod_geometry_header_size(void);

bool datapod_geometry_to_header_bytes(const DatapodGeometryHandle *handle,
                                      uint8_t *out,
                                      uintptr_t out_len);

DatapodGeometryHandle *datapod_geometry_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodIdentityHandle *datapod_identity_new_default(void);

void datapod_identity_free(DatapodIdentityHandle *handle);

uint64_t datapod_identity_type_hash(void);

uintptr_t datapod_identity_header_size(void);

bool datapod_identity_to_header_bytes(const DatapodIdentityHandle *handle,
                                      uint8_t *out,
                                      uintptr_t out_len);

DatapodIdentityHandle *datapod_identity_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodModelHandle *datapod_model_new_default(void);

void datapod_model_free(DatapodModelHandle *handle);

uint64_t datapod_model_type_hash(void);

uintptr_t datapod_model_header_size(void);

bool datapod_model_to_header_bytes(const DatapodModelHandle *handle,
                                   uint8_t *out,
                                   uintptr_t out_len);

DatapodModelHandle *datapod_model_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodMaterialHandle *datapod_material_new_default(void);

void datapod_material_free(DatapodMaterialHandle *handle);

uint64_t datapod_material_type_hash(void);

uintptr_t datapod_material_header_size(void);

bool datapod_material_to_header_bytes(const DatapodMaterialHandle *handle,
                                      uint8_t *out,
                                      uintptr_t out_len);

DatapodMaterialHandle *datapod_material_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodVisualHandle *datapod_visual_new_default(void);

void datapod_visual_free(DatapodVisualHandle *handle);

uint64_t datapod_visual_type_hash(void);

uintptr_t datapod_visual_header_size(void);

bool datapod_visual_to_header_bytes(const DatapodVisualHandle *handle,
                                    uint8_t *out,
                                    uintptr_t out_len);

DatapodVisualHandle *datapod_visual_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodCollisionHandle *datapod_collision_new_default(void);

void datapod_collision_free(DatapodCollisionHandle *handle);

uint64_t datapod_collision_type_hash(void);

uintptr_t datapod_collision_header_size(void);

bool datapod_collision_to_header_bytes(const DatapodCollisionHandle *handle,
                                       uint8_t *out,
                                       uintptr_t out_len);

DatapodCollisionHandle *datapod_collision_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodJointTypeHandle *datapod_joint_type_new_default(void);

void datapod_joint_type_free(DatapodJointTypeHandle *handle);

uint64_t datapod_joint_type_type_hash(void);

uintptr_t datapod_joint_type_header_size(void);

bool datapod_joint_type_to_header_bytes(const DatapodJointTypeHandle *handle,
                                        uint8_t *out,
                                        uintptr_t out_len);

DatapodJointTypeHandle *datapod_joint_type_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodJointHandle *datapod_joint_new_default(void);

void datapod_joint_free(DatapodJointHandle *handle);

uint64_t datapod_joint_type_hash(void);

uintptr_t datapod_joint_header_size(void);

bool datapod_joint_to_header_bytes(const DatapodJointHandle *handle,
                                   uint8_t *out,
                                   uintptr_t out_len);

DatapodJointHandle *datapod_joint_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodLinkHandle *datapod_link_new_default(void);

void datapod_link_free(DatapodLinkHandle *handle);

uint64_t datapod_link_type_hash(void);

uintptr_t datapod_link_header_size(void);

bool datapod_link_to_header_bytes(const DatapodLinkHandle *handle, uint8_t *out, uintptr_t out_len);

DatapodLinkHandle *datapod_link_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodSensorHandle *datapod_sensor_new_default(void);

void datapod_sensor_free(DatapodSensorHandle *handle);

uint64_t datapod_sensor_type_hash(void);

uintptr_t datapod_sensor_header_size(void);

bool datapod_sensor_to_header_bytes(const DatapodSensorHandle *handle,
                                    uint8_t *out,
                                    uintptr_t out_len);

DatapodSensorHandle *datapod_sensor_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodRobotHandle *datapod_robot_new_default(void);

void datapod_robot_free(DatapodRobotHandle *handle);

uint64_t datapod_robot_type_hash(void);

uintptr_t datapod_robot_header_size(void);

bool datapod_robot_to_header_bytes(const DatapodRobotHandle *handle,
                                   uint8_t *out,
                                   uintptr_t out_len);

DatapodRobotHandle *datapod_robot_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodActuatorHandle *datapod_actuator_new_default(void);

void datapod_actuator_free(DatapodActuatorHandle *handle);

uint64_t datapod_actuator_type_hash(void);

uintptr_t datapod_actuator_header_size(void);

bool datapod_actuator_to_header_bytes(const DatapodActuatorHandle *handle,
                                      uint8_t *out,
                                      uintptr_t out_len);

DatapodActuatorHandle *datapod_actuator_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodTransmissionJointHandle *datapod_transmission_joint_new_default(void);

void datapod_transmission_joint_free(DatapodTransmissionJointHandle *handle);

uint64_t datapod_transmission_joint_type_hash(void);

uintptr_t datapod_transmission_joint_header_size(void);

bool datapod_transmission_joint_to_header_bytes(const DatapodTransmissionJointHandle *handle,
                                                uint8_t *out,
                                                uintptr_t out_len);

DatapodTransmissionJointHandle *datapod_transmission_joint_from_header_bytes(const uint8_t *ptr,
                                                                             uintptr_t len);

DatapodTransmissionHandle *datapod_transmission_new_default(void);

void datapod_transmission_free(DatapodTransmissionHandle *handle);

uint64_t datapod_transmission_type_hash(void);

uintptr_t datapod_transmission_header_size(void);

bool datapod_transmission_to_header_bytes(const DatapodTransmissionHandle *handle,
                                          uint8_t *out,
                                          uintptr_t out_len);

DatapodTransmissionHandle *datapod_transmission_from_header_bytes(const uint8_t *ptr,
                                                                  uintptr_t len);

DatapodEncodingHandle *datapod_encoding_new_default(void);

void datapod_encoding_free(DatapodEncodingHandle *handle);

uint64_t datapod_encoding_type_hash(void);

uintptr_t datapod_encoding_header_size(void);

bool datapod_encoding_to_header_bytes(const DatapodEncodingHandle *handle,
                                      uint8_t *out,
                                      uintptr_t out_len);

DatapodEncodingHandle *datapod_encoding_from_header_bytes(const uint8_t *ptr, uintptr_t len);

DatapodEnvelopeHandle *datapod_envelope_new_default(void);

void datapod_envelope_free(DatapodEnvelopeHandle *handle);

uint64_t datapod_envelope_type_hash(void);

uintptr_t datapod_envelope_header_size(void);

bool datapod_envelope_to_header_bytes(const DatapodEnvelopeHandle *handle,
                                      uint8_t *out,
                                      uintptr_t out_len);

DatapodEnvelopeHandle *datapod_envelope_from_header_bytes(const uint8_t *ptr, uintptr_t len);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  /* DATAPOD_H */
