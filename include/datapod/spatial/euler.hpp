#pragma once

#include <cmath>
#include <tuple>

#include "datapod/matrix/vector.hpp"

namespace datapod {

    // Forward declaration
    struct Quaternion;

    /**
     * @brief Euler angles for 3D rotation (roll, pitch, yaw) - POD
     *
     * Pure aggregate struct with rotation utility methods.
     * Use aggregate initialization: Euler{0.1, 0.2, 0.3}
     * Fully serializable and reflectable.
     */
    struct Euler {
        double roll = 0.0;
        double pitch = 0.0;
        double yaw = 0.0;

        auto members() noexcept { return std::tie(roll, pitch, yaw); }
        auto members() const noexcept { return std::tie(roll, pitch, yaw); }

        // Utility
        inline bool is_set() const noexcept { return roll != 0.0 || pitch != 0.0 || yaw != 0.0; }

        inline double yaw_cos() const noexcept { return std::cos(yaw); }
        inline double yaw_sin() const noexcept { return std::sin(yaw); }

        // Normalization (wrap angles to [-PI, PI])
        inline Euler normalized() const noexcept {
            auto normalize_angle = [](double angle) -> double {
                constexpr double PI = 3.14159265358979323846;
                constexpr double TWO_PI = 2.0 * PI;
                // Wrap to [-PI, PI]
                double result = std::fmod(angle + PI, TWO_PI);
                if (result < 0.0)
                    result += TWO_PI;
                return result - PI;
            };

            return Euler{normalize_angle(roll), normalize_angle(pitch), normalize_angle(yaw)};
        }

        // Operators
        inline Euler operator+(const Euler &other) const noexcept {
            return Euler{roll + other.roll, pitch + other.pitch, yaw + other.yaw};
        }

        inline Euler operator-(const Euler &other) const noexcept {
            return Euler{roll - other.roll, pitch - other.pitch, yaw - other.yaw};
        }

        inline Euler operator*(double scale) const noexcept { return Euler{roll * scale, pitch * scale, yaw * scale}; }

        inline bool operator==(const Euler &other) const noexcept {
            return roll == other.roll && pitch == other.pitch && yaw == other.yaw;
        }

        inline bool operator!=(const Euler &other) const noexcept { return !(*this == other); }

        // Conversion to Quaternion (implementation in quaternion.hpp after Quaternion is defined)
        inline Quaternion to_quaternion() const noexcept;

        // Conversion to/from mat::vector for SIMD operations
        inline mat::vector<double, 3> to_mat() const noexcept { return mat::vector<double, 3>{roll, pitch, yaw}; }

        static inline Euler from_mat(const mat::vector<double, 3> &v) noexcept { return Euler{v[0], v[1], v[2]}; }
    };

} // namespace datapod
