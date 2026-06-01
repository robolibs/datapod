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
 * Opaque polygon handle.
 */
typedef struct DatapodPolygon DatapodPolygon;

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
 * A borrowed byte view.
 */
typedef struct {
  const uint8_t *ptr;
  uintptr_t len;
} DatapodBytes;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

const char *datapod_last_error_message(void);

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

DatapodPolygon *datapod_polygon_new(const DatapodPoint *vertices, uintptr_t len);

void datapod_polygon_free(DatapodPolygon *polygon);

uintptr_t datapod_polygon_len(const DatapodPolygon *polygon);

double datapod_polygon_area(const DatapodPolygon *polygon);

double datapod_polygon_perimeter(const DatapodPolygon *polygon);

bool datapod_polygon_contains(const DatapodPolygon *polygon, DatapodPoint point);

bool datapod_polygon_vertex(const DatapodPolygon *polygon, uintptr_t index, DatapodPoint *out);

DatapodBytes datapod_polygon_vertices(const DatapodPolygon *polygon);

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  /* DATAPOD_H */
