#pragma once

#include <tuple>

#include "datapod/pods/spatial/pose.hpp"
#include "datapod/types/types.hpp"

namespace datapod {
    namespace robot {

        /**
         * @brief Inertial - URDF-style inertial properties with full origin pose (POD)
         *
         * Represents the inertial properties of a rigid body with a full pose origin,
         * matching URDF's <inertial> element which includes:
         *   <origin xyz="..." rpy="..."/>  -> full Pose (position + rotation)
         *   <mass value="..."/>
         *   <inertia ixx="..." ixy="..." ixz="..." iyy="..." iyz="..." izz="..."/>
         *
         * The origin defines the transform from the link frame to the inertia frame.
         * The inertia tensor is expressed in the inertia frame at the center of mass.
         *
         * Fields:
         * - origin: Transform from link frame to inertia frame (position = COM, rotation = inertia axes)
         * - mass: Mass in kilograms [kg]
         * - ixx, ixy, ixz, iyy, iyz, izz: Inertia tensor components in kg·m²
         *
         * Inertia Tensor (symmetric 3x3 matrix in inertia frame):
         *     | ixx  ixy  ixz |
         * I = | ixy  iyy  iyz |
         *     | ixz  iyz  izz |
         */
        struct Inertial {
            Pose origin;    // Transform from link frame to inertia frame
            f64 mass = 0.0; // Mass [kg]

            // Inertia tensor components [kg·m²]
            f64 ixx = 0.0;
            f64 ixy = 0.0;
            f64 ixz = 0.0;
            f64 iyy = 0.0;
            f64 iyz = 0.0;
            f64 izz = 0.0;

            auto members() noexcept { return std::tie(origin, mass, ixx, ixy, ixz, iyy, iyz, izz); }
            auto members() const noexcept { return std::tie(origin, mass, ixx, ixy, ixz, iyy, iyz, izz); }

            inline bool is_set() const noexcept {
                return mass != 0.0 || origin.is_set() || ixx != 0.0 || iyy != 0.0 || izz != 0.0;
            }

            /// Trace of inertia tensor (sum of diagonal elements)
            inline f64 trace() const noexcept { return ixx + iyy + izz; }

            /// Check if inertia tensor is diagonal (products of inertia are zero)
            inline bool is_diagonal() const noexcept { return ixy == 0.0 && ixz == 0.0 && iyz == 0.0; }

            inline bool operator==(const Inertial &other) const noexcept {
                return origin == other.origin && mass == other.mass && ixx == other.ixx && ixy == other.ixy &&
                       ixz == other.ixz && iyy == other.iyy && iyz == other.iyz && izz == other.izz;
            }
            inline bool operator!=(const Inertial &other) const noexcept { return !(*this == other); }
        };

        namespace inertial {
            /// Create inertial with full origin pose
            inline Inertial make(const Pose &origin, f64 mass, f64 ixx, f64 ixy, f64 ixz, f64 iyy, f64 iyz,
                                 f64 izz) noexcept {
                return Inertial{origin, mass, ixx, ixy, ixz, iyy, iyz, izz};
            }

            /// Create inertial with only position (identity rotation)
            inline Inertial make(const Point &com, f64 mass, f64 ixx, f64 ixy, f64 ixz, f64 iyy, f64 iyz,
                                 f64 izz) noexcept {
                return Inertial{pose::make(com), mass, ixx, ixy, ixz, iyy, iyz, izz};
            }

            /// Create inertial with diagonal tensor (no products of inertia)
            inline Inertial diagonal(const Pose &origin, f64 mass, f64 ixx, f64 iyy, f64 izz) noexcept {
                return Inertial{origin, mass, ixx, 0.0, 0.0, iyy, 0.0, izz};
            }

            /// Create inertial for a point mass
            inline Inertial point_mass(f64 mass, const Point &com) noexcept {
                return Inertial{pose::make(com), mass, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
            }

            /// Create inertial for a uniform sphere at origin
            inline Inertial sphere(f64 mass, f64 radius) noexcept {
                f64 i = 0.4 * mass * radius * radius; // (2/5) * m * r²
                return Inertial{pose::identity(), mass, i, 0.0, 0.0, i, 0.0, i};
            }

            /// Create inertial for a uniform box at origin
            inline Inertial box(f64 mass, f64 width, f64 height, f64 depth) noexcept {
                f64 ixx = (mass / 12.0) * (height * height + depth * depth);
                f64 iyy = (mass / 12.0) * (width * width + depth * depth);
                f64 izz = (mass / 12.0) * (width * width + height * height);
                return Inertial{pose::identity(), mass, ixx, 0.0, 0.0, iyy, 0.0, izz};
            }

            /// Create inertial for a uniform cylinder at origin (axis along z)
            inline Inertial cylinder(f64 mass, f64 radius, f64 height) noexcept {
                f64 ixx = (mass / 12.0) * (3.0 * radius * radius + height * height);
                f64 iyy = ixx;
                f64 izz = 0.5 * mass * radius * radius;
                return Inertial{pose::identity(), mass, ixx, 0.0, 0.0, iyy, 0.0, izz};
            }
        } // namespace inertial

    } // namespace robot
} // namespace datapod
