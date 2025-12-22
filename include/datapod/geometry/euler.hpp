#pragma once

#include <tuple>

namespace datapod {

    struct Euler {
        double roll = 0.0;
        double pitch = 0.0;
        double yaw = 0.0;

        auto members() noexcept { return std::tie(roll, pitch, yaw); }
    };

} // namespace datapod
