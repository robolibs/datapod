#pragma once

#include <type_traits>

namespace datagram {

    // Marker template to indicate that elements should be tracked during serialization
    // (for pointer resolution within vector ranges)
    template <typename T> struct Indexed : public T {
        using value_type = T;
        using T::T;
        using T::operator=;
    };

    // Type trait to detect Indexed types
    template <typename Ptr> struct IsIndexedHelper : std::false_type {};

    template <typename T> struct IsIndexedHelper<Indexed<T>> : std::true_type {};

    template <class T> constexpr bool is_indexed_v = IsIndexedHelper<std::remove_cv_t<T>>::value;

} // namespace datagram
