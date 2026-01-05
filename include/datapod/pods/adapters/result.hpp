#pragma once

#include <tuple>
#include <utility>

#include "datapod/core/unit.hpp"
#include "error.hpp"
#include "optional.hpp"
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

        // Query operations with predicates
        template <typename F> inline bool is_ok_and(F &&predicate) const { return is_ok() && predicate(value()); }

        template <typename F> inline bool is_err_and(F &&predicate) const { return is_err() && predicate(error()); }

        // Inspect operations (for debugging/side effects)
        template <typename F> inline const Result &inspect(F &&f) const & {
            if (is_ok()) {
                f(value());
            }
            return *this;
        }

        template <typename F> inline Result &inspect(F &&f) & {
            if (is_ok()) {
                f(value());
            }
            return *this;
        }

        template <typename F> inline Result &&inspect(F &&f) && {
            if (is_ok()) {
                f(value());
            }
            return std::move(*this);
        }

        template <typename F> inline const Result &inspect_err(F &&f) const & {
            if (is_err()) {
                f(error());
            }
            return *this;
        }

        template <typename F> inline Result &inspect_err(F &&f) & {
            if (is_err()) {
                f(error());
            }
            return *this;
        }

        template <typename F> inline Result &&inspect_err(F &&f) && {
            if (is_err()) {
                f(error());
            }
            return std::move(*this);
        }

        // Expect with custom messages
        inline T &expect(const char *msg) & {
            if (is_err()) {
                throw std::runtime_error(msg);
            }
            return value();
        }

        inline const T &expect(const char *msg) const & {
            if (is_err()) {
                throw std::runtime_error(msg);
            }
            return value();
        }

        inline T &&expect(const char *msg) && {
            if (is_err()) {
                throw std::runtime_error(msg);
            }
            return std::move(*this).value();
        }

        inline E &expect_err(const char *msg) & {
            if (is_ok()) {
                throw std::runtime_error(msg);
            }
            return error();
        }

        inline const E &expect_err(const char *msg) const & {
            if (is_ok()) {
                throw std::runtime_error(msg);
            }
            return error();
        }

        inline E &&expect_err(const char *msg) && {
            if (is_ok()) {
                throw std::runtime_error(msg);
            }
            return std::move(*this).error();
        }

        // Unwrap operations with defaults
        inline T unwrap_or(const T &default_value) const & noexcept { return is_ok() ? value() : default_value; }

        inline T unwrap_or(T &&default_value) && noexcept {
            return is_ok() ? std::move(*this).value() : std::move(default_value);
        }

        template <typename F> inline T unwrap_or_else(F &&f) const & { return is_ok() ? value() : f(error()); }

        template <typename F> inline T unwrap_or_else(F &&f) && {
            return is_ok() ? std::move(*this).value() : f(std::move(*this).error());
        }

        template <typename U = T>
        inline auto unwrap_or_default() const & ->
            typename std::enable_if<std::is_default_constructible<U>::value, U>::type {
            return is_ok() ? value() : U{};
        }

        template <typename U = T>
        inline auto unwrap_or_default() && ->
            typename std::enable_if<std::is_default_constructible<U>::value, U>::type {
            return is_ok() ? std::move(*this).value() : U{};
        }

        // Flatten Result<Result<T, E>, E> to Result<T, E>
        template <typename U, typename F,
                  typename = typename std::enable_if<std::is_same<T, Result<U, F>>::value>::type>
        inline Result<U, E> flatten() && {
            if (is_ok()) {
                return std::move(*this).value();
            }
            return Result<U, E>::err(std::move(*this).error());
        }

        template <typename U, typename F,
                  typename = typename std::enable_if<std::is_same<T, Result<U, F>>::value>::type>
        inline Result<U, E> flatten() const & {
            if (is_ok()) {
                return value();
            }
            return Result<U, E>::err(error());
        }

        // Conversion to Optional
        inline Optional<T> ok() const & {
            if (is_ok()) {
                return Optional<T>{value()};
            }
            return nullopt;
        }

        inline Optional<T> ok() && {
            if (is_ok()) {
                return Optional<T>{std::move(*this).value()};
            }
            return nullopt;
        }

        inline Optional<E> err() const & {
            if (is_err()) {
                return Optional<E>{error()};
            }
            return nullopt;
        }

        inline Optional<E> err() && {
            if (is_err()) {
                return Optional<E>{std::move(*this).error()};
            }
            return nullopt;
        }

        // Transpose Result<Optional<T>, E> to Optional<Result<T, E>>
        template <typename U, typename = typename std::enable_if<std::is_same<T, Optional<U>>::value>::type>
        inline Optional<Result<U, E>> transpose() const & {
            if (is_err()) {
                return Optional<Result<U, E>>{Result<U, E>::err(error())};
            }
            const Optional<U> &opt = value();
            if (opt.has_value()) {
                return Optional<Result<U, E>>{Result<U, E>::ok(*opt)};
            }
            return nullopt;
        }

        template <typename U, typename = typename std::enable_if<std::is_same<T, Optional<U>>::value>::type>
        inline Optional<Result<U, E>> transpose() && {
            if (is_err()) {
                return Optional<Result<U, E>>{Result<U, E>::err(std::move(*this).error())};
            }
            Optional<U> opt = std::move(*this).value();
            if (opt.has_value()) {
                return Optional<Result<U, E>>{Result<U, E>::ok(std::move(*opt))};
            }
            return nullopt;
        }

        // Iterator support (iterates over Ok value, empty for Err)
        using iterator = T *;
        using const_iterator = T const *;

        inline iterator begin() noexcept { return is_ok() ? &value() : nullptr; }

        inline const_iterator begin() const noexcept { return is_ok() ? &value() : nullptr; }

        inline const_iterator cbegin() const noexcept { return begin(); }

        inline iterator end() noexcept { return is_ok() ? &value() + 1 : nullptr; }

        inline const_iterator end() const noexcept { return is_ok() ? &value() + 1 : nullptr; }

        inline const_iterator cend() const noexcept { return end(); }

        // Comparison
        inline bool operator==(const Result &other) const noexcept { return data == other.data; }

        inline bool operator!=(const Result &other) const noexcept { return !(*this == other); }
    };

    /**
     * @brief Result<void, E> - Specialization for operations with no return value
     *
     * Used for functions that can fail but don't return a meaningful value on success.
     * Internally stores only the error (if any), making it more efficient than
     * Result<Unit, E> while providing the same semantics.
     *
     * Example:
     * ```cpp
     * Result<void, Error> save_file(const String& path) {
     *     if (failed) return Result<void, Error>::err(Error::io_error("write failed"));
     *     return Result<void, Error>::ok();
     * }
     *
     * auto result = save_file("/tmp/data.txt");
     * if (result.is_ok()) {
     *     // Success!
     * } else {
     *     std::cout << "Error: " << result.error().message.c_str() << std::endl;
     * }
     * ```
     */
    template <typename E> struct Result<void, E> {
        Optional<E> error_;

        auto members() noexcept { return std::tie(error_); }
        auto members() const noexcept { return std::tie(error_); }

        // Construction
        Result() = default;

        // Factory methods
        static inline Result ok() noexcept {
            Result r;
            r.error_ = nullopt;
            return r;
        }

        static inline Result err(const E &error) noexcept {
            Result r;
            r.error_ = error;
            return r;
        }

        static inline Result err(E &&error) noexcept {
            Result r;
            r.error_ = std::move(error);
            return r;
        }

        // Queries
        inline bool is_ok() const noexcept { return !error_.has_value(); }

        inline bool is_err() const noexcept { return error_.has_value(); }

        inline explicit operator bool() const noexcept { return is_ok(); }

        // Access - no value() for void specialization
        inline E &error() & { return *error_; }

        inline const E &error() const & { return *error_; }

        inline E &&error() && { return std::move(*error_); }

        // Monadic operations

        // and_then: Chain operations that return Result
        // If Ok, applies function f and returns its Result
        // If Err(E), returns Err(E) unchanged
        template <typename F> inline auto and_then(F &&f) & -> decltype(f()) {
            using U = decltype(f());
            if (is_ok()) {
                return f();
            }
            return U::err(error());
        }

        template <typename F> inline auto and_then(F &&f) const & -> decltype(f()) {
            using U = decltype(f());
            if (is_ok()) {
                return f();
            }
            return U::err(error());
        }

        template <typename F> inline auto and_then(F &&f) && -> decltype(f()) {
            using U = decltype(f());
            if (is_ok()) {
                return f();
            }
            return U::err(std::move(*this).error());
        }

        // or_else: Recover from error
        // If Ok, returns Ok unchanged
        // If Err(E), applies function f and returns its Result
        template <typename F> inline auto or_else(F &&f) & -> decltype(f(error())) {
            if (is_err()) {
                return f(error());
            }
            using U = decltype(f(error()));
            return U::ok();
        }

        template <typename F> inline auto or_else(F &&f) const & -> decltype(f(error())) {
            if (is_err()) {
                return f(error());
            }
            using U = decltype(f(error()));
            return U::ok();
        }

        template <typename F> inline auto or_else(F &&f) && -> decltype(f(std::move(*this).error())) {
            if (is_err()) {
                return f(std::move(*this).error());
            }
            using U = decltype(f(std::move(*this).error()));
            return U::ok();
        }

        // map: Transform success (void -> U)
        // If Ok, applies function f and returns Ok(U)
        // If Err(E), returns Err(E) unchanged
        template <typename F> inline auto map(F &&f) & -> Result<decltype(f()), E> {
            using U = decltype(f());
            if (is_ok()) {
                return Result<U, E>::ok(f());
            }
            return Result<U, E>::err(error());
        }

        template <typename F> inline auto map(F &&f) const & -> Result<decltype(f()), E> {
            using U = decltype(f());
            if (is_ok()) {
                return Result<U, E>::ok(f());
            }
            return Result<U, E>::err(error());
        }

        template <typename F> inline auto map(F &&f) && -> Result<decltype(f()), E> {
            using U = decltype(f());
            if (is_ok()) {
                return Result<U, E>::ok(f());
            }
            return Result<U, E>::err(std::move(*this).error());
        }

        // map_err: Transform the error value
        // If Ok, returns Ok unchanged
        // If Err(E), applies function f to error and returns Err(F)
        template <typename F> inline auto map_err(F &&f) & -> Result<void, decltype(f(error()))> {
            using F_type = decltype(f(error()));
            if (is_err()) {
                return Result<void, F_type>::err(f(error()));
            }
            return Result<void, F_type>::ok();
        }

        template <typename F> inline auto map_err(F &&f) const & -> Result<void, decltype(f(error()))> {
            using F_type = decltype(f(error()));
            if (is_err()) {
                return Result<void, F_type>::err(f(error()));
            }
            return Result<void, F_type>::ok();
        }

        template <typename F> inline auto map_err(F &&f) && -> Result<void, decltype(f(std::move(*this).error()))> {
            using F_type = decltype(f(std::move(*this).error()));
            if (is_err()) {
                return Result<void, F_type>::err(f(std::move(*this).error()));
            }
            return Result<void, F_type>::ok();
        }

        // Query operations with predicates
        template <typename F> inline bool is_ok_and(F &&predicate) const { return is_ok() && predicate(); }

        template <typename F> inline bool is_err_and(F &&predicate) const { return is_err() && predicate(error()); }

        // Inspect operations (for debugging/side effects)
        template <typename F> inline const Result &inspect(F &&f) const & {
            if (is_ok()) {
                f();
            }
            return *this;
        }

        template <typename F> inline Result &inspect(F &&f) & {
            if (is_ok()) {
                f();
            }
            return *this;
        }

        template <typename F> inline Result &&inspect(F &&f) && {
            if (is_ok()) {
                f();
            }
            return std::move(*this);
        }

        template <typename F> inline const Result &inspect_err(F &&f) const & {
            if (is_err()) {
                f(error());
            }
            return *this;
        }

        template <typename F> inline Result &inspect_err(F &&f) & {
            if (is_err()) {
                f(error());
            }
            return *this;
        }

        template <typename F> inline Result &&inspect_err(F &&f) && {
            if (is_err()) {
                f(error());
            }
            return std::move(*this);
        }

        // Expect with custom messages
        inline void expect(const char *msg) const {
            if (is_err()) {
                throw std::runtime_error(msg);
            }
        }

        inline E &expect_err(const char *msg) & {
            if (is_ok()) {
                throw std::runtime_error(msg);
            }
            return error();
        }

        inline const E &expect_err(const char *msg) const & {
            if (is_ok()) {
                throw std::runtime_error(msg);
            }
            return error();
        }

        inline E &&expect_err(const char *msg) && {
            if (is_ok()) {
                throw std::runtime_error(msg);
            }
            return std::move(*this).error();
        }

        // Conversion to Optional (err only, no ok() since void has no value)
        inline Optional<E> err() const & {
            if (is_err()) {
                return Optional<E>{error()};
            }
            return nullopt;
        }

        inline Optional<E> err() && {
            if (is_err()) {
                return Optional<E>{std::move(*this).error()};
            }
            return nullopt;
        }

        // Comparison
        inline bool operator==(const Result &other) const noexcept { return error_ == other.error_; }

        inline bool operator!=(const Result &other) const noexcept { return !(*this == other); }
    };

    // Convenience type alias
    template <typename T> using Res = Result<T, Error>;

    /// Convenience alias for Result<void, Error>
    using VoidRes = Result<void, Error>;

    // Helper functions for copied/cloned (for Result<T*, E>)
    template <typename T, typename E>
    inline auto copied(const Result<T *, E> &result) ->
        typename std::enable_if<std::is_copy_constructible<T>::value, Result<T, E>>::type {
        if (result.is_ok()) {
            return Result<T, E>::ok(*result.value());
        }
        return Result<T, E>::err(result.error());
    }

    template <typename T, typename E>
    inline auto cloned(const Result<T *, E> &result) ->
        typename std::enable_if<std::is_copy_constructible<T>::value, Result<T, E>>::type {
        if (result.is_ok()) {
            return Result<T, E>::ok(*result.value());
        }
        return Result<T, E>::err(result.error());
    }

    namespace result {

        // ========================================================================
        // Intermediate types for ergonomic Result construction
        // ========================================================================

        /**
         * @brief Intermediate type for Ok values - converts to any Result<T, E>
         *
         * Used by ok() and Ok() to create a value that implicitly converts
         * to the appropriate Result type based on context.
         */
        template <typename T> struct OkValue {
            T value;

            template <typename E> operator Result<T, E>() && { return Result<T, E>::ok(std::move(value)); }

            template <typename E> operator Result<T, E>() const & { return Result<T, E>::ok(value); }
        };

        /**
         * @brief Intermediate type for Ok<void> - converts to any Result<void, E>
         *
         * Used by ok() and Ok() with no arguments to create a void success
         * that implicitly converts to Result<void, E>.
         */
        struct OkVoid {
            template <typename E> operator Result<void, E>() && { return Result<void, E>::ok(); }

            template <typename E> operator Result<void, E>() const & { return Result<void, E>::ok(); }
        };

        /**
         * @brief Intermediate type for Err values - converts to any Result<T, E>
         *
         * Used by err() and Err() to create an error that implicitly converts
         * to the appropriate Result type based on context.
         */
        template <typename E> struct ErrValue {
            E error;

            template <typename T> operator Result<T, E>() && { return Result<T, E>::err(std::move(error)); }

            template <typename T> operator Result<T, E>() const & { return Result<T, E>::err(error); }
        };

        // ========================================================================
        // Factory functions (snake_case style)
        // ========================================================================

        /**
         * @brief Create an Ok value that converts to Result<T, E>
         *
         * Example:
         * ```cpp
         * Result<int, Error> divide(int a, int b) {
         *     if (b == 0) return result::err(Error::invalid_argument("div by zero"));
         *     return result::ok(a / b);
         * }
         * ```
         */
        template <typename T> inline OkValue<std::decay_t<T>> ok(T &&value) { return {std::forward<T>(value)}; }

        /**
         * @brief Create an Ok<void> value that converts to Result<void, E>
         *
         * Example:
         * ```cpp
         * Result<void, Error> save() {
         *     if (failed) return result::err(Error::io_error("write failed"));
         *     return result::ok();
         * }
         * ```
         */
        inline OkVoid ok() { return {}; }

        /**
         * @brief Create an Err value that converts to Result<T, E>
         *
         * Example:
         * ```cpp
         * Result<int, Error> parse(const String& s) {
         *     if (s.empty()) return result::err(Error::invalid_argument("empty string"));
         *     return result::ok(std::stoi(s.c_str()));
         * }
         * ```
         */
        template <typename E> inline ErrValue<std::decay_t<E>> err(E &&error) { return {std::forward<E>(error)}; }

        // ========================================================================
        // Factory functions (PascalCase style - Rust-like)
        // ========================================================================

        /**
         * @brief Create an Ok value that converts to Result<T, E> (PascalCase variant)
         *
         * Example:
         * ```cpp
         * Result<int, Error> divide(int a, int b) {
         *     if (b == 0) return result::Err(Error::invalid_argument("div by zero"));
         *     return result::Ok(a / b);
         * }
         * ```
         */
        template <typename T> inline OkValue<std::decay_t<T>> Ok(T &&value) { return {std::forward<T>(value)}; }

        /**
         * @brief Create an Ok<void> value that converts to Result<void, E> (PascalCase variant)
         */
        inline OkVoid Ok() { return {}; }

        /**
         * @brief Create an Err value that converts to Result<T, E> (PascalCase variant)
         *
         * Example:
         * ```cpp
         * Result<int, Error> parse(const String& s) {
         *     if (s.empty()) return result::Err(Error::invalid_argument("empty string"));
         *     return result::Ok(std::stoi(s.c_str()));
         * }
         * ```
         */
        template <typename E> inline ErrValue<std::decay_t<E>> Err(E &&error) { return {std::forward<E>(error)}; }

    } // namespace result

} // namespace datapod
