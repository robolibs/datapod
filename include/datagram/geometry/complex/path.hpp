#pragma once

#include "../pose.hpp"
#include "datagram/containers/vector.hpp"

namespace datagram {

    /**
     * @brief Path as sequence of poses (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Represents a path with position AND orientation at each waypoint.
     * Fully serializable and reflectable.
     */
    struct Path {
        Vector<Pose> waypoints;
    };

} // namespace datagram
