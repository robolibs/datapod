#pragma once

#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

#include "bitcon/core/hash.hpp"
#include "bitcon/core/offset_t.hpp"
#include "bitcon/core/verify.hpp"
#include "bitcon/serialization/serialized_size.hpp"

namespace bitcon {

    // Byte buffer type (std::vector<uint8_t>)
    using ByteBuf = std::vector<std::uint8_t>;

    // Buffer target for serialization
    template <typename BufType = ByteBuf> struct Buf {
        Buf() = default;
        explicit Buf(BufType &&buf) : buf_{std::forward<BufType>(buf)} {}

        // Get address at offset
        std::uint8_t *addr(offset_t const offset) noexcept { return (&buf_[0U]) + offset; }

        // Get base address
        std::uint8_t *base() noexcept { return &buf_[0U]; }

        // Compute checksum from given start offset
        std::uint64_t checksum(offset_t const start = 0U) const noexcept {
            return hash(std::string_view{reinterpret_cast<char const *>(&buf_[static_cast<std::size_t>(start)]),
                                         buf_.size() - static_cast<std::size_t>(start)});
        }

        // Write a value at a specific position
        template <typename T> void write(std::size_t const pos, T const &val) {
            verify(buf_.size() >= pos + serialized_size<T>(), "out of bounds write");
            std::memcpy(&buf_[pos], &val, serialized_size<T>());
        }

        // Write raw data with optional alignment
        offset_t write(void const *ptr, std::size_t const num_bytes, std::size_t alignment = 0U) {
            auto start = static_cast<offset_t>(size());

            // Handle alignment
            if (alignment > 1U && buf_.size() != 0U) {
                auto unaligned_ptr = static_cast<void *>(addr(start));
                auto space = std::numeric_limits<std::size_t>::max();
                auto const aligned_ptr = std::align(alignment, num_bytes, unaligned_ptr, space);
                auto const new_offset =
                    static_cast<offset_t>(aligned_ptr ? static_cast<std::uint8_t *>(aligned_ptr) - base() : 0U);
                auto const adjustment = static_cast<offset_t>(new_offset - start);
                start += adjustment;
            }

            // Ensure buffer has enough space
            auto const space_left = static_cast<int64_t>(buf_.size()) - static_cast<int64_t>(start);
            if (space_left < static_cast<int64_t>(num_bytes)) {
                auto const missing = static_cast<std::size_t>(static_cast<int64_t>(num_bytes) - space_left);
                buf_.resize(buf_.size() + missing);
            }

            // Copy data
            std::memcpy(addr(start), ptr, num_bytes);

            return start;
        }

        // Array access operators
        std::uint8_t &operator[](std::size_t const i) noexcept { return buf_[i]; }
        std::uint8_t const &operator[](std::size_t const i) const noexcept { return buf_[i]; }

        // Get buffer size
        std::size_t size() const noexcept { return buf_.size(); }

        // Reset buffer
        void reset() { buf_.resize(0U); }

        BufType buf_;
    };

    // Deduction guide
    template <typename BufType> Buf(BufType &&) -> Buf<BufType>;

} // namespace bitcon
