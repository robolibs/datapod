#pragma once

#include <cmath>
#include <tuple>

#include "datapod/matrix/vector.hpp"

namespace datapod {

    /**
     * @brief 3D dimensions with double precision (POD)
     *
     * Pure aggregate struct with geometric utility methods.
     * Use aggregate initialization: Size{10.0, 20.0, 30.0}
     * Fully serializable and reflectable.
     */
    struct Size {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;

        auto members() noexcept { return std::tie(x, y, z); }
        auto members() const noexcept { return std::tie(x, y, z); }

        // Volume and area
        inline double volume() const noexcept { return x * y * z; }
        inline double area_xy() const noexcept { return x * y; }
        inline double area_xz() const noexcept { return x * z; }
        inline double area_yz() const noexcept { return y * z; }
        inline double diagonal() const noexcept { return std::sqrt(x * x + y * y + z * z); }
        inline double diagonal_2d() const noexcept { return std::sqrt(x * x + y * y); }

        // Utility
        inline bool is_set() const noexcept { return x != 0.0 || y != 0.0 || z != 0.0; }

        // Operators
        inline Size operator+(const Size &other) const noexcept { return Size{x + other.x, y + other.y, z + other.z}; }

        inline Size operator-(const Size &other) const noexcept { return Size{x - other.x, y - other.y, z - other.z}; }

        inline Size operator*(double scale) const noexcept { return Size{x * scale, y * scale, z * scale}; }

        inline Size operator/(double scale) const noexcept { return Size{x / scale, y / scale, z / scale}; }

        inline Size operator*(const Size &other) const noexcept { return Size{x * other.x, y * other.y, z * other.z}; }

        inline bool operator==(const Size &other) const noexcept {
            return x == other.x && y == other.y && z == other.z;
        }

        inline bool operator!=(const Size &other) const noexcept { return !(*this == other); }

        // Min/max helpers
        inline Size abs() const noexcept { return Size{std::abs(x), std::abs(y), std::abs(z)}; }

        inline Size max(const Size &other) const noexcept {
            return Size{std::max(x, other.x), std::max(y, other.y), std::max(z, other.z)};
        }

        inline Size min(const Size &other) const noexcept {
            return Size{std::min(x, other.x), std::min(y, other.y), std::min(z, other.z)};
        }

        // Conversion to/from mat::vector for SIMD operations
        inline mat::vector<double, 3> to_mat() const noexcept { return mat::vector<double, 3>{x, y, z}; }

        static inline Size from_mat(const mat::vector<double, 3> &v) noexcept { return Size{v[0], v[1], v[2]}; }
    };

} // namespace datapod
