#pragma once

#include <tuple>

#include "../complex/polygon.hpp"
#include "datapod/pods/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief Collection of multiple polygons (POD)
     *
     * MultiPolygon represents a collection of multiple polygonal regions
     * in space. Each polygon is independent and may or may not overlap
     * or touch others.
     *
     * Commonly used for representing countries with islands, property
     * parcels, or any collection of distinct areas.
     *
     * Features:
     * - POD-compatible
     * - Serializable via members()
     * - Use datapod::Vector for serializability
     *
     * Note: Uses datapod::Vector instead of std::vector for serializability.
     */
    struct MultiPolygon {
        Vector<Polygon> polygons;

        auto members() noexcept { return std::tie(polygons); }
        auto members() const noexcept { return std::tie(polygons); }

        inline size_t size() const noexcept { return polygons.size(); }
        inline bool empty() const noexcept { return polygons.empty(); }
    };

    namespace multi_polygon {
        /// Placeholder for container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace multi_polygon

} // namespace datapod
