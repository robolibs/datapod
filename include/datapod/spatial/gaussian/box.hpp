#pragma once

#include <tuple>

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

            auto members() noexcept { return std::tie(box, uncertainty); }
            auto members() const noexcept { return std::tie(box, uncertainty); }
        };

    } // namespace gaussian
} // namespace datapod
