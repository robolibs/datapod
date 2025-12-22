#pragma once

#include <utility>

namespace bitcon {

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
    };

    // Deduction guide
    template <typename F, typename S> Pair(F, S) -> Pair<F, S>;

    // make_pair helper
    template <typename F, typename S> constexpr Pair<F, S> make_pair(F &&f, S &&s) {
        return Pair<F, S>(std::forward<F>(f), std::forward<S>(s));
    }

} // namespace bitcon
