#pragma once

#include "../box.hpp"

namespace datagram {
    namespace gaussian {

        /**
         * @brief Gaussian Box with uncertainty (POD)
         *
         * Pure aggregate struct - no constructors, no methods.
         * Represents a box with Gaussian uncertainty value.
         * Fully serializable and reflectable.
         */
        struct Box {
            datagram::Box box;
            double uncertainty = 0.0;
        };

    } // namespace gaussian
} // namespace datagram
