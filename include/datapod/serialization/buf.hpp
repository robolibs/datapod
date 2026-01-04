#pragma once

#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

#include "datapod/core/hash.hpp"
#include "datapod/core/offset_t.hpp"
#include "datapod/core/verify.hpp"
#include "datapod/serialization/serialized_size.hpp"

namespace datapod {

    // Byte buffer type (std::vector<uint8_t>)
    using ByteBuf = std::vector<datapod::u8>;

    // Buffer target for serialization
    template <typename BufType = ByteBuf> struct Buf {
        Buf() = default;
        explicit Buf(BufType &&buf) : buf_{std::forward<BufType>(buf)} {}

        // Get address at offset
        datapod::u8 *addr(offset_t const offset) noexcept { return (&buf_[0U]) + offset; }

        // Get base address
        datapod::u8 *base() noexcept { return &buf_[0U]; }

        // Compute checksum from given start offset
        datapod::u64 checksum(offset_t const start = 0U) const noexcept {
            return hash(std::string_view{reinterpret_cast<char const *>(&buf_[static_cast<datapod::usize>(start)]),
                                         buf_.size() - static_cast<datapod::usize>(start)});
        }

        // Write a value at a specific position
        template <typename T> void write(datapod::usize const pos, T const &val) {
            verify(buf_.size() >= pos + serialized_size<T>(), "out of bounds write");
            std::memcpy(&buf_[pos], &val, serialized_size<T>());
        }

        // Write raw data with optional alignment
        offset_t write(void const *ptr, datapod::usize const num_bytes, datapod::usize alignment = 0U) {
            auto start = static_cast<offset_t>(size());

            // Handle alignment
            if (alignment > 1U && buf_.size() != 0U) {
                auto unaligned_ptr = static_cast<void *>(addr(start));
                auto space = static_cast<std::size_t>(std::numeric_limits<datapod::usize>::max());
                auto const aligned_ptr = std::align(alignment, num_bytes, unaligned_ptr, space);
                auto const new_offset =
                    static_cast<offset_t>(aligned_ptr ? static_cast<datapod::u8 *>(aligned_ptr) - base() : 0U);
                auto const adjustment = static_cast<offset_t>(new_offset - start);
                start += adjustment;
            }

            // Ensure buffer has enough space
            auto const space_left = static_cast<int64_t>(buf_.size()) - static_cast<int64_t>(start);
            if (space_left < static_cast<int64_t>(num_bytes)) {
                auto const missing = static_cast<datapod::usize>(static_cast<int64_t>(num_bytes) - space_left);
                buf_.resize(buf_.size() + missing);
            }

            // Copy data
            std::memcpy(addr(start), ptr, num_bytes);

            return start;
        }

        // Array access operators
        datapod::u8 &operator[](datapod::usize const i) noexcept { return buf_[i]; }
        datapod::u8 const &operator[](datapod::usize const i) const noexcept { return buf_[i]; }

        // Get buffer size
        datapod::usize size() const noexcept { return buf_.size(); }

        // Reset buffer
        void reset() { buf_.resize(0U); }

        BufType buf_;
    };

    // Deduction guide
    template <typename BufType> Buf(BufType &&) -> Buf<BufType>;

} // namespace datapod
