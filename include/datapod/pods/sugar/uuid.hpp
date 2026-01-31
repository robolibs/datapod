#pragma once

#include <random>
#include <tuple>

#include "datapod/pods/sequential/array.hpp"
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
        } // namespace uuid

    } // namespace sugar
} // namespace datapod
