#pragma once

/**
 * @file spatial.hpp
 * @brief All spatial types - Geometry and spatial indexing
 *
 * This header includes all spatial types for geometry,
 * bounding volumes, and spatial indexing structures.
 */

// Core spatial types
#include "pods/spatial/acceleration.hpp"
#include "pods/spatial/euler.hpp"
#include "pods/spatial/geo.hpp"
#include "pods/spatial/loc.hpp"
#include "pods/spatial/point.hpp"
#include "pods/spatial/pose.hpp"
#include "pods/spatial/quaternion.hpp"
#include "pods/spatial/size.hpp"
#include "pods/spatial/state.hpp"
#include "pods/spatial/transform.hpp"
#include "pods/spatial/utm.hpp"
#include "pods/spatial/velocity.hpp"

// Bounding volumes
#include "pods/spatial/aabb.hpp"
#include "pods/spatial/bounding_sphere.hpp"
#include "pods/spatial/box.hpp"
#include "pods/spatial/bs.hpp"
#include "pods/spatial/obb.hpp"

// Primitives
#include "pods/spatial/primitives/circle.hpp"
#include "pods/spatial/primitives/line.hpp"
#include "pods/spatial/primitives/rectangle.hpp"
#include "pods/spatial/primitives/segment.hpp"
#include "pods/spatial/primitives/square.hpp"
#include "pods/spatial/primitives/triangle.hpp"

// Complex geometry
#include "pods/spatial/complex/grid.hpp"
#include "pods/spatial/complex/layer.hpp"
#include "pods/spatial/complex/path.hpp"
#include "pods/spatial/complex/polygon.hpp"
#include "pods/spatial/complex/trajectory.hpp"
#include "pods/spatial/linestring.hpp"
#include "pods/spatial/ring.hpp"

// Multi-geometry
#include "pods/spatial/multi/multi_linestring.hpp"
#include "pods/spatial/multi/multi_point.hpp"
#include "pods/spatial/multi/multi_polygon.hpp"

// Gaussian types
#include "pods/spatial/gaussian/box.hpp"
#include "pods/spatial/gaussian/circle.hpp"
#include "pods/spatial/gaussian/point.hpp"
#include "pods/spatial/gaussian/rectangle.hpp"

// Robot types
#include "pods/spatial/robot/accel.hpp"
#include "pods/spatial/robot/collision.hpp"
#include "pods/spatial/robot/geometry.hpp"
#include "pods/spatial/robot/identity.hpp"
#include "pods/spatial/robot/inertial.hpp"
#include "pods/spatial/robot/joint.hpp"
#include "pods/spatial/robot/link.hpp"
#include "pods/spatial/robot/model.hpp"
#include "pods/spatial/robot/odom.hpp"
#include "pods/spatial/robot/robot.hpp"
#include "pods/spatial/robot/transmission.hpp"
#include "pods/spatial/robot/twist.hpp"
#include "pods/spatial/robot/visual.hpp"
#include "pods/spatial/robot/wrench.hpp"

// Spatial indexing
#include "pods/spatial/quadtree.hpp"
#include "pods/spatial/rtree.hpp"

// Optional short namespace alias (enabled with -DSHORT_NAMESPACE)
#if defined(SHORT_NAMESPACE)
#endif

// Short namespace alias (disable with -DNO_SHORT_NAMESPACE)
#if !defined(NO_SHORT_NAMESPACE)
namespace dp = datapod;
#endif
