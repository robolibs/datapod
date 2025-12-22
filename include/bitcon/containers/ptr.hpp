#pragma once

#include "bitcon/containers/offset_ptr.hpp"

namespace bitcon {

    // Mode tags for pointer selection
    struct RawMode {};
    struct OffsetMode {};

    // Pointer type selection based on mode
    namespace raw {
        template <typename T> using ptr = T *;
    }

    namespace offset {
        template <typename T> using ptr = OffsetPtr<T>;
    }

    // Generic pointer template that works with both modes
    template <typename T, typename Mode> struct Ptr;

    template <typename T> struct Ptr<T, RawMode> {
        using type = raw::ptr<T>;
    };

    template <typename T> struct Ptr<T, OffsetMode> {
        using type = offset::ptr<T>;
    };

    template <typename T, typename Mode> using ptr = typename Ptr<T, Mode>::type;

    // Type traits
    template <typename T> struct IsRawPtr : std::false_type {};

    template <typename T> struct IsRawPtr<T *> : std::true_type {};

    template <typename T> inline constexpr bool is_raw_ptr_v = IsRawPtr<T>::value;

    template <typename T> struct IsPtrType : std::false_type {};

    template <typename T> struct IsPtrType<T *> : std::true_type {};

    template <typename T> struct IsPtrType<OffsetPtr<T>> : std::true_type {};

    template <typename T> inline constexpr bool is_ptr_type_v = IsPtrType<T>::value;

    // Extract pointed-to type
    template <typename T> struct PtrValueType {
        using type = T;
    };

    template <typename T> struct PtrValueType<T *> {
        using type = T;
    };

    template <typename T> struct PtrValueType<OffsetPtr<T>> {
        using type = T;
    };

    template <typename T> using ptr_value_t = typename PtrValueType<T>::type;

} // namespace bitcon
