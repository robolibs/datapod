#pragma once

#include <tuple>

#include "../point.hpp"

namespace datapod {

    struct Triangle {
        Point a;
        Point b;
        Point c;

        auto members() noexcept { return std::tie(a, b, c); }
    };

} // namespace datapod
