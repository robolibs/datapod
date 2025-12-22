#pragma once

#include <tuple>

#include "../point.hpp"

namespace datapod {

    struct Circle {
        Point center;
        double radius = 0.0;

        auto members() noexcept { return std::tie(center, radius); }
    };

} // namespace datapod
