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
         *   Modular<uint32_t, 7> a{5};    // 5 mod 7
         *   Modular<uint32_t, 7> b{4};    // 4 mod 7
         *   auto c = a + b;               // 2 (since 9 mod 7 = 2)
         *   auto d = a * b;               // 6 (since 20 mod 7 = 6)
         *   auto inv = a.inverse();       // 3 (since 5*3 = 15 = 1 mod 7)
         */
        template <typename T, T N> struct Modular {
            static_assert(std::is_integral_v<T>, "Modular<T,N> requires integral type");
            static_assert(N > 0, "modulus must be positive");

            using value_type = T;
            static constexpr T modulus = N;
            static constexpr size_t rank = 0;

            T val{}; // Value in range [0, N)

            // ===== SERIALIZATION (required for datapod) =====
            auto members() noexcept { return std::tie(val); }
            auto members() const noexcept { return std::tie(val); }

            // Construction
            constexpr Modular() noexcept = default;

            constexpr Modular(T v) noexcept : val(reduce(v)) {}

            // Get the raw value
            constexpr T value() const noexcept { return val; }

            // Utility
            constexpr bool is_zero() const noexcept { return val == 0; }
            constexpr bool is_one() const noexcept { return val == 1; }
            constexpr bool is_set() const noexcept { return val != 0; }

            // Multiplicative inverse using extended Euclidean algorithm
            // Only exists if gcd(val, N) == 1
            constexpr Modular inverse() const noexcept {
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
                    return Modular{0}; // No inverse exists

                if (t < 0)
                    t += static_cast<SignedT>(N);
                return Modular{static_cast<T>(t)};
            }

            // Power using fast exponentiation
            constexpr Modular pow(T exp) const noexcept {
                if (exp == 0)
                    return Modular{1};
                Modular result{1};
                Modular base = *this;
                while (exp > 0) {
                    if (exp & 1)
                        result *= base;
                    base *= base;
                    exp >>= 1;
                }
                return result;
            }

            // Compound assignment
            constexpr Modular &operator+=(const Modular &other) noexcept {
                val = (val + other.val) % N;
                return *this;
            }

            constexpr Modular &operator-=(const Modular &other) noexcept {
                val = (val + N - other.val) % N;
                return *this;
            }

            constexpr Modular &operator*=(const Modular &other) noexcept {
                val = reduce_product(val, other.val);
                return *this;
            }

            constexpr Modular &operator/=(const Modular &other) noexcept {
                *this *= other.inverse();
                return *this;
            }

            // Increment/decrement
            constexpr Modular &operator++() noexcept {
                val = (val + 1) % N;
                return *this;
            }

            constexpr Modular operator++(int) noexcept {
                Modular tmp = *this;
                ++(*this);
                return tmp;
            }

            constexpr Modular &operator--() noexcept {
                val = (val + N - 1) % N;
                return *this;
            }

            constexpr Modular operator--(int) noexcept {
                Modular tmp = *this;
                --(*this);
                return tmp;
            }

            // Unary operators
            constexpr Modular operator-() const noexcept { return Modular{N - val}; }
            constexpr Modular operator+() const noexcept { return *this; }

            // Comparison
            constexpr bool operator==(const Modular &other) const noexcept { return val == other.val; }
            constexpr bool operator!=(const Modular &other) const noexcept { return val != other.val; }
            constexpr bool operator<(const Modular &other) const noexcept { return val < other.val; }
            constexpr bool operator<=(const Modular &other) const noexcept { return val <= other.val; }
            constexpr bool operator>(const Modular &other) const noexcept { return val > other.val; }
            constexpr bool operator>=(const Modular &other) const noexcept { return val >= other.val; }

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
        constexpr Modular<T, N> operator+(const Modular<T, N> &a, const Modular<T, N> &b) noexcept {
            return Modular<T, N>{static_cast<T>((a.val + b.val) % N)};
        }

        template <typename T, T N>
        constexpr Modular<T, N> operator-(const Modular<T, N> &a, const Modular<T, N> &b) noexcept {
            return Modular<T, N>{static_cast<T>((a.val + N - b.val) % N)};
        }

        template <typename T, T N>
        constexpr Modular<T, N> operator*(const Modular<T, N> &a, const Modular<T, N> &b) noexcept {
            Modular<T, N> result = a;
            result *= b;
            return result;
        }

        template <typename T, T N>
        constexpr Modular<T, N> operator/(const Modular<T, N> &a, const Modular<T, N> &b) noexcept {
            return a * b.inverse();
        }

        // Type traits
        template <typename T> struct is_modular : std::false_type {};
        template <typename T, T N> struct is_modular<Modular<T, N>> : std::true_type {};
        template <typename T> inline constexpr bool is_modular_v = is_modular<T>::value;

        // Common moduli
        template <uint32_t N> using mod32 = Modular<uint32_t, N>;
        template <uint64_t N> using mod64 = Modular<uint64_t, N>;

        // Common prime moduli for competitive programming / cryptography
        using mod_1e9_7 = Modular<uint32_t, 1000000007>;       // 10^9 + 7 (common prime)
        using mod_998244353 = Modular<uint32_t, 998244353>;    // NTT-friendly prime
        using mod_1e9_9 = Modular<uint32_t, 1000000009>;       // 10^9 + 9
        using mod_prime_32 = Modular<uint32_t, 4294967291>;    // Largest 32-bit prime
        using mod_mersenne_31 = Modular<uint32_t, 2147483647>; // 2^31 - 1 (Mersenne prime)

    } // namespace mat
} // namespace datapod
