#pragma once

#include <cmath>
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace datapod {
    namespace mat {

        /**
         * @brief Complex number (a + bi) - POD
         *
         * Pure aggregate struct representing a complex number.
         * Use aggregate initialization: complex<double>{3.0, 4.0}
         * Fully serializable and reflectable via members().
         *
         * Examples:
         *   complex<double> z{3.0, 4.0};  // 3 + 4i
         *   complex<float> w{1.0f, 0.0f}; // Real number
         *   auto mag = z.magnitude();     // 5.0
         *   auto conj = z.conjugate();    // 3 - 4i
         *   auto z2 = complex<double>::from_polar(5.0, 0.927);  // Polar form
         */
        template <typename T> struct complex {
            static_assert(std::is_floating_point_v<T>, "complex<T> requires floating-point type");

            using value_type = T;
            static constexpr size_t rank = 0;

            T real{}; // Real part
            T imag{}; // Imaginary part

            // ===== SERIALIZATION (required for datapod) =====
            auto members() noexcept { return std::tie(real, imag); }
            auto members() const noexcept { return std::tie(real, imag); }

            // Construction
            constexpr complex() noexcept = default;
            constexpr complex(T r) noexcept : real(r), imag{} {}
            constexpr complex(T r, T i) noexcept : real(r), imag(i) {}

            // Polar form factory
            static inline complex from_polar(T magnitude, T phase) noexcept {
                return complex{magnitude * std::cos(phase), magnitude * std::sin(phase)};
            }

            // Unit imaginary
            static constexpr complex i() noexcept { return complex{T{0}, T{1}}; }

            // Properties
            constexpr T magnitude_squared() const noexcept { return real * real + imag * imag; }
            inline T magnitude() const noexcept { return std::sqrt(magnitude_squared()); }
            inline T abs() const noexcept { return magnitude(); }
            inline T phase() const noexcept { return std::atan2(imag, real); }
            inline T arg() const noexcept { return phase(); }

            // Utility
            constexpr bool is_set() const noexcept { return real != T{} || imag != T{}; }
            constexpr bool is_real() const noexcept { return imag == T{}; }
            constexpr bool is_imaginary() const noexcept { return real == T{}; }
            constexpr bool is_zero() const noexcept { return real == T{} && imag == T{}; }

            // Complex operations
            constexpr complex conjugate() const noexcept { return complex{real, -imag}; }

            inline complex inverse() const noexcept {
                T denom = magnitude_squared();
                return complex{real / denom, -imag / denom};
            }

            inline complex normalized() const noexcept {
                T mag = magnitude();
                return complex{real / mag, imag / mag};
            }

            // Compound assignment - complex
            constexpr complex &operator+=(const complex &other) noexcept {
                real += other.real;
                imag += other.imag;
                return *this;
            }

            constexpr complex &operator-=(const complex &other) noexcept {
                real -= other.real;
                imag -= other.imag;
                return *this;
            }

            constexpr complex &operator*=(const complex &other) noexcept {
                T r = real * other.real - imag * other.imag;
                T i = real * other.imag + imag * other.real;
                real = r;
                imag = i;
                return *this;
            }

            inline complex &operator/=(const complex &other) noexcept { return *this *= other.inverse(); }

            // Compound assignment - scalar
            constexpr complex &operator*=(T s) noexcept {
                real *= s;
                imag *= s;
                return *this;
            }

            constexpr complex &operator/=(T s) noexcept {
                real /= s;
                imag /= s;
                return *this;
            }

            // Unary operators
            constexpr complex operator-() const noexcept { return complex{-real, -imag}; }
            constexpr complex operator+() const noexcept { return *this; }

            // Comparison
            constexpr bool operator==(const complex &other) const noexcept {
                return real == other.real && imag == other.imag;
            }
            constexpr bool operator!=(const complex &other) const noexcept { return !(*this == other); }
        };

        // Binary operators - complex-complex
        template <typename T> constexpr complex<T> operator+(const complex<T> &a, const complex<T> &b) noexcept {
            return complex<T>{a.real + b.real, a.imag + b.imag};
        }

        template <typename T> constexpr complex<T> operator-(const complex<T> &a, const complex<T> &b) noexcept {
            return complex<T>{a.real - b.real, a.imag - b.imag};
        }

        template <typename T> constexpr complex<T> operator*(const complex<T> &a, const complex<T> &b) noexcept {
            return complex<T>{a.real * b.real - a.imag * b.imag, a.real * b.imag + a.imag * b.real};
        }

        template <typename T> inline complex<T> operator/(const complex<T> &a, const complex<T> &b) noexcept {
            return a * b.inverse();
        }

        // Binary operators - complex-scalar
        template <typename T> constexpr complex<T> operator*(const complex<T> &z, T s) noexcept {
            return complex<T>{z.real * s, z.imag * s};
        }

        template <typename T> constexpr complex<T> operator*(T s, const complex<T> &z) noexcept {
            return complex<T>{s * z.real, s * z.imag};
        }

        template <typename T> constexpr complex<T> operator/(const complex<T> &z, T s) noexcept {
            return complex<T>{z.real / s, z.imag / s};
        }

        // Transcendental functions
        template <typename T> inline complex<T> exp(const complex<T> &z) noexcept {
            T ea = std::exp(z.real);
            return complex<T>{ea * std::cos(z.imag), ea * std::sin(z.imag)};
        }

        template <typename T> inline complex<T> log(const complex<T> &z) noexcept {
            return complex<T>{std::log(z.magnitude()), z.phase()};
        }

        template <typename T> inline complex<T> sqrt(const complex<T> &z) noexcept {
            T r = std::sqrt(z.magnitude());
            T half_arg = z.phase() / T{2};
            return complex<T>{r * std::cos(half_arg), r * std::sin(half_arg)};
        }

        template <typename T> inline complex<T> pow(const complex<T> &base, T exp) noexcept {
            if (base.is_zero())
                return complex<T>{};
            T r = std::pow(base.magnitude(), exp);
            T theta = base.phase() * exp;
            return complex<T>{r * std::cos(theta), r * std::sin(theta)};
        }

        // Trigonometric functions
        template <typename T> inline complex<T> sin(const complex<T> &z) noexcept {
            return complex<T>{std::sin(z.real) * std::cosh(z.imag), std::cos(z.real) * std::sinh(z.imag)};
        }

        template <typename T> inline complex<T> cos(const complex<T> &z) noexcept {
            return complex<T>{std::cos(z.real) * std::cosh(z.imag), -std::sin(z.real) * std::sinh(z.imag)};
        }

        template <typename T> inline complex<T> tan(const complex<T> &z) noexcept { return sin(z) / cos(z); }

        // Hyperbolic functions
        template <typename T> inline complex<T> sinh(const complex<T> &z) noexcept {
            return complex<T>{std::sinh(z.real) * std::cos(z.imag), std::cosh(z.real) * std::sin(z.imag)};
        }

        template <typename T> inline complex<T> cosh(const complex<T> &z) noexcept {
            return complex<T>{std::cosh(z.real) * std::cos(z.imag), std::sinh(z.real) * std::sin(z.imag)};
        }

        template <typename T> inline complex<T> tanh(const complex<T> &z) noexcept { return sinh(z) / cosh(z); }

        // Type traits
        template <typename T> struct is_complex : std::false_type {};
        template <typename T> struct is_complex<complex<T>> : std::true_type {};
        template <typename T> inline constexpr bool is_complex_v = is_complex<T>::value;

        // Type aliases
        using complexf = complex<float>;
        using complexd = complex<double>;

    } // namespace mat
} // namespace datapod
