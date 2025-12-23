#pragma once

#include <tuple>

#include "pose.hpp"
#include "velocity.hpp"

namespace datapod {

    /**
     * @brief Robot/vehicle state with pose and velocities (POD)
     *
     * Combines spatial pose (position + orientation) with linear and angular velocities.
     * Pure aggregate struct - no constructors.
     * Use aggregate initialization: State{pose, linear_vel, angular_vel}
     * Fully serializable and reflectable.
     *
     * Fields:
     * - pose: Position and orientation in 3D space
     * - linear_velocity: Linear velocity (vx, vy, vz) in m/s
     * - angular_velocity: Angular velocity (ωx, ωy, ωz) in rad/s
     */
    struct State {
        Pose pose;                 // Position and orientation
        Velocity linear_velocity;  // Linear velocity (vx, vy, vz) m/s
        Velocity angular_velocity; // Angular velocity (ωx, ωy, ωz) rad/s

        auto members() noexcept { return std::tie(pose, linear_velocity, angular_velocity); }
        auto members() const noexcept { return std::tie(pose, linear_velocity, angular_velocity); }

        inline bool is_set() const noexcept {
            return pose.is_set() || linear_velocity.is_set() || angular_velocity.is_set();
        }
    };

} // namespace datapod
