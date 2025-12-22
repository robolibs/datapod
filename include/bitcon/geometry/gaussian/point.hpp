#pragma once

#include "../point.hpp"

namespace bitcon {
    namespace gaussian {

        /**
         * @brief Gaussian Point with uncertainty (POD)
         *
         * Pure aggregate struct - no constructors, no methods.
         * Represents a point with Gaussian uncertainty value.
         * Fully serializable and reflectable.
         */
        struct Point {
            bitcon::Point point;
            double uncertainty = 0.0;
        };

    } // namespace gaussian
} // namespace bitcon
