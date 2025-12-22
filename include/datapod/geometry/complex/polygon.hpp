#pragma once

#include <tuple>

#include "../point.hpp"
#include "datapod/containers/vector.hpp"

namespace datapod {

    /**
     * @brief Polygon defined by a sequence of vertices (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Use aggregate initialization: Polygon{Vector<Point>{{p1, p2, p3}}}
     * Fully serializable and reflectable.
     *
     * Note: Uses datapod::Vector instead of std::vector for serializability.
     */
    struct Polygon {
        Vector<Point> vertices;

        auto members() noexcept { return std::tie(vertices); }
    };

} // namespace datapod
