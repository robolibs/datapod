#pragma once

#include "euler.hpp"
#include "point.hpp"

namespace bitcon {

    struct Pose {
        Point point;
        Euler angle;
    };

} // namespace bitcon
