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

        namespace box {
            /// Create a Gaussian box from box and uncertainty
            inline Box make(const datapod::Box &b, double uncertainty) noexcept { return Box{b, uncertainty}; }
        } // namespace box

    } // namespace gaussian
} // namespace datapod
