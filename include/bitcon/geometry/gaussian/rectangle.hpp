#pragma once

#include "../primitives/rectangle.hpp"

namespace bitcon {
    namespace gaussian {

        /**
         * @brief Gaussian Rectangle with uncertainty (POD)
         *
         * Pure aggregate struct - no constructors, no methods.
         * Represents a rectangle with Gaussian uncertainty value.
         * Fully serializable and reflectable.
         */
        struct Rectangle {
            bitcon::Rectangle rectangle;
            double uncertainty = 0.0;
        };

    } // namespace gaussian
} // namespace bitcon
