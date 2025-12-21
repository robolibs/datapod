#pragma once

#include "../state.hpp"
#include "datagram/containers/vector.hpp"

namespace datagram {

    /**
     * @brief Trajectory as sequence of states (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Represents a trajectory with pose AND velocities at each point.
     * Fully serializable and reflectable.
     */
    struct Trajectory {
        Vector<State> states;
    };

} // namespace datagram
