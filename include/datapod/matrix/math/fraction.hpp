#pragma once

#include <cstddef>
#include <cstdint>
#include <numeric>
#include <tuple>
#include <type_traits>

namespace datapod {
    namespace mat {

        /**
         * @brief Rational number (numerator/denominator) - POD
         *
         * Exact representation of fractions without floating-point errors.
         * Automatically reduces to lowest terms after operations.
         * Fully serializable via members().
         *
         * Examples:
         *   fraction<int> f{1, 2};     // 1/2
         *   fraction<int> g{2, 3};     // 2/3
         *   auto h = f + g;            // 7/6
         *   auto d = f.to_double();    // 0.5
         */
        template <typename T> struct fraction {
            static_assert(std::is_integral_v<T>, "fraction<T> requires integral type");

            using value_type = T;
            static constexpr size_t rank = 0;

            T num{};  // Numerator
            T den{1}; // Denominator (never zero)

            // ===== SERIALIZATION (required for datapod) =====
            auto members() noexcept { return std::tie(num, den); }
            auto members() const noexcept { return std::tie(num, den); }

            // Construction
            constexpr fraction() noexcept = default;
            constexpr fraction(T n) noexcept : num(n), den{1} {}
            constexpr fraction(T n, T d) noexcept : num(n), den(d) { normalize(); }

            // From floating point (approximate)
            static inline fraction from_double(double value, T max_denom = 1000000) noexcept {
                if (value == 0.0)
                    return fraction{T{0}, T{1}};

                bool negative = value < 0;
                value = negative ? -value : value;

                T n0 = 0, d0 = 1;
                T n1 = 1, d1 = 0;

                double x = value;
                while (d1 <= max_denom) {
                    T a = static_cast<T>(x);
                    T n2 = a * n1 + n0;
                    T d2 = a * d1 + d0;

                    if (d2 > max_denom)
                        break;

                    n0 = n1;
                    d0 = d1;
                    n1 = n2;
                    d1 = d2;

                    if (x == static_cast<double>(a))
                        break;
                    x = 1.0 / (x - static_cast<double>(a));
                }

                return fraction{negative ? -n1 : n1, d1};
            }

            // Normalization (reduce to lowest terms, ensure positive denominator)
            constexpr void normalize() noexcept {
                if (den == 0) {
                    den = 1;
                    num = 0;
                    return;
                }
                if (den < 0) {
                    num = -num;
                    den = -den;
                }
                if (num == 0) {
                    den = 1;
                    return;
                }
                T g = gcd_impl(num < 0 ? -num : num, den);
                num /= g;
                den /= g;
            }

            // Conversion to floating point
            constexpr double to_double() const noexcept { return static_cast<double>(num) / static_cast<double>(den); }
            constexpr float to_float() const noexcept { return static_cast<float>(num) / static_cast<float>(den); }

            // Properties
            constexpr bool is_zero() const noexcept { return num == 0; }
            constexpr bool is_positive() const noexcept { return num > 0; }
            constexpr bool is_negative() const noexcept { return num < 0; }
            constexpr bool is_integer() const noexcept { return den == 1; }
            constexpr bool is_set() const noexcept { return num != 0; }

            // Absolute value
            constexpr fraction abs() const noexcept { return fraction{num < 0 ? -num : num, den}; }

            // Reciprocal
            constexpr fraction reciprocal() const noexcept { return fraction{den, num}; }

            // Floor and ceiling
            constexpr T floor() const noexcept {
                if (num >= 0)
                    return num / den;
                return (num - den + 1) / den;
            }

            constexpr T ceil() const noexcept {
                if (num >= 0)
                    return (num + den - 1) / den;
                return num / den;
            }

            // Compound assignment
            constexpr fraction &operator+=(const fraction &other) noexcept {
                num = num * other.den + other.num * den;
                den = den * other.den;
                normalize();
                return *this;
            }

            constexpr fraction &operator-=(const fraction &other) noexcept {
                num = num * other.den - other.num * den;
                den = den * other.den;
                normalize();
                return *this;
            }

            constexpr fraction &operator*=(const fraction &other) noexcept {
                num *= other.num;
                den *= other.den;
                normalize();
                return *this;
            }

            constexpr fraction &operator/=(const fraction &other) noexcept {
                num *= other.den;
                den *= other.num;
                normalize();
                return *this;
            }

            // Unary operators
            constexpr fraction operator-() const noexcept { return fraction{-num, den}; }
            constexpr fraction operator+() const noexcept { return *this; }

            // Comparison
            constexpr bool operator==(const fraction &other) const noexcept {
                return num == other.num && den == other.den;
            }
            constexpr bool operator!=(const fraction &other) const noexcept { return !(*this == other); }
            constexpr bool operator<(const fraction &other) const noexcept { return num * other.den < other.num * den; }
            constexpr bool operator<=(const fraction &other) const noexcept {
                return num * other.den <= other.num * den;
            }
            constexpr bool operator>(const fraction &other) const noexcept { return num * other.den > other.num * den; }
            constexpr bool operator>=(const fraction &other) const noexcept {
                return num * other.den >= other.num * den;
            }

          private:
            static constexpr T gcd_impl(T a, T b) noexcept {
                while (b != 0) {
                    T t = b;
                    b = a % b;
                    a = t;
                }
                return a;
            }
        };

        // Binary operators
        template <typename T> constexpr fraction<T> operator+(const fraction<T> &a, const fraction<T> &b) noexcept {
            fraction<T> r{a.num * b.den + b.num * a.den, a.den * b.den};
            r.normalize();
            return r;
        }

        template <typename T> constexpr fraction<T> operator-(const fraction<T> &a, const fraction<T> &b) noexcept {
            fraction<T> r{a.num * b.den - b.num * a.den, a.den * b.den};
            r.normalize();
            return r;
        }

        template <typename T> constexpr fraction<T> operator*(const fraction<T> &a, const fraction<T> &b) noexcept {
            fraction<T> r{a.num * b.num, a.den * b.den};
            r.normalize();
            return r;
        }

        template <typename T> constexpr fraction<T> operator/(const fraction<T> &a, const fraction<T> &b) noexcept {
            fraction<T> r{a.num * b.den, a.den * b.num};
            r.normalize();
            return r;
        }

        // Scalar operations
        template <typename T> constexpr fraction<T> operator*(const fraction<T> &f, T s) noexcept {
            fraction<T> r{f.num * s, f.den};
            r.normalize();
            return r;
        }

        template <typename T> constexpr fraction<T> operator*(T s, const fraction<T> &f) noexcept { return f * s; }

        template <typename T> constexpr fraction<T> operator/(const fraction<T> &f, T s) noexcept {
            fraction<T> r{f.num, f.den * s};
            r.normalize();
            return r;
        }

        // Power function
        template <typename T> constexpr fraction<T> pow(const fraction<T> &base, int exp) noexcept {
            if (exp == 0)
                return fraction<T>{T{1}, T{1}};
            if (exp < 0)
                return pow(base.reciprocal(), -exp);

            fraction<T> result{T{1}, T{1}};
            fraction<T> b = base;
            while (exp > 0) {
                if (exp & 1)
                    result *= b;
                b *= b;
                exp >>= 1;
            }
            return result;
        }

        // Type traits
        template <typename T> struct is_fraction : std::false_type {};
        template <typename T> struct is_fraction<fraction<T>> : std::true_type {};
        template <typename T> inline constexpr bool is_fraction_v = is_fraction<T>::value;

        // Type aliases
        using fraction32 = fraction<int32_t>;
        using fraction64 = fraction<int64_t>;

    } // namespace mat
} // namespace datapod
