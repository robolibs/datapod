#pragma once

#include "pose.hpp"

namespace datapod {

    /**
     * @brief Robot/vehicle state with pose and velocities (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Use aggregate initialization: State{pose, 1.5, 0.3}
     * Fully serializable and reflectable.
     */
    struct State {
        Pose pose;
        double linear_velocity = 0.0;  // m/s
        double angular_velocity = 0.0; // rad/s
    };

} // namespace datapod
