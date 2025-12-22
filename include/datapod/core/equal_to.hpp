#pragma once

#include <algorithm>
#include <type_traits>

#include "datapod/adapters/pair.hpp"
#include "datapod/core/decay.hpp"
#include "datapod/core/type_traits.hpp"
#include "datapod/reflection/to_tuple.hpp"

namespace datapod {

    namespace detail {

        template <class F, class Tuple, std::size_t... I>
        constexpr bool tuple_equal_impl(F &&is_equal, Tuple &&a, Tuple &&b, std::index_sequence<I...>) {
            return (is_equal(std::get<I>(std::forward<Tuple>(a)), std::get<I>(std::forward<Tuple>(b))) && ...);
        }

    } // namespace detail

    template <class F, class Tuple> constexpr decltype(auto) tuple_equal(F &&is_equal, Tuple &&a, Tuple &&b) {
        return detail::tuple_equal_impl(std::forward<F>(is_equal), std::forward<Tuple>(a), std::forward<Tuple>(b),
                                        std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
    }

    // Type trait to detect if two types are equality comparable
    template <typename A, typename B, typename = void> struct is_eq_comparable : std::false_type {};

    template <typename A, typename B>
    struct is_eq_comparable<A, B, std::void_t<decltype(std::declval<A>() == std::declval<B>())>> : std::true_type {};

    template <typename A, typename B> inline constexpr bool is_eq_comparable_v = is_eq_comparable<A, B>::value;

    // Check if to_tuple works for a type
    template <typename T, typename = void> struct to_tuple_works : std::false_type {};

    template <typename T>
    struct to_tuple_works<T, std::void_t<decltype(to_tuple(std::declval<T &>()))>> : std::true_type {};

    template <typename T> inline constexpr bool to_tuple_works_v = to_tuple_works<T>::value;

    // Generic equality comparison functor
    template <typename T> struct EqualTo {
        template <typename T1> constexpr bool operator()(T const &a, T1 const &b) const {
            using Type = decay_t<T>;
            using Type1 = decay_t<T1>;

            // IMPORTANT: Check operator== FIRST before to_tuple!
            // Otherwise primitive types like int will use to_tuple and compare incorrectly
            if constexpr (is_eq_comparable_v<Type, Type1>) {
                // Has operator== - use it
                return a == b;
            } else if constexpr (is_iterable_v<Type> && is_iterable_v<Type1>) {
                // Both are iterable - compare element-wise
                using std::begin;
                using std::end;
                return std::equal(begin(a), end(a), begin(b), end(b),
                                  [](auto &&x, auto &&y) { return EqualTo<decltype(x)>{}(x, y); });
            } else if constexpr (to_tuple_works_v<Type> && to_tuple_works_v<Type1>) {
                // Both are aggregate types - compare via reflection
                return tuple_equal([](auto &&x, auto &&y) { return EqualTo<decltype(x)>{}(x, y); }, to_tuple(a),
                                   to_tuple(b));
            } else {
                static_assert(is_eq_comparable_v<Type, Type1> || is_iterable_v<Type> || to_tuple_works_v<Type>,
                              "Type must be equality comparable, iterable, or an aggregate type");
                return false;
            }
        }
    };

    // Specialization for Pair
    template <typename A, typename B> struct EqualTo<Pair<A, B>> {
        template <typename T1> constexpr bool operator()(Pair<A, B> const &a, T1 const &b) const {
            return a.first == b.first && a.second == b.second;
        }
    };

} // namespace datapod
