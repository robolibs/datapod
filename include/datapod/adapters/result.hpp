#pragma once

#include <tuple>
#include <utility>

#include "error.hpp"
#include "variant.hpp"

namespace datapod {

    /**
     * @brief Result<T, E> - Type-safe error handling (POD when T and E are POD)
     *
     * Rust-style Result type for functions that can fail.
     * Contains either a success value (T) or an error (E).
     *
     * Template struct for error handling without exceptions.
     * Fully serializable and reflectable.
     *
     * Fields:
     * - data: Variant holding either T (success) or E (error)
     *
     * Use cases:
     * - Type-safe error handling
     * - Functions that can fail with specific error types
     * - Error propagation without exceptions
     * - Embedded/real-time systems (no exception overhead)
     *
     * Example:
     * ```cpp
     * Result<int, Error> divide(int a, int b) {
     *     if (b == 0) return Result<int, Error>::err(Error::invalid_argument("Division by zero"));
     *     return Result<int, Error>::ok(a / b);
     * }
     *
     * auto result = divide(10, 2);
     * if (result.is_ok()) {
     *     std::cout << "Result: " << result.value() << std::endl;
     * } else {
     *     std::cout << "Error: " << result.error().message.c_str() << std::endl;
     * }
     * ```
     */
    template <typename T, typename E = Error> struct Result {
        Variant<T, E> data;

        auto members() noexcept { return std::tie(data); }
        auto members() const noexcept { return std::tie(data); }

        // Construction
        Result() = default;

        // Factory methods for Ok and Err
        static inline Result ok(const T &value) noexcept {
            Result r;
            r.data.template emplace<0>(value);
            return r;
        }

        static inline Result ok(T &&value) noexcept {
            Result r;
            r.data.template emplace<0>(std::move(value));
            return r;
        }

        static inline Result err(const E &error) noexcept {
            Result r;
            r.data.template emplace<1>(error);
            return r;
        }

        static inline Result err(E &&error) noexcept {
            Result r;
            r.data.template emplace<1>(std::move(error));
            return r;
        }

        // Queries
        inline bool is_ok() const noexcept { return data.index() == 0; }

        inline bool is_err() const noexcept { return data.index() == 1; }

        inline explicit operator bool() const noexcept { return is_ok(); }

        // Access - throws if wrong variant
        inline T &value() & { return get<T>(data); }

        inline const T &value() const & { return get<T>(data); }

        inline T &&value() && { return std::move(get<T>(data)); }

        inline E &error() & { return get<E>(data); }

        inline const E &error() const & { return get<E>(data); }

        inline E &&error() && { return std::move(get<E>(data)); }

        // Safe access with defaults
        inline T value_or(const T &default_value) const & noexcept { return is_ok() ? value() : default_value; }

        inline T value_or(T &&default_value) && noexcept {
            return is_ok() ? std::move(value()) : std::move(default_value);
        }

        // Monadic operations (Rust-style)

        // and_then: Chain operations that return Result
        // If Ok(T), applies function f and returns its Result
        // If Err(E), returns Err(E) unchanged
        template <typename F> inline auto and_then(F &&f) & -> decltype(f(value())) {
            using U = decltype(f(value()));
            if (is_ok()) {
                return f(value());
            }
            return U::err(error());
        }

        template <typename F> inline auto and_then(F &&f) const & -> decltype(f(value())) {
            using U = decltype(f(value()));
            if (is_ok()) {
                return f(value());
            }
            return U::err(error());
        }

        template <typename F> inline auto and_then(F &&f) && -> decltype(f(std::move(*this).value())) {
            using U = decltype(f(std::move(*this).value()));
            if (is_ok()) {
                return f(std::move(*this).value());
            }
            return U::err(std::move(*this).error());
        }

        // or_else: Recover from error
        // If Ok(T), returns Ok(T) unchanged
        // If Err(E), applies function f and returns its Result
        template <typename F> inline auto or_else(F &&f) & -> decltype(f(error())) {
            if (is_err()) {
                return f(error());
            }
            using U = decltype(f(error()));
            return U::ok(value());
        }

        template <typename F> inline auto or_else(F &&f) const & -> decltype(f(error())) {
            if (is_err()) {
                return f(error());
            }
            using U = decltype(f(error()));
            return U::ok(value());
        }

        template <typename F> inline auto or_else(F &&f) && -> decltype(f(std::move(*this).error())) {
            if (is_err()) {
                return f(std::move(*this).error());
            }
            using U = decltype(f(std::move(*this).error()));
            return U::ok(std::move(*this).value());
        }

        // map: Transform the success value
        // If Ok(T), applies function f to value and returns Ok(U)
        // If Err(E), returns Err(E) unchanged
        template <typename F> inline auto map(F &&f) & -> Result<decltype(f(value())), E> {
            using U = decltype(f(value()));
            if (is_ok()) {
                return Result<U, E>::ok(f(value()));
            }
            return Result<U, E>::err(error());
        }

        template <typename F> inline auto map(F &&f) const & -> Result<decltype(f(value())), E> {
            using U = decltype(f(value()));
            if (is_ok()) {
                return Result<U, E>::ok(f(value()));
            }
            return Result<U, E>::err(error());
        }

        template <typename F> inline auto map(F &&f) && -> Result<decltype(f(std::move(*this).value())), E> {
            using U = decltype(f(std::move(*this).value()));
            if (is_ok()) {
                return Result<U, E>::ok(f(std::move(*this).value()));
            }
            return Result<U, E>::err(std::move(*this).error());
        }

        // map_err: Transform the error value
        // If Ok(T), returns Ok(T) unchanged
        // If Err(E), applies function f to error and returns Err(F)
        template <typename F> inline auto map_err(F &&f) & -> Result<T, decltype(f(error()))> {
            using F_type = decltype(f(error()));
            if (is_err()) {
                return Result<T, F_type>::err(f(error()));
            }
            return Result<T, F_type>::ok(value());
        }

        template <typename F> inline auto map_err(F &&f) const & -> Result<T, decltype(f(error()))> {
            using F_type = decltype(f(error()));
            if (is_err()) {
                return Result<T, F_type>::err(f(error()));
            }
            return Result<T, F_type>::ok(value());
        }

        template <typename F> inline auto map_err(F &&f) && -> Result<T, decltype(f(std::move(*this).error()))> {
            using F_type = decltype(f(std::move(*this).error()));
            if (is_err()) {
                return Result<T, F_type>::err(f(std::move(*this).error()));
            }
            return Result<T, F_type>::ok(std::move(*this).value());
        }

        // Comparison
        inline bool operator==(const Result &other) const noexcept { return data == other.data; }

        inline bool operator!=(const Result &other) const noexcept { return !(*this == other); }
    };

    // Convenience type alias
    template <typename T> using Res = Result<T, Error>;

} // namespace datapod
