#pragma once

#include <tuple>

#include "../point.hpp"

namespace datapod {
    namespace gaussian {

        /**
         * @brief Gaussian Point with uncertainty (POD)
         *
         * Pure aggregate struct - no constructors, no methods.
         * Represents a point with Gaussian uncertainty value.
         * Fully serializable and reflectable.
         */
        struct Point {
            datapod::Point point;
            double uncertainty = 0.0;

            auto members() noexcept { return std::tie(point, uncertainty); }
            auto members() const noexcept { return std::tie(point, uncertainty); }
        };

        namespace point {
            /// Create a Gaussian point from point and uncertainty
            inline Point make(const datapod::Point &pt, double uncertainty) noexcept { return Point{pt, uncertainty}; }

            /// Create a Gaussian point from coordinates and uncertainty
            inline Point make(double x, double y, double z, double uncertainty) noexcept {
                return Point{datapod::Point{x, y, z}, uncertainty};
            }
        } // namespace point

    } // namespace gaussian
} // namespace datapod
