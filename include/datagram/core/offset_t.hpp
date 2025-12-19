#pragma once

#include <cinttypes>
#include <limits>

// Printf format specifier for offset_t
#define PRI_O PRIdPTR

namespace datagram {

    // Core offset type used for self-relative pointers
    using offset_t = std::intptr_t;

    // Sentinel value representing nullptr in offset space
    constexpr auto const NULLPTR_OFFSET = std::numeric_limits<offset_t>::min();

    // Sentinel value representing a dangling pointer
    constexpr auto const DANGLING = std::numeric_limits<offset_t>::min() + 1;

} // namespace datagram
