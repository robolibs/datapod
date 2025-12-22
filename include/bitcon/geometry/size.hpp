#pragma once

namespace bitcon {

    /**
     * @brief 3D dimensions with double precision (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Use aggregate initialization: Size{10.0, 20.0, 30.0}
     * Fully serializable and reflectable.
     */
    struct Size {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    };

} // namespace bitcon
