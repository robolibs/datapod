#pragma once

#include "euler.hpp"
#include "point.hpp"

namespace datapod {

    struct Pose {
        Point point;
        Euler angle;
    };

} // namespace datapod
