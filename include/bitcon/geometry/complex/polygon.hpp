#pragma once

#include "../point.hpp"
#include "bitcon/containers/vector.hpp"

namespace bitcon {

    /**
     * @brief Polygon defined by a sequence of vertices (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Use aggregate initialization: Polygon{Vector<Point>{{p1, p2, p3}}}
     * Fully serializable and reflectable.
     *
     * Note: Uses bitcon::Vector instead of std::vector for serializability.
     */
    struct Polygon {
        Vector<Point> vertices;
    };

} // namespace bitcon
