#pragma once

#include <tuple>

#include "../point.hpp"

namespace datapod {

    /**
     * @brief Inertia - Rigid body inertial properties (POD)
     *
     * Represents the inertial properties of a rigid body: mass, center of mass, and inertia tensor.
     * This is the ROS geometry_msgs/Inertia equivalent.
     *
     * Pure aggregate struct with inertial parameters for dynamics.
     * Use aggregate initialization: Inertia{mass, com, ixx, ixy, ...}
     * Fully serializable and reflectable.
     *
     * Fields:
     * - m: Mass in kilograms [kg]
     * - com: Center of mass position in meters [m]
     * - ixx, ixy, ixz, iyy, iyz, izz: Inertia tensor components in kg⋅m² [kg⋅m²]
     *
     * Inertia Tensor (symmetric 3x3 matrix):
     *     | ixx  ixy  ixz |
     * I = | ixy  iyy  iyz |
     *     | ixz  iyz  izz |
     *
     * Use cases:
     * - Rigid body dynamics simulation
     * - Robot link inertial parameters (URDF)
     * - Model-based control (computed torque, impedance)
     * - Motion planning with dynamics
     *
     * Note: Inertia tensor is typically expressed in the body frame at the center of mass
     */
    struct Inertia {
        double m = 0.0; // Mass [kg]
        Point com;      // Center of mass [m]

        // Inertia tensor components [kg⋅m²]
        double ixx = 0.0; // Moment of inertia about x-axis
        double ixy = 0.0; // Product of inertia xy
        double ixz = 0.0; // Product of inertia xz
        double iyy = 0.0; // Moment of inertia about y-axis
        double iyz = 0.0; // Product of inertia yz
        double izz = 0.0; // Moment of inertia about z-axis

        auto members() noexcept { return std::tie(m, com, ixx, ixy, ixz, iyy, iyz, izz); }
        auto members() const noexcept { return std::tie(m, com, ixx, ixy, ixz, iyy, iyz, izz); }

        // Utility
        inline bool is_set() const noexcept {
            return m != 0.0 || com.is_set() || ixx != 0.0 || iyy != 0.0 || izz != 0.0;
        }

        // Trace of inertia tensor (sum of diagonal elements)
        inline double trace() const noexcept { return ixx + iyy + izz; }

        // Check if inertia tensor is diagonal (products of inertia are zero)
        inline bool is_diagonal() const noexcept { return ixy == 0.0 && ixz == 0.0 && iyz == 0.0; }

        // Comparison
        inline bool operator==(const Inertia &other) const noexcept {
            return m == other.m && com == other.com && ixx == other.ixx && ixy == other.ixy && ixz == other.ixz &&
                   iyy == other.iyy && iyz == other.iyz && izz == other.izz;
        }

        inline bool operator!=(const Inertia &other) const noexcept { return !(*this == other); }
    };

} // namespace datapod
