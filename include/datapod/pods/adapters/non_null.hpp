#pragma once

#include <cassert>
#include <stdexcept>
#include <utility>

namespace datapod {

    /// NonNull<T> - Guaranteed non-null pointer wrapper
    /// Similar to Rust's NonNull and C++ Core Guidelines gsl::not_null
    /// Provides compile-time and runtime null safety
    /// T must be a pointer type
    template <typename T> class NonNull {
        static_assert(std::is_pointer_v<T>, "NonNull<T> requires T to be a pointer type");

      public:
        using pointer = T;
        using element_type = std::remove_pointer_t<T>;

        /// Construct from non-null pointer (asserts if null)
        constexpr explicit NonNull(T ptr) : ptr_(ptr) {
            if (ptr_ == nullptr) {
                throw std::invalid_argument("NonNull constructed with nullptr");
            }
        }

        /// No default construction (would be null)
        NonNull() = delete;

        /// No construction from nullptr
        NonNull(std::nullptr_t) = delete;

        /// Copy constructor
        constexpr NonNull(NonNull const &other) noexcept = default;

        /// Move constructor
        constexpr NonNull(NonNull &&other) noexcept = default;

        /// Copy assignment
        constexpr NonNull &operator=(NonNull const &other) noexcept = default;

        /// Move assignment
        constexpr NonNull &operator=(NonNull &&other) noexcept = default;

        /// No assignment from nullptr
        NonNull &operator=(std::nullptr_t) = delete;

        /// Dereference
        constexpr element_type &operator*() const noexcept { return *ptr_; }

        /// Arrow operator
        constexpr T operator->() const noexcept { return ptr_; }

        /// Get the raw pointer
        constexpr T get() const noexcept { return ptr_; }

        /// Implicit conversion to raw pointer
        constexpr operator T() const noexcept { return ptr_; }

        /// Create from reference (always non-null)
        static constexpr NonNull from_ref(element_type &ref) noexcept { return NonNull(&ref); }

        /// Cast to a different pointer type
        template <typename U> constexpr NonNull<U> cast() const noexcept { return NonNull<U>(static_cast<U>(ptr_)); }

        /// Comparison operators
        friend constexpr bool operator==(NonNull const &a, NonNull const &b) noexcept { return a.ptr_ == b.ptr_; }

        friend constexpr bool operator!=(NonNull const &a, NonNull const &b) noexcept { return a.ptr_ != b.ptr_; }

        friend constexpr bool operator<(NonNull const &a, NonNull const &b) noexcept { return a.ptr_ < b.ptr_; }

        friend constexpr bool operator<=(NonNull const &a, NonNull const &b) noexcept { return a.ptr_ <= b.ptr_; }

        friend constexpr bool operator>(NonNull const &a, NonNull const &b) noexcept { return a.ptr_ > b.ptr_; }

        friend constexpr bool operator>=(NonNull const &a, NonNull const &b) noexcept { return a.ptr_ >= b.ptr_; }

        /// No comparison with nullptr (would always be false/true)
        friend bool operator==(NonNull const &, std::nullptr_t) = delete;
        friend bool operator==(std::nullptr_t, NonNull const &) = delete;
        friend bool operator!=(NonNull const &, std::nullptr_t) = delete;
        friend bool operator!=(std::nullptr_t, NonNull const &) = delete;

      private:
        T ptr_;
    };

    /// Deduction guide for references
    template <typename T> NonNull(T *) -> NonNull<T *>;

    /// Helper function to create NonNull from pointer (throws if null)
    template <typename T> constexpr NonNull<T *> make_non_null(T *ptr) { return NonNull<T *>(ptr); }

    namespace non_null {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace non_null

} // namespace datapod
