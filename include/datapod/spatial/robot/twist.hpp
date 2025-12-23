#pragma once

#include <tuple>

#include "../velocity.hpp"

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
    };

} // namespace datapod
