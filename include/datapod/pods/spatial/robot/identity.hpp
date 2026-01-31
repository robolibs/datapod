#pragma once

#include <tuple>

#include "datapod/pods/sequential/string.hpp"
#include "datapod/pods/sugar/ip.hpp"
#include "datapod/pods/sugar/uuid.hpp"
#include "datapod/types/types.hpp"

namespace datapod {
    namespace robot {

        /**
         * @brief Identity - High-level robot identity and semantics (POD)
         */
        struct Identity {
            String name;
            sugar::UUID uuid;
            sugar::IP ip;
            u8 rci = 0;

            auto members() noexcept { return std::tie(name, uuid, ip, rci); }
            auto members() const noexcept { return std::tie(name, uuid, ip, rci); }

            inline bool operator==(const Identity &other) const noexcept {
                return name == other.name && uuid == other.uuid && ip == other.ip && rci == other.rci;
            }
            inline bool operator!=(const Identity &other) const noexcept { return !(*this == other); }
        };

        namespace identity {
            inline Identity make(const String &name, const sugar::UUID &uuid, const sugar::IP &ip = {}) noexcept {
                return Identity{name, uuid, ip, 0};
            }
        } // namespace identity

    } // namespace robot
} // namespace datapod
