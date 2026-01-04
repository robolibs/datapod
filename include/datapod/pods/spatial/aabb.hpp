#pragma once

#include <algorithm>
#include <cmath>
#include <tuple>

#include "../matrix/vector.hpp"
#include "point.hpp"

namespace datapod {

    /**
     * @brief Axis-Aligned Bounding Box (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Use aggregate initialization: AABB{min_point, max_point}
     * Fully serializable and reflectable.
     */
    struct AABB {
        Point min_point;
        Point max_point;

        auto members() noexcept { return std::tie(min_point, max_point); }
        auto members() const noexcept { return std::tie(min_point, max_point); }

        // Get the center of the AABB
        inline Point center() const noexcept {
            return Point{(min_point.x + max_point.x) / 2.0, (min_point.y + max_point.y) / 2.0,
                         (min_point.z + max_point.z) / 2.0};
        }

        // Calculate the volume of the AABB
        inline double volume() const noexcept {
            double dx = max_point.x - min_point.x;
            double dy = max_point.y - min_point.y;
            double dz = max_point.z - min_point.z;
            return dx * dy * dz;
        }

        // Calculate the surface area of the AABB
        inline double surface_area() const noexcept {
            double dx = max_point.x - min_point.x;
            double dy = max_point.y - min_point.y;
            double dz = max_point.z - min_point.z;
            return 2.0 * (dx * dy + dy * dz + dz * dx);
        }

        // Check if a point is inside the AABB (inclusive of boundaries)
        inline bool contains(const Point &p) const noexcept {
            return p.x >= min_point.x && p.x <= max_point.x && p.y >= min_point.y && p.y <= max_point.y &&
                   p.z >= min_point.z && p.z <= max_point.z;
        }

        // Check if this AABB intersects with another AABB
        inline bool intersects(const AABB &other) const noexcept {
            // No overlap if separated along any axis
            if (max_point.x < other.min_point.x || min_point.x > other.max_point.x)
                return false;
            if (max_point.y < other.min_point.y || min_point.y > other.max_point.y)
                return false;
            if (max_point.z < other.min_point.z || min_point.z > other.max_point.z)
                return false;
            return true;
        }

        // Expand the AABB to include a point (mutating operation)
        inline void expand(const Point &p) noexcept {
            min_point.x = std::min(min_point.x, p.x);
            min_point.y = std::min(min_point.y, p.y);
            min_point.z = std::min(min_point.z, p.z);
            max_point.x = std::max(max_point.x, p.x);
            max_point.y = std::max(max_point.y, p.y);
            max_point.z = std::max(max_point.z, p.z);
        }

        // Expand the AABB to include another AABB (mutating operation)
        inline void expand(const AABB &other) noexcept {
            min_point.x = std::min(min_point.x, other.min_point.x);
            min_point.y = std::min(min_point.y, other.min_point.y);
            min_point.z = std::min(min_point.z, other.min_point.z);
            max_point.x = std::max(max_point.x, other.max_point.x);
            max_point.y = std::max(max_point.y, other.max_point.y);
            max_point.z = std::max(max_point.z, other.max_point.z);
        }

        // Distance from AABB to a point (0 if point is inside)
        inline double distance_to_point(const Point &p) const noexcept {
            double dx = std::max({min_point.x - p.x, 0.0, p.x - max_point.x});
            double dy = std::max({min_point.y - p.y, 0.0, p.y - max_point.y});
            double dz = std::max({min_point.z - p.z, 0.0, p.z - max_point.z});
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        }

        // Comparison operators
        inline bool operator==(const AABB &other) const noexcept {
            return min_point == other.min_point && max_point == other.max_point;
        }

        inline bool operator!=(const AABB &other) const noexcept { return !(*this == other); }

        // SIMD conversion: AABB → mat::Vector<double, 6> (min_x, min_y, min_z, max_x, max_y, max_z)
        inline mat::Vector<double, 6> to_mat() const noexcept {
            mat::Vector<double, 6> v;
            v[0] = min_point.x;
            v[1] = min_point.y;
            v[2] = min_point.z;
            v[3] = max_point.x;
            v[4] = max_point.y;
            v[5] = max_point.z;
            return v;
        }

        // SIMD conversion: mat::Vector<double, 6> → AABB
        static inline AABB from_mat(const mat::Vector<double, 6> &v) noexcept {
            return AABB{Point{v[0], v[1], v[2]}, Point{v[3], v[4], v[5]}};
        }
    };

} // namespace datapod
