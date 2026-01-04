#pragma once

#include <tuple>

#include "../pose.hpp"
#include "datapod/pods/matrix/vector.hpp"
#include "twist.hpp"

namespace datapod {

    /**
     * @brief Odom - Odometry estimate (POD)
     *
     * Represents an estimate of position and velocity in free space.
     * Simplified version of ROS nav_msgs/Odometry without covariances.
     *
     * Pure aggregate struct combining pose and twist for robot state estimation.
     * Use aggregate initialization: Odom{pose, twist}
     * Fully serializable and reflectable.
     *
     * Fields:
     * - pose: Estimated pose (position + orientation) relative to fixed world frame
     * - twist: Estimated linear and angular velocity
     *
     * Use cases:
     * - Mobile robot odometry output
     * - Wheel encoder-based state estimation
     * - Visual odometry
     * - Sensor fusion results (without uncertainty)
     * - Simple robot state tracking
     *
     * Note: This is the simplified version without covariance matrices.
     * The pose represents position in the world frame.
     * The twist represents velocity in the robot's local frame.
     */
    struct Odom {
        Pose pose;   // Estimated pose (position + orientation)
        Twist twist; // Estimated velocity (linear + angular)

        auto members() noexcept { return std::tie(pose, twist); }
        auto members() const noexcept { return std::tie(pose, twist); }

        // Utility
        inline bool is_set() const noexcept { return pose.is_set() || twist.is_set(); }

        // Comparison
        inline bool operator==(const Odom &other) const noexcept { return pose == other.pose && twist == other.twist; }

        inline bool operator!=(const Odom &other) const noexcept { return !(*this == other); }

        // Conversion to/from mat::vector for SIMD operations (13-DOF: pose + twist)
        inline mat::Vector<double, 13> to_mat() const noexcept {
            return mat::Vector<double, 13>{pose.point.x,    pose.point.y,    pose.point.z,     pose.rotation.w,
                                           pose.rotation.x, pose.rotation.y, pose.rotation.z,  twist.linear.vx,
                                           twist.linear.vy, twist.linear.vz, twist.angular.vx, twist.angular.vy,
                                           twist.angular.vz};
        }

        static inline Odom from_mat(const mat::Vector<double, 13> &v) noexcept {
            return Odom{Pose{Point{v[0], v[1], v[2]}, Quaternion{v[3], v[4], v[5], v[6]}},
                        Twist{Velocity{v[7], v[8], v[9]}, Velocity{v[10], v[11], v[12]}}};
        }
    };

} // namespace datapod
