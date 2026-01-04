#pragma once

#include <tuple>

#include "../primitives/rectangle.hpp"

namespace datapod {
    namespace gaussian {

        /**
         * @brief Gaussian Rectangle with uncertainty (POD)
         *
         * Pure aggregate struct - no constructors, no methods.
         * Represents a rectangle with Gaussian uncertainty value.
         * Fully serializable and reflectable.
         */
        struct Rectangle {
            datapod::Rectangle rectangle;
            double uncertainty = 0.0;

            auto members() noexcept { return std::tie(rectangle, uncertainty); }
            auto members() const noexcept { return std::tie(rectangle, uncertainty); }
        };

        namespace rectangle {
            /// Create a Gaussian rectangle from rectangle and uncertainty
            inline Rectangle make(const datapod::Rectangle &r, double uncertainty) noexcept {
                return Rectangle{r, uncertainty};
            }
        } // namespace rectangle

    } // namespace gaussian
} // namespace datapod
