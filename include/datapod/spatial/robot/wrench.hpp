#pragma once

#include <tuple>

#include "../point.hpp"
#include "datapod/matrix/vector.hpp"

namespace datapod {

    /**
     * @brief Wrench - 6-DOF force and torque (POD)
     *
     * Represents force in free space separated into linear (force) and angular (torque) parts.
     * This is the ROS geometry_msgs/Wrench equivalent.
     *
     * Pure aggregate struct with robot-specific force/torque representation.
     * Use aggregate initialization: Wrench{force_vec, torque_vec}
     * Fully serializable and reflectable.
     *
     * Fields:
     * - force: Force vector (Fx, Fy, Fz) in Newtons [N]
     * - torque: Torque vector (Tx, Ty, Tz) in Newton-meters [N⋅m]
     *
     * Use cases:
     * - Force/torque sensor readings
     * - Contact forces in manipulation
     * - Impedance control
     * - Wrench commands for compliant motion
     *
     * Note: Using Point for force/torque vectors (generic 3D vector representation)
     */
    struct Wrench {
        Point force;  // Force (Fx, Fy, Fz) N
        Point torque; // Torque (Tx, Ty, Tz) N⋅m

        auto members() noexcept { return std::tie(force, torque); }
        auto members() const noexcept { return std::tie(force, torque); }

        // Utility
        inline bool is_set() const noexcept { return force.is_set() || torque.is_set(); }

        // Magnitude calculations
        inline double force_magnitude() const noexcept { return force.magnitude(); }

        inline double torque_magnitude() const noexcept { return torque.magnitude(); }

        // Comparison
        inline bool operator==(const Wrench &other) const noexcept {
            return force == other.force && torque == other.torque;
        }

        inline bool operator!=(const Wrench &other) const noexcept { return !(*this == other); }

        // Wrench arithmetic
        inline Wrench operator+(const Wrench &other) const noexcept {
            return Wrench{force + other.force, torque + other.torque};
        }

        inline Wrench operator-(const Wrench &other) const noexcept {
            return Wrench{force - other.force, torque - other.torque};
        }

        inline Wrench operator*(double scale) const noexcept { return Wrench{force * scale, torque * scale}; }

        inline Wrench operator/(double scale) const noexcept { return Wrench{force / scale, torque / scale}; }

        // Conversion to/from mat::vector for SIMD operations (6-DOF)
        inline mat::Vector<double, 6> to_mat() const noexcept {
            return mat::Vector<double, 6>{force.x, force.y, force.z, torque.x, torque.y, torque.z};
        }

        static inline Wrench from_mat(const mat::Vector<double, 6> &v) noexcept {
            return Wrench{Point{v[0], v[1], v[2]}, Point{v[3], v[4], v[5]}};
        }
    };

} // namespace datapod
