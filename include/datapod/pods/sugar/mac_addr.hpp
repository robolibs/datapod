#pragma once

#include <cctype>
#include <cstdio>
#include <stdexcept>
#include <string_view>
#include <tuple>

#include "datapod/core/exception.hpp"
#include "datapod/pods/sequential/array.hpp"
#include "datapod/pods/sequential/string.hpp"
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

            /// Convert to string in standard MAC address format: XX:XX:XX:XX:XX:XX
            String to_string() const {
                char buf[18]; // 17 chars + null terminator
                auto hex = [](u8 v) -> char { return (v < 10) ? char('0' + v) : char('a' + (v - 10)); };

                dp::usize pos = 0;
                for (dp::usize i = 0; i < 6; ++i) {
                    buf[pos++] = hex(bytes[i] >> 4);
                    buf[pos++] = hex(bytes[i] & 0xF);
                    if (i < 5) {
                        buf[pos++] = ':';
                    }
                }
                buf[pos] = '\0';
                return String{buf};
            }

            /// Parse a MAC address from string in standard format.
            ///
            /// Supports formats:
            /// - XX:XX:XX:XX:XX:XX (lowercase or uppercase hex)
            /// - XX-XX-XX-XX-XX-XX (lowercase or uppercase hex)
            static MacAddr from_string(std::string_view s);
        };

        namespace mac_addr {
            inline MacAddr make(u8 a, u8 b, u8 c, u8 d, u8 e, u8 f) noexcept { return MacAddr{{a, b, c, d, e, f}}; }

            inline MacAddr make(const Array<u8, 6> &bytes) noexcept { return MacAddr{bytes}; }

            inline void skip_ws(std::string_view s, dp::usize &i) noexcept {
                while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
                    ++i;
                }
            }

            inline MacAddr from_string(std::string_view s) {
                dp::usize i = 0;
                skip_ws(s, i);

                auto hex_val = [](char c) -> int {
                    if (c >= '0' && c <= '9') {
                        return c - '0';
                    }
                    if (c >= 'a' && c <= 'f') {
                        return 10 + (c - 'a');
                    }
                    if (c >= 'A' && c <= 'F') {
                        return 10 + (c - 'A');
                    }
                    return -1;
                };

                Array<u8, 6> bytes{};

                for (dp::usize j = 0; j < 6; ++j) {
                    // Parse first hex digit
                    if (i >= s.size()) {
                        throw_exception(std::runtime_error{"MacAddr::from_string: unexpected end of input"});
                    }
                    int hv = hex_val(s[i]);
                    if (hv < 0) {
                        throw_exception(std::runtime_error{"MacAddr::from_string: invalid hex character"});
                    }
                    u8 byte_val = static_cast<u8>(hv << 4);
                    ++i;

                    // Parse second hex digit
                    if (i >= s.size()) {
                        throw_exception(std::runtime_error{"MacAddr::from_string: unexpected end of input"});
                    }
                    hv = hex_val(s[i]);
                    if (hv < 0) {
                        throw_exception(std::runtime_error{"MacAddr::from_string: invalid hex character"});
                    }
                    byte_val |= static_cast<u8>(hv);
                    ++i;

                    bytes[j] = byte_val;

                    // Parse separator (':' or '-') or end
                    if (j < 5) {
                        if (i >= s.size()) {
                            throw_exception(std::runtime_error{"MacAddr::from_string: expected separator"});
                        }
                        if (s[i] != ':' && s[i] != '-') {
                            throw_exception(std::runtime_error{"MacAddr::from_string: expected ':' or '-' separator"});
                        }
                        ++i;
                    }
                }

                skip_ws(s, i);
                if (i != s.size()) {
                    throw_exception(std::runtime_error{"MacAddr::from_string: extra characters after MAC address"});
                }

                return make(bytes);
            }
        } // namespace mac_addr

        inline MacAddr MacAddr::from_string(std::string_view s) { return mac_addr::from_string(s); }

    } // namespace sugar
} // namespace datapod
