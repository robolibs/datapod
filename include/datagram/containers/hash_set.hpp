#pragma once

#include "datagram/containers/hash_storage.hpp"
#include "datagram/core/equal_to.hpp"
#include "datagram/hashing.hpp"

namespace datagram {

    // Identity functor for hash_set (GetKey returns the element itself)
    struct Identity {
        template <typename T> decltype(auto) operator()(T &&t) const noexcept { return std::forward<T>(t); }
    };

    // Hash set using raw pointers
    template <typename T, typename Hash = Hasher<T>, typename Eq = EqualTo<T>>
    using HashSet = HashStorage<T, raw::ptr, Identity, Identity, Hash, Eq>;

    // Hash set using offset pointers (for serialization)
    namespace offset {
        template <typename T, typename Hash = Hasher<T>, typename Eq = EqualTo<T>>
        using HashSet = HashStorage<T, offset::ptr, Identity, Identity, Hash, Eq>;
    } // namespace offset

} // namespace datagram
