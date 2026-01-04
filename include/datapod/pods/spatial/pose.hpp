#pragma once

#include <cmath>
#include <tuple>

#include "datapod/pods/matrix/vector.hpp"
#include "point.hpp"
#include "quaternion.hpp"

namespace datapod {

    /**
     * @brief 3D pose with position and orientation (POD)
     *
     * Represents a position (x, y, z) and orientation (quaternion) in 3D space.
     * Fully serializable and reflectable.
     */
    struct Pose {
        Point point;         // Position
        Quaternion rotation; // Orientation as unit quaternion

        auto members() noexcept { return std::tie(point, rotation); }
        auto members() const noexcept { return std::tie(point, rotation); }

        // Utility
        inline bool is_set() const noexcept { return point.is_set() || rotation.is_set(); }

        // 3D transformation using quaternion rotation
        inline Point transform_point(const Point &local_point) const noexcept {
            // Rotate point using quaternion: p' = q * p * q_conj
            const Quaternion q = rotation;
            const Quaternion p{0.0, local_point.x, local_point.y, local_point.z};
            const Quaternion q_conj = q.conjugate();
            const Quaternion rotated = q * p * q_conj;

            return Point{point.x + rotated.x, point.y + rotated.y, point.z + rotated.z};
        }

        inline Point inverse_transform_point(const Point &world_point) const noexcept {
            // Translate to local origin
            const Point translated{world_point.x - point.x, world_point.y - point.y, world_point.z - point.z};

            // Rotate using inverse quaternion: p' = q_conj * p * q
            const Quaternion q_conj = rotation.conjugate();
            const Quaternion p{0.0, translated.x, translated.y, translated.z};
            const Quaternion rotated = q_conj * p * rotation;

            return Point{rotated.x, rotated.y, rotated.z};
        }

        // Pose composition (this * other = apply this, then other)
        inline Pose operator*(const Pose &other) const noexcept {
            return Pose{transform_point(other.point), rotation * other.rotation};
        }

        // Inverse pose
        inline Pose inverse() const noexcept {
            const Quaternion q_inv = rotation.conjugate();
            const Quaternion neg_pos{0.0, -point.x, -point.y, -point.z};
            const Quaternion rotated = q_inv * neg_pos * rotation;

            return Pose{Point{rotated.x, rotated.y, rotated.z}, q_inv};
        }

        // Comparison operators
        inline bool operator==(const Pose &other) const noexcept {
            return point == other.point && rotation == other.rotation;
        }

        inline bool operator!=(const Pose &other) const noexcept { return !(*this == other); }

        // Conversion to/from mat::vector for SIMD operations (7-DOF: position + quaternion)
        inline mat::Vector<double, 7> to_mat() const noexcept {
            return mat::Vector<double, 7>{point.x, point.y, point.z, rotation.w, rotation.x, rotation.y, rotation.z};
        }

        static inline Pose from_mat(const mat::Vector<double, 7> &v) noexcept {
            return Pose{Point{v[0], v[1], v[2]}, Quaternion{v[3], v[4], v[5], v[6]}};
        }
    };

} // namespace datapod
