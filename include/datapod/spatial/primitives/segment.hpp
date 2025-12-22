#pragma once

#include <cmath>
#include <tuple>

#include "../point.hpp"

namespace datapod {

    /**
     * @brief Finite line segment between two points
     *
     * Segment represents a finite line between a start and end point.
     * Use this for bounded line segments (edges, rays with endpoints, etc.)
     *
     * For infinite lines, use Line (point + direction).
     *
     * Features:
     * - POD-compatible
     * - Serializable via members()
     * - Can compute length, midpoint, distance, etc.
     */
    struct Segment {
        Point start; ///< Start point
        Point end;   ///< End point

        auto members() noexcept { return std::tie(start, end); }
        auto members() const noexcept { return std::tie(start, end); }

        // Geometric properties
        inline double length() const noexcept { return start.distance_to(end); }

        inline Point midpoint() const noexcept {
            return Point{(start.x + end.x) * 0.5, (start.y + end.y) * 0.5, (start.z + end.z) * 0.5};
        }

        // Distance queries
        inline Point closest_point(const Point &p) const noexcept {
            const Point diff = end - start;
            const double len_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

            // Degenerate segment (start == end)
            if (len_sq < 1e-10) {
                return start;
            }

            const Point to_p = p - start;
            const double t = (to_p.x * diff.x + to_p.y * diff.y + to_p.z * diff.z) / len_sq;

            // Clamp t to [0, 1] to stay on segment
            if (t <= 0.0)
                return start;
            if (t >= 1.0)
                return end;

            return Point{start.x + t * diff.x, start.y + t * diff.y, start.z + t * diff.z};
        }

        inline double distance_to(const Point &p) const noexcept { return p.distance_to(closest_point(p)); }
    };

} // namespace datapod
