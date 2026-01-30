#pragma once

/**
 * @file robot.hpp
 * @brief Robot model types - URDF/SDF-style robot definitions
 *
 * This header includes all robot model types for kinematic tree representation:
 * - Geometry shapes (box, sphere, cylinder, mesh)
 * - Visual and collision elements
 * - Joints (fixed, revolute, continuous, prismatic, floating, planar)
 * - Links (rigid bodies with inertia, visuals, collisions)
 * - Model (complete kinematic tree)
 *
 * All types are in the datapod::robot:: namespace (or dp::robot::).
 */

// Robot model types
#include "pods/spatial/robot/collision.hpp"
#include "pods/spatial/robot/geometry.hpp"
#include "pods/spatial/robot/joint.hpp"
#include "pods/spatial/robot/link.hpp"
#include "pods/spatial/robot/model.hpp"
#include "pods/spatial/robot/visual.hpp"

// Short namespace alias
#if !defined(NO_SHORT_NAMESPACE)
namespace dp = datapod;
#endif
