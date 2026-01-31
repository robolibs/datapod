#pragma once

#include <tuple>

#include "datapod/pods/sequential/array.hpp"
#include "datapod/types/types.hpp"

namespace datapod {
    namespace sugar {

        /**
         * @brief MacAddr - 48-bit Ethernet MAC address (POD)
         */
        struct MacAddr {
            Array<u8, 6> bytes;

            auto members() noexcept { return std::tie(bytes); }
            auto members() const noexcept { return std::tie(bytes); }

            inline bool operator==(const MacAddr &other) const noexcept { return bytes == other.bytes; }
            inline bool operator!=(const MacAddr &other) const noexcept { return !(*this == other); }
        };

        namespace mac_addr {
            inline MacAddr make(u8 a, u8 b, u8 c, u8 d, u8 e, u8 f) noexcept { return MacAddr{{a, b, c, d, e, f}}; }

            inline MacAddr make(const Array<u8, 6> &bytes) noexcept { return MacAddr{bytes}; }
        } // namespace mac_addr

    } // namespace sugar
} // namespace datapod
