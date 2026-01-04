#pragma once

#include <cmath>
#include <tuple>

#include "datapod/pods/sequential/vector.hpp"
#include "point.hpp"

namespace datapod {

    /**
     * @brief Closed ring (polygon boundary) defined by points (POD)
     *
     * Ring represents a closed loop in space, typically used as the
     * boundary of a polygon (exterior ring) or as holes (interior rings).
     *
     * Convention: First point should equal last point to close the ring.
     * This is not enforced by the struct but is expected by algorithms.
     *
     * Unlike Linestring, a Ring is closed (first == last point).
     * Unlike Polygon, a Ring is just the boundary (no interior/exterior distinction).
     *
     * Features:
     * - POD-compatible
     * - Serializable via members()
     * - Use datapod::Vector for serializability
     *
     * Note: Uses datapod::Vector instead of std::vector for serializability.
     */
    struct Ring {
        Vector<Point> points; ///< Points forming closed ring (first == last)

        auto members() noexcept { return std::tie(points); }
        auto members() const noexcept { return std::tie(points); }

        // Geometric properties
        inline double length() const noexcept {
            if (points.size() < 2)
                return 0.0;
            double total = 0.0;
            for (size_t i = 1; i < points.size(); ++i) {
                total += points[i - 1].distance_to(points[i]);
            }
            return total;
        }

        inline double area() const noexcept {
            if (points.size() < 3)
                return 0.0;
            // Shoelace formula for polygon area
            double sum = 0.0;
            for (size_t i = 0; i < points.size() - 1; ++i) {
                sum += points[i].x * points[i + 1].y - points[i + 1].x * points[i].y;
            }
            return std::abs(sum) * 0.5;
        }

        // Utility
        inline size_t num_points() const noexcept { return points.size(); }
        inline bool empty() const noexcept { return points.empty(); }
        inline bool is_closed() const noexcept { return points.size() >= 3 && points.front() == points.back(); }
    };

} // namespace datapod
