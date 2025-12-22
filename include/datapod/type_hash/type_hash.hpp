#pragma once

#include <type_traits>

#include "datapod/adapters/optional.hpp"
#include "datapod/adapters/pair.hpp"
#include "datapod/adapters/tuple.hpp"
#include "datapod/adapters/unique_ptr.hpp"
#include "datapod/adapters/variant.hpp"
#include "datapod/associative/hash_storage.hpp"
#include "datapod/associative/map.hpp"
#include "datapod/core/decay.hpp"
#include "datapod/core/equal_to.hpp"
#include "datapod/core/hash.hpp"
#include "datapod/core/strong.hpp"
#include "datapod/core/type_traits.hpp"
#include "datapod/reflection/for_each_field.hpp"
#include "datapod/sequential/array.hpp"
#include "datapod/sequential/string.hpp"
#include "datapod/sequential/vector.hpp"
#include "datapod/type_hash/type_name.hpp"

namespace datapod {

    template <typename T> hash_t type2str_hash() noexcept {
        return hash_combine(hash(canonical_type_str<decay_t<T>>()), sizeof(T));
    }

    template <typename T> hash_t type_hash(T const &, hash_t, Map<hash_t, unsigned> &) noexcept;

    // Array specialization
    template <typename T, std::size_t Size>
    hash_t type_hash(Array<T, Size> const &, hash_t h, Map<hash_t, unsigned> &done) noexcept {
        h = hash_combine(h, hash("array"));
        h = hash_combine(h, Size);
        return type_hash(T{}, h, done);
    }

    // Base template - handles pointers, integrals, scalars, and aggregates
    template <typename T> hash_t type_hash(T const &el, hash_t h, Map<hash_t, unsigned> &done) noexcept {
        using Type = decay_t<T>;

        auto const base_hash = type2str_hash<Type>();
        auto it = done.find(base_hash);
        if (it != done.end()) {
            return hash_combine(h, it->second);
        }
        done.emplace(base_hash, static_cast<unsigned>(done.size()));

        if constexpr (std::is_pointer_v<Type>) {
            using PointeeType = std::remove_pointer_t<Type>;
            if constexpr (std::is_same_v<PointeeType, void>) {
                return hash_combine(h, hash("void*"));
            } else {
                return type_hash(std::remove_pointer_t<Type>{}, hash_combine(h, hash("pointer")), done);
            }
        } else if constexpr (std::is_integral_v<Type>) {
            return hash_combine(h, hash("i"), sizeof(Type));
        } else if constexpr (std::is_scalar_v<Type>) {
            return hash_combine(h, type2str_hash<T>());
        } else {
            static_assert(to_tuple_works_v<Type>, "Please implement custom type hash.");
            h = hash_combine(h, hash("struct"));
            for_each_field(el, [&](auto const &member) noexcept { h = type_hash(member, h, done); });
            return h;
        }
    }

    // Pair specialization
    template <typename A, typename B>
    hash_t type_hash(Pair<A, B> const &, hash_t h, Map<hash_t, unsigned> &done) noexcept {
        h = type_hash(A{}, h, done);
        h = type_hash(B{}, h, done);
        return hash_combine(h, hash("pair"));
    }

    // Vector specialization
    template <typename T, typename Ptr, typename Alloc>
    hash_t type_hash(BasicVector<T, Ptr, Alloc> const &, hash_t h, Map<hash_t, unsigned> &done) noexcept {
        h = hash_combine(h, hash("vector"));
        return type_hash(T{}, h, done);
    }

    // UniquePtr specialization
    template <typename T, typename Ptr>
    hash_t type_hash(UniquePtr<T, Ptr> const &, hash_t h, Map<hash_t, unsigned> &done) noexcept {
        h = hash_combine(h, hash("unique_ptr"));
        return type_hash(T{}, h, done);
    }

    // HashStorage specialization
    template <typename T, template <typename> typename Ptr, typename GetKey, typename GetValue, typename Hash,
              typename Eq>
    hash_t type_hash(HashStorage<T, Ptr, GetKey, GetValue, Hash, Eq> const &, hash_t h,
                     Map<hash_t, unsigned> &done) noexcept {
        h = hash_combine(h, hash("hash_storage"));
        return type_hash(T{}, h, done);
    }

    // Variant specialization
    template <typename... T> hash_t type_hash(Variant<T...> const &, hash_t h, Map<hash_t, unsigned> &done) noexcept {
        h = hash_combine(h, hash("variant"));
        ((h = type_hash(T{}, h, done)), ...);
        return h;
    }

    // Tuple specialization
    template <typename... T> hash_t type_hash(Tuple<T...> const &, hash_t h, Map<hash_t, unsigned> &done) noexcept {
        h = hash_combine(h, hash("tuple"));
        ((h = type_hash(T{}, h, done)), ...);
        return h;
    }

    // String specialization
    template <typename Ptr> hash_t type_hash(BasicString<Ptr> const &, hash_t h, Map<hash_t, unsigned> &) noexcept {
        return hash_combine(h, hash("string"));
    }

    // Strong typedef specialization
    template <typename T, typename Tag>
    hash_t type_hash(Strong<T, Tag> const &, hash_t h, Map<hash_t, unsigned> &done) noexcept {
        h = hash_combine(h, hash("strong"));
        h = type_hash(T{}, h, done);
        h = hash_combine(hash(canonical_type_str<Tag>()), h);
        return h;
    }

    // Optional specialization
    template <typename T> hash_t type_hash(Optional<T> const &, hash_t h, Map<hash_t, unsigned> &done) noexcept {
        h = hash_combine(h, hash("optional"));
        h = type_hash(T{}, h, done);
        return h;
    }

    // Entry point: compute type hash for a type
    template <typename T> hash_t type_hash() {
        auto done = Map<hash_t, unsigned>{};
        return type_hash(T{}, BASE_HASH, done);
    }

} // namespace datapod
