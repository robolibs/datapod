#pragma once

namespace datagram {

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
    };

} // namespace datagram
