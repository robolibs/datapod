#pragma once

#include <cmath>
#include <tuple>

#include "../aabb.hpp"
#include "../point.hpp"
#include "datapod/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief Polygon defined by a sequence of vertices (POD)
     *
     * Pure aggregate struct with geometric methods.
     * Use aggregate initialization: Polygon{Vector<Point>{{p1, p2, p3}}}
     * Fully serializable and reflectable.
     *
     * Note: Uses datapod::Vector instead of std::vector for serializability.
     */
    struct Polygon {
        Vector<Point> vertices;

        auto members() noexcept { return std::tie(vertices); }
        auto members() const noexcept { return std::tie(vertices); }

        // Geometric properties
        inline double perimeter() const noexcept {
            if (vertices.size() < 2)
                return 0.0;
            double per = 0.0;
            for (size_t i = 1; i < vertices.size(); ++i) {
                per += vertices[i - 1].distance_to(vertices[i]);
            }
            // Close the polygon
            per += vertices.back().distance_to(vertices.front());
            return per;
        }

        inline double area() const noexcept {
            if (vertices.size() < 3)
                return 0.0;
            // Shoelace formula
            double a = 0.0;
            for (size_t i = 0, j = vertices.size() - 1; i < vertices.size(); j = i++) {
                const auto &pi = vertices[i];
                const auto &pj = vertices[j];
                a += (pj.x + pi.x) * (pj.y - pi.y);
            }
            return std::abs(a * 0.5);
        }

        // Ray casting algorithm for point containment
        inline bool contains(const Point &p) const noexcept {
            if (vertices.size() < 3)
                return false;
            bool c = false;
            for (size_t i = 0, j = vertices.size() - 1; i < vertices.size(); j = i++) {
                const auto &pi = vertices[i];
                const auto &pj = vertices[j];
                if (((pi.y > p.y) != (pj.y > p.y)) && (p.x < (pj.x - pi.x) * (p.y - pi.y) / (pj.y - pi.y) + pi.x)) {
                    c = !c;
                }
            }
            return c;
        }

        // Utility
        inline size_t num_vertices() const noexcept { return vertices.size(); }
        inline bool is_valid() const noexcept { return vertices.size() >= 3; }
        inline bool empty() const noexcept { return vertices.empty(); }

        // Bounding box
        inline AABB get_aabb() const noexcept {
            if (vertices.empty()) {
                return AABB{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
            }

            Point min_pt = vertices[0];
            Point max_pt = vertices[0];

            for (size_t i = 1; i < vertices.size(); ++i) {
                const auto &v = vertices[i];
                if (v.x < min_pt.x)
                    min_pt.x = v.x;
                if (v.y < min_pt.y)
                    min_pt.y = v.y;
                if (v.z < min_pt.z)
                    min_pt.z = v.z;
                if (v.x > max_pt.x)
                    max_pt.x = v.x;
                if (v.y > max_pt.y)
                    max_pt.y = v.y;
                if (v.z > max_pt.z)
                    max_pt.z = v.z;
            }

            return AABB{min_pt, max_pt};
        }
    };

} // namespace datapod
