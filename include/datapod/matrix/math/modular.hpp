#pragma once

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>

namespace datapod {
    namespace mat {

        /**
         * @brief Modular arithmetic integer (Z/nZ) - POD
         *
         * Represents integers modulo N. All operations automatically reduce mod N.
         * Useful for cryptography, hash functions, cyclic counters, and number theory.
         * Fully serializable via members().
         *
         * Examples:
         *   modular<uint32_t, 7> a{5};    // 5 mod 7
         *   modular<uint32_t, 7> b{4};    // 4 mod 7
         *   auto c = a + b;               // 2 (since 9 mod 7 = 2)
         *   auto d = a * b;               // 6 (since 20 mod 7 = 6)
         *   auto inv = a.inverse();       // 3 (since 5*3 = 15 = 1 mod 7)
         */
        template <typename T, T N> struct modular {
            static_assert(std::is_integral_v<T>, "modular<T,N> requires integral type");
            static_assert(N > 0, "modulus must be positive");

            using value_type = T;
            static constexpr T modulus = N;
            static constexpr size_t rank = 0;

            T val{}; // Value in range [0, N)

            // ===== SERIALIZATION (required for datapod) =====
            auto members() noexcept { return std::tie(val); }
            auto members() const noexcept { return std::tie(val); }

            // Construction
            constexpr modular() noexcept = default;

            constexpr modular(T v) noexcept : val(reduce(v)) {}

            // Get the raw value
            constexpr T value() const noexcept { return val; }

            // Utility
            constexpr bool is_zero() const noexcept { return val == 0; }
            constexpr bool is_one() const noexcept { return val == 1; }
            constexpr bool is_set() const noexcept { return val != 0; }

            // Multiplicative inverse using extended Euclidean algorithm
            // Only exists if gcd(val, N) == 1
            constexpr modular inverse() const noexcept {
                // Extended Euclidean algorithm
                using SignedT = std::make_signed_t<T>;
                SignedT t = 0, new_t = 1;
                SignedT r = static_cast<SignedT>(N);
                SignedT new_r = static_cast<SignedT>(val);

                while (new_r != 0) {
                    SignedT quotient = r / new_r;
                    SignedT tmp = new_t;
                    new_t = t - quotient * new_t;
                    t = tmp;
                    tmp = new_r;
                    new_r = r - quotient * new_r;
                    r = tmp;
                }

                // r should be 1 if inverse exists (gcd = 1)
                if (r > 1)
                    return modular{0}; // No inverse exists

                if (t < 0)
                    t += static_cast<SignedT>(N);
                return modular{static_cast<T>(t)};
            }

            // Power using fast exponentiation
            constexpr modular pow(T exp) const noexcept {
                if (exp == 0)
                    return modular{1};
                modular result{1};
                modular base = *this;
                while (exp > 0) {
                    if (exp & 1)
                        result *= base;
                    base *= base;
                    exp >>= 1;
                }
                return result;
            }

            // Compound assignment
            constexpr modular &operator+=(const modular &other) noexcept {
                val = (val + other.val) % N;
                return *this;
            }

            constexpr modular &operator-=(const modular &other) noexcept {
                val = (val + N - other.val) % N;
                return *this;
            }

            constexpr modular &operator*=(const modular &other) noexcept {
                val = reduce_product(val, other.val);
                return *this;
            }

            constexpr modular &operator/=(const modular &other) noexcept {
                *this *= other.inverse();
                return *this;
            }

            // Increment/decrement
            constexpr modular &operator++() noexcept {
                val = (val + 1) % N;
                return *this;
            }

            constexpr modular operator++(int) noexcept {
                modular tmp = *this;
                ++(*this);
                return tmp;
            }

            constexpr modular &operator--() noexcept {
                val = (val + N - 1) % N;
                return *this;
            }

            constexpr modular operator--(int) noexcept {
                modular tmp = *this;
                --(*this);
                return tmp;
            }

            // Unary operators
            constexpr modular operator-() const noexcept { return modular{N - val}; }
            constexpr modular operator+() const noexcept { return *this; }

            // Comparison
            constexpr bool operator==(const modular &other) const noexcept { return val == other.val; }
            constexpr bool operator!=(const modular &other) const noexcept { return val != other.val; }
            constexpr bool operator<(const modular &other) const noexcept { return val < other.val; }
            constexpr bool operator<=(const modular &other) const noexcept { return val <= other.val; }
            constexpr bool operator>(const modular &other) const noexcept { return val > other.val; }
            constexpr bool operator>=(const modular &other) const noexcept { return val >= other.val; }

          private:
            // Reduce value to [0, N)
            static constexpr T reduce(T v) noexcept {
                if constexpr (std::is_signed_v<T>) {
                    v = v % static_cast<T>(N);
                    return v < 0 ? v + N : v;
                } else {
                    return v % N;
                }
            }

            // Reduce product, handling potential overflow for large types
            static constexpr T reduce_product(T a, T b) noexcept {
                // For types where overflow is a concern, use wider type
                if constexpr (sizeof(T) < sizeof(uint64_t)) {
                    return static_cast<T>((static_cast<uint64_t>(a) * static_cast<uint64_t>(b)) % N);
                } else {
                    // For 64-bit types, use __uint128_t if available, otherwise simple mod
#ifdef __SIZEOF_INT128__
                    return static_cast<T>((static_cast<__uint128_t>(a) * static_cast<__uint128_t>(b)) %
                                          static_cast<__uint128_t>(N));
#else
                    return (a * b) % N;
#endif
                }
            }
        };

        // Binary operators
        template <typename T, T N>
        constexpr modular<T, N> operator+(const modular<T, N> &a, const modular<T, N> &b) noexcept {
            return modular<T, N>{static_cast<T>((a.val + b.val) % N)};
        }

        template <typename T, T N>
        constexpr modular<T, N> operator-(const modular<T, N> &a, const modular<T, N> &b) noexcept {
            return modular<T, N>{static_cast<T>((a.val + N - b.val) % N)};
        }

        template <typename T, T N>
        constexpr modular<T, N> operator*(const modular<T, N> &a, const modular<T, N> &b) noexcept {
            modular<T, N> result = a;
            result *= b;
            return result;
        }

        template <typename T, T N>
        constexpr modular<T, N> operator/(const modular<T, N> &a, const modular<T, N> &b) noexcept {
            return a * b.inverse();
        }

        // Type traits
        template <typename T> struct is_modular : std::false_type {};
        template <typename T, T N> struct is_modular<modular<T, N>> : std::true_type {};
        template <typename T> inline constexpr bool is_modular_v = is_modular<T>::value;

        // Common moduli
        template <uint32_t N> using mod32 = modular<uint32_t, N>;
        template <uint64_t N> using mod64 = modular<uint64_t, N>;

        // Common prime moduli for competitive programming / cryptography
        using mod_1e9_7 = modular<uint32_t, 1000000007>;       // 10^9 + 7 (common prime)
        using mod_998244353 = modular<uint32_t, 998244353>;    // NTT-friendly prime
        using mod_1e9_9 = modular<uint32_t, 1000000009>;       // 10^9 + 9
        using mod_prime_32 = modular<uint32_t, 4294967291>;    // Largest 32-bit prime
        using mod_mersenne_31 = modular<uint32_t, 2147483647>; // 2^31 - 1 (Mersenne prime)

    } // namespace mat
} // namespace datapod
