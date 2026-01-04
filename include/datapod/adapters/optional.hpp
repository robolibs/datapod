#pragma once

#include <functional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace datapod {

    // Forward declarations
    template <typename T, typename E> struct Result;

    struct nullopt_t {
        explicit constexpr nullopt_t(int) {}
    };

    inline constexpr nullopt_t nullopt{0};

    // Optional value container
    template <typename T> class Optional {
      public:
        using value_type = T;

        // Constructors
        constexpr Optional() noexcept : has_value_(false) {}

        constexpr Optional(nullopt_t) noexcept : has_value_(false) {}

        constexpr Optional(T const &value) : has_value_(true) { new (&storage_) T(value); }

        constexpr Optional(T &&value) : has_value_(true) { new (&storage_) T(std::move(value)); }

        // Copy constructor
        constexpr Optional(Optional const &other) : has_value_(other.has_value_) {
            if (has_value_) {
                new (&storage_) T(*other);
            }
        }

        // Move constructor
        constexpr Optional(Optional &&other) noexcept : has_value_(other.has_value_) {
            if (has_value_) {
                new (&storage_) T(std::move(*other));
                other.reset();
            }
        }

        // Destructor
        ~Optional() { reset(); }

        // Assignment
        Optional &operator=(nullopt_t) noexcept {
            reset();
            return *this;
        }

        Optional &operator=(T const &value) {
            if (has_value_) {
                **this = value;
            } else {
                new (&storage_) T(value);
                has_value_ = true;
            }
            return *this;
        }

        Optional &operator=(T &&value) {
            if (has_value_) {
                **this = std::move(value);
            } else {
                new (&storage_) T(std::move(value));
                has_value_ = true;
            }
            return *this;
        }

        Optional &operator=(Optional const &other) {
            if (this != &other) {
                if (other.has_value_) {
                    *this = *other;
                } else {
                    reset();
                }
            }
            return *this;
        }

        Optional &operator=(Optional &&other) noexcept {
            if (this != &other) {
                if (other.has_value_) {
                    *this = std::move(*other);
                    other.reset();
                } else {
                    reset();
                }
            }
            return *this;
        }

        // Observers
        constexpr T const *operator->() const noexcept { return reinterpret_cast<T const *>(&storage_); }

        constexpr T *operator->() noexcept { return reinterpret_cast<T *>(&storage_); }

        constexpr T const &operator*() const & noexcept { return *reinterpret_cast<T const *>(&storage_); }

        constexpr T &operator*() & noexcept { return *reinterpret_cast<T *>(&storage_); }

        constexpr T const &&operator*() const && noexcept { return std::move(*reinterpret_cast<T const *>(&storage_)); }

        constexpr T &&operator*() && noexcept { return std::move(*reinterpret_cast<T *>(&storage_)); }

        constexpr explicit operator bool() const noexcept { return has_value_; }

        constexpr bool has_value() const noexcept { return has_value_; }

        constexpr T &value() & {
            if (!has_value_) {
                throw std::runtime_error("bad optional access");
            }
            return **this;
        }

        constexpr T const &value() const & {
            if (!has_value_) {
                throw std::runtime_error("bad optional access");
            }
            return **this;
        }

        constexpr T &&value() && {
            if (!has_value_) {
                throw std::runtime_error("bad optional access");
            }
            return std::move(**this);
        }

        template <typename U> constexpr T value_or(U &&default_value) const & {
            return has_value_ ? **this : static_cast<T>(std::forward<U>(default_value));
        }

        template <typename U> constexpr T value_or(U &&default_value) && {
            return has_value_ ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
        }

        // Modifiers
        void reset() noexcept {
            if (has_value_) {
                reinterpret_cast<T *>(&storage_)->~T();
                has_value_ = false;
            }
        }

        template <typename... Args> T &emplace(Args &&...args) {
            reset();
            new (&storage_) T(std::forward<Args>(args)...);
            has_value_ = true;
            return **this;
        }

        void swap(Optional &other) noexcept {
            if (has_value_ && other.has_value_) {
                std::swap(**this, *other);
            } else if (has_value_) {
                new (&other.storage_) T(std::move(**this));
                other.has_value_ = true;
                reset();
            } else if (other.has_value_) {
                new (&storage_) T(std::move(*other));
                has_value_ = true;
                other.reset();
            }
        }

        // Monadic operations (C++23)
        template <typename F> constexpr auto and_then(F &&f) & {
            using U = std::invoke_result_t<F, T &>;
            static_assert(is_optional_impl<std::remove_cvref_t<U>>::value, "F must return an Optional");
            if (has_value_) {
                return std::invoke(std::forward<F>(f), **this);
            } else {
                return std::remove_cvref_t<U>{};
            }
        }

        template <typename F> constexpr auto and_then(F &&f) const & {
            using U = std::invoke_result_t<F, T const &>;
            static_assert(is_optional_impl<std::remove_cvref_t<U>>::value, "F must return an Optional");
            if (has_value_) {
                return std::invoke(std::forward<F>(f), **this);
            } else {
                return std::remove_cvref_t<U>{};
            }
        }

        template <typename F> constexpr auto and_then(F &&f) && {
            using U = std::invoke_result_t<F, T &&>;
            static_assert(is_optional_impl<std::remove_cvref_t<U>>::value, "F must return an Optional");
            if (has_value_) {
                return std::invoke(std::forward<F>(f), std::move(**this));
            } else {
                return std::remove_cvref_t<U>{};
            }
        }

        template <typename F> constexpr auto transform(F &&f) & {
            using U = std::invoke_result_t<F, T &>;
            if (has_value_) {
                return Optional<U>{std::invoke(std::forward<F>(f), **this)};
            } else {
                return Optional<U>{};
            }
        }

        template <typename F> constexpr auto transform(F &&f) const & {
            using U = std::invoke_result_t<F, T const &>;
            if (has_value_) {
                return Optional<U>{std::invoke(std::forward<F>(f), **this)};
            } else {
                return Optional<U>{};
            }
        }

        template <typename F> constexpr auto transform(F &&f) && {
            using U = std::invoke_result_t<F, T &&>;
            if (has_value_) {
                return Optional<U>{std::invoke(std::forward<F>(f), std::move(**this))};
            } else {
                return Optional<U>{};
            }
        }

        template <typename F> constexpr auto or_else(F &&f) const & {
            using U = std::invoke_result_t<F>;
            static_assert(std::is_same_v<std::remove_cvref_t<U>, Optional>, "F must return Optional<T>");
            if (has_value_) {
                return *this;
            } else {
                return std::invoke(std::forward<F>(f));
            }
        }

        template <typename F> constexpr auto or_else(F &&f) && {
            using U = std::invoke_result_t<F>;
            static_assert(std::is_same_v<std::remove_cvref_t<U>, Optional>, "F must return Optional<T>");
            if (has_value_) {
                return std::move(*this);
            } else {
                return std::invoke(std::forward<F>(f));
            }
        }

        // Query operations with predicates
        template <typename F> constexpr bool is_some_and(F &&predicate) const {
            return has_value_ && std::invoke(std::forward<F>(predicate), **this);
        }

        template <typename F> constexpr bool is_none_or(F &&predicate) const {
            return !has_value_ || std::invoke(std::forward<F>(predicate), **this);
        }

        // Filter operation
        template <typename F> constexpr Optional filter(F &&predicate) const & {
            if (has_value_ && std::invoke(std::forward<F>(predicate), **this)) {
                return *this;
            }
            return nullopt;
        }

        template <typename F> constexpr Optional filter(F &&predicate) && {
            if (has_value_ && std::invoke(std::forward<F>(predicate), **this)) {
                return std::move(*this);
            }
            return nullopt;
        }

        // Inspect operation (for debugging/side effects)
        template <typename F> constexpr Optional const &inspect(F &&f) const & {
            if (has_value_) {
                std::invoke(std::forward<F>(f), **this);
            }
            return *this;
        }

        template <typename F> constexpr Optional &inspect(F &&f) & {
            if (has_value_) {
                std::invoke(std::forward<F>(f), **this);
            }
            return *this;
        }

        template <typename F> constexpr Optional &&inspect(F &&f) && {
            if (has_value_) {
                std::invoke(std::forward<F>(f), **this);
            }
            return std::move(*this);
        }

        // Expect with custom message
        constexpr T &expect(char const *msg) & {
            if (!has_value_) {
                throw std::runtime_error(msg);
            }
            return **this;
        }

        constexpr T const &expect(char const *msg) const & {
            if (!has_value_) {
                throw std::runtime_error(msg);
            }
            return **this;
        }

        constexpr T &&expect(char const *msg) && {
            if (!has_value_) {
                throw std::runtime_error(msg);
            }
            return std::move(**this);
        }

        // Take value, leaving None
        constexpr Optional take() noexcept {
            if (has_value_) {
                Optional result{std::move(**this)};
                reset();
                return result;
            }
            return nullopt;
        }

        // Take if predicate passes
        template <typename F> constexpr Optional take_if(F &&predicate) {
            if (has_value_ && std::invoke(std::forward<F>(predicate), **this)) {
                Optional result{std::move(**this)};
                reset();
                return result;
            }
            return nullopt;
        }

        // Replace value, return old
        constexpr Optional replace(T &&value) {
            Optional old = take();
            emplace(std::move(value));
            return old;
        }

        constexpr Optional replace(T const &value) {
            Optional old = take();
            emplace(value);
            return old;
        }

        // Flatten Optional<Optional<U>> to Optional<U>
        // Only enabled when T is Optional<U>
        template <typename U, typename = std::enable_if_t<std::is_same_v<T, Optional<U>>>>
        constexpr Optional<U> flatten() && {
            if (has_value_) {
                return std::move(**this);
            }
            return Optional<U>{};
        }

        template <typename U, typename = std::enable_if_t<std::is_same_v<T, Optional<U>>>>
        constexpr Optional<U> flatten() const & {
            if (has_value_) {
                return **this;
            }
            return Optional<U>{};
        }

        // Zip two Optionals into Optional<Pair>
        template <typename U> constexpr auto zip(Optional<U> const &other) const {
            if (has_value_ && other.has_value()) {
                return Optional<std::pair<T, U>>{std::make_pair(**this, *other)};
            }
            return Optional<std::pair<T, U>>{};
        }

        template <typename U> constexpr auto zip(Optional<U> &&other) {
            if (has_value_ && other.has_value()) {
                return Optional<std::pair<T, U>>{std::make_pair(std::move(**this), std::move(*other))};
            }
            return Optional<std::pair<T, U>>{};
        }

        // Zip with function
        template <typename U, typename F> constexpr auto zip_with(Optional<U> const &other, F &&f) const {
            using R = std::invoke_result_t<F, T const &, U const &>;
            if (has_value_ && other.has_value()) {
                return Optional<R>{std::invoke(std::forward<F>(f), **this, *other)};
            }
            return Optional<R>{};
        }

        template <typename U, typename F> constexpr auto zip_with(Optional<U> &&other, F &&f) {
            using R = std::invoke_result_t<F, T &&, U &&>;
            if (has_value_ && other.has_value()) {
                return Optional<R>{std::invoke(std::forward<F>(f), std::move(**this), std::move(*other))};
            }
            return Optional<R>{};
        }

        // Unwrap with default (requires Default trait)
        template <typename U = T>
        constexpr auto unwrap_or_default() && -> std::enable_if_t<std::is_default_constructible_v<U>, U> {
            if (has_value_) {
                return std::move(**this);
            }
            return U{};
        }

        template <typename U = T>
        constexpr auto unwrap_or_default() const & -> std::enable_if_t<std::is_default_constructible_v<U>, U> {
            if (has_value_) {
                return **this;
            }
            return U{};
        }

        // Get or insert value
        constexpr T &get_or_insert(T &&value) {
            if (!has_value_) {
                emplace(std::move(value));
            }
            return **this;
        }

        constexpr T &get_or_insert(T const &value) {
            if (!has_value_) {
                emplace(value);
            }
            return **this;
        }

        // Get or insert with function
        template <typename F> constexpr T &get_or_insert_with(F &&f) {
            if (!has_value_) {
                emplace(std::invoke(std::forward<F>(f)));
            }
            return **this;
        }

        // Conversion methods - forward declaration of Result needed
        template <typename E> constexpr auto ok_or(E &&err) const & -> Result<T, E>;
        template <typename E> constexpr auto ok_or(E &&err) && -> Result<T, E>;
        template <typename F>
        constexpr auto ok_or_else(F &&f) const & -> Result<T, decltype(std::invoke(std::forward<F>(f)))>;
        template <typename F>
        constexpr auto ok_or_else(F &&f) && -> Result<T, decltype(std::invoke(std::forward<F>(f)))>;

        // Serialization support
        auto members() noexcept { return std::tie(has_value_, storage_); }

      private:
        // Helper trait to detect Optional types
        template <typename> struct is_optional_impl : std::false_type {};
        template <typename U> struct is_optional_impl<Optional<U>> : std::true_type {};
        template <typename U> static constexpr bool is_optional = is_optional_impl<std::remove_cvref_t<U>>::value;

        bool has_value_;
        alignas(T) unsigned char storage_[sizeof(T)];
    };

    // Comparison operators
    template <typename T> constexpr bool operator==(Optional<T> const &lhs, Optional<T> const &rhs) {
        if (lhs.has_value() != rhs.has_value()) {
            return false;
        }
        if (!lhs.has_value()) {
            return true;
        }
        return *lhs == *rhs;
    }

    template <typename T> constexpr bool operator!=(Optional<T> const &lhs, Optional<T> const &rhs) {
        return !(lhs == rhs);
    }

    template <typename T> constexpr bool operator==(Optional<T> const &opt, nullopt_t) noexcept {
        return !opt.has_value();
    }

    template <typename T> constexpr bool operator==(nullopt_t, Optional<T> const &opt) noexcept {
        return !opt.has_value();
    }

    template <typename T> constexpr bool operator!=(Optional<T> const &opt, nullopt_t) noexcept {
        return opt.has_value();
    }

    template <typename T> constexpr bool operator!=(nullopt_t, Optional<T> const &opt) noexcept {
        return opt.has_value();
    }

    // make_optional helper
    template <typename T> constexpr Optional<T> make_optional(T &&value) { return Optional<T>(std::forward<T>(value)); }

    // Helper functions for Optional<T&> - copied() and cloned()
    // For Optional<T*> or Optional<T&>, convert to Optional<T> by copying
    template <typename T>
    constexpr auto copied(Optional<T *> const &opt) -> std::enable_if_t<std::is_copy_constructible_v<T>, Optional<T>> {
        if (opt.has_value()) {
            return Optional<T>{**opt};
        }
        return nullopt;
    }

    template <typename T>
    constexpr auto cloned(Optional<T *> const &opt) -> std::enable_if_t<std::is_copy_constructible_v<T>, Optional<T>> {
        if (opt.has_value()) {
            return Optional<T>{**opt};
        }
        return nullopt;
    }

} // namespace datapod
