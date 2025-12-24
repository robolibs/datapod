#pragma once

#include <cmath>
#include <tuple>

#include "datapod/matrix/vector.hpp"

namespace datapod {

    /**
     * @brief 3D acceleration with double precision components (POD)
     *
     * Represents linear acceleration in 3D space (ax, ay, az).
     * Pure aggregate struct with acceleration-specific utility methods.
     * Use aggregate initialization: Acceleration{0.5, 0.0, -9.81}
     * Fully serializable and reflectable.
     */
    struct Acceleration {
        double ax = 0.0; // Acceleration in x direction (m/s²)
        double ay = 0.0; // Acceleration in y direction (m/s²)
        double az = 0.0; // Acceleration in z direction (m/s²)

        auto members() noexcept { return std::tie(ax, ay, az); }
        auto members() const noexcept { return std::tie(ax, ay, az); }

        // Magnitude of acceleration
        inline double magnitude() const noexcept { return std::sqrt(ax * ax + ay * ay + az * az); }

        inline double magnitude_2d() const noexcept { return std::sqrt(ax * ax + ay * ay); }

        // For force calculations (F = m * a, requires mass externally)
        inline double magnitude_squared() const noexcept { return ax * ax + ay * ay + az * az; }

        // Utility
        inline bool is_set() const noexcept { return ax != 0.0 || ay != 0.0 || az != 0.0; }

        // Operators - acceleration arithmetic
        inline Acceleration operator+(const Acceleration &other) const noexcept {
            return Acceleration{ax + other.ax, ay + other.ay, az + other.az};
        }

        inline Acceleration operator-(const Acceleration &other) const noexcept {
            return Acceleration{ax - other.ax, ay - other.ay, az - other.az};
        }

        inline Acceleration operator*(double scale) const noexcept {
            return Acceleration{ax * scale, ay * scale, az * scale};
        }

        inline Acceleration operator/(double scale) const noexcept {
            return Acceleration{ax / scale, ay / scale, az / scale};
        }

        inline bool operator==(const Acceleration &other) const noexcept {
            return ax == other.ax && ay == other.ay && az == other.az;
        }

        inline bool operator!=(const Acceleration &other) const noexcept { return !(*this == other); }

        // Conversion to/from mat::vector for SIMD operations
        inline mat::vector<double, 3> to_mat() const noexcept { return mat::vector<double, 3>{ax, ay, az}; }

        static inline Acceleration from_mat(const mat::vector<double, 3> &v) noexcept {
            return Acceleration{v[0], v[1], v[2]};
        }
    };

} // namespace datapod
