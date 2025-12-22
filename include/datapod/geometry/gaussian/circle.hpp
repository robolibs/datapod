#pragma once

#include <tuple>

#include "../primitives/circle.hpp"

namespace datapod {
    namespace gaussian {

        /**
         * @brief Gaussian Circle with uncertainty (POD)
         *
         * Pure aggregate struct - no constructors, no methods.
         * Represents a circle with Gaussian uncertainty value.
         * Fully serializable and reflectable.
         */
        struct Circle {
            datapod::Circle circle;
            double uncertainty = 0.0;

            auto members() noexcept { return std::tie(circle, uncertainty); }
        };

    } // namespace gaussian
} // namespace datapod
