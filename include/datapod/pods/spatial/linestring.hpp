#pragma once

#include <tuple>

#include "datapod/pods/sequential/vector.hpp"
#include "point.hpp"

namespace datapod {

    /**
     * @brief Sequence of connected points forming a path (POD)
     *
     * Linestring represents an open path through space defined by a
     * sequence of connected points. Each consecutive pair of points
     * forms a segment.
     *
     * Unlike Ring, a Linestring is NOT closed (first != last point).
     * Unlike Path, a Linestring has no orientation/pose information.
     *
     * Features:
     * - POD-compatible
     * - Serializable via members()
     * - Use datapod::Vector for serializability
     *
     * Note: Uses datapod::Vector instead of std::vector for serializability.
     */
    struct Linestring {
        Vector<Point> points;

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

        // Utility
        inline size_t num_points() const noexcept { return points.size(); }
        inline bool empty() const noexcept { return points.empty(); }
    };

} // namespace datapod
