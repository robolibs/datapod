#pragma once

#include <cmath>
#include <tuple>

#include "datapod/matrix/vector.hpp"

namespace datapod {

    /**
     * @brief 3D point with double precision coordinates (POD)
     *
     * Pure aggregate struct with geometric utility methods.
     * Use aggregate initialization: Point{1.0, 2.0, 3.0}
     * Fully serializable and reflectable.
     */
    struct Point {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;

        auto members() noexcept { return std::tie(x, y, z); }
        auto members() const noexcept { return std::tie(x, y, z); }

        // Distance and magnitude
        inline double magnitude() const noexcept { return std::sqrt(x * x + y * y + z * z); }

        inline double distance_to(const Point &other) const noexcept {
            const double dx = x - other.x;
            const double dy = y - other.y;
            const double dz = z - other.z;
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        }

        inline double distance_to_2d(const Point &other) const noexcept {
            const double dx = x - other.x;
            const double dy = y - other.y;
            return std::sqrt(dx * dx + dy * dy);
        }

        // Utility
        inline bool is_set() const noexcept { return x != 0.0 || y != 0.0 || z != 0.0; }

        // Operators
        inline Point operator+(const Point &other) const noexcept {
            return Point{x + other.x, y + other.y, z + other.z};
        }

        inline Point operator-(const Point &other) const noexcept {
            return Point{x - other.x, y - other.y, z - other.z};
        }

        inline Point operator*(double scale) const noexcept { return Point{x * scale, y * scale, z * scale}; }

        inline Point operator/(double scale) const noexcept { return Point{x / scale, y / scale, z / scale}; }

        inline bool operator==(const Point &other) const noexcept {
            return x == other.x && y == other.y && z == other.z;
        }

        inline bool operator!=(const Point &other) const noexcept { return !(*this == other); }

        // Conversion to/from mat::vector for SIMD operations
        inline mat::vector<double, 3> to_mat() const noexcept { return mat::vector<double, 3>{x, y, z}; }

        static inline Point from_mat(const mat::vector<double, 3> &v) noexcept { return Point{v[0], v[1], v[2]}; }
    };

} // namespace datapod
