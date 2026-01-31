#pragma once

#include <tuple>

#include "datapod/pods/associative/map.hpp"
#include "identity.hpp"
#include "model.hpp"

namespace datapod {
    namespace robot {

        /**
         * @brief Robot - Identity + kinematic model wrapper (POD)
         */
        struct Robot {
            Identity id;
            Model model;
            Map<String, String> props;

            auto members() noexcept { return std::tie(id, model, props); }
            auto members() const noexcept { return std::tie(id, model, props); }

            inline bool operator==(const Robot &other) const noexcept {
                return id == other.id && model == other.model && props == other.props;
            }
            inline bool operator!=(const Robot &other) const noexcept { return !(*this == other); }
        };

        namespace robot {
            inline Robot make(const Identity &id, const Model &model, const Map<String, String> &props = {}) noexcept {
                return Robot{id, model, props};
            }
        } // namespace robot

    } // namespace robot
} // namespace datapod
