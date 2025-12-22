#pragma once

#include <tuple>

#include "euler.hpp"
#include "point.hpp"
#include "size.hpp"

namespace datapod {

    /**
     * @brief Oriented Bounding Box (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Use aggregate initialization: OBB{center, half_extents, orientation}
     * Fully serializable and reflectable.
     *
     * Note: Similar to Box, but uses half_extents instead of full size
     * and separate orientation instead of full Pose.
     */
    struct OBB {
        Point center;
        Size half_extents;
        Euler orientation;

        auto members() noexcept { return std::tie(center, half_extents, orientation); }
    };

} // namespace datapod
