#pragma once

#include "../point.hpp"
#include "datagram/containers/vector.hpp"

namespace datagram {

    /**
     * @brief Polygon defined by a sequence of vertices (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Use aggregate initialization: Polygon{Vector<Point>{{p1, p2, p3}}}
     * Fully serializable and reflectable.
     *
     * Note: Uses datagram::Vector instead of std::vector for serializability.
     */
    struct Polygon {
        Vector<Point> vertices;
    };

} // namespace datagram
