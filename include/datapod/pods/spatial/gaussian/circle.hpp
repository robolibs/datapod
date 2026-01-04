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
            auto members() const noexcept { return std::tie(circle, uncertainty); }
        };

        namespace circle {
            /// Create a Gaussian circle from circle and uncertainty
            inline Circle make(const datapod::Circle &c, double uncertainty) noexcept { return Circle{c, uncertainty}; }

            /// Create a Gaussian circle from center, radius, and uncertainty
            inline Circle make(const datapod::Point &center, double radius, double uncertainty) noexcept {
                return Circle{datapod::Circle{center, radius}, uncertainty};
            }
        } // namespace circle

    } // namespace gaussian
} // namespace datapod
