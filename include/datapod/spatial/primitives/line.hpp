#pragma once

#include <tuple>

#include "../point.hpp"

namespace datapod {

    struct Line {
        Point start;
        Point end;

        auto members() noexcept { return std::tie(start, end); }
    };

} // namespace datapod
