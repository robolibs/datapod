#pragma once

#include <algorithm>
#include <atomic>
#include <cinttypes>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace datagram {

    // Atomic OR operation - returns the old value before OR
    inline std::uint64_t fetch_or(std::uint64_t &block, std::uint64_t const mask) {
#if defined(_MSC_VER)
        return _InterlockedOr64(reinterpret_cast<std::int64_t *>(&block), mask);
#elif defined(__cpp_lib_atomic_ref)
        return std::atomic_ref{block}.fetch_or(mask);
#else
        return __atomic_fetch_or(&block, mask, __ATOMIC_RELAXED);
#endif
    }

    // Atomic AND operation - returns the old value before AND
    inline std::uint64_t fetch_and(std::uint64_t &block, std::uint64_t const mask) {
#if defined(_MSC_VER)
        return _InterlockedAnd64(reinterpret_cast<std::int64_t *>(&block), mask);
#elif defined(__cpp_lib_atomic_ref)
        return std::atomic_ref{block}.fetch_and(mask);
#else
        return __atomic_fetch_and(&block, mask, __ATOMIC_RELAXED);
#endif
    }

    // Atomic minimum operation - returns the old value before update
    inline std::int16_t fetch_min(std::int16_t &block, std::int16_t const val) {
        auto const a = reinterpret_cast<std::atomic_int16_t *>(&block);
        auto old = a->load();
        if (old > val) {
            while (!a->compare_exchange_weak(old, std::min(old, val), std::memory_order_release,
                                             std::memory_order_relaxed)) {
            }
        }
        return old;
    }

    // Atomic maximum operation - returns the old value before update
    inline std::int16_t fetch_max(std::int16_t &block, std::int16_t const val) {
        auto const a = reinterpret_cast<std::atomic_int16_t *>(&block);
        auto old = a->load();
        if (old < val) {
            while (!a->compare_exchange_weak(old, std::max(old, val), std::memory_order_release,
                                             std::memory_order_relaxed)) {
            }
        }
        return old;
    }

} // namespace datagram
