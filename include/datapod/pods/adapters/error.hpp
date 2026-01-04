#pragma once

#include <cstdint>
#include <tuple>

#include "../sequential/string.hpp"

namespace datapod {

    /**
     * @brief Error - Simple error type with code and message (POD)
     *
     * Represents an error with a numeric code and descriptive message.
     * Designed for use with Result<T, E> for type-safe error handling.
     *
     * Pure aggregate struct for error representation.
     * Use aggregate initialization: Error{code, "message"}
     * Fully serializable and reflectable.
     *
     * Fields:
     * - code: Error code (0 = no error)
     * - message: Human-readable error description
     *
     * Use cases:
     * - Type-safe error handling with Result<T, Error>
     * - Function return values that can fail
     * - Error propagation without exceptions
     * - Serializable error states
     *
     * Common error codes:
     * - 0: No error (success)
     * - 1: Invalid argument
     * - 2: Out of range
     * - 3: Not found
     * - 4: Permission denied
     * - 5: Already exists
     * - 6: Timeout
     * - 7: IO error
     * - 8: Network error
     * - 9: Parse error
     * - 10+: Application-specific errors
     */
    struct Error {
        uint32_t code = 0; // Error code (0 = no error)
        String message;    // Error description

        auto members() noexcept { return std::tie(code, message); }
        auto members() const noexcept { return std::tie(code, message); }

        // Construction
        Error() = default;
        Error(uint32_t c, const String &msg) : code(c), message(msg) {}
        Error(uint32_t c, const char *msg) : code(c), message(msg) {}

        // Common error codes
        static constexpr uint32_t OK = 0;
        static constexpr uint32_t INVALID_ARGUMENT = 1;
        static constexpr uint32_t OUT_OF_RANGE = 2;
        static constexpr uint32_t NOT_FOUND = 3;
        static constexpr uint32_t PERMISSION_DENIED = 4;
        static constexpr uint32_t ALREADY_EXISTS = 5;
        static constexpr uint32_t TIMEOUT = 6;
        static constexpr uint32_t IO_ERROR = 7;
        static constexpr uint32_t NETWORK_ERROR = 8;
        static constexpr uint32_t PARSE_ERROR = 9;

        // Factory methods for common errors
        static inline Error ok() noexcept { return Error{OK, ""}; }

        static inline Error invalid_argument(const String &msg) noexcept { return Error{INVALID_ARGUMENT, msg}; }

        static inline Error out_of_range(const String &msg) noexcept { return Error{OUT_OF_RANGE, msg}; }

        static inline Error not_found(const String &msg) noexcept { return Error{NOT_FOUND, msg}; }

        static inline Error permission_denied(const String &msg) noexcept { return Error{PERMISSION_DENIED, msg}; }

        static inline Error already_exists(const String &msg) noexcept { return Error{ALREADY_EXISTS, msg}; }

        static inline Error timeout(const String &msg) noexcept { return Error{TIMEOUT, msg}; }

        static inline Error io_error(const String &msg) noexcept { return Error{IO_ERROR, msg}; }

        static inline Error network_error(const String &msg) noexcept { return Error{NETWORK_ERROR, msg}; }

        static inline Error parse_error(const String &msg) noexcept { return Error{PARSE_ERROR, msg}; }

        // Queries
        inline bool is_ok() const noexcept { return code == OK; }

        inline bool is_err() const noexcept { return code != OK; }

        inline explicit operator bool() const noexcept { return is_err(); }

        // Comparison
        inline bool operator==(const Error &other) const noexcept {
            return code == other.code && message == other.message;
        }

        inline bool operator!=(const Error &other) const noexcept { return !(*this == other); }

        // Compare by code only
        inline bool same_code(const Error &other) const noexcept { return code == other.code; }
    };

    namespace error {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace error

} // namespace datapod
