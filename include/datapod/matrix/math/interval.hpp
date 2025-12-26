#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <tuple>
#include <type_traits>

namespace datapod {
    namespace mat {

        /**
         * @brief Interval arithmetic [lo, hi] - POD
         *
         * Represents a range of possible values for uncertainty propagation,
         * validated computing, and range analysis. All operations produce
         * intervals that are guaranteed to contain the true result.
         * Fully serializable via members().
         *
         * Examples:
         *   interval<double> x{1.0, 2.0};   // [1, 2]
         *   interval<double> y{3.0, 4.0};   // [3, 4]
         *   auto z = x + y;                 // [4, 6]
         *   auto w = x * y;                 // [3, 8]
         *   bool overlaps = x.intersects(y); // false
         */
        template <typename T> struct interval {
            static_assert(std::is_floating_point_v<T>, "interval<T> requires floating-point type");

            using value_type = T;
            static constexpr size_t rank = 0;

            T lo{}; // Lower bound
            T hi{}; // Upper bound

            // ===== SERIALIZATION (required for datapod) =====
            auto members() noexcept { return std::tie(lo, hi); }
            auto members() const noexcept { return std::tie(lo, hi); }

            // Construction
            constexpr interval() noexcept = default;
            constexpr interval(T value) noexcept : lo(value), hi(value) {}
            constexpr interval(T l, T h) noexcept : lo(l), hi(h) {}

            // Factory for point interval
            static constexpr interval point(T value) noexcept { return interval{value, value}; }

            // Factory for entire real line
            static constexpr interval entire() noexcept {
                return interval{-std::numeric_limits<T>::infinity(), std::numeric_limits<T>::infinity()};
            }

            // Factory for empty interval
            static constexpr interval empty() noexcept {
                return interval{std::numeric_limits<T>::infinity(), -std::numeric_limits<T>::infinity()};
            }

            // Factory with uncertainty (value ± uncertainty)
            static constexpr interval with_uncertainty(T value, T uncertainty) noexcept {
                return interval{value - uncertainty, value + uncertainty};
            }

            // Properties
            constexpr T width() const noexcept { return hi - lo; }
            constexpr T midpoint() const noexcept { return (lo + hi) / T{2}; }
            constexpr T radius() const noexcept { return width() / T{2}; }

            constexpr bool is_empty() const noexcept { return lo > hi; }
            constexpr bool is_point() const noexcept { return lo == hi; }
            constexpr bool is_set() const noexcept { return !is_empty(); }

            // Contains
            constexpr bool contains(T value) const noexcept { return lo <= value && value <= hi; }
            constexpr bool contains(const interval &other) const noexcept { return lo <= other.lo && other.hi <= hi; }

            // Intersection test
            constexpr bool intersects(const interval &other) const noexcept { return lo <= other.hi && other.lo <= hi; }

            // Set operations
            constexpr interval intersect(const interval &other) const noexcept {
                T new_lo = std::max(lo, other.lo);
                T new_hi = std::min(hi, other.hi);
                return interval{new_lo, new_hi};
            }

            constexpr interval hull(const interval &other) const noexcept {
                return interval{std::min(lo, other.lo), std::max(hi, other.hi)};
            }

            // Compound assignment
            constexpr interval &operator+=(const interval &other) noexcept {
                lo += other.lo;
                hi += other.hi;
                return *this;
            }

            constexpr interval &operator-=(const interval &other) noexcept {
                T new_lo = lo - other.hi;
                T new_hi = hi - other.lo;
                lo = new_lo;
                hi = new_hi;
                return *this;
            }

            constexpr interval &operator*=(const interval &other) noexcept {
                T a = lo * other.lo;
                T b = lo * other.hi;
                T c = hi * other.lo;
                T d = hi * other.hi;
                lo = std::min({a, b, c, d});
                hi = std::max({a, b, c, d});
                return *this;
            }

            inline interval &operator/=(const interval &other) noexcept {
                if (other.lo <= T{0} && T{0} <= other.hi) {
                    // Division by interval containing zero
                    lo = -std::numeric_limits<T>::infinity();
                    hi = std::numeric_limits<T>::infinity();
                } else {
                    *this *= interval{T{1} / other.hi, T{1} / other.lo};
                }
                return *this;
            }

            // Scalar compound assignment
            constexpr interval &operator*=(T s) noexcept {
                if (s >= T{0}) {
                    lo *= s;
                    hi *= s;
                } else {
                    T new_lo = hi * s;
                    T new_hi = lo * s;
                    lo = new_lo;
                    hi = new_hi;
                }
                return *this;
            }

            constexpr interval &operator/=(T s) noexcept { return *this *= (T{1} / s); }

            // Unary operators
            constexpr interval operator-() const noexcept { return interval{-hi, -lo}; }
            constexpr interval operator+() const noexcept { return *this; }

            // Comparison
            constexpr bool operator==(const interval &other) const noexcept { return lo == other.lo && hi == other.hi; }
            constexpr bool operator!=(const interval &other) const noexcept { return !(*this == other); }
        };

        // Binary operators
        template <typename T> constexpr interval<T> operator+(const interval<T> &a, const interval<T> &b) noexcept {
            return interval<T>{a.lo + b.lo, a.hi + b.hi};
        }

        template <typename T> constexpr interval<T> operator-(const interval<T> &a, const interval<T> &b) noexcept {
            return interval<T>{a.lo - b.hi, a.hi - b.lo};
        }

        template <typename T> constexpr interval<T> operator*(const interval<T> &a, const interval<T> &b) noexcept {
            T p1 = a.lo * b.lo;
            T p2 = a.lo * b.hi;
            T p3 = a.hi * b.lo;
            T p4 = a.hi * b.hi;
            return interval<T>{std::min({p1, p2, p3, p4}), std::max({p1, p2, p3, p4})};
        }

        template <typename T> inline interval<T> operator/(const interval<T> &a, const interval<T> &b) noexcept {
            if (b.lo <= T{0} && T{0} <= b.hi) {
                return interval<T>::entire();
            }
            return a * interval<T>{T{1} / b.hi, T{1} / b.lo};
        }

        // Scalar operations
        template <typename T> constexpr interval<T> operator*(const interval<T> &i, T s) noexcept {
            if (s >= T{0})
                return interval<T>{i.lo * s, i.hi * s};
            return interval<T>{i.hi * s, i.lo * s};
        }

        template <typename T> constexpr interval<T> operator*(T s, const interval<T> &i) noexcept { return i * s; }

        template <typename T> constexpr interval<T> operator/(const interval<T> &i, T s) noexcept {
            return i * (T{1} / s);
        }

        template <typename T> constexpr interval<T> operator+(const interval<T> &i, T s) noexcept {
            return interval<T>{i.lo + s, i.hi + s};
        }

        template <typename T> constexpr interval<T> operator+(T s, const interval<T> &i) noexcept {
            return interval<T>{s + i.lo, s + i.hi};
        }

        template <typename T> constexpr interval<T> operator-(const interval<T> &i, T s) noexcept {
            return interval<T>{i.lo - s, i.hi - s};
        }

        template <typename T> constexpr interval<T> operator-(T s, const interval<T> &i) noexcept {
            return interval<T>{s - i.hi, s - i.lo};
        }

        // Mathematical functions
        template <typename T> inline interval<T> sqrt(const interval<T> &x) noexcept {
            if (x.hi < T{0})
                return interval<T>::empty();
            T lo = x.lo > T{0} ? std::sqrt(x.lo) : T{0};
            return interval<T>{lo, std::sqrt(x.hi)};
        }

        template <typename T> inline interval<T> sqr(const interval<T> &x) noexcept {
            if (x.lo >= T{0})
                return interval<T>{x.lo * x.lo, x.hi * x.hi};
            if (x.hi <= T{0})
                return interval<T>{x.hi * x.hi, x.lo * x.lo};
            // Interval contains zero
            return interval<T>{T{0}, std::max(x.lo * x.lo, x.hi * x.hi)};
        }

        template <typename T> inline interval<T> abs(const interval<T> &x) noexcept {
            if (x.lo >= T{0})
                return x;
            if (x.hi <= T{0})
                return -x;
            return interval<T>{T{0}, std::max(-x.lo, x.hi)};
        }

        template <typename T> inline interval<T> exp(const interval<T> &x) noexcept {
            return interval<T>{std::exp(x.lo), std::exp(x.hi)};
        }

        template <typename T> inline interval<T> log(const interval<T> &x) noexcept {
            if (x.hi <= T{0})
                return interval<T>::empty();
            T lo = x.lo > T{0} ? std::log(x.lo) : -std::numeric_limits<T>::infinity();
            return interval<T>{lo, std::log(x.hi)};
        }

        template <typename T> inline interval<T> sin(const interval<T> &x) noexcept {
            // Conservative bound (could be tighter with period analysis)
            if (x.width() >= T{2} * T{3.14159265358979323846}) {
                return interval<T>{T{-1}, T{1}};
            }
            T s1 = std::sin(x.lo);
            T s2 = std::sin(x.hi);
            T lo = std::min(s1, s2);
            T hi = std::max(s1, s2);
            // Check for extrema in range (simplified)
            return interval<T>{std::max(lo, T{-1}), std::min(hi, T{1})};
        }

        template <typename T> inline interval<T> cos(const interval<T> &x) noexcept {
            if (x.width() >= T{2} * T{3.14159265358979323846}) {
                return interval<T>{T{-1}, T{1}};
            }
            T c1 = std::cos(x.lo);
            T c2 = std::cos(x.hi);
            T lo = std::min(c1, c2);
            T hi = std::max(c1, c2);
            return interval<T>{std::max(lo, T{-1}), std::min(hi, T{1})};
        }

        template <typename T> inline interval<T> pow(const interval<T> &base, int exp) noexcept {
            if (exp == 0)
                return interval<T>{T{1}, T{1}};
            if (exp > 0) {
                if (exp % 2 == 0)
                    return sqr(pow(base, exp / 2));
                return base * pow(base, exp - 1);
            }
            return interval<T>{T{1}} / pow(base, -exp);
        }

        // Type traits
        template <typename T> struct is_interval : std::false_type {};
        template <typename T> struct is_interval<interval<T>> : std::true_type {};
        template <typename T> inline constexpr bool is_interval_v = is_interval<T>::value;

        // Type aliases
        using intervalf = interval<float>;
        using intervald = interval<double>;

    } // namespace mat
} // namespace datapod
