#pragma once

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
     * - Can compute length, midpoint, etc.
     */
    struct Segment {
        Point start; ///< Start point
        Point end;   ///< End point

        auto members() noexcept { return std::tie(start, end); }
        auto members() const noexcept { return std::tie(start, end); }
    };

} // namespace datapod
