#pragma once

#include "datapod/containers/hash_storage.hpp"
#include "datapod/containers/pair.hpp"
#include "datapod/core/equal_to.hpp"
#include "datapod/hashing.hpp"

namespace datapod {

    // Helper functors for Map
    struct GetFirst {
        template <typename T> auto &&operator()(T &&t) const noexcept { return t.first; }
    };

    struct GetSecond {
        template <typename T> auto &&operator()(T &&t) const noexcept { return t.second; }
    };

    // Map using raw pointers (default)
    template <typename Key, typename Value, typename Hash = Hasher<Key>, typename Eq = EqualTo<Key>>
    using Map = HashStorage<Pair<Key, Value>, raw::ptr, GetFirst, GetSecond, Hash, Eq>;

    // Map using offset pointers (for serialization)
    namespace offset {
        template <typename Key, typename Value, typename Hash = Hasher<Key>, typename Eq = EqualTo<Key>>
        using Map = HashStorage<Pair<Key, Value>, offset::ptr, GetFirst, GetSecond, Hash, Eq>;
    } // namespace offset

    // Backward compatibility - deprecated aliases
    template <typename Key, typename Value, typename Hash = Hasher<Key>, typename Eq = EqualTo<Key>>
    using HashMap [[deprecated("Use Map instead")]] = Map<Key, Value, Hash, Eq>;

    namespace offset {
        template <typename Key, typename Value, typename Hash = Hasher<Key>, typename Eq = EqualTo<Key>>
        using HashMap [[deprecated("Use offset::Map instead")]] = Map<Key, Value, Hash, Eq>;
    } // namespace offset

} // namespace datapod
