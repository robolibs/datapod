#pragma once

#include "datapod/core/equal_to.hpp"
#include "datapod/hashing.hpp"
#include "datapod/pods/associative/hash_storage.hpp"

namespace datapod {

    // Identity functor for Set (GetKey returns the element itself)
    struct Identity {
        template <typename T> decltype(auto) operator()(T &&t) const noexcept { return std::forward<T>(t); }
    };

    // Set using raw pointers (default)
    template <typename T, typename Hash = Hasher<T>, typename Eq = EqualTo<T>>
    using Set = HashStorage<T, raw::ptr, Identity, Identity, Hash, Eq>;

    // Set using offset pointers (for serialization)
    namespace offset {
        template <typename T, typename Hash = Hasher<T>, typename Eq = EqualTo<T>>
        using Set = HashStorage<T, offset::ptr, Identity, Identity, Hash, Eq>;
    } // namespace offset

} // namespace datapod
