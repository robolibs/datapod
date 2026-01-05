#pragma once

namespace datapod {

    /**
     * @brief Unit - Zero-sized type representing "no value"
     *
     * A unit type (also called "void type" or "empty tuple") that represents
     * the absence of a meaningful value. Unlike `void`, Unit is a complete type
     * that can be stored, passed, and returned.
     *
     * Use cases:
     * - Result<Unit, E> for operations that can fail but return no value
     * - Generic programming where a type parameter is required but no value needed
     * - Signaling success without data (alternative to bool)
     *
     * Example:
     * ```cpp
     * Result<Unit, Error> save_file(const String& path) {
     *     if (failed) return Result<Unit, Error>::err(Error::io_error("write failed"));
     *     return Result<Unit, Error>::ok(Unit{});
     * }
     *
     * auto result = save_file("/tmp/data.txt");
     * if (result.is_ok()) {
     *     // Success, no value to extract
     * }
     * ```
     */
    struct Unit {
        constexpr Unit() noexcept = default;

        constexpr bool operator==(const Unit &) const noexcept { return true; }
        constexpr bool operator!=(const Unit &) const noexcept { return false; }
        constexpr bool operator<(const Unit &) const noexcept { return false; }
        constexpr bool operator<=(const Unit &) const noexcept { return true; }
        constexpr bool operator>(const Unit &) const noexcept { return false; }
        constexpr bool operator>=(const Unit &) const noexcept { return true; }
    };

    /// Alias for Unit - alternative name matching C++ void keyword
    using Void = Unit;

    /// Global constant for convenience
    inline constexpr Unit unit{};

} // namespace datapod
