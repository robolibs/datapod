#pragma once

#include "../box.hpp"

namespace datapod {
    namespace gaussian {

        /**
         * @brief Gaussian Box with uncertainty (POD)
         *
         * Pure aggregate struct - no constructors, no methods.
         * Represents a box with Gaussian uncertainty value.
         * Fully serializable and reflectable.
         */
        struct Box {
            datapod::Box box;
            double uncertainty = 0.0;
        };

    } // namespace gaussian
} // namespace datapod
