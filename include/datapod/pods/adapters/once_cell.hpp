#pragma once

#include "datapod/pods/adapters/optional.hpp"
#include "datapod/pods/adapters/result.hpp"
#include <atomic>
#include <mutex>
#include <utility>

namespace datapod {

    /// OnceCell<T> - A cell that can be written to exactly once
    /// Similar to Rust's OnceCell/OnceLock
    /// Thread-safe initialization with lazy evaluation support
    template <typename T> class OnceCell {
      public:
        constexpr OnceCell() noexcept = default;

        OnceCell(OnceCell const &) = delete;
        OnceCell &operator=(OnceCell const &) = delete;

        OnceCell(OnceCell &&other) noexcept : value_(std::move(other.value_)), initialized_(other.initialized_.load()) {
            other.initialized_.store(false);
        }

        OnceCell &operator=(OnceCell &&other) noexcept {
            if (this != &other) {
                value_ = std::move(other.value_);
                initialized_.store(other.initialized_.load());
                other.initialized_.store(false);
            }
            return *this;
        }

        ~OnceCell() = default;

        /// Get pointer to the value if initialized, nullptr otherwise
        T const *get() const noexcept {
            if (initialized_.load(std::memory_order_acquire)) {
                return &value_.value();
            }
            return nullptr;
        }

        /// Get mutable pointer if initialized
        T *get_mut() noexcept {
            if (initialized_.load(std::memory_order_acquire)) {
                return &value_.value();
            }
            return nullptr;
        }

        /// Set the value if not already set
        /// Returns ok if successful, err(value) if already set
        Result<int, T> set(T value) noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            if (initialized_.load(std::memory_order_relaxed)) {
                return Result<int, T>::err(std::move(value));
            }
            value_ = Optional<T>(std::move(value));
            initialized_.store(true, std::memory_order_release);
            return Result<int, T>::ok(0);
        }

        /// Get or initialize with a function
        template <typename F> T &get_or_init(F &&f) {
            if (!initialized_.load(std::memory_order_acquire)) {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!initialized_.load(std::memory_order_relaxed)) {
                    value_ = Optional<T>(std::forward<F>(f)());
                    initialized_.store(true, std::memory_order_release);
                }
            }
            return value_.value();
        }

        /// Try to get or initialize, returns nullptr if function returns nullopt
        template <typename F> T *get_or_try_init(F &&f) {
            if (!initialized_.load(std::memory_order_acquire)) {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!initialized_.load(std::memory_order_relaxed)) {
                    auto result = std::forward<F>(f)();
                    if (!result.has_value()) {
                        return nullptr;
                    }
                    value_ = Optional<T>(std::move(result.value()));
                    initialized_.store(true, std::memory_order_release);
                }
            }
            return &value_.value();
        }

        /// Check if the cell has been initialized
        constexpr bool is_initialized() const noexcept { return initialized_.load(std::memory_order_acquire); }

        /// Take the value out of the cell, leaving it uninitialized
        Optional<T> take() noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            if (initialized_.load(std::memory_order_relaxed)) {
                initialized_.store(false, std::memory_order_release);
                return value_.take();
            }
            return nullopt;
        }

        /// Get the value, panicking if not initialized
        T const &unwrap() const {
            if (!initialized_.load(std::memory_order_acquire)) {
                throw std::runtime_error("OnceCell::unwrap() called on uninitialized cell");
            }
            return value_.value();
        }

        /// Get mutable reference, panicking if not initialized
        T &unwrap_mut() {
            if (!initialized_.load(std::memory_order_acquire)) {
                throw std::runtime_error("OnceCell::unwrap_mut() called on uninitialized cell");
            }
            return value_.value();
        }

        /// Get the value or a default
        T get_or_default() const
        requires std::is_default_constructible_v<T>
        {
            if (initialized_.load(std::memory_order_acquire)) {
                return value_.value();
            }
            return T{};
        }

        /// Into inner Optional
        constexpr Optional<T> into_inner() noexcept {
            initialized_.store(false, std::memory_order_release);
            return std::move(value_);
        }

      private:
        Optional<T> value_;
        std::atomic<bool> initialized_{false};
        mutable std::mutex mutex_;
    };

    namespace once_cell {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace once_cell

} // namespace datapod
