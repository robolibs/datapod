#pragma once

#include <cstddef>

namespace bitcon {

    // Round up to the next power of two
    template <typename T> constexpr T next_power_of_two(T n) noexcept {
        --n;
        n |= n >> 1U;
        n |= n >> 2U;
        n |= n >> 4U;
        if constexpr (sizeof(T) > 1U) {
            n |= n >> 8U;
        }
        if constexpr (sizeof(T) > 2U) {
            n |= n >> 16U;
        }
        if constexpr (sizeof(T) > 4U) {
            n |= n >> 32U;
        }
        ++n;
        return n;
    }

    // Round up n to the next multiple of 'multiple'
    template <typename T1, typename T2> constexpr T1 to_next_multiple(T1 const n, T2 const multiple) noexcept {
        auto const r = n % multiple;
        return r == 0 ? n : n + multiple - r;
    }

} // namespace bitcon
