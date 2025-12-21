#pragma once

#include "pose.hpp"
#include "size.hpp"

namespace datagram {

    struct Box {
        Pose pose;
        Size size;
    };

} // namespace datagram
