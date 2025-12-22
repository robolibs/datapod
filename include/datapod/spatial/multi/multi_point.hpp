#pragma once

#include <tuple>

#include "../point.hpp"
#include "datapod/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief Collection of multiple points (POD)
     *
     * MultiPoint represents a collection of discrete points in space.
     * Commonly used for representing scattered data, point clouds,
     * or sets of locations.
     *
     * Features:
     * - POD-compatible
     * - Serializable via members()
     * - Use datapod::Vector for serializability
     *
     * Note: Uses datapod::Vector instead of std::vector for serializability.
     */
    struct MultiPoint {
        Vector<Point> points;

        auto members() noexcept { return std::tie(points); }
        auto members() const noexcept { return std::tie(points); }

        inline size_t size() const noexcept { return points.size(); }
        inline bool empty() const noexcept { return points.empty(); }
    };

} // namespace datapod
