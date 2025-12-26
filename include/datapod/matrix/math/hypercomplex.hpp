#pragma once

#include <cmath>
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace datapod {
    namespace mat {

        /**
         * @brief Octonion - 8-dimensional hypercomplex number - POD
         *
         * Octonions extend quaternions to 8 dimensions. They are non-associative
         * but still have division. Used in physics (string theory, special relativity)
         * and some specialized applications.
         *
         * o = a + bi + cj + dk + eE + fI + gJ + hK
         * where i² = j² = k² = E² = I² = J² = K² = -1
         *
         * Fully serializable via members().
         *
         * Examples:
         *   octonion<double> o{1, 2, 3, 4, 5, 6, 7, 8};
         *   auto norm = o.magnitude();
         *   auto conj = o.conjugate();
         */
        template <typename T> struct octonion {
            static_assert(std::is_floating_point_v<T>, "octonion<T> requires floating-point type");

            using value_type = T;
            static constexpr size_t rank = 0;

            // Components: scalar + 7 imaginary units
            T e0{}; // Real (scalar) part
            T e1{}; // i
            T e2{}; // j
            T e3{}; // k
            T e4{}; // E (or l)
            T e5{}; // I (or il)
            T e6{}; // J (or jl)
            T e7{}; // K (or kl)

            // ===== SERIALIZATION (required for datapod) =====
            auto members() noexcept { return std::tie(e0, e1, e2, e3, e4, e5, e6, e7); }
            auto members() const noexcept { return std::tie(e0, e1, e2, e3, e4, e5, e6, e7); }

            // Construction
            constexpr octonion() noexcept = default;

            constexpr octonion(T s) noexcept : e0(s) {}

            constexpr octonion(T a, T b, T c, T d, T e, T f, T g, T h) noexcept
                : e0(a), e1(b), e2(c), e3(d), e4(e), e5(f), e6(g), e7(h) {}

            // From two quaternions: o = q1 + q2 * E
            static constexpr octonion from_quaternions(T q1w, T q1x, T q1y, T q1z, T q2w, T q2x, T q2y,
                                                       T q2z) noexcept {
                return octonion{q1w, q1x, q1y, q1z, q2w, q2x, q2y, q2z};
            }

            // Unit octonion
            static constexpr octonion unit(size_t idx) noexcept {
                octonion o;
                switch (idx) {
                case 0:
                    o.e0 = T{1};
                    break;
                case 1:
                    o.e1 = T{1};
                    break;
                case 2:
                    o.e2 = T{1};
                    break;
                case 3:
                    o.e3 = T{1};
                    break;
                case 4:
                    o.e4 = T{1};
                    break;
                case 5:
                    o.e5 = T{1};
                    break;
                case 6:
                    o.e6 = T{1};
                    break;
                case 7:
                    o.e7 = T{1};
                    break;
                }
                return o;
            }

            // Properties
            constexpr T scalar() const noexcept { return e0; }

            constexpr T norm_squared() const noexcept {
                return e0 * e0 + e1 * e1 + e2 * e2 + e3 * e3 + e4 * e4 + e5 * e5 + e6 * e6 + e7 * e7;
            }

            inline T norm() const noexcept { return std::sqrt(norm_squared()); }
            inline T magnitude() const noexcept { return norm(); }

            // Utility
            constexpr bool is_real() const noexcept {
                return e1 == T{0} && e2 == T{0} && e3 == T{0} && e4 == T{0} && e5 == T{0} && e6 == T{0} && e7 == T{0};
            }

            constexpr bool is_set() const noexcept {
                return e0 != T{0} || e1 != T{0} || e2 != T{0} || e3 != T{0} || e4 != T{0} || e5 != T{0} || e6 != T{0} ||
                       e7 != T{0};
            }

            // Conjugate: negate all imaginary parts
            constexpr octonion conjugate() const noexcept { return octonion{e0, -e1, -e2, -e3, -e4, -e5, -e6, -e7}; }

            // Inverse: conj / |o|²
            inline octonion inverse() const noexcept {
                T n2 = norm_squared();
                return octonion{e0 / n2, -e1 / n2, -e2 / n2, -e3 / n2, -e4 / n2, -e5 / n2, -e6 / n2, -e7 / n2};
            }

            // Normalized (unit octonion)
            inline octonion normalized() const noexcept {
                T n = norm();
                return octonion{e0 / n, e1 / n, e2 / n, e3 / n, e4 / n, e5 / n, e6 / n, e7 / n};
            }

            // Addition
            constexpr octonion &operator+=(const octonion &other) noexcept {
                e0 += other.e0;
                e1 += other.e1;
                e2 += other.e2;
                e3 += other.e3;
                e4 += other.e4;
                e5 += other.e5;
                e6 += other.e6;
                e7 += other.e7;
                return *this;
            }

            constexpr octonion &operator-=(const octonion &other) noexcept {
                e0 -= other.e0;
                e1 -= other.e1;
                e2 -= other.e2;
                e3 -= other.e3;
                e4 -= other.e4;
                e5 -= other.e5;
                e6 -= other.e6;
                e7 -= other.e7;
                return *this;
            }

            // Octonion multiplication (Cayley-Dickson construction)
            // Non-associative! (a*b)*c ≠ a*(b*c) in general
            constexpr octonion &operator*=(const octonion &other) noexcept {
                *this = *this * other;
                return *this;
            }

            // Scalar multiplication
            constexpr octonion &operator*=(T s) noexcept {
                e0 *= s;
                e1 *= s;
                e2 *= s;
                e3 *= s;
                e4 *= s;
                e5 *= s;
                e6 *= s;
                e7 *= s;
                return *this;
            }

            constexpr octonion &operator/=(T s) noexcept {
                e0 /= s;
                e1 /= s;
                e2 /= s;
                e3 /= s;
                e4 /= s;
                e5 /= s;
                e6 /= s;
                e7 /= s;
                return *this;
            }

            // Unary operators
            constexpr octonion operator-() const noexcept { return octonion{-e0, -e1, -e2, -e3, -e4, -e5, -e6, -e7}; }
            constexpr octonion operator+() const noexcept { return *this; }

            // Comparison
            constexpr bool operator==(const octonion &other) const noexcept {
                return e0 == other.e0 && e1 == other.e1 && e2 == other.e2 && e3 == other.e3 && e4 == other.e4 &&
                       e5 == other.e5 && e6 == other.e6 && e7 == other.e7;
            }
            constexpr bool operator!=(const octonion &other) const noexcept { return !(*this == other); }
        };

        // Binary addition/subtraction
        template <typename T> constexpr octonion<T> operator+(const octonion<T> &a, const octonion<T> &b) noexcept {
            return octonion<T>{a.e0 + b.e0, a.e1 + b.e1, a.e2 + b.e2, a.e3 + b.e3,
                               a.e4 + b.e4, a.e5 + b.e5, a.e6 + b.e6, a.e7 + b.e7};
        }

        template <typename T> constexpr octonion<T> operator-(const octonion<T> &a, const octonion<T> &b) noexcept {
            return octonion<T>{a.e0 - b.e0, a.e1 - b.e1, a.e2 - b.e2, a.e3 - b.e3,
                               a.e4 - b.e4, a.e5 - b.e5, a.e6 - b.e6, a.e7 - b.e7};
        }

        // Octonion multiplication using Cayley-Dickson construction
        // Treating as (q1, q2) * (r1, r2) = (q1*r1 - r2_conj*q2, r2*q1 + q2*r1_conj)
        template <typename T> constexpr octonion<T> operator*(const octonion<T> &a, const octonion<T> &b) noexcept {
            // Full multiplication table for octonions
            // Using the standard Cayley multiplication table
            T n0 = a.e0 * b.e0 - a.e1 * b.e1 - a.e2 * b.e2 - a.e3 * b.e3 - a.e4 * b.e4 - a.e5 * b.e5 - a.e6 * b.e6 -
                   a.e7 * b.e7;

            T n1 = a.e0 * b.e1 + a.e1 * b.e0 + a.e2 * b.e3 - a.e3 * b.e2 + a.e4 * b.e5 - a.e5 * b.e4 - a.e6 * b.e7 +
                   a.e7 * b.e6;

            T n2 = a.e0 * b.e2 - a.e1 * b.e3 + a.e2 * b.e0 + a.e3 * b.e1 + a.e4 * b.e6 + a.e5 * b.e7 - a.e6 * b.e4 -
                   a.e7 * b.e5;

            T n3 = a.e0 * b.e3 + a.e1 * b.e2 - a.e2 * b.e1 + a.e3 * b.e0 + a.e4 * b.e7 - a.e5 * b.e6 + a.e6 * b.e5 -
                   a.e7 * b.e4;

            T n4 = a.e0 * b.e4 - a.e1 * b.e5 - a.e2 * b.e6 - a.e3 * b.e7 + a.e4 * b.e0 + a.e5 * b.e1 + a.e6 * b.e2 +
                   a.e7 * b.e3;

            T n5 = a.e0 * b.e5 + a.e1 * b.e4 - a.e2 * b.e7 + a.e3 * b.e6 - a.e4 * b.e1 + a.e5 * b.e0 - a.e6 * b.e3 +
                   a.e7 * b.e2;

            T n6 = a.e0 * b.e6 + a.e1 * b.e7 + a.e2 * b.e4 - a.e3 * b.e5 - a.e4 * b.e2 + a.e5 * b.e3 + a.e6 * b.e0 -
                   a.e7 * b.e1;

            T n7 = a.e0 * b.e7 - a.e1 * b.e6 + a.e2 * b.e5 + a.e3 * b.e4 - a.e4 * b.e3 - a.e5 * b.e2 + a.e6 * b.e1 +
                   a.e7 * b.e0;

            return octonion<T>{n0, n1, n2, n3, n4, n5, n6, n7};
        }

        // Division: a / b = a * b^(-1)
        template <typename T> inline octonion<T> operator/(const octonion<T> &a, const octonion<T> &b) noexcept {
            return a * b.inverse();
        }

        // Scalar operations
        template <typename T> constexpr octonion<T> operator*(const octonion<T> &o, T s) noexcept {
            return octonion<T>{o.e0 * s, o.e1 * s, o.e2 * s, o.e3 * s, o.e4 * s, o.e5 * s, o.e6 * s, o.e7 * s};
        }

        template <typename T> constexpr octonion<T> operator*(T s, const octonion<T> &o) noexcept { return o * s; }

        template <typename T> constexpr octonion<T> operator/(const octonion<T> &o, T s) noexcept {
            return octonion<T>{o.e0 / s, o.e1 / s, o.e2 / s, o.e3 / s, o.e4 / s, o.e5 / s, o.e6 / s, o.e7 / s};
        }

        // Exponential and logarithm
        template <typename T> inline octonion<T> exp(const octonion<T> &o) noexcept {
            T vnorm = std::sqrt(o.e1 * o.e1 + o.e2 * o.e2 + o.e3 * o.e3 + o.e4 * o.e4 + o.e5 * o.e5 + o.e6 * o.e6 +
                                o.e7 * o.e7);
            T ea = std::exp(o.e0);

            if (vnorm < T{1e-10}) {
                return octonion<T>{ea, T{0}, T{0}, T{0}, T{0}, T{0}, T{0}, T{0}};
            }

            T s = ea * std::sin(vnorm) / vnorm;
            T c = ea * std::cos(vnorm);

            return octonion<T>{c, s * o.e1, s * o.e2, s * o.e3, s * o.e4, s * o.e5, s * o.e6, s * o.e7};
        }

        template <typename T> inline octonion<T> log(const octonion<T> &o) noexcept {
            T n = o.norm();
            T vnorm = std::sqrt(o.e1 * o.e1 + o.e2 * o.e2 + o.e3 * o.e3 + o.e4 * o.e4 + o.e5 * o.e5 + o.e6 * o.e6 +
                                o.e7 * o.e7);

            if (vnorm < T{1e-10}) {
                return octonion<T>{std::log(n), T{0}, T{0}, T{0}, T{0}, T{0}, T{0}, T{0}};
            }

            T s = std::acos(o.e0 / n) / vnorm;

            return octonion<T>{std::log(n), s * o.e1, s * o.e2, s * o.e3, s * o.e4, s * o.e5, s * o.e6, s * o.e7};
        }

        // Type traits
        template <typename T> struct is_octonion : std::false_type {};
        template <typename T> struct is_octonion<octonion<T>> : std::true_type {};
        template <typename T> inline constexpr bool is_octonion_v = is_octonion<T>::value;

        // Type aliases
        using octonionf = octonion<float>;
        using octoniond = octonion<double>;

    } // namespace mat
} // namespace datapod
