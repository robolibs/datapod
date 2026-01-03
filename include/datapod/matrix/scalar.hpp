#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>

namespace datapod {
    namespace mat {

        /**
         * @brief Scalar (rank-0 tensor) - semantic wrapper for numeric types
         *
         * Pure POD wrapper that provides semantic meaning to numeric values.
         * Used for type safety and reflection in mathematical contexts.
         *
         * Examples:
         *   Scalar<double> temperature{23.5};
         *   Scalar<float> mass{10.5f};
         *   Scalar<int> count{42};
         *
         * Design:
         * - POD-compatible (trivially copyable)
         * - Serializable via members()
         * - Implicit conversion to/from T for ergonomics
         * - All standard numeric operations pass through
         */
        template <typename T> struct Scalar {
            static_assert(std::is_arithmetic_v<T>, "Scalar<T> requires arithmetic type");

            using value_type = T;
            static constexpr size_t rank = 0; // Rank-0 tensor

            T value{};

            // Serialization support
            auto members() noexcept { return std::tie(value); }
            auto members() const noexcept { return std::tie(value); }

            // Construction
            constexpr Scalar() noexcept = default;
            constexpr Scalar(T v) noexcept : value(v) {}

            // Implicit conversion to underlying type (for ergonomics)
            constexpr operator T() const noexcept { return value; }
            constexpr operator T &() noexcept { return value; }

            // Explicit access
            constexpr T get() const noexcept { return value; }
            constexpr T &get() noexcept { return value; }

            // Arithmetic operators (compound assignment)
            constexpr Scalar &operator+=(const Scalar &other) noexcept {
                value += other.value;
                return *this;
            }

            constexpr Scalar &operator-=(const Scalar &other) noexcept {
                value -= other.value;
                return *this;
            }

            constexpr Scalar &operator*=(const Scalar &other) noexcept {
                value *= other.value;
                return *this;
            }

            constexpr Scalar &operator/=(const Scalar &other) noexcept {
                value /= other.value;
                return *this;
            }

            // Allow compound assignment with raw values
            constexpr Scalar &operator+=(T v) noexcept {
                value += v;
                return *this;
            }

            constexpr Scalar &operator-=(T v) noexcept {
                value -= v;
                return *this;
            }

            constexpr Scalar &operator*=(T v) noexcept {
                value *= v;
                return *this;
            }

            constexpr Scalar &operator/=(T v) noexcept {
                value /= v;
                return *this;
            }

            // Unary operators
            constexpr Scalar operator-() const noexcept { return Scalar{-value}; }
            constexpr Scalar operator+() const noexcept { return *this; }

            // Comparison operators
            constexpr bool operator==(const Scalar &other) const noexcept { return value == other.value; }
            constexpr bool operator!=(const Scalar &other) const noexcept { return value != other.value; }
            constexpr bool operator<(const Scalar &other) const noexcept { return value < other.value; }
            constexpr bool operator<=(const Scalar &other) const noexcept { return value <= other.value; }
            constexpr bool operator>(const Scalar &other) const noexcept { return value > other.value; }
            constexpr bool operator>=(const Scalar &other) const noexcept { return value >= other.value; }

            // Comparison with raw values
            constexpr bool operator==(T v) const noexcept { return value == v; }
            constexpr bool operator!=(T v) const noexcept { return value != v; }
            constexpr bool operator<(T v) const noexcept { return value < v; }
            constexpr bool operator<=(T v) const noexcept { return value <= v; }
            constexpr bool operator>(T v) const noexcept { return value > v; }
            constexpr bool operator>=(T v) const noexcept { return value >= v; }
        };

        // Binary arithmetic operators
        template <typename T> constexpr Scalar<T> operator+(const Scalar<T> &lhs, const Scalar<T> &rhs) noexcept {
            return Scalar<T>{lhs.value + rhs.value};
        }

        template <typename T> constexpr Scalar<T> operator-(const Scalar<T> &lhs, const Scalar<T> &rhs) noexcept {
            return Scalar<T>{lhs.value - rhs.value};
        }

        template <typename T> constexpr Scalar<T> operator*(const Scalar<T> &lhs, const Scalar<T> &rhs) noexcept {
            return Scalar<T>{lhs.value * rhs.value};
        }

        template <typename T> constexpr Scalar<T> operator/(const Scalar<T> &lhs, const Scalar<T> &rhs) noexcept {
            return Scalar<T>{lhs.value / rhs.value};
        }

        // Mixed Scalar-value operations
        template <typename T> constexpr Scalar<T> operator+(const Scalar<T> &lhs, T rhs) noexcept {
            return Scalar<T>{lhs.value + rhs};
        }

        template <typename T> constexpr Scalar<T> operator+(T lhs, const Scalar<T> &rhs) noexcept {
            return Scalar<T>{lhs + rhs.value};
        }

        template <typename T> constexpr Scalar<T> operator-(const Scalar<T> &lhs, T rhs) noexcept {
            return Scalar<T>{lhs.value - rhs};
        }

        template <typename T> constexpr Scalar<T> operator-(T lhs, const Scalar<T> &rhs) noexcept {
            return Scalar<T>{lhs - rhs.value};
        }

        template <typename T> constexpr Scalar<T> operator*(const Scalar<T> &lhs, T rhs) noexcept {
            return Scalar<T>{lhs.value * rhs};
        }

        template <typename T> constexpr Scalar<T> operator*(T lhs, const Scalar<T> &rhs) noexcept {
            return Scalar<T>{lhs * rhs.value};
        }

        template <typename T> constexpr Scalar<T> operator/(const Scalar<T> &lhs, T rhs) noexcept {
            return Scalar<T>{lhs.value / rhs};
        }

        template <typename T> constexpr Scalar<T> operator/(T lhs, const Scalar<T> &rhs) noexcept {
            return Scalar<T>{lhs / rhs.value};
        }

        // Type traits
        template <typename T> struct is_scalar : std::false_type {};
        template <typename T> struct is_scalar<Scalar<T>> : std::true_type {};
        template <typename T> inline constexpr bool is_scalar_v = is_scalar<T>::value;

    } // namespace mat
} // namespace datapod
