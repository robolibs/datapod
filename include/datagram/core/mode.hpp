#pragma once

#include <type_traits>

namespace datagram {

    // Serialization mode flags (bitmask)
    enum class Mode {
        NONE = 0U,
        UNCHECKED = 1U << 0U,            // Skip all safety checks
        WITH_VERSION = 1U << 1U,         // Include type version hash
        WITH_INTEGRITY = 1U << 2U,       // Include data integrity hash
        SERIALIZE_BIG_ENDIAN = 1U << 3U, // Serialize in big-endian format
        DEEP_CHECK = 1U << 4U,           // Perform deep pointer validation
        CAST = 1U << 5U,                 // Allow type casting on deserialize
        WITH_STATIC_VERSION = 1U << 6U,  // Use static (constexpr) version hash
        SKIP_INTEGRITY = 1U << 7U,       // Skip integrity check on deserialize
        SKIP_VERSION = 1U << 8U,         // Skip version check on deserialize
        _CONST = 1U << 29U,              // Internal: const data marker
        _PHASE_II = 1U << 30U            // Internal: second serialization phase
    };

    // Bitwise OR for combining mode flags
    constexpr Mode operator|(Mode const &a, Mode const &b) noexcept {
        return Mode{static_cast<std::underlying_type_t<Mode>>(a) | static_cast<std::underlying_type_t<Mode>>(b)};
    }

    // Bitwise AND for checking mode flags
    constexpr Mode operator&(Mode const &a, Mode const &b) noexcept {
        return Mode{static_cast<std::underlying_type_t<Mode>>(a) & static_cast<std::underlying_type_t<Mode>>(b)};
    }

    // Check if a specific mode flag is enabled
    constexpr bool is_mode_enabled(Mode const in, Mode const flag) noexcept { return (in & flag) == flag; }

    // Check if a specific mode flag is disabled
    constexpr bool is_mode_disabled(Mode const in, Mode const flag) noexcept { return (in & flag) == Mode::NONE; }

} // namespace datagram
