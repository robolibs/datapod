#pragma once

#include "../primitives/rectangle.hpp"

namespace datagram {
    namespace gaussian {

        /**
         * @brief Gaussian Rectangle with uncertainty (POD)
         *
         * Pure aggregate struct - no constructors, no methods.
         * Represents a rectangle with Gaussian uncertainty value.
         * Fully serializable and reflectable.
         */
        struct Rectangle {
            datagram::Rectangle rectangle;
            double uncertainty = 0.0;
        };

    } // namespace gaussian
} // namespace datagram
