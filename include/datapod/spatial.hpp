#pragma once

/**
 * @file spatial.hpp
 * @brief All spatial types - Geometry and spatial indexing
 *
 * This header includes all spatial types for geometry,
 * bounding volumes, and spatial indexing structures.
 */

// Core spatial types
#include "spatial/acceleration.hpp"
#include "spatial/euler.hpp"
#include "spatial/geo.hpp"
#include "spatial/point.hpp"
#include "spatial/pose.hpp"
#include "spatial/quaternion.hpp"
#include "spatial/size.hpp"
#include "spatial/state.hpp"
#include "spatial/transform.hpp"
#include "spatial/velocity.hpp"

// Bounding volumes
#include "spatial/aabb.hpp"
#include "spatial/bounding_sphere.hpp"
#include "spatial/box.hpp"
#include "spatial/bs.hpp"
#include "spatial/obb.hpp"

// Primitives
#include "spatial/primitives/circle.hpp"
#include "spatial/primitives/line.hpp"
#include "spatial/primitives/rectangle.hpp"
#include "spatial/primitives/segment.hpp"
#include "spatial/primitives/square.hpp"
#include "spatial/primitives/triangle.hpp"

// Complex geometry
#include "spatial/complex/grid.hpp"
#include "spatial/complex/layer.hpp"
#include "spatial/complex/path.hpp"
#include "spatial/complex/polygon.hpp"
#include "spatial/complex/trajectory.hpp"
#include "spatial/linestring.hpp"
#include "spatial/ring.hpp"

// Multi-geometry
#include "spatial/multi/multi_linestring.hpp"
#include "spatial/multi/multi_point.hpp"
#include "spatial/multi/multi_polygon.hpp"

// Gaussian types
#include "spatial/gaussian/box.hpp"
#include "spatial/gaussian/circle.hpp"
#include "spatial/gaussian/point.hpp"
#include "spatial/gaussian/rectangle.hpp"

// Robot types
#include "spatial/robot/accel.hpp"
#include "spatial/robot/inertia.hpp"
#include "spatial/robot/odom.hpp"
#include "spatial/robot/twist.hpp"
#include "spatial/robot/wrench.hpp"

// Spatial indexing
#include "spatial/quadtree.hpp"
#include "spatial/rtree.hpp"

// Optional short namespace alias (enabled with -DSHORT_NAMESPACE)
#if defined(SHORT_NAMESPACE)
namespace dp = datapod;
#endif
