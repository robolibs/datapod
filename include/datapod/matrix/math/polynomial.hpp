#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace datapod {
    namespace mat {

        /**
         * @brief Fixed-degree Polynomial coefficients[0] + coefficients[1]*x + ... - POD
         *
         * Represents a Polynomial of fixed maximum degree N-1 (N coefficients).
         * Coefficients are stored in ascending order: c[0] + c[1]*x + c[2]*x² + ...
         * Fully serializable via members().
         *
         * Examples:
         *   Polynomial<double, 3> p{1.0, 2.0, 3.0};  // 1 + 2x + 3x²
         *   double y = p.eval(2.0);                   // 1 + 4 + 12 = 17
         *   auto dp = p.derivative();                 // 2 + 6x
         *   auto ip = p.integral();                   // x + x² + x³
         */
        template <typename T, size_t N> struct Polynomial {
            static_assert(std::is_floating_point_v<T>, "Polynomial<T,N> requires floating-point type");
            static_assert(N > 0, "Polynomial requires at least one coefficient");

            using value_type = T;
            static constexpr size_t degree = N - 1;
            static constexpr size_t size = N;

            std::array<T, N> coeffs{}; // coeffs[i] is coefficient of x^i

            // ===== SERIALIZATION (required for datapod) =====
            auto members() noexcept { return std::tie(coeffs); }
            auto members() const noexcept { return std::tie(coeffs); }

            // Construction
            constexpr Polynomial() noexcept = default;

            // From initializer (variadic)
            template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == N>>
            constexpr Polynomial(Args... args) noexcept : coeffs{static_cast<T>(args)...} {}

            // From array
            constexpr Polynomial(const std::array<T, N> &c) noexcept : coeffs(c) {}

            // Element access
            constexpr T &operator[](size_t i) noexcept { return coeffs[i]; }
            constexpr const T &operator[](size_t i) const noexcept { return coeffs[i]; }

            // Evaluate Polynomial at x using Horner's method
            constexpr T eval(T x) const noexcept {
                T result = coeffs[N - 1];
                for (size_t i = N - 1; i > 0; --i) {
                    result = result * x + coeffs[i - 1];
                }
                return result;
            }

            // Call operator for evaluation
            constexpr T operator()(T x) const noexcept { return eval(x); }

            // Derivative (returns Polynomial of degree N-2)
            constexpr Polynomial<T, (N > 1 ? N - 1 : 1)> derivative() const noexcept {
                if constexpr (N == 1) {
                    return Polynomial<T, 1>{T{0}};
                } else {
                    Polynomial<T, N - 1> result;
                    for (size_t i = 1; i < N; ++i) {
                        result.coeffs[i - 1] = coeffs[i] * static_cast<T>(i);
                    }
                    return result;
                }
            }

            // Integral (returns Polynomial of degree N, constant term = 0)
            constexpr Polynomial<T, N + 1> integral(T constant = T{0}) const noexcept {
                Polynomial<T, N + 1> result;
                result.coeffs[0] = constant;
                for (size_t i = 0; i < N; ++i) {
                    result.coeffs[i + 1] = coeffs[i] / static_cast<T>(i + 1);
                }
                return result;
            }

            // Definite integral from a to b
            constexpr T integrate(T a, T b) const noexcept {
                auto anti = integral();
                return anti.eval(b) - anti.eval(a);
            }

            // Get actual degree (highest non-zero coefficient)
            constexpr size_t actual_degree() const noexcept {
                for (size_t i = N; i > 0; --i) {
                    if (coeffs[i - 1] != T{0})
                        return i - 1;
                }
                return 0;
            }

            // Utility
            constexpr bool is_zero() const noexcept {
                for (size_t i = 0; i < N; ++i) {
                    if (coeffs[i] != T{0})
                        return false;
                }
                return true;
            }

            constexpr bool is_set() const noexcept { return !is_zero(); }

            // Compound assignment
            constexpr Polynomial &operator+=(const Polynomial &other) noexcept {
                for (size_t i = 0; i < N; ++i)
                    coeffs[i] += other.coeffs[i];
                return *this;
            }

            constexpr Polynomial &operator-=(const Polynomial &other) noexcept {
                for (size_t i = 0; i < N; ++i)
                    coeffs[i] -= other.coeffs[i];
                return *this;
            }

            constexpr Polynomial &operator*=(T s) noexcept {
                for (size_t i = 0; i < N; ++i)
                    coeffs[i] *= s;
                return *this;
            }

            constexpr Polynomial &operator/=(T s) noexcept {
                for (size_t i = 0; i < N; ++i)
                    coeffs[i] /= s;
                return *this;
            }

            // Unary operators
            constexpr Polynomial operator-() const noexcept {
                Polynomial result;
                for (size_t i = 0; i < N; ++i)
                    result.coeffs[i] = -coeffs[i];
                return result;
            }

            constexpr Polynomial operator+() const noexcept { return *this; }

            // Comparison
            constexpr bool operator==(const Polynomial &other) const noexcept {
                for (size_t i = 0; i < N; ++i) {
                    if (coeffs[i] != other.coeffs[i])
                        return false;
                }
                return true;
            }

            constexpr bool operator!=(const Polynomial &other) const noexcept { return !(*this == other); }
        };

        // Binary operators - same degree
        template <typename T, size_t N>
        constexpr Polynomial<T, N> operator+(const Polynomial<T, N> &a, const Polynomial<T, N> &b) noexcept {
            Polynomial<T, N> result;
            for (size_t i = 0; i < N; ++i)
                result.coeffs[i] = a.coeffs[i] + b.coeffs[i];
            return result;
        }

        template <typename T, size_t N>
        constexpr Polynomial<T, N> operator-(const Polynomial<T, N> &a, const Polynomial<T, N> &b) noexcept {
            Polynomial<T, N> result;
            for (size_t i = 0; i < N; ++i)
                result.coeffs[i] = a.coeffs[i] - b.coeffs[i];
            return result;
        }

        // Polynomial multiplication (degree adds)
        template <typename T, size_t N, size_t M>
        constexpr Polynomial<T, N + M - 1> operator*(const Polynomial<T, N> &a, const Polynomial<T, M> &b) noexcept {
            Polynomial<T, N + M - 1> result;
            for (size_t i = 0; i < N; ++i) {
                for (size_t j = 0; j < M; ++j) {
                    result.coeffs[i + j] += a.coeffs[i] * b.coeffs[j];
                }
            }
            return result;
        }

        // Scalar operations
        template <typename T, size_t N> constexpr Polynomial<T, N> operator*(const Polynomial<T, N> &p, T s) noexcept {
            Polynomial<T, N> result;
            for (size_t i = 0; i < N; ++i)
                result.coeffs[i] = p.coeffs[i] * s;
            return result;
        }

        template <typename T, size_t N> constexpr Polynomial<T, N> operator*(T s, const Polynomial<T, N> &p) noexcept {
            return p * s;
        }

        template <typename T, size_t N> constexpr Polynomial<T, N> operator/(const Polynomial<T, N> &p, T s) noexcept {
            Polynomial<T, N> result;
            for (size_t i = 0; i < N; ++i)
                result.coeffs[i] = p.coeffs[i] / s;
            return result;
        }

        // Composition: p(q(x))
        template <typename T, size_t N, size_t M>
        constexpr Polynomial<T, (N - 1) * (M - 1) + 1> compose(const Polynomial<T, N> &p,
                                                               const Polynomial<T, M> &q) noexcept {
            constexpr size_t ResultSize = (N - 1) * (M - 1) + 1;
            Polynomial<T, ResultSize> result;
            Polynomial<T, ResultSize> q_power; // q^i
            q_power.coeffs[0] = T{1};          // q^0 = 1

            for (size_t i = 0; i < N; ++i) {
                // Add p[i] * q^i to result
                for (size_t j = 0; j < ResultSize; ++j) {
                    result.coeffs[j] += p.coeffs[i] * q_power.coeffs[j];
                }

                // Compute q^(i+1) for next iteration
                if (i + 1 < N) {
                    Polynomial<T, ResultSize> new_q_power;
                    for (size_t j = 0; j < ResultSize; ++j) {
                        for (size_t k = 0; k < M && j + k < ResultSize; ++k) {
                            new_q_power.coeffs[j + k] += q_power.coeffs[j] * q.coeffs[k];
                        }
                    }
                    q_power = new_q_power;
                }
            }
            return result;
        }

        // Type traits
        template <typename T> struct is_polynomial : std::false_type {};
        template <typename T, size_t N> struct is_polynomial<Polynomial<T, N>> : std::true_type {};
        template <typename T> inline constexpr bool is_polynomial_v = is_polynomial<T>::value;

        // Common type aliases
        template <typename T> using linear = Polynomial<T, 2>;    // ax + b
        template <typename T> using quadratic = Polynomial<T, 3>; // ax² + bx + c
        template <typename T> using cubic = Polynomial<T, 4>;     // ax³ + bx² + cx + d
        template <typename T> using quartic = Polynomial<T, 5>;   // degree 4
        template <typename T> using quintic = Polynomial<T, 6>;   // degree 5

        using linearf = linear<float>;
        using lineard = linear<double>;
        using quadraticf = quadratic<float>;
        using quadraticd = quadratic<double>;
        using cubicf = cubic<float>;
        using cubicd = cubic<double>;

    } // namespace mat
} // namespace datapod
