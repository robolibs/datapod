#pragma once

#include <tuple>

namespace datapod {

    struct Quaternion {
        double w = 1.0;
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;

        auto members() noexcept { return std::tie(w, x, y, z); }
    };

} // namespace datapod
