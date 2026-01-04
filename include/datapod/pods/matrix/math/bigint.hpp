#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>

namespace datapod {
    namespace mat {

        /**
         * @brief Fixed-size big integer with N 64-bit limbs - POD
         *
         * Represents integers up to N * 64 bits. Useful for cryptography,
         * extended precision arithmetic, and large number computations.
         * Limbs are stored in little-endian order (limbs[0] is least significant).
         * Fully serializable via members().
         *
         * Examples:
         *   Bigint<4> x;                      // 256-bit integer
         *   Bigint<4> y = Bigint<4>::from_u64(12345);
         *   auto z = x + y;
         *   auto product = x * y;
         */
        template <size_t N> struct Bigint {
            static_assert(N > 0, "Bigint requires at least one limb");

            using limb_type = uint64_t;
            static constexpr size_t num_limbs = N;
            static constexpr size_t total_bits = N * 64;
            static constexpr size_t rank = 0;

            std::array<uint64_t, N> limbs{}; // Little-endian: limbs[0] is LSB

            // ===== SERIALIZATION (required for datapod) =====
            auto members() noexcept { return std::tie(limbs); }
            auto members() const noexcept { return std::tie(limbs); }

            // Construction
            constexpr Bigint() noexcept = default;

            constexpr Bigint(uint64_t value) noexcept : limbs{} { limbs[0] = value; }

            constexpr Bigint(const std::array<uint64_t, N> &l) noexcept : limbs(l) {}

            // Factory from uint64
            static constexpr Bigint from_u64(uint64_t value) noexcept {
                Bigint result;
                result.limbs[0] = value;
                return result;
            }

            // Get as uint64 (truncates to lower 64 bits)
            constexpr uint64_t to_u64() const noexcept { return limbs[0]; }

            // Check if fits in uint64
            constexpr bool fits_u64() const noexcept {
                for (size_t i = 1; i < N; ++i) {
                    if (limbs[i] != 0)
                        return false;
                }
                return true;
            }

            // Properties
            constexpr bool is_zero() const noexcept {
                for (size_t i = 0; i < N; ++i) {
                    if (limbs[i] != 0)
                        return false;
                }
                return true;
            }

            constexpr bool is_one() const noexcept {
                if (limbs[0] != 1)
                    return false;
                for (size_t i = 1; i < N; ++i) {
                    if (limbs[i] != 0)
                        return false;
                }
                return true;
            }

            constexpr bool is_set() const noexcept { return !is_zero(); }

            // Get bit at position
            constexpr bool get_bit(size_t pos) const noexcept {
                if (pos >= total_bits)
                    return false;
                size_t limb_idx = pos / 64;
                size_t bit_idx = pos % 64;
                return (limbs[limb_idx] >> bit_idx) & 1;
            }

            // Set bit at position
            constexpr void set_bit(size_t pos, bool value = true) noexcept {
                if (pos >= total_bits)
                    return;
                size_t limb_idx = pos / 64;
                size_t bit_idx = pos % 64;
                if (value) {
                    limbs[limb_idx] |= (uint64_t{1} << bit_idx);
                } else {
                    limbs[limb_idx] &= ~(uint64_t{1} << bit_idx);
                }
            }

            // Count leading zeros
            constexpr size_t leading_zeros() const noexcept {
                for (size_t i = N; i > 0; --i) {
                    if (limbs[i - 1] != 0) {
                        size_t lz = 0;
                        uint64_t v = limbs[i - 1];
                        while (v != 0 && (v & (uint64_t{1} << 63)) == 0) {
                            ++lz;
                            v <<= 1;
                        }
                        return (N - i) * 64 + lz;
                    }
                }
                return total_bits;
            }

            // Bit width (position of highest set bit + 1)
            constexpr size_t bit_width() const noexcept { return total_bits - leading_zeros(); }

            // Addition with carry
            constexpr Bigint &operator+=(const Bigint &other) noexcept {
                uint64_t carry = 0;
                for (size_t i = 0; i < N; ++i) {
                    uint64_t sum = limbs[i] + other.limbs[i] + carry;
                    carry = (sum < limbs[i]) || (carry && sum == limbs[i]) ? 1 : 0;
                    limbs[i] = sum;
                }
                return *this;
            }

            // Subtraction with borrow
            constexpr Bigint &operator-=(const Bigint &other) noexcept {
                uint64_t borrow = 0;
                for (size_t i = 0; i < N; ++i) {
                    uint64_t prev = limbs[i];
                    limbs[i] = prev - other.limbs[i] - borrow;
                    borrow = (prev < other.limbs[i]) || (borrow && prev == other.limbs[i]) ? 1 : 0;
                }
                return *this;
            }

            // Multiplication (full result needs 2N limbs, we truncate)
            constexpr Bigint &operator*=(const Bigint &other) noexcept {
                *this = *this * other;
                return *this;
            }

            // Bitwise operations
            constexpr Bigint &operator&=(const Bigint &other) noexcept {
                for (size_t i = 0; i < N; ++i)
                    limbs[i] &= other.limbs[i];
                return *this;
            }

            constexpr Bigint &operator|=(const Bigint &other) noexcept {
                for (size_t i = 0; i < N; ++i)
                    limbs[i] |= other.limbs[i];
                return *this;
            }

            constexpr Bigint &operator^=(const Bigint &other) noexcept {
                for (size_t i = 0; i < N; ++i)
                    limbs[i] ^= other.limbs[i];
                return *this;
            }

            // Left shift
            constexpr Bigint &operator<<=(size_t shift) noexcept {
                if (shift >= total_bits) {
                    for (size_t i = 0; i < N; ++i)
                        limbs[i] = 0;
                    return *this;
                }
                size_t limb_shift = shift / 64;
                size_t bit_shift = shift % 64;

                if (limb_shift > 0) {
                    for (size_t i = N; i > limb_shift; --i) {
                        limbs[i - 1] = limbs[i - 1 - limb_shift];
                    }
                    for (size_t i = 0; i < limb_shift; ++i) {
                        limbs[i] = 0;
                    }
                }

                if (bit_shift > 0) {
                    uint64_t carry = 0;
                    for (size_t i = 0; i < N; ++i) {
                        uint64_t new_carry = limbs[i] >> (64 - bit_shift);
                        limbs[i] = (limbs[i] << bit_shift) | carry;
                        carry = new_carry;
                    }
                }
                return *this;
            }

            // Right shift
            constexpr Bigint &operator>>=(size_t shift) noexcept {
                if (shift >= total_bits) {
                    for (size_t i = 0; i < N; ++i)
                        limbs[i] = 0;
                    return *this;
                }
                size_t limb_shift = shift / 64;
                size_t bit_shift = shift % 64;

                if (limb_shift > 0) {
                    for (size_t i = 0; i < N - limb_shift; ++i) {
                        limbs[i] = limbs[i + limb_shift];
                    }
                    for (size_t i = N - limb_shift; i < N; ++i) {
                        limbs[i] = 0;
                    }
                }

                if (bit_shift > 0) {
                    uint64_t carry = 0;
                    for (size_t i = N; i > 0; --i) {
                        uint64_t new_carry = limbs[i - 1] << (64 - bit_shift);
                        limbs[i - 1] = (limbs[i - 1] >> bit_shift) | carry;
                        carry = new_carry;
                    }
                }
                return *this;
            }

            // Unary operators
            constexpr Bigint operator~() const noexcept {
                Bigint result;
                for (size_t i = 0; i < N; ++i)
                    result.limbs[i] = ~limbs[i];
                return result;
            }

            // Comparison
            constexpr bool operator==(const Bigint &other) const noexcept {
                for (size_t i = 0; i < N; ++i) {
                    if (limbs[i] != other.limbs[i])
                        return false;
                }
                return true;
            }

            constexpr bool operator!=(const Bigint &other) const noexcept { return !(*this == other); }

            constexpr bool operator<(const Bigint &other) const noexcept {
                for (size_t i = N; i > 0; --i) {
                    if (limbs[i - 1] < other.limbs[i - 1])
                        return true;
                    if (limbs[i - 1] > other.limbs[i - 1])
                        return false;
                }
                return false;
            }

            constexpr bool operator<=(const Bigint &other) const noexcept { return !(other < *this); }

            constexpr bool operator>(const Bigint &other) const noexcept { return other < *this; }

            constexpr bool operator>=(const Bigint &other) const noexcept { return !(*this < other); }

            // Increment/decrement
            constexpr Bigint &operator++() noexcept {
                for (size_t i = 0; i < N; ++i) {
                    if (++limbs[i] != 0)
                        break;
                }
                return *this;
            }

            constexpr Bigint operator++(int) noexcept {
                Bigint tmp = *this;
                ++(*this);
                return tmp;
            }

            constexpr Bigint &operator--() noexcept {
                for (size_t i = 0; i < N; ++i) {
                    if (limbs[i]-- != 0)
                        break;
                }
                return *this;
            }

            constexpr Bigint operator--(int) noexcept {
                Bigint tmp = *this;
                --(*this);
                return tmp;
            }
        };

        // Binary operators
        template <size_t N> constexpr Bigint<N> operator+(const Bigint<N> &a, const Bigint<N> &b) noexcept {
            Bigint<N> result = a;
            result += b;
            return result;
        }

        template <size_t N> constexpr Bigint<N> operator-(const Bigint<N> &a, const Bigint<N> &b) noexcept {
            Bigint<N> result = a;
            result -= b;
            return result;
        }

        // Multiplication (schoolbook algorithm, truncated to N limbs)
        template <size_t N> constexpr Bigint<N> operator*(const Bigint<N> &a, const Bigint<N> &b) noexcept {
            Bigint<N> result;

            for (size_t i = 0; i < N; ++i) {
                uint64_t carry = 0;
                for (size_t j = 0; j < N - i; ++j) {
                    // Compute a[i] * b[j] + result[i+j] + carry
                    // Need 128-bit arithmetic
#ifdef __SIZEOF_INT128__
                    __uint128_t prod = static_cast<__uint128_t>(a.limbs[i]) * static_cast<__uint128_t>(b.limbs[j]) +
                                       static_cast<__uint128_t>(result.limbs[i + j]) + static_cast<__uint128_t>(carry);
                    result.limbs[i + j] = static_cast<uint64_t>(prod);
                    carry = static_cast<uint64_t>(prod >> 64);
#else
                    // Fallback for platforms without 128-bit integers
                    uint64_t al = a.limbs[i] & 0xFFFFFFFF;
                    uint64_t ah = a.limbs[i] >> 32;
                    uint64_t bl = b.limbs[j] & 0xFFFFFFFF;
                    uint64_t bh = b.limbs[j] >> 32;

                    uint64_t ll = al * bl;
                    uint64_t lh = al * bh;
                    uint64_t hl = ah * bl;
                    uint64_t hh = ah * bh;

                    uint64_t mid = (ll >> 32) + (lh & 0xFFFFFFFF) + (hl & 0xFFFFFFFF);
                    uint64_t lo = (ll & 0xFFFFFFFF) | (mid << 32);
                    uint64_t hi = hh + (lh >> 32) + (hl >> 32) + (mid >> 32);

                    // Add to result and carry
                    uint64_t sum1 = result.limbs[i + j] + lo;
                    uint64_t c1 = sum1 < result.limbs[i + j] ? 1 : 0;
                    uint64_t sum2 = sum1 + carry;
                    uint64_t c2 = sum2 < sum1 ? 1 : 0;
                    result.limbs[i + j] = sum2;
                    carry = hi + c1 + c2;
#endif
                }
            }
            return result;
        }

        template <size_t N> constexpr Bigint<N> operator&(const Bigint<N> &a, const Bigint<N> &b) noexcept {
            Bigint<N> result = a;
            result &= b;
            return result;
        }

        template <size_t N> constexpr Bigint<N> operator|(const Bigint<N> &a, const Bigint<N> &b) noexcept {
            Bigint<N> result = a;
            result |= b;
            return result;
        }

        template <size_t N> constexpr Bigint<N> operator^(const Bigint<N> &a, const Bigint<N> &b) noexcept {
            Bigint<N> result = a;
            result ^= b;
            return result;
        }

        template <size_t N> constexpr Bigint<N> operator<<(const Bigint<N> &a, size_t shift) noexcept {
            Bigint<N> result = a;
            result <<= shift;
            return result;
        }

        template <size_t N> constexpr Bigint<N> operator>>(const Bigint<N> &a, size_t shift) noexcept {
            Bigint<N> result = a;
            result >>= shift;
            return result;
        }

        // Type traits
        template <typename T> struct is_bigint : std::false_type {};
        template <size_t N> struct is_bigint<Bigint<N>> : std::true_type {};
        template <typename T> inline constexpr bool is_bigint_v = is_bigint<T>::value;

        // Common sizes
        using uint128 = Bigint<2>;   // 128-bit
        using uint256 = Bigint<4>;   // 256-bit
        using uint512 = Bigint<8>;   // 512-bit
        using uint1024 = Bigint<16>; // 1024-bit

    } // namespace mat
} // namespace datapod
