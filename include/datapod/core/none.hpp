#pragma once

#include <cstddef>

namespace datapod {

    /**
     * @brief None - Alias for nullptr
     *
     * Provides a more expressive name for nullptr, similar to Rust's None.
     * Can be used with Optional, Result, or raw pointers.
     *
     * Usage:
     * ```cpp
     * Optional<int> opt = None;  // Empty optional
     * int* ptr = None;            // Null pointer
     *
     * if (ptr == None) {
     *     // Handle null case
     * }
     * ```
     */
    constexpr std::nullptr_t None = nullptr;

} // namespace datapod
