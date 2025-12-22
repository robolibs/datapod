#pragma once

#include <tuple>

#include "datapod/sequential/vector.hpp"
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
    };

} // namespace datapod
