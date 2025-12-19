#pragma once

#include <cstddef>
#include <type_traits>

namespace datagram {

    // Detect if T is a character array (char[N] or char const[N])
    template <typename T, typename = void> struct IsCharArrayHelper : std::false_type {};

    template <std::size_t N> struct IsCharArrayHelper<char const[N]> : std::true_type {};

    template <std::size_t N> struct IsCharArrayHelper<char[N]> : std::true_type {};

    template <typename T> constexpr bool is_char_array_v = IsCharArrayHelper<T>::value;

    // Detect if T is a string type (specialized by string.hpp)
    template <typename Ptr> struct IsStringHelper : std::false_type {};

    template <typename T> constexpr bool is_string_v = IsStringHelper<std::remove_cv_t<T>>::value;

    // Detect if T is iterable (has begin/end)
    template <typename T, typename = void> struct IsIterableHelper : std::false_type {};

    template <typename T>
    struct IsIterableHelper<T, std::void_t<decltype(std::declval<T>().begin()), decltype(std::declval<T>().end())>>
        : std::true_type {};

    template <typename T> constexpr bool is_iterable_v = IsIterableHelper<T>::value;

} // namespace datagram
