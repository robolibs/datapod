#pragma once

#include <cmath>
#include <tuple>

#include "../../matrix/vector.hpp"
#include "../../sequential/array.hpp"
#include "../point.hpp"

namespace datapod {

    struct Square {
        Point center;
        double side = 0.0;

        auto members() noexcept { return std::tie(center, side); }
        auto members() const noexcept { return std::tie(center, side); }

        // Calculate the area of the square
        inline double area() const noexcept { return side * side; }

        // Calculate the perimeter of the square
        inline double perimeter() const noexcept { return 4.0 * side; }

        // Calculate the diagonal length of the square
        inline double diagonal() const noexcept { return side * std::sqrt(2.0); }

        // Check if a point is inside the square (2D check, ignores Z)
        inline bool contains(const Point &p) const noexcept {
            double half_side = side / 2.0;
            double dx = std::abs(p.x - center.x);
            double dy = std::abs(p.y - center.y);
            return dx <= half_side && dy <= half_side;
        }

        // Get all four corners of the square
        inline Array<Point, 4> get_corners() const noexcept {
            double half_side = side / 2.0;
            Array<Point, 4> corners;
            // Order: bottom-left, bottom-right, top-right, top-left
            corners[0] = Point{center.x - half_side, center.y - half_side, center.z};
            corners[1] = Point{center.x + half_side, center.y - half_side, center.z};
            corners[2] = Point{center.x + half_side, center.y + half_side, center.z};
            corners[3] = Point{center.x - half_side, center.y + half_side, center.z};
            return corners;
        }

        // SIMD conversion: Square → mat::Vector<double, 4> (center(3), side)
        inline mat::Vector<double, 4> to_mat() const noexcept {
            mat::Vector<double, 4> v;
            v[0] = center.x;
            v[1] = center.y;
            v[2] = center.z;
            v[3] = side;
            return v;
        }

        // SIMD conversion: mat::Vector<double, 4> → Square
        static inline Square from_mat(const mat::Vector<double, 4> &v) noexcept {
            return Square{Point{v[0], v[1], v[2]}, v[3]};
        }
    };

} // namespace datapod
