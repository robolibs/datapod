#pragma once

#include "../state.hpp"
#include "datapod/containers/vector.hpp"

namespace datapod {

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

} // namespace datapod
