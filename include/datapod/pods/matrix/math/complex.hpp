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
         * Pure aggregate struct representing a Complex number.
         * Use aggregate initialization: Complex<double>{3.0, 4.0}
         * Fully serializable and reflectable via members().
         *
         * Examples:
         *   Complex<double> z{3.0, 4.0};  // 3 + 4i
         *   Complex<float> w{1.0f, 0.0f}; // Real number
         *   auto mag = z.magnitude();     // 5.0
         *   auto conj = z.conjugate();    // 3 - 4i
         *   auto z2 = Complex<double>::from_polar(5.0, 0.927);  // Polar form
         */
        template <typename T> struct Complex {
            static_assert(std::is_floating_point_v<T>, "Complex<T> requires floating-point type");

            using value_type = T;
            static constexpr size_t rank = 0;

            T real{}; // Real part
            T imag{}; // Imaginary part

            // ===== SERIALIZATION (required for datapod) =====
            auto members() noexcept { return std::tie(real, imag); }
            auto members() const noexcept { return std::tie(real, imag); }

            // Construction
            constexpr Complex() noexcept = default;
            constexpr Complex(T r) noexcept : real(r), imag{} {}
            constexpr Complex(T r, T i) noexcept : real(r), imag(i) {}

            // Polar form factory
            static inline Complex from_polar(T magnitude, T phase) noexcept {
                return Complex{magnitude * std::cos(phase), magnitude * std::sin(phase)};
            }

            // Unit imaginary
            static constexpr Complex i() noexcept { return Complex{T{0}, T{1}}; }

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
            constexpr Complex conjugate() const noexcept { return Complex{real, -imag}; }

            inline Complex inverse() const noexcept {
                T denom = magnitude_squared();
                return Complex{real / denom, -imag / denom};
            }

            inline Complex normalized() const noexcept {
                T mag = magnitude();
                return Complex{real / mag, imag / mag};
            }

            // Compound assignment - Complex
            constexpr Complex &operator+=(const Complex &other) noexcept {
                real += other.real;
                imag += other.imag;
                return *this;
            }

            constexpr Complex &operator-=(const Complex &other) noexcept {
                real -= other.real;
                imag -= other.imag;
                return *this;
            }

            constexpr Complex &operator*=(const Complex &other) noexcept {
                T r = real * other.real - imag * other.imag;
                T i = real * other.imag + imag * other.real;
                real = r;
                imag = i;
                return *this;
            }

            inline Complex &operator/=(const Complex &other) noexcept { return *this *= other.inverse(); }

            // Compound assignment - scalar
            constexpr Complex &operator*=(T s) noexcept {
                real *= s;
                imag *= s;
                return *this;
            }

            constexpr Complex &operator/=(T s) noexcept {
                real /= s;
                imag /= s;
                return *this;
            }

            // Unary operators
            constexpr Complex operator-() const noexcept { return Complex{-real, -imag}; }
            constexpr Complex operator+() const noexcept { return *this; }

            // Comparison
            constexpr bool operator==(const Complex &other) const noexcept {
                return real == other.real && imag == other.imag;
            }
            constexpr bool operator!=(const Complex &other) const noexcept { return !(*this == other); }
        };

        // Binary operators - Complex-Complex
        template <typename T> constexpr Complex<T> operator+(const Complex<T> &a, const Complex<T> &b) noexcept {
            return Complex<T>{a.real + b.real, a.imag + b.imag};
        }

        template <typename T> constexpr Complex<T> operator-(const Complex<T> &a, const Complex<T> &b) noexcept {
            return Complex<T>{a.real - b.real, a.imag - b.imag};
        }

        template <typename T> constexpr Complex<T> operator*(const Complex<T> &a, const Complex<T> &b) noexcept {
            return Complex<T>{a.real * b.real - a.imag * b.imag, a.real * b.imag + a.imag * b.real};
        }

        template <typename T> inline Complex<T> operator/(const Complex<T> &a, const Complex<T> &b) noexcept {
            return a * b.inverse();
        }

        // Binary operators - Complex-scalar
        template <typename T> constexpr Complex<T> operator*(const Complex<T> &z, T s) noexcept {
            return Complex<T>{z.real * s, z.imag * s};
        }

        template <typename T> constexpr Complex<T> operator*(T s, const Complex<T> &z) noexcept {
            return Complex<T>{s * z.real, s * z.imag};
        }

        template <typename T> constexpr Complex<T> operator/(const Complex<T> &z, T s) noexcept {
            return Complex<T>{z.real / s, z.imag / s};
        }

        // Transcendental functions
        template <typename T> inline Complex<T> exp(const Complex<T> &z) noexcept {
            T ea = std::exp(z.real);
            return Complex<T>{ea * std::cos(z.imag), ea * std::sin(z.imag)};
        }

        template <typename T> inline Complex<T> log(const Complex<T> &z) noexcept {
            return Complex<T>{std::log(z.magnitude()), z.phase()};
        }

        template <typename T> inline Complex<T> sqrt(const Complex<T> &z) noexcept {
            T r = std::sqrt(z.magnitude());
            T half_arg = z.phase() / T{2};
            return Complex<T>{r * std::cos(half_arg), r * std::sin(half_arg)};
        }

        template <typename T> inline Complex<T> pow(const Complex<T> &base, T exp) noexcept {
            if (base.is_zero())
                return Complex<T>{};
            T r = std::pow(base.magnitude(), exp);
            T theta = base.phase() * exp;
            return Complex<T>{r * std::cos(theta), r * std::sin(theta)};
        }

        // Trigonometric functions
        template <typename T> inline Complex<T> sin(const Complex<T> &z) noexcept {
            return Complex<T>{std::sin(z.real) * std::cosh(z.imag), std::cos(z.real) * std::sinh(z.imag)};
        }

        template <typename T> inline Complex<T> cos(const Complex<T> &z) noexcept {
            return Complex<T>{std::cos(z.real) * std::cosh(z.imag), -std::sin(z.real) * std::sinh(z.imag)};
        }

        template <typename T> inline Complex<T> tan(const Complex<T> &z) noexcept { return sin(z) / cos(z); }

        // Hyperbolic functions
        template <typename T> inline Complex<T> sinh(const Complex<T> &z) noexcept {
            return Complex<T>{std::sinh(z.real) * std::cos(z.imag), std::cosh(z.real) * std::sin(z.imag)};
        }

        template <typename T> inline Complex<T> cosh(const Complex<T> &z) noexcept {
            return Complex<T>{std::cosh(z.real) * std::cos(z.imag), std::sinh(z.real) * std::sin(z.imag)};
        }

        template <typename T> inline Complex<T> tanh(const Complex<T> &z) noexcept { return sinh(z) / cosh(z); }

        // Type traits
        template <typename T> struct is_complex : std::false_type {};
        template <typename T> struct is_complex<Complex<T>> : std::true_type {};
        template <typename T> inline constexpr bool is_complex_v = is_complex<T>::value;

        // Type aliases
        using complexf = Complex<float>;
        using complexd = Complex<double>;

    } // namespace mat

    namespace mat_complex {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace mat_complex

} // namespace datapod
