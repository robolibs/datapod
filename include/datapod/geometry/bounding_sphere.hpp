#pragma once

#include "point.hpp"

namespace datapod {

    /**
     * @brief Bounding Sphere (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Use aggregate initialization: BoundingSphere{center, radius}
     * Fully serializable and reflectable.
     */
    struct BoundingSphere {
        Point center;
        double radius = 0.0;
    };

} // namespace datapod
