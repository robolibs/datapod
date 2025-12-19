#pragma once

#include "datagram/containers/hash_storage.hpp"
#include "datagram/containers/pair.hpp"
#include "datagram/core/equal_to.hpp"
#include "datagram/hashing.hpp"

namespace datagram {

    // Helper functors for hash_map
    struct GetFirst {
        template <typename T> auto &&operator()(T &&t) const noexcept { return t.first; }
    };

    struct GetSecond {
        template <typename T> auto &&operator()(T &&t) const noexcept { return t.second; }
    };

    // Hash map using raw pointers
    template <typename Key, typename Value, typename Hash = Hasher<Key>, typename Eq = EqualTo<Key>>
    using HashMap = HashStorage<Pair<Key, Value>, raw::ptr, GetFirst, GetSecond, Hash, Eq>;

    // Hash map using offset pointers (for serialization)
    namespace offset {
        template <typename Key, typename Value, typename Hash = Hasher<Key>, typename Eq = EqualTo<Key>>
        using HashMap = HashStorage<Pair<Key, Value>, offset::ptr, GetFirst, GetSecond, Hash, Eq>;
    } // namespace offset

} // namespace datagram
