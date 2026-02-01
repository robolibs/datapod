#pragma once

/**
 * @file robot.hpp
 * @brief Robot model types - URDF/SDF-style robot definitions
 *
 * This header includes all robot model types for kinematic tree representation:
 * - Identity (name, UUID, IP, RCI)
 * - Geometry shapes (box, sphere, cylinder, mesh)
 * - Visual and collision elements
 * - Joints (fixed, revolute, continuous, prismatic, floating, planar)
 * - Links (rigid bodies with inertia, visuals, collisions)
 * - Model (complete kinematic tree)
 * - Robot (Identity + Model wrapper)
 * - Motion types (accel, twist, wrench, odom)
 * - Transmission
 *
 * All types are in the datapod::robot:: namespace (or dp::robot::).
 */

// Robot model types
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

// Short namespace alias
#if !defined(NO_SHORT_NAMESPACE)
namespace dp = datapod;
#endif
