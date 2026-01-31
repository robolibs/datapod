#pragma once

#include <cctype>
#include <cstdio>
#include <string_view>
#include <tuple>

#include "datapod/pods/adapters/variant.hpp"
#include "datapod/pods/sequential/array.hpp"
#include "datapod/pods/sequential/string.hpp"
#include "datapod/types/types.hpp"

namespace datapod {
    namespace sugar {

        /**
         * @brief IP - Internet Protocol address (POD)
         *
         * A single address type that can represent either IPv4 or IPv6.
         * Internally stored as a datapod::Variant of two fixed-size byte arrays.
         */
        struct IP {
          private:
            struct V4 {
                Array<u8, 4> bytes;
                auto members() noexcept { return std::tie(bytes); }
                auto members() const noexcept { return std::tie(bytes); }
                inline bool operator==(const V4 &other) const noexcept { return bytes == other.bytes; }
                inline bool operator!=(const V4 &other) const noexcept { return !(*this == other); }
            };

            struct V6 {
                Array<u8, 16> bytes;
                auto members() noexcept { return std::tie(bytes); }
                auto members() const noexcept { return std::tie(bytes); }
                inline bool operator==(const V6 &other) const noexcept { return bytes == other.bytes; }
                inline bool operator!=(const V6 &other) const noexcept { return !(*this == other); }
            };

          public:
            Variant<V4, V6> addr;

            auto members() noexcept { return std::tie(addr); }
            auto members() const noexcept { return std::tie(addr); }

            inline bool is_v4() const noexcept { return holds_alternative<V4>(addr); }
            inline bool is_v6() const noexcept { return holds_alternative<V6>(addr); }

            inline const Array<u8, 4> *v4_bytes() const noexcept {
                return get_if<V4>(addr) ? &get<V4>(addr).bytes : nullptr;
            }

            inline const Array<u8, 16> *v6_bytes() const noexcept {
                return get_if<V6>(addr) ? &get<V6>(addr).bytes : nullptr;
            }

            inline bool operator==(const IP &other) const noexcept { return addr == other.addr; }
            inline bool operator!=(const IP &other) const noexcept { return !(*this == other); }

            static inline IP from_v4_bytes(const Array<u8, 4> &bytes) noexcept { return IP{V4{bytes}}; }
            static inline IP from_v6_bytes(const Array<u8, 16> &bytes) noexcept { return IP{V6{bytes}}; }

            /// Convert to string. IPv4 uses dotted decimal; IPv6 uses canonical hex with ':' groups.
            String to_string() const {
                if (auto b = v4_bytes()) {
                    char buf[16];
                    std::snprintf(buf, sizeof(buf), "%u.%u.%u.%u", (unsigned)(*b)[0], (unsigned)(*b)[1],
                                  (unsigned)(*b)[2], (unsigned)(*b)[3]);
                    return String{buf};
                }

                auto b6 = v6_bytes();
                if (!b6) {
                    return String{};
                }

                auto hex = [](u8 v) -> char { return (v < 10) ? char('0' + v) : char('a' + (v - 10)); };

                // 8 groups of 16-bit in network order
                char buf[40];
                dp::usize pos = 0;
                for (dp::usize g = 0; g < 8; ++g) {
                    u16 w = (static_cast<u16>((*b6)[2 * g]) << 8) | static_cast<u16>((*b6)[2 * g + 1]);
                    // Always emit 4 hex digits for stable/parseable output.
                    buf[pos++] = hex(static_cast<u8>((w >> 12) & 0xF));
                    buf[pos++] = hex(static_cast<u8>((w >> 8) & 0xF));
                    buf[pos++] = hex(static_cast<u8>((w >> 4) & 0xF));
                    buf[pos++] = hex(static_cast<u8>(w & 0xF));
                    if (g != 7) {
                        buf[pos++] = ':';
                    }
                }
                buf[pos] = '\0';
                return String{buf};
            }

            /// Parse an IP address from string.
            ///
            /// Supports:
            /// - IPv4 dotted decimal (e.g. "192.168.0.1")
            /// - IPv6 full form (8 groups of 1-4 hex digits, e.g. "2001:db8:0:0:0:0:0:1")
            ///
            /// Does not currently support IPv6 "::" compression.
            static IP from_string(std::string_view s);
        };

        namespace ip {
            inline IP v4(u8 a, u8 b, u8 c, u8 d) noexcept { return IP::from_v4_bytes({a, b, c, d}); }

            inline IP v4(const Array<u8, 4> &bytes) noexcept { return IP::from_v4_bytes(bytes); }

            inline IP v6(const Array<u8, 16> &bytes) noexcept { return IP::from_v6_bytes(bytes); }

            inline void skip_ws(std::string_view s, dp::usize &i) noexcept {
                while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
                    ++i;
                }
            }

            inline bool parse_v4(std::string_view s, IP &out) noexcept {
                auto skip_ws = [&](dp::usize &i) { ip::skip_ws(s, i); };

                dp::usize i = 0;
                skip_ws(i);

                Array<u8, 4> b{0, 0, 0, 0};
                for (dp::usize part = 0; part < 4; ++part) {
                    if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i]))) {
                        return false;
                    }
                    unsigned val = 0;
                    unsigned digits = 0;
                    while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
                        val = val * 10u + static_cast<unsigned>(s[i] - '0');
                        if (val > 255u) {
                            return false;
                        }
                        ++i;
                        ++digits;
                        if (digits > 3) {
                            return false;
                        }
                    }
                    b[part] = static_cast<u8>(val);

                    if (part < 3) {
                        if (i >= s.size() || s[i] != '.') {
                            return false;
                        }
                        ++i;
                    }
                }

                skip_ws(i);
                if (i != s.size()) {
                    return false;
                }

                out = v4(b);
                return true;
            }

            inline bool parse_v6(std::string_view s, IP &out) noexcept {
                dp::usize i = 0;
                ip::skip_ws(s, i);

                Array<u8, 16> bytes{};

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

                for (dp::usize g = 0; g < 8; ++g) {
                    if (i >= s.size()) {
                        return false;
                    }

                    // parse 1..4 hex digits
                    unsigned w = 0;
                    unsigned digits = 0;
                    while (i < s.size()) {
                        int hv = hex_val(s[i]);
                        if (hv < 0) {
                            break;
                        }
                        w = (w << 4) | static_cast<unsigned>(hv);
                        ++i;
                        ++digits;
                        if (digits > 4) {
                            return false;
                        }
                    }

                    if (digits == 0) {
                        return false;
                    }

                    bytes[2 * g] = static_cast<u8>((w >> 8) & 0xFF);
                    bytes[2 * g + 1] = static_cast<u8>(w & 0xFF);

                    if (g < 7) {
                        if (i >= s.size() || s[i] != ':') {
                            return false;
                        }
                        ++i;
                    }
                }

                ip::skip_ws(s, i);
                if (i != s.size()) {
                    return false;
                }

                out = v6(bytes);
                return true;
            }

            inline IP from_string(std::string_view s) {
                IP out;
                if (parse_v4(s, out)) {
                    return out;
                }
                if (parse_v6(s, out)) {
                    return out;
                }
                throw_exception(std::runtime_error{"IP::from_string: invalid IP string"});
            }
        } // namespace ip

        inline IP IP::from_string(std::string_view s) { return ip::from_string(s); }

    } // namespace sugar
} // namespace datapod
