#pragma once

#include <cmath>
#include <tuple>

#include "../point.hpp"
#include "datapod/pods/matrix/vector.hpp"

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

    namespace circle {
        /// Create a 2D circle (z = 0)
        inline Circle make(double x, double y, double radius) noexcept { return Circle{Point{x, y, 0.0}, radius}; }

        /// Create a 3D circle
        inline Circle make(double x, double y, double z, double radius) noexcept {
            return Circle{Point{x, y, z}, radius};
        }

        /// Create a circle from center point and radius
        inline Circle make(const Point &center, double radius) noexcept { return Circle{center, radius}; }

        /// Create a unit circle at origin
        inline Circle unit() noexcept { return Circle{Point{0.0, 0.0, 0.0}, 1.0}; }
    } // namespace circle

} // namespace datapod
