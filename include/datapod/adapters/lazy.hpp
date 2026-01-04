#pragma once

#include "datapod/adapters/optional.hpp"
#include <atomic>
#include <functional>
#include <mutex>
#include <utility>

namespace datapod {

    /// Lazy<T> - Deferred computation with memoization
    /// Similar to Kotlin's lazy and Scala's lazy val
    /// Computes value on first access and caches the result
    /// Thread-safe initialization
    template <typename T> class Lazy {
      public:
        /// Construct from a function that produces T
        template <typename F> explicit Lazy(F &&f) : initializer_(std::forward<F>(f)), initialized_(false) {}

        /// No copy (contains function)
        Lazy(Lazy const &) = delete;
        Lazy &operator=(Lazy const &) = delete;

        /// Move constructor
        Lazy(Lazy &&other) noexcept
            : value_(std::move(other.value_)), initializer_(std::move(other.initializer_)),
              initialized_(other.initialized_.load()) {
            other.initialized_.store(false);
        }

        /// Move assignment
        Lazy &operator=(Lazy &&other) noexcept {
            if (this != &other) {
                value_ = std::move(other.value_);
                initializer_ = std::move(other.initializer_);
                initialized_.store(other.initialized_.load());
                other.initialized_.store(false);
            }
            return *this;
        }

        ~Lazy() = default;

        /// Get the value, computing it if necessary
        T const &operator*() const { return get(); }

        /// Get the value, computing it if necessary
        T const *operator->() const { return &get(); }

        /// Get the value, computing it if necessary
        T const &get() const {
            if (!initialized_.load(std::memory_order_acquire)) {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!initialized_.load(std::memory_order_relaxed)) {
                    value_ = Optional<T>(initializer_());
                    initialized_.store(true, std::memory_order_release);
                }
            }
            return value_.value();
        }

        /// Get mutable reference (computes if necessary)
        T &get_mut() {
            if (!initialized_.load(std::memory_order_acquire)) {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!initialized_.load(std::memory_order_relaxed)) {
                    value_ = Optional<T>(initializer_());
                    initialized_.store(true, std::memory_order_release);
                }
            }
            return value_.value();
        }

        /// Check if value has been computed
        bool is_initialized() const noexcept { return initialized_.load(std::memory_order_acquire); }

        /// Force computation without accessing the value
        void force() const { get(); }

        /// Take the value if initialized, leaving the Lazy in uninitialized state
        /// Returns nullopt if not yet initialized
        Optional<T> take() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (initialized_.load(std::memory_order_relaxed)) {
                initialized_.store(false, std::memory_order_release);
                return value_.take();
            }
            return nullopt;
        }

        /// Peek at the value without forcing computation
        /// Returns nullptr if not yet initialized
        T const *peek() const noexcept {
            if (initialized_.load(std::memory_order_acquire)) {
                return &value_.value();
            }
            return nullptr;
        }

        /// Reset to uninitialized state (will recompute on next access)
        void reset() {
            std::lock_guard<std::mutex> lock(mutex_);
            value_ = nullopt;
            initialized_.store(false, std::memory_order_release);
        }

      private:
        mutable Optional<T> value_;
        mutable std::function<T()> initializer_;
        mutable std::atomic<bool> initialized_;
        mutable std::mutex mutex_;
    };

    /// Helper function to create Lazy from a lambda
    template <typename F> auto make_lazy(F &&f) {
        using T = decltype(f());
        return Lazy<T>(std::forward<F>(f));
    }

} // namespace datapod
