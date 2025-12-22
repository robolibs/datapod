#pragma once

#include <stdexcept>
#include <utility>

namespace datapod {

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

        // Serialization support
        auto members() noexcept { return std::tie(has_value_, storage_); }

      private:
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

} // namespace datapod
