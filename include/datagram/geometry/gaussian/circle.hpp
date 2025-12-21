#pragma once

#include "../primitives/circle.hpp"

namespace datagram {
    namespace gaussian {

        /**
         * @brief Gaussian Circle with uncertainty (POD)
         *
         * Pure aggregate struct - no constructors, no methods.
         * Represents a circle with Gaussian uncertainty value.
         * Fully serializable and reflectable.
         */
        struct Circle {
            datagram::Circle circle;
            double uncertainty = 0.0;
        };

    } // namespace gaussian
} // namespace datagram
