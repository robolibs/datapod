#pragma once

#include <algorithm>
#include <cmath>
#include <tuple>

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
    };

} // namespace datapod
