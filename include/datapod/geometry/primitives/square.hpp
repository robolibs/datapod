#pragma once

#include <tuple>

#include "../point.hpp"

namespace datapod {

    struct Square {
        Point center;
        double side = 0.0;

        auto members() noexcept { return std::tie(center, side); }
    };

} // namespace datapod
