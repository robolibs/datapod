#pragma once

#include <type_traits>

namespace datagram {

    // Helper to enable types based on existence of a type
    template <class T, class R = void> struct EnableIfType {
        using type = R;
    };

    // Type trait to detect if T has a const_iterator member type
    template <typename T, typename = void> struct HasConstIterator : std::false_type {};

    template <typename T>
    struct HasConstIterator<T, typename EnableIfType<typename T::const_iterator>::type> : std::true_type {};

    template <typename T> inline constexpr bool has_const_iterator_v = HasConstIterator<T>::value;

    // Select const_iterator if available, otherwise iterator
    template <typename Container, typename Enable = void> struct ConstIterator {
        using type = typename Container::iterator;
    };

    template <typename Container> struct ConstIterator<Container, std::enable_if_t<has_const_iterator_v<Container>>> {
        using type = typename Container::const_iterator;
    };

    template <typename T> using const_iterator_t = typename ConstIterator<T>::type;

} // namespace datagram
