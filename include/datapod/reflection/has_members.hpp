#pragma once

#include <type_traits>
#include <utility>

namespace datapod {
    namespace detail {

        // SFINAE detection for members() function
        // Tests both const and non-const versions

        template <typename T, typename = void> struct has_members_impl : std::false_type {};

        template <typename T>
        struct has_members_impl<T, std::void_t<decltype(std::declval<T &>().members())>> : std::true_type {};

        template <typename T> inline constexpr bool has_members_v = has_members_impl<T>::value;

        // Also check const version
        template <typename T, typename = void> struct has_const_members_impl : std::false_type {};

        template <typename T>
        struct has_const_members_impl<T, std::void_t<decltype(std::declval<const T &>().members())>> : std::true_type {
        };

        template <typename T> inline constexpr bool has_const_members_v = has_const_members_impl<T>::value;

        // Helper: Check if members() returns a tuple-like type
        template <typename T, typename = void> struct members_returns_tuple : std::false_type {};

        template <typename T>
        struct members_returns_tuple<
            T, std::void_t<decltype(std::tuple_size<decltype(std::declval<T &>().members())>::value)>>
            : std::true_type {};

        template <typename T> inline constexpr bool members_returns_tuple_v = members_returns_tuple<T>::value;

    } // namespace detail

    // Public API
    template <typename T> inline constexpr bool has_members_v = detail::has_members_v<T>;

    template <typename T> inline constexpr bool has_const_members_v = detail::has_const_members_v<T>;

} // namespace datapod
