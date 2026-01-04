/**
 * @file pin.hpp
 * @brief Pin adapter to prevent moving of values
 *
 * Provides Pin<T> which prevents moving of the contained value.
 * Useful for self-referential structs and async/await patterns.
 * Similar to Rust's Pin<P>.
 */

#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace datapod {

    /**
     * @brief Marker trait for types that can be unpinned
     *
     * Types that are Unpin can be safely moved even when pinned.
     * Most types are Unpin by default. Types that contain self-references
     * should NOT be Unpin.
     */
    template <typename T> struct Unpin : std::true_type {};

    /**
     * @brief Pin wrapper that prevents moving of the contained value
     *
     * Pin<T*> wraps a pointer and prevents the pointed-to value from being moved.
     * This is useful for:
     * - Self-referential structs (structs that contain pointers to themselves)
     * - Async/await patterns where futures may contain self-references
     * - Intrusive data structures
     *
     * Pin guarantees:
     * - The pointed-to value will not be moved (unless it's Unpin)
     * - The memory location remains stable
     *
     * @tparam T Pointer type (T*, const T*, etc.)
     */
    template <typename T> class Pin {
      public:
        using element_type = std::remove_pointer_t<T>;
        using pointer = T;

        /**
         * @brief Construct a Pin from a pointer (unsafe - caller must ensure validity)
         *
         * SAFETY: The caller must ensure that:
         * - The pointer is valid and will remain valid
         * - The pointed-to value will not be moved
         * - The pointed-to value will not be deallocated while pinned
         */
        static constexpr Pin new_unchecked(T ptr) noexcept { return Pin(ptr); }

        /**
         * @brief Construct a Pin from a pointer to an Unpin type (safe)
         *
         * This is safe because Unpin types can be moved even when pinned.
         */
        template <typename U = element_type, typename = std::enable_if_t<Unpin<U>::value>>
        static constexpr Pin new_pin(T ptr) noexcept {
            return Pin(ptr);
        }

        /**
         * @brief Get the pinned pointer (const access)
         *
         * Returns a const pointer to prevent moving the value.
         */
        constexpr const element_type *get() const noexcept { return static_cast<const element_type *>(ptr_); }

        /**
         * @brief Get the pinned pointer (mutable access for Unpin types)
         *
         * Only available for Unpin types, which can be safely moved.
         */
        template <typename U = element_type, typename = std::enable_if_t<Unpin<U>::value>>
        constexpr element_type *get_mut() noexcept {
            return const_cast<element_type *>(ptr_);
        }

        /**
         * @brief Get the raw pointer (unsafe)
         *
         * SAFETY: The caller must not move the pointed-to value.
         */
        constexpr T get_unchecked_mut() noexcept { return ptr_; }

        /**
         * @brief Dereference the pinned pointer (const)
         */
        constexpr const element_type &operator*() const noexcept { return *ptr_; }

        /**
         * @brief Dereference the pinned pointer (mutable for Unpin types)
         */
        template <typename U = element_type, typename = std::enable_if_t<Unpin<U>::value>>
        constexpr element_type &operator*() noexcept {
            return *ptr_;
        }

        /**
         * @brief Access members of the pinned value (const)
         */
        constexpr const element_type *operator->() const noexcept { return ptr_; }

        /**
         * @brief Access members of the pinned value (mutable for Unpin types)
         */
        template <typename U = element_type, typename = std::enable_if_t<Unpin<U>::value>>
        constexpr element_type *operator->() noexcept {
            return ptr_;
        }

        /**
         * @brief Convert to bool (check if pointer is non-null)
         */
        constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }

        /**
         * @brief Equality comparison
         */
        constexpr bool operator==(const Pin &other) const noexcept { return ptr_ == other.ptr_; }

        /**
         * @brief Inequality comparison
         */
        constexpr bool operator!=(const Pin &other) const noexcept { return ptr_ != other.ptr_; }

        /**
         * @brief Less-than comparison
         */
        constexpr bool operator<(const Pin &other) const noexcept { return ptr_ < other.ptr_; }

        /**
         * @brief Get the underlying pointer (for Unpin types)
         */
        template <typename U = element_type, typename = std::enable_if_t<Unpin<U>::value>>
        constexpr T into_inner() noexcept {
            return ptr_;
        }

        // Pin is not copyable or movable (to maintain the pinning guarantee)
        Pin(const Pin &) = delete;
        Pin &operator=(const Pin &) = delete;
        Pin(Pin &&) = delete;
        Pin &operator=(Pin &&) = delete;

      private:
        explicit constexpr Pin(T ptr) noexcept : ptr_(ptr) {}

        T ptr_;
    };

    /**
     * @brief Helper function to create a Pin from a pointer (unsafe)
     */
    template <typename T> constexpr Pin<T *> pin_unchecked(T *ptr) noexcept { return Pin<T *>::new_unchecked(ptr); }

    /**
     * @brief Helper function to create a Pin from a pointer to an Unpin type
     */
    template <typename T, typename = std::enable_if_t<Unpin<T>::value>> constexpr Pin<T *> pin(T *ptr) noexcept {
        return Pin<T *>::new_pin(ptr);
    }

    /**
     * @brief Pin specialization for references
     *
     * Pin<T&> is similar to Pin<T*> but works with references.
     */
    template <typename T> class Pin<T &> {
      public:
        using element_type = T;

        /**
         * @brief Construct a Pin from a reference (unsafe)
         */
        static constexpr Pin new_unchecked(T &ref) noexcept { return Pin(ref); }

        /**
         * @brief Construct a Pin from a reference to an Unpin type
         */
        template <typename U = T, typename = std::enable_if_t<Unpin<U>::value>>
        static constexpr Pin new_pin(T &ref) noexcept {
            return Pin(ref);
        }

        /**
         * @brief Get the pinned reference (const)
         */
        constexpr const T &get() const noexcept { return ref_; }

        /**
         * @brief Get the pinned reference (mutable for Unpin types)
         */
        template <typename U = T, typename = std::enable_if_t<Unpin<U>::value>> constexpr T &get_mut() noexcept {
            return ref_;
        }

        /**
         * @brief Get the raw reference (unsafe)
         */
        constexpr T &get_unchecked_mut() noexcept { return ref_; }

        /**
         * @brief Dereference the pinned reference (const)
         */
        constexpr const T &operator*() const noexcept { return ref_; }

        /**
         * @brief Dereference the pinned reference (mutable for Unpin types)
         */
        template <typename U = T, typename = std::enable_if_t<Unpin<U>::value>> constexpr T &operator*() noexcept {
            return ref_;
        }

        /**
         * @brief Access members of the pinned value (const)
         */
        constexpr const T *operator->() const noexcept { return &ref_; }

        /**
         * @brief Access members of the pinned value (mutable for Unpin types)
         */
        template <typename U = T, typename = std::enable_if_t<Unpin<U>::value>> constexpr T *operator->() noexcept {
            return &ref_;
        }

        /**
         * @brief Get the underlying reference (for Unpin types)
         */
        template <typename U = T, typename = std::enable_if_t<Unpin<U>::value>> constexpr T &into_inner() noexcept {
            return ref_;
        }

        // Pin is not copyable or movable
        Pin(const Pin &) = delete;
        Pin &operator=(const Pin &) = delete;
        Pin(Pin &&) = delete;
        Pin &operator=(Pin &&) = delete;

      private:
        explicit constexpr Pin(T &ref) noexcept : ref_(ref) {}

        T &ref_;
    };

/**
 * @brief Helper to mark a type as !Unpin (not safe to move when pinned)
 *
 * Use this for types that contain self-references or should not be moved.
 *
 * Example:
 * struct SelfReferential {
 *     int* ptr;
 *     int value;
 *     SelfReferential() : value(42) { ptr = &value; }
 * };
 * template<> struct datapod::Unpin<SelfReferential> : std::false_type {};
 */
#define DATAPOD_NOT_UNPIN(Type)                                                                                        \
    namespace datapod {                                                                                                \
        template <> struct Unpin<Type> : std::false_type {};                                                           \
    }

    namespace pin_ns {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace pin_ns

} // namespace datapod
