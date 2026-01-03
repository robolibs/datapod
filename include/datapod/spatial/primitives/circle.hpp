#pragma once

#include <cmath>
#include <tuple>

#include "../point.hpp"
#include "datapod/matrix/vector.hpp"

namespace datapod {

    struct Circle {
        Point center;
        double radius = 0.0;

        auto members() noexcept { return std::tie(center, radius); }
        auto members() const noexcept { return std::tie(center, radius); }

        // Geometric properties
        inline double area() const noexcept { return M_PI * radius * radius; }

        inline double perimeter() const noexcept { return 2.0 * M_PI * radius; }

        // Containment
        inline bool contains(const Point &p) const noexcept { return center.distance_to(p) <= radius; }

        // Conversion to/from mat::vector for SIMD operations (4 values: center + radius)
        inline mat::Vector<double, 4> to_mat() const noexcept {
            return mat::Vector<double, 4>{center.x, center.y, center.z, radius};
        }

        static inline Circle from_mat(const mat::Vector<double, 4> &v) noexcept {
            return Circle{Point{v[0], v[1], v[2]}, v[3]};
        }
    };

} // namespace datapod
