#pragma once

#include <tuple>

#include "../state.hpp"
#include "datapod/sequential/vector.hpp"

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

        auto members() noexcept { return std::tie(states); }
        auto members() const noexcept { return std::tie(states); }

        inline size_t size() const noexcept { return states.size(); }
        inline bool empty() const noexcept { return states.empty(); }
    };

} // namespace datapod
