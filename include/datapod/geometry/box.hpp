#pragma once

#include "pose.hpp"
#include "size.hpp"

namespace datapod {

    struct Box {
        Pose pose;
        Size size;
    };

} // namespace datapod
