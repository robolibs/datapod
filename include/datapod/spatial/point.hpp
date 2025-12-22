#pragma once

#include <tuple>

namespace datapod {

    /**
     * @brief 3D point with double precision coordinates (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Use aggregate initialization: Point{1.0, 2.0, 3.0}
     * Fully serializable and reflectable.
     */
    struct Point {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;

        auto members() noexcept { return std::tie(x, y, z); }
    };

} // namespace datapod
