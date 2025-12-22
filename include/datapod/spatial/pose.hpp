#pragma once

#include <tuple>

#include "euler.hpp"
#include "point.hpp"

namespace datapod {

    struct Pose {
        Point point;
        Euler angle;

        auto members() noexcept { return std::tie(point, angle); }
    };

} // namespace datapod
