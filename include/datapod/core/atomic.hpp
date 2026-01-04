#pragma once
#include <datapod/types/types.hpp>

#include <algorithm>
#include <atomic>
#include <cinttypes>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace datapod {

    // Atomic OR operation - returns the old value before OR
    inline datapod::u64 fetch_or(datapod::u64 &block, datapod::u64 const mask) {
#if defined(_MSC_VER)
        return _InterlockedOr64(reinterpret_cast<datapod::i64 *>(&block), mask);
#elif defined(__cpp_lib_atomic_ref)
        return std::atomic_ref{block}.fetch_or(mask);
#else
        return __atomic_fetch_or(&block, mask, __ATOMIC_RELAXED);
#endif
    }

    // Atomic AND operation - returns the old value before AND
    inline datapod::u64 fetch_and(datapod::u64 &block, datapod::u64 const mask) {
#if defined(_MSC_VER)
        return _InterlockedAnd64(reinterpret_cast<datapod::i64 *>(&block), mask);
#elif defined(__cpp_lib_atomic_ref)
        return std::atomic_ref{block}.fetch_and(mask);
#else
        return __atomic_fetch_and(&block, mask, __ATOMIC_RELAXED);
#endif
    }

    // Atomic minimum operation - returns the old value before update
    inline datapod::i16 fetch_min(datapod::i16 &block, datapod::i16 const val) {
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
    inline datapod::i16 fetch_max(datapod::i16 &block, datapod::i16 const val) {
        auto const a = reinterpret_cast<std::atomic_int16_t *>(&block);
        auto old = a->load();
        if (old < val) {
            while (!a->compare_exchange_weak(old, std::max(old, val), std::memory_order_release,
                                             std::memory_order_relaxed)) {
            }
        }
        return old;
    }

} // namespace datapod
