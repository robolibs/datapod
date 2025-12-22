#pragma once

#include <tuple>

#include "point.hpp"

namespace datapod {

    /**
     * @brief Axis-Aligned Bounding Box (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Use aggregate initialization: AABB{min_point, max_point}
     * Fully serializable and reflectable.
     */
    struct AABB {
        Point min_point;
        Point max_point;

        auto members() noexcept { return std::tie(min_point, max_point); }
    };

} // namespace datapod
