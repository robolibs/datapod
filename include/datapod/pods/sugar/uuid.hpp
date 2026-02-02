#pragma once

#include <cctype>
#include <random>
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
         * @brief UUID - 128-bit universally unique identifier (POD)
         *
         * Stored as 16 bytes. String conversion/generation is expected to be
         * provided by higher-level code; this is a portable storage POD.
         */
        struct UUID {
            Array<u8, 16> bytes;

            auto members() noexcept { return std::tie(bytes); }
            auto members() const noexcept { return std::tie(bytes); }

            inline bool operator==(const UUID &other) const noexcept { return bytes == other.bytes; }
            inline bool operator!=(const UUID &other) const noexcept { return !(*this == other); }

            /// Convert to string in standard UUID format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
            String to_string() const {
                char buf[37]; // 36 chars + null terminator
                auto hex = [](u8 v) -> char { return (v < 10) ? char('0' + v) : char('a' + (v - 10)); };

                // Format: 8-4-4-4-12 hex digits with hyphens
                dp::usize pos = 0;

                // First 8 hex digits (bytes 0-3)
                for (dp::usize i = 0; i < 4; ++i) {
                    buf[pos++] = hex(bytes[i] >> 4);
                    buf[pos++] = hex(bytes[i] & 0xF);
                }
                buf[pos++] = '-';

                // Next 4 hex digits (bytes 4-5)
                for (dp::usize i = 4; i < 6; ++i) {
                    buf[pos++] = hex(bytes[i] >> 4);
                    buf[pos++] = hex(bytes[i] & 0xF);
                }
                buf[pos++] = '-';

                // Next 4 hex digits (bytes 6-7)
                for (dp::usize i = 6; i < 8; ++i) {
                    buf[pos++] = hex(bytes[i] >> 4);
                    buf[pos++] = hex(bytes[i] & 0xF);
                }
                buf[pos++] = '-';

                // Next 4 hex digits (bytes 8-9)
                for (dp::usize i = 8; i < 10; ++i) {
                    buf[pos++] = hex(bytes[i] >> 4);
                    buf[pos++] = hex(bytes[i] & 0xF);
                }
                buf[pos++] = '-';

                // Last 12 hex digits (bytes 10-15)
                for (dp::usize i = 10; i < 16; ++i) {
                    buf[pos++] = hex(bytes[i] >> 4);
                    buf[pos++] = hex(bytes[i] & 0xF);
                }

                buf[pos] = '\0';
                return String{buf};
            }

            /// Parse a UUID from string in standard format.
            ///
            /// Supports format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (lowercase or uppercase).
            static UUID from_string(std::string_view s);
        };

        namespace uuid {
            /// Make a UUID from raw bytes.
            inline UUID make(const Array<u8, 16> &bytes) noexcept { return UUID{bytes}; }

            /// Make the all-zero UUID.
            inline UUID nil() noexcept { return UUID{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}; }

            /// Generate a random UUID (RFC 4122 version 4).
            ///
            /// Notes:
            /// - Uses std::random_device to seed a local PRNG.
            /// - Sets version (4) and variant (RFC 4122) bits.
            inline UUID generate_v4() {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<int> dist(0, 255);

                Array<u8, 16> b;
                for (auto &v : b) {
                    v = static_cast<u8>(dist(gen));
                }

                // Set version to 4 (0100)
                b[6] = static_cast<u8>((b[6] & 0x0F) | 0x40);
                // Set variant to RFC 4122 (10xx)
                b[8] = static_cast<u8>((b[8] & 0x3F) | 0x80);

                return make(b);
            }

            inline void skip_ws(std::string_view s, dp::usize &i) noexcept {
                while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
                    ++i;
                }
            }

            inline UUID from_string(std::string_view s) {
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

                auto parse_hex = [&](dp::usize count) -> u8 {
                    u8 result = 0;
                    for (dp::usize j = 0; j < count; ++j) {
                        if (i >= s.size()) {
                            throw_exception(std::runtime_error{"UUID::from_string: unexpected end of input"});
                        }
                        int hv = hex_val(s[i]);
                        if (hv < 0) {
                            throw_exception(std::runtime_error{"UUID::from_string: invalid hex character"});
                        }
                        result = static_cast<u8>((result << 4) | static_cast<u8>(hv));
                        ++i;
                    }
                    return result;
                };

                Array<u8, 16> bytes{};

                // Parse 8 hex digits
                for (dp::usize j = 0; j < 4; ++j) {
                    bytes[j] = parse_hex(2);
                }
                if (i >= s.size() || s[i] != '-') {
                    throw_exception(std::runtime_error{"UUID::from_string: expected '-' after first 8 hex digits"});
                }
                ++i;

                // Parse 4 hex digits
                for (dp::usize j = 0; j < 2; ++j) {
                    bytes[4 + j] = parse_hex(2);
                }
                if (i >= s.size() || s[i] != '-') {
                    throw_exception(std::runtime_error{"UUID::from_string: expected '-' after second 4 hex digits"});
                }
                ++i;

                // Parse 4 hex digits
                for (dp::usize j = 0; j < 2; ++j) {
                    bytes[6 + j] = parse_hex(2);
                }
                if (i >= s.size() || s[i] != '-') {
                    throw_exception(std::runtime_error{"UUID::from_string: expected '-' after third 4 hex digits"});
                }
                ++i;

                // Parse 4 hex digits
                for (dp::usize j = 0; j < 2; ++j) {
                    bytes[8 + j] = parse_hex(2);
                }
                if (i >= s.size() || s[i] != '-') {
                    throw_exception(std::runtime_error{"UUID::from_string: expected '-' after fourth 4 hex digits"});
                }
                ++i;

                // Parse 12 hex digits
                for (dp::usize j = 0; j < 6; ++j) {
                    bytes[10 + j] = parse_hex(2);
                }

                skip_ws(s, i);
                if (i != s.size()) {
                    throw_exception(std::runtime_error{"UUID::from_string: extra characters after UUID"});
                }

                return make(bytes);
            }
        } // namespace uuid

        inline UUID UUID::from_string(std::string_view s) { return uuid::from_string(s); }

    } // namespace sugar
} // namespace datapod
