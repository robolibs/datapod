#pragma once

#include <tuple>

#include "../velocity.hpp"
#include "datapod/pods/matrix/vector.hpp"

namespace datapod {

    /**
     * @brief Twist - 6-DOF velocity (linear + angular) (POD)
     *
     * Represents velocity in free space broken into linear and angular parts.
     * This is the standard ROS geometry_msgs/Twist equivalent.
     *
     * Pure aggregate struct with robot-specific velocity representation.
     * Use aggregate initialization: Twist{linear_vel, angular_vel}
     * Fully serializable and reflectable.
     *
     * Fields:
     * - linear: Linear velocity (vx, vy, vz) in m/s
     * - angular: Angular velocity (ωx, ωy, ωz) in rad/s
     *
     * Use cases:
     * - Robot velocity commands (e.g., cmd_vel)
     * - Odometry velocity output
     * - End-effector velocities
     */
    struct Twist {
        Velocity linear;  // Linear velocity (vx, vy, vz) m/s
        Velocity angular; // Angular velocity (ωx, ωy, ωz) rad/s

        auto members() noexcept { return std::tie(linear, angular); }
        auto members() const noexcept { return std::tie(linear, angular); }

        // Utility
        inline bool is_set() const noexcept { return linear.is_set() || angular.is_set(); }

        // Comparison
        inline bool operator==(const Twist &other) const noexcept {
            return linear == other.linear && angular == other.angular;
        }

        inline bool operator!=(const Twist &other) const noexcept { return !(*this == other); }

        // Conversion to/from mat::vector for SIMD operations (6-DOF)
        inline mat::Vector<double, 6> to_mat() const noexcept {
            return mat::Vector<double, 6>{linear.vx, linear.vy, linear.vz, angular.vx, angular.vy, angular.vz};
        }

        static inline Twist from_mat(const mat::Vector<double, 6> &v) noexcept {
            return Twist{Velocity{v[0], v[1], v[2]}, Velocity{v[3], v[4], v[5]}};
        }
    };

    namespace twist {
        /// Create a twist from linear and angular velocities
        inline Twist make(const Velocity &linear, const Velocity &angular) noexcept { return Twist{linear, angular}; }

        /// Create a twist from velocity components
        inline Twist make(double vx, double vy, double vz, double wx, double wy, double wz) noexcept {
            return Twist{Velocity{vx, vy, vz}, Velocity{wx, wy, wz}};
        }

        /// Create a twist with only linear velocity (zero angular)
        inline Twist linear(const Velocity &vel) noexcept { return Twist{vel, Velocity{0.0, 0.0, 0.0}}; }

        /// Create a twist with only angular velocity (zero linear)
        inline Twist angular(const Velocity &vel) noexcept { return Twist{Velocity{0.0, 0.0, 0.0}, vel}; }

        /// Create a zero twist (stationary)
        inline Twist zero() noexcept { return Twist{Velocity{0.0, 0.0, 0.0}, Velocity{0.0, 0.0, 0.0}}; }
    } // namespace twist

} // namespace datapod
