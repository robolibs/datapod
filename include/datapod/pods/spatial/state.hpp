#pragma once

#include <tuple>

#include "datapod/pods/matrix/vector.hpp"
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

        // Conversion to/from mat::vector for SIMD operations (13-DOF: pose + velocities)
        inline mat::Vector<double, 13> to_mat() const noexcept {
            return mat::Vector<double, 13>{
                pose.point.x,        pose.point.y,        pose.point.z,       pose.rotation.w,    pose.rotation.x,
                pose.rotation.y,     pose.rotation.z,     linear_velocity.vx, linear_velocity.vy, linear_velocity.vz,
                angular_velocity.vx, angular_velocity.vy, angular_velocity.vz};
        }

        static inline State from_mat(const mat::Vector<double, 13> &v) noexcept {
            return State{Pose{Point{v[0], v[1], v[2]}, Quaternion{v[3], v[4], v[5], v[6]}}, Velocity{v[7], v[8], v[9]},
                         Velocity{v[10], v[11], v[12]}};
        }
    };

    namespace state {
        /// Create a state from pose and velocities
        inline State make(const Pose &pose, const Velocity &linear_vel, const Velocity &angular_vel) noexcept {
            return State{pose, linear_vel, angular_vel};
        }

        /// Create a state from pose only (zero velocities)
        inline State make(const Pose &pose) noexcept {
            return State{pose, Velocity{0.0, 0.0, 0.0}, Velocity{0.0, 0.0, 0.0}};
        }

        /// Create a state at rest (identity pose, zero velocities)
        inline State at_rest() noexcept {
            return State{Pose{Point{0.0, 0.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}}, Velocity{0.0, 0.0, 0.0},
                         Velocity{0.0, 0.0, 0.0}};
        }
    } // namespace state

} // namespace datapod
