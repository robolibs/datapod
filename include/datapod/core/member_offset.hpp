#pragma once
#include <datapod/types/types.hpp>

#include <cinttypes>
#include <cstddef>
#include <type_traits>

#include "datapod/core/offset_t.hpp"

namespace datapod {

    // Calculate byte offset of a member within a struct (pointer to object, pointer to member)
    template <typename T, typename Member> offset_t member_offset(T const *t, Member const *m) {
        static_assert(std::is_trivially_copyable_v<T>, "member_offset requires trivially copyable type");
        return static_cast<offset_t>(reinterpret_cast<datapod::u8 const *>(m) -
                                     reinterpret_cast<datapod::u8 const *>(t));
    }

    // Calculate byte offset of a member within a struct (pointer to object, pointer-to-member)
    template <typename T, typename Member> offset_t member_offset(T const *t, Member T::*m) {
        static_assert(std::is_trivially_copyable_v<T>, "member_offset requires trivially copyable type");
        return static_cast<offset_t>(reinterpret_cast<datapod::u8 const *>(&(t->*m)) -
                                     reinterpret_cast<datapod::u8 const *>(t));
    }

} // namespace datapod

// Macro for compile-time member offset when possible, runtime otherwise
#ifndef DATAGRAM_MEMBER_OFFSET
#define DATAGRAM_MEMBER_OFFSET(Type, Member)                                                                           \
    ([]() {                                                                                                            \
        if constexpr (std::is_standard_layout_v<Type>) {                                                               \
            return static_cast<::datapod::offset_t>(offsetof(Type, Member));                                           \
        } else {                                                                                                       \
            return ::datapod::member_offset(static_cast<Type *>(nullptr), &Type::Member);                              \
        }                                                                                                              \
    }())
#endif
