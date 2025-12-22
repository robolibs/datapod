#pragma once

#include <cstddef>
#include <cstring>
#include <type_traits>

#include "datapod/core/hash.hpp"
#include "datapod/core/type_traits.hpp"
#include "datapod/reflection/for_each_field.hpp"

// Include container headers directly since this is meant to be included after them
#include "datapod/containers/array.hpp"
#include "datapod/containers/optional.hpp"
#include "datapod/containers/pair.hpp"
#include "datapod/containers/string.hpp"
#include "datapod/containers/unique_ptr.hpp"
#include "datapod/containers/vector.hpp"

namespace datapod {

    // Type trait to detect if a type has a hash_value() member function
    template <typename T, typename = void> struct has_hash_value : std::false_type {};

    template <typename T>
    struct has_hash_value<T, std::void_t<decltype(std::declval<T const &>().hash_value())>> : std::true_type {};

    template <typename T> inline constexpr bool has_hash_value_v = has_hash_value<T>::value;

    // Primary hashing template
    template <typename T> struct Hasher;

    // Generic hash function - dispatches to appropriate Hasher specialization
    template <typename T> constexpr hash_t hash_value(T const &val, hash_t h = BASE_HASH) noexcept {
        return Hasher<T>{}(val, h);
    }

    // ==================== Hasher Specializations ====================

    // Hasher for fundamental types (int, float, etc.)
    template <typename T>
    requires std::is_fundamental_v<T>
    struct Hasher<T> {
        constexpr hash_t operator()(T const &val, hash_t h = BASE_HASH) const noexcept {
            if constexpr (sizeof(T) == 1) {
                return hash_combine(h, static_cast<std::uint8_t>(val));
            } else if constexpr (sizeof(T) <= sizeof(std::uint64_t)) {
                // Hash bytes of the value
                auto const ptr = reinterpret_cast<unsigned char const *>(&val);
                for (std::size_t i = 0; i < sizeof(T); ++i) {
                    h = hash_combine(h, ptr[i]);
                }
                return h;
            } else {
                // Larger types - hash as byte array
                return hash(std::string_view{reinterpret_cast<char const *>(&val), sizeof(T)}, h);
            }
        }
    };

    // Hasher for enums
    template <typename T>
    requires std::is_enum_v<T>
    struct Hasher<T> {
        constexpr hash_t operator()(T const &val, hash_t h = BASE_HASH) const noexcept {
            return hash_value(static_cast<std::underlying_type_t<T>>(val), h);
        }
    };

    // Hasher for pointers
    template <typename T>
    requires std::is_pointer_v<T>
    struct Hasher<T> {
        constexpr hash_t operator()(T const &val, hash_t h = BASE_HASH) const noexcept {
            return hash_value(reinterpret_cast<std::uintptr_t>(val), h);
        }
    };

    // Hasher for types with hash_value() member function
    template <typename T>
    requires has_hash_value_v<T> && (!std::is_fundamental_v<T>) && (!std::is_enum_v<T>) && (!std::is_pointer_v<T>)
    struct Hasher<T> {
        constexpr hash_t operator()(T const &val, hash_t h = BASE_HASH) const noexcept { return val.hash_value(h); }
    };

    // Hasher for BasicString
    template <typename Ptr> struct Hasher<BasicString<Ptr>> {
        constexpr hash_t operator()(BasicString<Ptr> const &str, hash_t h = BASE_HASH) const noexcept {
            return hash(std::string_view{str.data(), str.size()}, h);
        }
    };

    // Hasher for BasicVector
    template <typename T, typename Ptr, typename Alloc> struct Hasher<BasicVector<T, Ptr, Alloc>> {
        constexpr hash_t operator()(BasicVector<T, Ptr, Alloc> const &vec, hash_t h = BASE_HASH) const noexcept {
            h = hash_combine(h, vec.size());
            for (auto const &elem : vec) {
                h = hash_value(elem, h);
            }
            return h;
        }
    };

    // Hasher for Optional
    template <typename T> struct Hasher<Optional<T>> {
        constexpr hash_t operator()(Optional<T> const &opt, hash_t h = BASE_HASH) const noexcept {
            if (opt.has_value()) {
                h = hash_combine(h, 1); // has value marker
                return hash_value(opt.value(), h);
            } else {
                return hash_combine(h, 0); // no value marker
            }
        }
    };

    // Hasher for Array
    template <typename T, std::size_t N> struct Hasher<Array<T, N>> {
        constexpr hash_t operator()(Array<T, N> const &arr, hash_t h = BASE_HASH) const noexcept {
            for (std::size_t i = 0; i < N; ++i) {
                h = hash_value(arr[i], h);
            }
            return h;
        }
    };

    // Hasher for Pair
    template <typename K, typename V> struct Hasher<Pair<K, V>> {
        constexpr hash_t operator()(Pair<K, V> const &p, hash_t h = BASE_HASH) const noexcept {
            h = hash_value(p.first, h);
            h = hash_value(p.second, h);
            return h;
        }
    };

    // Hasher for aggregate types using reflection
    template <typename T>
    requires std::is_aggregate_v<T> && (!has_hash_value_v<T>) && (!is_iterable_v<std::remove_cv_t<T>>) &&
             (!std::is_fundamental_v<T>) && (!std::is_enum_v<T>) && (!std::is_pointer_v<T>)
    struct Hasher<T> {
        constexpr hash_t operator()(T const &val, hash_t h = BASE_HASH) const noexcept {
            for_each_field(const_cast<T &>(val), [&h](auto &&field) { h = hash_value(field, h); });
            return h;
        }
    };

    // Hasher for std::string_view (commonly used)
    template <> struct Hasher<std::string_view> {
        constexpr hash_t operator()(std::string_view const &sv, hash_t h = BASE_HASH) const noexcept {
            return hash(sv, h);
        }
    };

    // Hasher for C-style strings
    template <std::size_t N> struct Hasher<char[N]> {
        constexpr hash_t operator()(char const (&str)[N], hash_t h = BASE_HASH) const noexcept { return hash(str, h); }
    };

    // Hasher for const char*
    template <> struct Hasher<char const *> {
        constexpr hash_t operator()(char const *str, hash_t h = BASE_HASH) const noexcept {
            return hash(std::string_view{str}, h);
        }
    };

    // ==================== STL Compatibility ====================

    // STL-compatible hash functor (for std::unordered_map, etc.)
    template <typename T> struct Hash {
        constexpr std::size_t operator()(T const &val) const noexcept {
            return static_cast<std::size_t>(hash_value(val));
        }
    };

} // namespace datapod
