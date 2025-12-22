#pragma once

#include <tuple>

#include "datapod/sequential/vector.hpp"
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
    };

} // namespace datapod
