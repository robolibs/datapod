#pragma once

#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

namespace datapod {

    /// MaybeUninit<T> - Wrapper for potentially uninitialized memory
    /// Similar to Rust's MaybeUninit<T>
    /// Allows manual initialization and deferred construction
    /// Useful for performance optimizations and low-level code
    template <typename T> class MaybeUninit {
      public:
        /// Construct uninitialized
        MaybeUninit() noexcept {}

        /// Construct with a value (initializes)
        explicit MaybeUninit(T const &value) { new (&storage_) T(value); }

        explicit MaybeUninit(T &&value) { new (&storage_) T(std::move(value)); }

        /// No copy/move by default (user must explicitly handle initialization state)
        MaybeUninit(MaybeUninit const &) = delete;
        MaybeUninit &operator=(MaybeUninit const &) = delete;
        MaybeUninit(MaybeUninit &&) = delete;
        MaybeUninit &operator=(MaybeUninit &&) = delete;

        /// Destructor does NOT destroy the value (user must call drop if initialized)
        ~MaybeUninit() noexcept {}

        /// Create an uninitialized instance
        static MaybeUninit uninit() noexcept { return MaybeUninit(); }

        /// Create an initialized instance
        static MaybeUninit init(T value) { return MaybeUninit(std::move(value)); }

        /// Create an array of uninitialized values
        template <std::size_t N> static void uninit_array(MaybeUninit<T> (&array)[N]) noexcept {
            // Array is already uninitialized by default
        }

        /// Write a value (assumes currently uninitialized)
        void write(T const &value) { new (&storage_) T(value); }

        void write(T &&value) { new (&storage_) T(std::move(value)); }

        /// Get a pointer to the value (may be uninitialized - unsafe!)
        T *as_ptr() noexcept { return reinterpret_cast<T *>(&storage_); }

        T const *as_ptr() const noexcept { return reinterpret_cast<T const *>(&storage_); }

        /// Get a mutable reference (assumes initialized - unsafe!)
        T &assume_init_mut() noexcept { return *as_ptr(); }

        /// Get a const reference (assumes initialized - unsafe!)
        T const &assume_init_ref() const noexcept { return *as_ptr(); }

        /// Take the value out (assumes initialized - unsafe!)
        T assume_init() noexcept {
            T value = std::move(*as_ptr());
            return value;
        }

        /// Drop the value if initialized (calls destructor)
        void drop() noexcept {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                as_ptr()->~T();
            }
        }

        /// Zero out the memory
        void zeroed() noexcept { std::memset(&storage_, 0, sizeof(T)); }

        /// Get raw storage pointer
        void *as_mut_ptr() noexcept { return &storage_; }

        void const *as_mut_ptr() const noexcept { return &storage_; }

      private:
        alignas(T) unsigned char storage_[sizeof(T)];
    };

    /// Helper to create uninitialized MaybeUninit
    template <typename T> MaybeUninit<T> uninit() noexcept { return MaybeUninit<T>::uninit(); }

    /// Helper to create initialized MaybeUninit
    template <typename T> MaybeUninit<T> init(T value) { return MaybeUninit<T>::init(std::move(value)); }

} // namespace datapod
