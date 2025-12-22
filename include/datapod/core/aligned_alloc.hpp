#pragma once

#include <cstdlib>

#include "datapod/core/next_power_of_2.hpp"

namespace datapod {

    // Cross-platform aligned allocation
    inline void *aligned_alloc(std::size_t alignment, std::size_t size) {
        alignment = next_power_of_two(alignment);
        size = to_next_multiple(size, alignment);

#if defined(_MSC_VER)
        return _aligned_malloc(size, alignment);
#elif defined(_LIBCPP_HAS_C11_FEATURES) || defined(_GLIBCXX_HAVE_ALIGNED_ALLOC)
        return std::aligned_alloc(alignment, size);
#else
        return std::malloc(size);
#endif
    }

    // Cross-platform aligned free
    inline void aligned_free([[maybe_unused]] std::size_t alignment, void *ptr) {
#if defined(_MSC_VER)
        _aligned_free(ptr);
#else
        std::free(ptr);
#endif
    }

} // namespace datapod

// Macros for convenience (matching cista style)
#define DATAGRAM_ALIGNED_ALLOC(alignment, size) (datapod::aligned_alloc((alignment), (size)))

#define DATAGRAM_ALIGNED_FREE(alignment, ptr) (datapod::aligned_free((alignment), (ptr)))
