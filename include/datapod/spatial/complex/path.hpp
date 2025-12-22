#pragma once

#include <tuple>

#include "../pose.hpp"
#include "datapod/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief Path as sequence of poses (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Represents a path with position AND orientation at each waypoint.
     * Fully serializable and reflectable.
     */
    struct Path {
        Vector<Pose> waypoints;

        auto members() noexcept { return std::tie(waypoints); }
        auto members() const noexcept { return std::tie(waypoints); }

        inline size_t size() const noexcept { return waypoints.size(); }
        inline bool empty() const noexcept { return waypoints.empty(); }
    };

} // namespace datapod
