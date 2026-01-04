#pragma once

#include <cassert>
#include <cinttypes>
#include <iosfwd>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "datapod/core/bit_counting.hpp"
#include "datapod/sequential/array.hpp"

namespace datapod {

    template <std::size_t Size> struct Bitset {
        using block_t = std::uint64_t;
        static constexpr auto const bits_per_block = sizeof(block_t) * 8U;
        static constexpr auto const num_blocks = Size / bits_per_block + (Size % bits_per_block == 0U ? 0U : 1U);

        constexpr Bitset() noexcept = default;
        constexpr Bitset(std::string_view s) noexcept { set(s); }

        static constexpr Bitset max() {
            Bitset ret;
            for (auto &b : ret.blocks_) {
                b = std::numeric_limits<block_t>::max();
            }
            return ret;
        }

        auto members() noexcept { return std::tie(blocks_); }

        void zero_out() {
            for (auto &b : blocks_) {
                b = 0U;
            }
        }

        void one_out() {
            for (auto &b : blocks_) {
                b = ~block_t{0};
            }
        }

        constexpr Bitset &set(std::string_view s) noexcept {
            for (std::size_t i = 0U; i != std::min(Size, s.size()); ++i) {
                set(i, s[s.size() - i - 1U] != '0');
            }
            return *this;
        }

        constexpr Bitset &set(std::size_t const i, bool const val = true) noexcept {
            assert((i / bits_per_block) < num_blocks);
            auto &block = blocks_[i / bits_per_block];
            auto const bit = i % bits_per_block;
            auto const mask = block_t{1U} << bit;
            if (val) {
                block |= mask;
            } else {
                block &= (~block_t{0U} ^ mask);
            }
            return *this;
        }

        // Set all bits to true
        constexpr Bitset &set() noexcept {
            one_out();
            return *this;
        }

        // Reset single bit to false
        constexpr Bitset &reset(std::size_t const i) noexcept {
            set(i, false);
            return *this;
        }

        // Reset all bits to false
        void reset() noexcept { blocks_ = {}; }

        // Flip single bit
        constexpr Bitset &flip(std::size_t const i) noexcept {
            assert((i / bits_per_block) < num_blocks);
            auto &block = blocks_[i / bits_per_block];
            auto const bit = i % bits_per_block;
            block ^= (block_t{1U} << bit);
            return *this;
        }

        // Flip all bits
        constexpr Bitset &flip() noexcept {
            for (auto &b : blocks_) {
                b = ~b;
            }
            return *this;
        }

        bool operator[](std::size_t const i) const noexcept { return test(i); }

        std::size_t count() const noexcept {
            std::size_t sum = 0U;
            for (std::size_t i = 0U; i != num_blocks - 1U; ++i) {
                sum += popcount(blocks_[i]);
            }
            return sum + popcount(sanitized_last_block());
        }

        constexpr bool test(std::size_t const i) const noexcept {
            if (i >= Size) {
                return false;
            }
            auto const block = blocks_[i / bits_per_block];
            auto const bit = (i % bits_per_block);
            return (block & (block_t{1U} << bit)) != 0U;
        }

        std::size_t size() const noexcept { return Size; }

        bool any() const noexcept {
            for (std::size_t i = 0U; i != num_blocks - 1U; ++i) {
                if (blocks_[i] != 0U) {
                    return true;
                }
            }
            return sanitized_last_block() != 0U;
        }

        bool none() const noexcept { return !any(); }

        bool all() const noexcept {
            // Check all full blocks
            for (std::size_t i = 0U; i != num_blocks - 1U; ++i) {
                if (blocks_[i] != ~block_t{0U}) {
                    return false;
                }
            }
            // Check last block (may be partial)
            if constexpr ((Size % bits_per_block) != 0U) {
                auto const expected_mask = ~((~block_t{0U}) << (Size % bits_per_block));
                return (blocks_[num_blocks - 1U] & expected_mask) == expected_mask;
            } else {
                return blocks_[num_blocks - 1U] == ~block_t{0U};
            }
        }

        block_t sanitized_last_block() const noexcept {
            if constexpr ((Size % bits_per_block) != 0U) {
                return blocks_[num_blocks - 1U] & ~((~block_t{0U}) << (Size % bits_per_block));
            } else {
                return blocks_[num_blocks - 1U];
            }
        }

        template <typename Fn> void for_each_set_bit(Fn &&f) const {
            auto const check_block = [&](std::size_t const i, block_t const block) {
                if (block != 0U) {
                    for (auto bit = std::size_t{0U}; bit != bits_per_block; ++bit) {
                        if ((block & (block_t{1U} << bit)) != 0U) {
                            f(std::size_t{i * bits_per_block + bit});
                        }
                    }
                }
            };
            for (auto i = std::size_t{0U}; i != blocks_.size() - 1; ++i) {
                check_block(i, blocks_[i]);
            }
            check_block(blocks_.size() - 1, sanitized_last_block());
        }

        std::string to_string() const {
            std::string s{};
            s.resize(Size);
            for (std::size_t i = 0U; i != Size; ++i) {
                s[i] = test(Size - i - 1U) ? '1' : '0';
            }
            return s;
        }

        // Convert to unsigned long (throws if Size > 64 and value doesn't fit)
        unsigned long to_ulong() const {
            if constexpr (Size == 0U) {
                return 0UL;
            } else if constexpr (Size < 64U) {
                return static_cast<unsigned long>(blocks_[0U] & ((1ULL << Size) - 1ULL));
            } else if constexpr (Size == 64U) {
                return static_cast<unsigned long>(blocks_[0U]);
            } else {
                // Check if higher blocks are zero
                for (std::size_t i = 1U; i < num_blocks; ++i) {
                    if (blocks_[i] != 0U) {
                        throw std::overflow_error("bitset value cannot fit in unsigned long");
                    }
                }
                return static_cast<unsigned long>(blocks_[0U]);
            }
        }

        // Convert to unsigned long long (throws if Size > 64 and value doesn't fit)
        unsigned long long to_ullong() const {
            if constexpr (Size == 0U) {
                return 0ULL;
            } else if constexpr (Size < 64U) {
                return blocks_[0U] & ((1ULL << Size) - 1ULL);
            } else if constexpr (Size == 64U) {
                return blocks_[0U];
            } else {
                // Check if higher blocks are zero
                for (std::size_t i = 1U; i < num_blocks; ++i) {
                    if (blocks_[i] != 0U) {
                        throw std::overflow_error("bitset value cannot fit in unsigned long long");
                    }
                }
                return blocks_[0U];
            }
        }

        // Count number of 1 bits (alias for count())
        constexpr std::size_t count_ones() const noexcept { return count(); }

        // Count number of 0 bits
        constexpr std::size_t count_zeros() const noexcept { return Size - count(); }

        // Count leading zeros from MSB
        constexpr std::size_t leading_zeros() const noexcept {
            if constexpr (Size == 0U) {
                return 0U;
            }

            std::size_t total_lz = 0U;

            // Start from the most significant block
            for (std::size_t i = num_blocks; i-- > 0;) {
                auto const block = (i == num_blocks - 1U) ? sanitized_last_block() : blocks_[i];

                if (block != 0U) {
                    // Found a non-zero block
                    auto const lz = datapod::leading_zeros(block);

                    // For the last block, adjust for unused bits
                    if (i == num_blocks - 1U && (Size % bits_per_block) != 0U) {
                        auto const unused_bits = bits_per_block - (Size % bits_per_block);
                        return total_lz + (lz - unused_bits);
                    }

                    return total_lz + lz;
                }

                // This block is all zeros, count them
                if (i == num_blocks - 1U && (Size % bits_per_block) != 0U) {
                    total_lz += (Size % bits_per_block);
                } else {
                    total_lz += bits_per_block;
                }
            }

            return Size; // All zeros
        }

        // Count trailing zeros from LSB
        constexpr std::size_t trailing_zeros() const noexcept {
            if constexpr (Size == 0U) {
                return 0U;
            }

            // Start from the least significant block
            for (std::size_t i = 0U; i < num_blocks; ++i) {
                auto const block = blocks_[i];
                if (block != 0U) {
                    auto const tz = datapod::trailing_zeros(block);
                    return i * bits_per_block + tz;
                }
            }
            return Size; // All zeros
        }

        // Rotate bits left by n positions
        constexpr Bitset &rotate_left(std::size_t n) noexcept {
            if constexpr (Size == 0U) {
                return *this;
            }

            n %= Size; // Normalize rotation amount
            if (n == 0U) {
                return *this;
            }

            // Save the bits that will wrap around
            Bitset temp = (*this) >> (Size - n);
            (*this) <<= n;
            (*this) |= temp;

            // Sanitize last block if needed
            if constexpr ((Size % bits_per_block) != 0U) {
                blocks_[num_blocks - 1U] = sanitized_last_block();
            }

            return *this;
        }

        // Rotate bits right by n positions
        constexpr Bitset &rotate_right(std::size_t n) noexcept {
            if constexpr (Size == 0U) {
                return *this;
            }

            n %= Size; // Normalize rotation amount
            if (n == 0U) {
                return *this;
            }

            // Save the bits that will wrap around
            Bitset temp = (*this) << (Size - n);
            (*this) >>= n;
            (*this) |= temp;

            // Sanitize last block if needed
            if constexpr ((Size % bits_per_block) != 0U) {
                blocks_[num_blocks - 1U] = sanitized_last_block();
            }

            return *this;
        }

        friend bool operator==(Bitset const &a, Bitset const &b) noexcept {
            for (std::size_t i = 0U; i != num_blocks - 1U; ++i) {
                if (a.blocks_[i] != b.blocks_[i]) {
                    return false;
                }
            }
            return a.sanitized_last_block() == b.sanitized_last_block();
        }

        friend bool operator<(Bitset const &a, Bitset const &b) noexcept {
            auto const a_last = a.sanitized_last_block();
            auto const b_last = b.sanitized_last_block();
            if (a_last < b_last) {
                return true;
            }
            if (b_last < a_last) {
                return false;
            }

            if constexpr (num_blocks > 1) {
                for (std::size_t i = num_blocks - 1; i-- > 0;) {
                    auto const x = a.blocks_[i];
                    auto const y = b.blocks_[i];
                    if (x < y) {
                        return true;
                    }
                    if (y < x) {
                        return false;
                    }
                }
            }

            return false;
        }

        friend bool operator!=(Bitset const &a, Bitset const &b) noexcept { return !(a == b); }

        friend bool operator>(Bitset const &a, Bitset const &b) noexcept { return b < a; }

        friend bool operator<=(Bitset const &a, Bitset const &b) noexcept { return !(a > b); }

        friend bool operator>=(Bitset const &a, Bitset const &b) noexcept { return !(a < b); }

        Bitset &operator&=(Bitset const &o) noexcept {
            for (auto i = 0U; i < num_blocks; ++i) {
                blocks_[i] &= o.blocks_[i];
            }
            return *this;
        }

        Bitset &operator|=(Bitset const &o) noexcept {
            for (auto i = 0U; i < num_blocks; ++i) {
                blocks_[i] |= o.blocks_[i];
            }
            return *this;
        }

        Bitset &operator^=(Bitset const &o) noexcept {
            for (auto i = 0U; i < num_blocks; ++i) {
                blocks_[i] ^= o.blocks_[i];
            }
            return *this;
        }

        Bitset operator~() const noexcept {
            auto copy = *this;
            for (auto &b : copy.blocks_) {
                b = ~b;
            }
            return copy;
        }

        friend Bitset operator&(Bitset const &lhs, Bitset const &rhs) noexcept {
            auto copy = lhs;
            copy &= rhs;
            return copy;
        }

        friend Bitset operator|(Bitset const &lhs, Bitset const &rhs) noexcept {
            auto copy = lhs;
            copy |= rhs;
            return copy;
        }

        friend Bitset operator^(Bitset const &lhs, Bitset const &rhs) noexcept {
            auto copy = lhs;
            copy ^= rhs;
            return copy;
        }

        Bitset &operator>>=(std::size_t const shift) noexcept {
            if (shift >= Size) {
                reset();
                return *this;
            }

            if constexpr ((Size % bits_per_block) != 0U) {
                blocks_[num_blocks - 1U] = sanitized_last_block();
            }

            if constexpr (num_blocks == 1U) {
                blocks_[0U] >>= shift;
                return *this;
            } else {
                if (shift == 0U) {
                    return *this;
                }

                auto const shift_blocks = shift / bits_per_block;
                auto const shift_bits = shift % bits_per_block;
                auto const border = num_blocks - shift_blocks - 1U;

                if (shift_bits == 0U) {
                    for (std::size_t i = 0U; i <= border; ++i) {
                        blocks_[i] = blocks_[i + shift_blocks];
                    }
                } else {
                    for (std::size_t i = 0U; i < border; ++i) {
                        blocks_[i] = (blocks_[i + shift_blocks] >> shift_bits) |
                                     (blocks_[i + shift_blocks + 1] << (bits_per_block - shift_bits));
                    }
                    blocks_[border] = (blocks_[num_blocks - 1] >> shift_bits);
                }

                for (auto i = border + 1U; i != num_blocks; ++i) {
                    blocks_[i] = 0U;
                }

                return *this;
            }
        }

        Bitset &operator<<=(std::size_t const shift) noexcept {
            if (shift >= Size) {
                reset();
                return *this;
            }

            if constexpr (num_blocks == 1U) {
                blocks_[0U] <<= shift;
                return *this;
            } else {
                if (shift == 0U) {
                    return *this;
                }

                auto const shift_blocks = shift / bits_per_block;
                auto const shift_bits = shift % bits_per_block;

                if (shift_bits == 0U) {
                    for (auto i = std::size_t{num_blocks - 1}; i >= shift_blocks; --i) {
                        blocks_[i] = blocks_[i - shift_blocks];
                    }
                } else {
                    for (auto i = std::size_t{num_blocks - 1}; i != shift_blocks; --i) {
                        blocks_[i] = (blocks_[i - shift_blocks] << shift_bits) |
                                     (blocks_[i - shift_blocks - 1U] >> (bits_per_block - shift_bits));
                    }
                    blocks_[shift_blocks] = blocks_[0U] << shift_bits;
                }

                for (auto i = 0U; i != shift_blocks; ++i) {
                    blocks_[i] = 0U;
                }

                return *this;
            }
        }

        Bitset operator>>(std::size_t const i) const noexcept {
            auto copy = *this;
            copy >>= i;
            return copy;
        }

        Bitset operator<<(std::size_t const i) const noexcept {
            auto copy = *this;
            copy <<= i;
            return copy;
        }

        friend std::ostream &operator<<(std::ostream &out, Bitset const &b) { return out << b.to_string(); }

        Array<block_t, num_blocks> blocks_{};
    };

} // namespace datapod
