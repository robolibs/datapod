#pragma once

#include <cmath>
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace datapod {
    namespace mat {

        /**
         * @brief Dual number (a + bε) for automatic differentiation - POD
         *
         * Dual numbers extend real numbers with an infinitesimal ε where ε² = 0.
         * This property makes them perfect for automatic differentiation:
         * f(a + bε) = f(a) + f'(a)·b·ε
         *
         * The 'real' part holds the value, 'eps' part holds the derivative.
         * Fully serializable via members().
         *
         * Examples:
         *   Dual<double> x{3.0, 1.0};  // x = 3, dx/dx = 1
         *   auto y = x * x;            // y.real = 9, y.eps = 6 (derivative of x²)
         *   auto z = sin(x);           // z.real = sin(3), z.eps = cos(3)
         */
        template <typename T> struct Dual {
            static_assert(std::is_floating_point_v<T>, "Dual<T> requires floating-point type");

            using value_type = T;
            static constexpr size_t rank = 0;

            T real{}; // Value (primal)
            T eps{};  // Derivative (tangent/epsilon part)

            // ===== SERIALIZATION (required for datapod) =====
            auto members() noexcept { return std::tie(real, eps); }
            auto members() const noexcept { return std::tie(real, eps); }

            // Construction
            constexpr Dual() noexcept = default;
            constexpr Dual(T r) noexcept : real(r), eps{} {}
            constexpr Dual(T r, T d) noexcept : real(r), eps(d) {}

            // Create a variable for differentiation (derivative = 1)
            static constexpr Dual variable(T value) noexcept { return Dual{value, T{1}}; }

            // Create a constant (derivative = 0)
            static constexpr Dual constant(T value) noexcept { return Dual{value, T{0}}; }

            // Accessors
            constexpr T value() const noexcept { return real; }
            constexpr T derivative() const noexcept { return eps; }

            // Utility
            constexpr bool is_set() const noexcept { return real != T{} || eps != T{}; }

            // Compound assignment
            constexpr Dual &operator+=(const Dual &other) noexcept {
                real += other.real;
                eps += other.eps;
                return *this;
            }

            constexpr Dual &operator-=(const Dual &other) noexcept {
                real -= other.real;
                eps -= other.eps;
                return *this;
            }

            constexpr Dual &operator*=(const Dual &other) noexcept {
                // (a + bε)(c + dε) = ac + (ad + bc)ε  (ε² = 0)
                eps = real * other.eps + eps * other.real;
                real = real * other.real;
                return *this;
            }

            constexpr Dual &operator/=(const Dual &other) noexcept {
                // (a + bε)/(c + dε) = a/c + (bc - ad)/c²·ε
                T denom = other.real * other.real;
                eps = (eps * other.real - real * other.eps) / denom;
                real = real / other.real;
                return *this;
            }

            // Scalar compound assignment
            constexpr Dual &operator*=(T s) noexcept {
                real *= s;
                eps *= s;
                return *this;
            }

            constexpr Dual &operator/=(T s) noexcept {
                real /= s;
                eps /= s;
                return *this;
            }

            // Unary operators
            constexpr Dual operator-() const noexcept { return Dual{-real, -eps}; }
            constexpr Dual operator+() const noexcept { return *this; }

            // Comparison (based on real part only)
            constexpr bool operator==(const Dual &other) const noexcept {
                return real == other.real && eps == other.eps;
            }
            constexpr bool operator!=(const Dual &other) const noexcept { return !(*this == other); }
            constexpr bool operator<(const Dual &other) const noexcept { return real < other.real; }
            constexpr bool operator<=(const Dual &other) const noexcept { return real <= other.real; }
            constexpr bool operator>(const Dual &other) const noexcept { return real > other.real; }
            constexpr bool operator>=(const Dual &other) const noexcept { return real >= other.real; }
        };

        // Binary operators
        template <typename T> constexpr Dual<T> operator+(const Dual<T> &a, const Dual<T> &b) noexcept {
            return Dual<T>{a.real + b.real, a.eps + b.eps};
        }

        template <typename T> constexpr Dual<T> operator-(const Dual<T> &a, const Dual<T> &b) noexcept {
            return Dual<T>{a.real - b.real, a.eps - b.eps};
        }

        template <typename T> constexpr Dual<T> operator*(const Dual<T> &a, const Dual<T> &b) noexcept {
            return Dual<T>{a.real * b.real, a.real * b.eps + a.eps * b.real};
        }

        template <typename T> constexpr Dual<T> operator/(const Dual<T> &a, const Dual<T> &b) noexcept {
            T denom = b.real * b.real;
            return Dual<T>{a.real / b.real, (a.eps * b.real - a.real * b.eps) / denom};
        }

        // Scalar operations
        template <typename T> constexpr Dual<T> operator*(const Dual<T> &d, T s) noexcept {
            return Dual<T>{d.real * s, d.eps * s};
        }

        template <typename T> constexpr Dual<T> operator*(T s, const Dual<T> &d) noexcept {
            return Dual<T>{s * d.real, s * d.eps};
        }

        template <typename T> constexpr Dual<T> operator/(const Dual<T> &d, T s) noexcept {
            return Dual<T>{d.real / s, d.eps / s};
        }

        template <typename T> constexpr Dual<T> operator+(const Dual<T> &d, T s) noexcept {
            return Dual<T>{d.real + s, d.eps};
        }

        template <typename T> constexpr Dual<T> operator+(T s, const Dual<T> &d) noexcept {
            return Dual<T>{s + d.real, d.eps};
        }

        template <typename T> constexpr Dual<T> operator-(const Dual<T> &d, T s) noexcept {
            return Dual<T>{d.real - s, d.eps};
        }

        template <typename T> constexpr Dual<T> operator-(T s, const Dual<T> &d) noexcept {
            return Dual<T>{s - d.real, -d.eps};
        }

        // Transcendental functions with automatic differentiation
        template <typename T> inline Dual<T> sqrt(const Dual<T> &x) noexcept {
            T s = std::sqrt(x.real);
            return Dual<T>{s, x.eps / (T{2} * s)};
        }

        template <typename T> inline Dual<T> exp(const Dual<T> &x) noexcept {
            T e = std::exp(x.real);
            return Dual<T>{e, e * x.eps};
        }

        template <typename T> inline Dual<T> log(const Dual<T> &x) noexcept {
            return Dual<T>{std::log(x.real), x.eps / x.real};
        }

        template <typename T> inline Dual<T> pow(const Dual<T> &base, T exp) noexcept {
            T p = std::pow(base.real, exp);
            return Dual<T>{p, exp * std::pow(base.real, exp - T{1}) * base.eps};
        }

        template <typename T> inline Dual<T> pow(const Dual<T> &base, const Dual<T> &exp) noexcept {
            // d/dx[f^g] = f^g * (g' * ln(f) + g * f'/f)
            T p = std::pow(base.real, exp.real);
            T dp = p * (exp.eps * std::log(base.real) + exp.real * base.eps / base.real);
            return Dual<T>{p, dp};
        }

        // Trigonometric functions
        template <typename T> inline Dual<T> sin(const Dual<T> &x) noexcept {
            return Dual<T>{std::sin(x.real), std::cos(x.real) * x.eps};
        }

        template <typename T> inline Dual<T> cos(const Dual<T> &x) noexcept {
            return Dual<T>{std::cos(x.real), -std::sin(x.real) * x.eps};
        }

        template <typename T> inline Dual<T> tan(const Dual<T> &x) noexcept {
            T c = std::cos(x.real);
            return Dual<T>{std::tan(x.real), x.eps / (c * c)};
        }

        template <typename T> inline Dual<T> asin(const Dual<T> &x) noexcept {
            return Dual<T>{std::asin(x.real), x.eps / std::sqrt(T{1} - x.real * x.real)};
        }

        template <typename T> inline Dual<T> acos(const Dual<T> &x) noexcept {
            return Dual<T>{std::acos(x.real), -x.eps / std::sqrt(T{1} - x.real * x.real)};
        }

        template <typename T> inline Dual<T> atan(const Dual<T> &x) noexcept {
            return Dual<T>{std::atan(x.real), x.eps / (T{1} + x.real * x.real)};
        }

        // Hyperbolic functions
        template <typename T> inline Dual<T> sinh(const Dual<T> &x) noexcept {
            return Dual<T>{std::sinh(x.real), std::cosh(x.real) * x.eps};
        }

        template <typename T> inline Dual<T> cosh(const Dual<T> &x) noexcept {
            return Dual<T>{std::cosh(x.real), std::sinh(x.real) * x.eps};
        }

        template <typename T> inline Dual<T> tanh(const Dual<T> &x) noexcept {
            T c = std::cosh(x.real);
            return Dual<T>{std::tanh(x.real), x.eps / (c * c)};
        }

        // Absolute value
        template <typename T> inline Dual<T> abs(const Dual<T> &x) noexcept { return x.real >= T{0} ? x : -x; }

        // Type traits
        template <typename T> struct is_dual : std::false_type {};
        template <typename T> struct is_dual<Dual<T>> : std::true_type {};
        template <typename T> inline constexpr bool is_dual_v = is_dual<T>::value;

        // Type aliases
        using dualf = Dual<float>;
        using duald = Dual<double>;

    } // namespace mat

    namespace dual {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace dual

} // namespace datapod
