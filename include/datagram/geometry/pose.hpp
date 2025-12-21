#pragma once

#include "euler.hpp"
#include "point.hpp"

namespace datagram {

    struct Pose {
        Point point;
        Euler angle;
    };

} // namespace datagram
