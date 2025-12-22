#pragma once

#include <tuple>

#include "../point.hpp"

namespace datapod {

    struct Rectangle {
        Point top_left;
        Point top_right;
        Point bottom_left;
        Point bottom_right;

        auto members() noexcept { return std::tie(top_left, top_right, bottom_left, bottom_right); }
    };

} // namespace datapod
