#pragma once
#include <datapod/types/types.hpp>

#include <cinttypes>
#include <cstddef>
#include <string_view>

namespace datapod {

    // Hash type alias
    using hash_t = datapod::u64;

    // FNV-1a base hash value
    constexpr auto const BASE_HASH = 14695981039346656037ULL;

    // FNV-1a prime
    constexpr auto const FNV_PRIME = 1099511628211ULL;

    // Combine multiple values into a hash using FNV-1a algorithm
    template <typename... Args> constexpr hash_t hash_combine(hash_t h, Args... val) noexcept {
        auto fnv = [&](auto arg) noexcept { h = (h ^ static_cast<hash_t>(arg)) * FNV_PRIME; };
        ((fnv(val)), ...);
        return h;
    }

    // Hash a string_view using FNV-1a
    constexpr hash_t hash(std::string_view s, hash_t h = BASE_HASH) noexcept {
        auto const ptr = s.data();
        for (datapod::usize i = 0U; i < s.size(); ++i) {
            h = hash_combine(h, static_cast<datapod::u8>(ptr[i]));
        }
        return h;
    }

    // Hash a character array literal
    template <datapod::usize N> constexpr hash_t hash(char const (&str)[N], hash_t const h = BASE_HASH) noexcept {
        return hash(std::string_view{str, N - 1U}, h);
    }

    // Hash a buffer/container with size() and operator[]
    template <typename T> constexpr hash_t hash(T const &buf, hash_t const h = BASE_HASH) noexcept {
        return buf.size() == 0U ? h : hash(std::string_view{reinterpret_cast<char const *>(&buf[0U]), buf.size()}, h);
    }

} // namespace datapod
