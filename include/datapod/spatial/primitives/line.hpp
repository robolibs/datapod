#pragma once

#include <tuple>

#include "../point.hpp"

namespace datapod {

    /**
     * @brief Infinite line defined by origin point and direction vector
     *
     * Line represents an infinite line in 3D space, defined by a point
     * and a direction vector. Any point on the line can be expressed as:
     * P(t) = origin + t * direction (for any real t)
     *
     * Use this for infinite lines (rays, axes, etc.)
     * For bounded line segments, use Segment (start + end points).
     *
     * Features:
     * - POD-compatible
     * - Serializable via members()
     * - Direction vector should typically be normalized
     */
    struct Line {
        Point origin;    ///< Point on the line
        Point direction; ///< Direction vector (typically normalized)

        auto members() noexcept { return std::tie(origin, direction); }
        auto members() const noexcept { return std::tie(origin, direction); }
    };

} // namespace datapod
