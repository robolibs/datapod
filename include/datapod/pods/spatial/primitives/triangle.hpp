#pragma once

#include <cmath>
#include <tuple>

#include "../../matrix/vector.hpp"
#include "../point.hpp"

namespace datapod {

    struct Triangle {
        Point a;
        Point b;
        Point c;

        auto members() noexcept { return std::tie(a, b, c); }
        auto members() const noexcept { return std::tie(a, b, c); }

        // Calculate the area of the triangle using the cross product formula
        inline double area() const noexcept {
            // Area = 0.5 * |AB × AC|
            Point ab = b - a;
            Point ac = c - a;

            // Cross product components
            double cross_x = ab.y * ac.z - ab.z * ac.y;
            double cross_y = ab.z * ac.x - ab.x * ac.z;
            double cross_z = ab.x * ac.y - ab.y * ac.x;

            // Magnitude of cross product
            double cross_magnitude = std::sqrt(cross_x * cross_x + cross_y * cross_y + cross_z * cross_z);

            return 0.5 * cross_magnitude;
        }

        // Calculate the perimeter of the triangle
        inline double perimeter() const noexcept {
            double ab = a.distance_to(b);
            double bc = b.distance_to(c);
            double ca = c.distance_to(a);
            return ab + bc + ca;
        }

        // Check if a point is inside the triangle (works for 2D and 3D coplanar triangles)
        inline bool contains(const Point &p) const noexcept {
            // Using the sign method (barycentric coordinates approach)
            // For a point to be inside, it must be on the same side of all three edges

            auto sign = [](const Point &p1, const Point &p2, const Point &p3) -> double {
                return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
            };

            double d1 = sign(p, a, b);
            double d2 = sign(p, b, c);
            double d3 = sign(p, c, a);

            bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

            // Point is inside if all signs are the same (not both negative and positive)
            return !(has_neg && has_pos);
        }

        // SIMD conversion: Triangle → mat::Vector<double, 9> (3 points × 3 components each)
        inline mat::Vector<double, 9> to_mat() const noexcept {
            mat::Vector<double, 9> v;
            v[0] = a.x;
            v[1] = a.y;
            v[2] = a.z;
            v[3] = b.x;
            v[4] = b.y;
            v[5] = b.z;
            v[6] = c.x;
            v[7] = c.y;
            v[8] = c.z;
            return v;
        }

        // SIMD conversion: mat::Vector<double, 9> → Triangle
        static inline Triangle from_mat(const mat::Vector<double, 9> &v) noexcept {
            return Triangle{Point{v[0], v[1], v[2]}, Point{v[3], v[4], v[5]}, Point{v[6], v[7], v[8]}};
        }
    };

    namespace triangle {
        /// Create a triangle from three points
        inline Triangle make(const Point &a, const Point &b, const Point &c) noexcept { return Triangle{a, b, c}; }

        /// Create a triangle from three point coordinates (3D)
        inline Triangle make(double ax, double ay, double az, double bx, double by, double bz, double cx, double cy,
                             double cz) noexcept {
            return Triangle{Point{ax, ay, az}, Point{bx, by, bz}, Point{cx, cy, cz}};
        }

        /// Create a 2D triangle (z = 0)
        inline Triangle make(double ax, double ay, double bx, double by, double cx, double cy) noexcept {
            return Triangle{Point{ax, ay, 0.0}, Point{bx, by, 0.0}, Point{cx, cy, 0.0}};
        }
    } // namespace triangle

} // namespace datapod
