#pragma once

#include <tuple>
#include <utility>

namespace datapod {

    // Simple pair type (can be used with both raw and offset pointers)
    template <typename First, typename Second> struct Pair {
        using first_type = First;
        using second_type = Second;

        First first;
        Second second;

        constexpr Pair() = default;

        constexpr Pair(First const &f, Second const &s) : first(f), second(s) {}

        constexpr Pair(First &&f, Second &&s) : first(std::move(f)), second(std::move(s)) {}

        template <typename F, typename S>
        constexpr Pair(Pair<F, S> const &other) : first(other.first), second(other.second) {}

        template <typename F, typename S>
        constexpr Pair(Pair<F, S> &&other) : first(std::move(other.first)), second(std::move(other.second)) {}

        constexpr bool operator==(Pair const &other) const { return first == other.first && second == other.second; }

        constexpr bool operator!=(Pair const &other) const { return !(*this == other); }

        constexpr bool operator<(Pair const &other) const {
            return first < other.first || (!(other.first < first) && second < other.second);
        }

        constexpr bool operator<=(Pair const &other) const { return !(other < *this); }

        constexpr bool operator>(Pair const &other) const { return other < *this; }

        constexpr bool operator>=(Pair const &other) const { return !(*this < other); }

        // Structured binding support - get by index
        template <std::size_t I> constexpr auto &get() & {
            static_assert(I < 2, "Index out of range for Pair");
            if constexpr (I == 0) {
                return first;
            } else {
                return second;
            }
        }

        template <std::size_t I> constexpr auto const &get() const & {
            static_assert(I < 2, "Index out of range for Pair");
            if constexpr (I == 0) {
                return first;
            } else {
                return second;
            }
        }

        template <std::size_t I> constexpr auto &&get() && {
            static_assert(I < 2, "Index out of range for Pair");
            if constexpr (I == 0) {
                return std::move(first);
            } else {
                return std::move(second);
            }
        }

        // Swap
        constexpr void swap(Pair &other) noexcept(std::is_nothrow_swappable_v<First> &&
                                                  std::is_nothrow_swappable_v<Second>) {
            using std::swap;
            swap(first, other.first);
            swap(second, other.second);
        }

        // Serialization support
        auto members() noexcept { return std::tie(first, second); }
    };

    // Deduction guide
    template <typename F, typename S> Pair(F, S) -> Pair<F, S>;

    // make_pair helper
    template <typename F, typename S> constexpr Pair<F, S> make_pair(F &&f, S &&s) {
        return Pair<F, S>(std::forward<F>(f), std::forward<S>(s));
    }

    // Free function get() overloads for structured bindings
    template <std::size_t I, typename First, typename Second> constexpr auto &get(Pair<First, Second> &p) {
        return p.template get<I>();
    }

    template <std::size_t I, typename First, typename Second> constexpr auto const &get(Pair<First, Second> const &p) {
        return p.template get<I>();
    }

    template <std::size_t I, typename First, typename Second> constexpr auto &&get(Pair<First, Second> &&p) {
        return std::move(p).template get<I>();
    }

    // Free function swap
    template <typename First, typename Second>
    constexpr void swap(Pair<First, Second> &a, Pair<First, Second> &b) noexcept(noexcept(a.swap(b))) {
        a.swap(b);
    }

    namespace pair {
        /// Placeholder for template type (no useful make() function)
        inline void unimplemented() {}
    } // namespace pair

} // namespace datapod

// Specializations for std::tuple_size and std::tuple_element to enable structured bindings
namespace std {

    template <typename First, typename Second>
    struct tuple_size<datapod::Pair<First, Second>> : integral_constant<size_t, 2> {};

    template <typename First, typename Second> struct tuple_element<0, datapod::Pair<First, Second>> {
        using type = First;
    };

    template <typename First, typename Second> struct tuple_element<1, datapod::Pair<First, Second>> {
        using type = Second;
    };

} // namespace std
