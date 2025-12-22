#pragma once

#include <cmath>
#include <tuple>

namespace datapod {

    // Forward declaration
    struct Euler;

    /**
     * @brief Unit quaternion for 3D rotation (w, x, y, z) - POD
     *
     * Pure aggregate struct with rotation utility methods.
     * Use aggregate initialization: Quaternion{1.0, 0.0, 0.0, 0.0}
     * w is the real part, (x, y, z) is the imaginary part.
     * Fully serializable and reflectable.
     */
    struct Quaternion {
        double w = 1.0; // Real part
        double x = 0.0; // Imaginary i
        double y = 0.0; // Imaginary j
        double z = 0.0; // Imaginary k

        auto members() noexcept { return std::tie(w, x, y, z); }
        auto members() const noexcept { return std::tie(w, x, y, z); }

        // Utility
        inline bool is_set() const noexcept { return w != 1.0 || x != 0.0 || y != 0.0 || z != 0.0; }

        inline double magnitude() const noexcept { return std::sqrt(w * w + x * x + y * y + z * z); }

        inline Quaternion normalized() const noexcept {
            const double mag = magnitude();
            if (mag < 1e-10) {
                return Quaternion{1.0, 0.0, 0.0, 0.0}; // Identity quaternion
            }
            return Quaternion{w / mag, x / mag, y / mag, z / mag};
        }

        inline Quaternion conjugate() const noexcept { return Quaternion{w, -x, -y, -z}; }

        // Operators
        inline Quaternion operator*(const Quaternion &other) const noexcept {
            return Quaternion{w * other.w - x * other.x - y * other.y - z * other.z,
                              w * other.x + x * other.w + y * other.z - z * other.y,
                              w * other.y - x * other.z + y * other.w + z * other.x,
                              w * other.z + x * other.y - y * other.x + z * other.w};
        }

        inline bool operator==(const Quaternion &other) const noexcept {
            return w == other.w && x == other.x && y == other.y && z == other.z;
        }

        inline bool operator!=(const Quaternion &other) const noexcept { return !(*this == other); }

        // Conversion to Euler (implementation below, after including euler.hpp)
        inline Euler to_euler() const noexcept;
    };

} // namespace datapod

// Include Euler after Quaternion struct is complete
#include "euler.hpp"

namespace datapod {

    // Implementation of conversion methods (both structs are now complete)

    inline Quaternion Euler::to_quaternion() const noexcept {
        const double cr = std::cos(roll * 0.5);
        const double sr = std::sin(roll * 0.5);
        const double cp = std::cos(pitch * 0.5);
        const double sp = std::sin(pitch * 0.5);
        const double cy = std::cos(yaw * 0.5);
        const double sy = std::sin(yaw * 0.5);

        return Quaternion{
            cr * cp * cy + sr * sp * sy, // w
            sr * cp * cy - cr * sp * sy, // x
            cr * sp * cy + sr * cp * sy, // y
            cr * cp * sy - sr * sp * cy  // z
        };
    }

    inline Euler Quaternion::to_euler() const noexcept {
        // Roll (x-axis rotation)
        const double sinr_cosp = 2.0 * (w * x + y * z);
        const double cosr_cosp = 1.0 - 2.0 * (x * x + y * y);
        const double roll = std::atan2(sinr_cosp, cosr_cosp);

        // Pitch (y-axis rotation)
        const double sinp = 2.0 * (w * y - z * x);
        double pitch;
        if (std::abs(sinp) >= 1.0) {
            // Gimbal lock case
            pitch = std::copysign(1.5707963267948966, sinp); // Use 90 degrees (PI/2)
        } else {
            pitch = std::asin(sinp);
        }

        // Yaw (z-axis rotation)
        const double siny_cosp = 2.0 * (w * z + x * y);
        const double cosy_cosp = 1.0 - 2.0 * (y * y + z * z);
        const double yaw = std::atan2(siny_cosp, cosy_cosp);

        return Euler{roll, pitch, yaw};
    }

} // namespace datapod
