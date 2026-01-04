#pragma once

#include <tuple>

#include "../point.hpp"
#include "datapod/pods/matrix/vector.hpp"

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

        // Conversion to/from mat::vector for SIMD operations (10 values: mass + com + inertia tensor)
        inline mat::Vector<double, 10> to_mat() const noexcept {
            return mat::Vector<double, 10>{m, com.x, com.y, com.z, ixx, ixy, ixz, iyy, iyz, izz};
        }

        static inline Inertia from_mat(const mat::Vector<double, 10> &v) noexcept {
            return Inertia{v[0], Point{v[1], v[2], v[3]}, v[4], v[5], v[6], v[7], v[8], v[9]};
        }
    };

    namespace inertia {
        /// Create inertia from mass, center of mass, and inertia tensor components
        inline Inertia make(double mass, const Point &com, double ixx, double ixy, double ixz, double iyy, double iyz,
                            double izz) noexcept {
            return Inertia{mass, com, ixx, ixy, ixz, iyy, iyz, izz};
        }

        /// Create inertia with diagonal tensor (no products of inertia)
        inline Inertia diagonal(double mass, const Point &com, double ixx, double iyy, double izz) noexcept {
            return Inertia{mass, com, ixx, 0.0, 0.0, iyy, 0.0, izz};
        }

        /// Create inertia for a point mass at center of mass
        inline Inertia point_mass(double mass, const Point &com) noexcept {
            return Inertia{mass, com, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        }

        /// Create inertia for a uniform sphere
        inline Inertia sphere(double mass, double radius) noexcept {
            double i = 0.4 * mass * radius * radius; // (2/5) * m * r²
            return Inertia{mass, Point{0.0, 0.0, 0.0}, i, 0.0, 0.0, i, 0.0, i};
        }

        /// Create inertia for a uniform box
        inline Inertia box(double mass, double width, double height, double depth) noexcept {
            double ixx = (mass / 12.0) * (height * height + depth * depth);
            double iyy = (mass / 12.0) * (width * width + depth * depth);
            double izz = (mass / 12.0) * (width * width + height * height);
            return Inertia{mass, Point{0.0, 0.0, 0.0}, ixx, 0.0, 0.0, iyy, 0.0, izz};
        }

        /// Create inertia for a uniform cylinder (axis along z)
        inline Inertia cylinder(double mass, double radius, double height) noexcept {
            double ixx = (mass / 12.0) * (3.0 * radius * radius + height * height);
            double iyy = ixx;
            double izz = 0.5 * mass * radius * radius;
            return Inertia{mass, Point{0.0, 0.0, 0.0}, ixx, 0.0, 0.0, iyy, 0.0, izz};
        }
    } // namespace inertia

} // namespace datapod
