#pragma once

#include <tuple>

#include "../acceleration.hpp"
#include "datapod/matrix/vector.hpp"

namespace datapod {

    /**
     * @brief Accel - 6-DOF acceleration (linear + angular) (POD)
     *
     * Represents acceleration in free space broken into linear and angular parts.
     * This is the ROS geometry_msgs/Accel equivalent.
     *
     * Pure aggregate struct with robot-specific acceleration representation.
     * Use aggregate initialization: Accel{linear_accel, angular_accel}
     * Fully serializable and reflectable.
     *
     * Fields:
     * - linear: Linear acceleration (ax, ay, az) in m/s²
     * - angular: Angular acceleration (αx, αy, αz) in rad/s²
     *
     * Use cases:
     * - Dynamics calculations
     * - Jerk limits for trajectory planning
     * - Force/torque estimation (F = m*a, τ = I*α)
     */
    struct Accel {
        Acceleration linear;  // Linear acceleration (ax, ay, az) m/s²
        Acceleration angular; // Angular acceleration (αx, αy, αz) rad/s²

        auto members() noexcept { return std::tie(linear, angular); }
        auto members() const noexcept { return std::tie(linear, angular); }

        // Utility
        inline bool is_set() const noexcept { return linear.is_set() || angular.is_set(); }

        // Comparison
        inline bool operator==(const Accel &other) const noexcept {
            return linear == other.linear && angular == other.angular;
        }

        inline bool operator!=(const Accel &other) const noexcept { return !(*this == other); }

        // Conversion to/from mat::vector for SIMD operations (6-DOF)
        inline mat::Vector<double, 6> to_mat() const noexcept {
            return mat::Vector<double, 6>{linear.ax, linear.ay, linear.az, angular.ax, angular.ay, angular.az};
        }

        static inline Accel from_mat(const mat::Vector<double, 6> &v) noexcept {
            return Accel{Acceleration{v[0], v[1], v[2]}, Acceleration{v[3], v[4], v[5]}};
        }
    };

} // namespace datapod
