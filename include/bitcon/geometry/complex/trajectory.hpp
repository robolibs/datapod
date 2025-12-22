#pragma once

#include "../state.hpp"
#include "bitcon/containers/vector.hpp"

namespace bitcon {

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

} // namespace bitcon
