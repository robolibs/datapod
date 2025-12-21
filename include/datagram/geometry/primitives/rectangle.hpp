#pragma once

#include "../point.hpp"

namespace datagram {

    struct Rectangle {
        Point top_left;
        Point top_right;
        Point bottom_left;
        Point bottom_right;
    };

} // namespace datagram
