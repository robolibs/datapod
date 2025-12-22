#pragma once

#include "pose.hpp"
#include "size.hpp"

namespace bitcon {

    struct Box {
        Pose pose;
        Size size;
    };

} // namespace bitcon
