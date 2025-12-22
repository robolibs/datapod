#pragma once

#include <limits>
#include <ostream>
#include <type_traits>
#include <utility>

namespace bitcon {

    // Strong typedef template - creates distinct types from underlying types
    template <typename T, typename Tag> struct Strong {
        using value_t = T;

        constexpr Strong() = default;

        explicit constexpr Strong(T const &v) noexcept(std::is_nothrow_copy_constructible_v<T>) : v_{v} {}

        explicit constexpr Strong(T &&v) noexcept(std::is_nothrow_move_constructible_v<T>) : v_{std::move(v)} {}

        template <typename X,
                  std::enable_if_t<std::is_integral_v<std::decay_t<X>> && std::is_integral_v<std::decay_t<T>>, int> = 0>
        explicit constexpr Strong(X &&x) : v_{static_cast<T>(x)} {}

        constexpr Strong(Strong &&o) noexcept(std::is_nothrow_move_constructible_v<T>) = default;
        constexpr Strong &operator=(Strong &&o) noexcept(std::is_nothrow_move_constructible_v<T>) = default;

        constexpr Strong(Strong const &o) = default;
        constexpr Strong &operator=(Strong const &o) = default;

        static constexpr Strong invalid() { return Strong{std::numeric_limits<T>::max()}; }

        constexpr Strong &operator++() {
            ++v_;
            return *this;
        }

        constexpr Strong operator++(int) {
            auto cpy = *this;
            ++v_;
            return cpy;
        }

        constexpr Strong &operator--() {
            --v_;
            return *this;
        }

        constexpr Strong const operator--(int) {
            auto cpy = *this;
            --v_;
            return cpy;
        }

        constexpr Strong operator+(Strong const &s) const { return Strong{static_cast<value_t>(v_ + s.v_)}; }
        constexpr Strong operator-(Strong const &s) const { return Strong{static_cast<value_t>(v_ - s.v_)}; }
        constexpr Strong operator*(Strong const &s) const { return Strong{static_cast<value_t>(v_ * s.v_)}; }
        constexpr Strong operator/(Strong const &s) const { return Strong{static_cast<value_t>(v_ / s.v_)}; }
        constexpr Strong operator+(T const &i) const { return Strong{static_cast<value_t>(v_ + i)}; }
        constexpr Strong operator-(T const &i) const { return Strong{static_cast<value_t>(v_ - i)}; }
        constexpr Strong operator*(T const &i) const { return Strong{static_cast<value_t>(v_ * i)}; }
        constexpr Strong operator/(T const &i) const { return Strong{static_cast<value_t>(v_ / i)}; }

        constexpr Strong &operator+=(T const &i) {
            v_ += i;
            return *this;
        }
        constexpr Strong &operator-=(T const &i) {
            v_ -= i;
            return *this;
        }

        constexpr Strong operator>>(T const &i) const { return Strong{static_cast<value_t>(v_ >> i)}; }
        constexpr Strong operator<<(T const &i) const { return Strong{static_cast<value_t>(v_ << i)}; }
        constexpr Strong operator>>(Strong const &o) const { return v_ >> o.v_; }
        constexpr Strong operator<<(Strong const &o) const { return v_ << o.v_; }

        constexpr Strong &operator|=(Strong const &o) {
            v_ |= o.v_;
            return *this;
        }
        constexpr Strong &operator&=(Strong const &o) {
            v_ &= o.v_;
            return *this;
        }

        constexpr bool operator==(Strong const &o) const { return v_ == o.v_; }
        constexpr bool operator!=(Strong const &o) const { return v_ != o.v_; }
        constexpr bool operator<=(Strong const &o) const { return v_ <= o.v_; }
        constexpr bool operator>=(Strong const &o) const { return v_ >= o.v_; }
        constexpr bool operator<(Strong const &o) const { return v_ < o.v_; }
        constexpr bool operator>(Strong const &o) const { return v_ > o.v_; }

        constexpr bool operator==(T const &o) const { return v_ == o; }
        constexpr bool operator!=(T const &o) const { return v_ != o; }
        constexpr bool operator<=(T const &o) const { return v_ <= o; }
        constexpr bool operator>=(T const &o) const { return v_ >= o; }
        constexpr bool operator<(T const &o) const { return v_ < o; }
        constexpr bool operator>(T const &o) const { return v_ > o; }

        constexpr explicit operator T const &() const & noexcept { return v_; }

        friend std::ostream &operator<<(std::ostream &o, Strong const &t) { return o << t.v_; }

        T v_;
    };

    // Type trait to detect Strong types
    template <typename T> struct IsStrong : std::false_type {};

    template <typename T, typename Tag> struct IsStrong<Strong<T, Tag>> : std::true_type {};

    template <typename T> constexpr auto const is_strong_v = IsStrong<T>::value;

    // Extract the underlying value from a Strong type
    template <typename T, typename Tag>
    inline constexpr typename Strong<T, Tag>::value_t to_idx(Strong<T, Tag> const &s) {
        return s.v_;
    }

    // Pass-through for non-Strong types
    template <typename T> constexpr T to_idx(T const &t) { return t; }

    // Extract base type from Strong or return T itself
    template <typename T> struct BaseType {
        using type = T;
    };

    template <typename T, typename Tag> struct BaseType<Strong<T, Tag>> {
        using type = T;
    };

    template <typename T> using base_t = typename BaseType<T>::type;

} // namespace bitcon

// Specialize std::numeric_limits for Strong types
namespace std {

    template <typename T, typename Tag> class numeric_limits<bitcon::Strong<T, Tag>> {
      public:
        static constexpr bitcon::Strong<T, Tag> min() noexcept {
            return bitcon::Strong<T, Tag>{std::numeric_limits<T>::min()};
        }
        static constexpr bitcon::Strong<T, Tag> max() noexcept {
            return bitcon::Strong<T, Tag>{std::numeric_limits<T>::max()};
        }
        static constexpr bool is_integer = std::is_integral_v<T>;
    };

    template <typename T, typename Tag> struct hash<bitcon::Strong<T, Tag>> {
        size_t operator()(bitcon::Strong<T, Tag> const &t) const { return hash<T>{}(t.v_); }
    };

} // namespace std
