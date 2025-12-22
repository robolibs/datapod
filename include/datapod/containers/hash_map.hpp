#pragma once

#include "datapod/containers/hash_storage.hpp"
#include "datapod/containers/pair.hpp"
#include "datapod/core/equal_to.hpp"
#include "datapod/hashing.hpp"

namespace datapod {

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

} // namespace datapod
