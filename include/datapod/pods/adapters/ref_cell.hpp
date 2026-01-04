/**
 * @file ref_cell.hpp
 * @brief Interior mutability with runtime borrow checking
 *
 * Provides RefCell<T>, Ref<T>, and RefMut<T> for interior mutability pattern.
 * Similar to Rust's RefCell, allows mutable borrows of immutable data with
 * runtime borrow checking.
 */

#pragma once
#include <datapod/types/types.hpp>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace datapod {

    /**
     * @brief Exception thrown when borrow rules are violated
     */
    class BorrowError : public std::runtime_error {
      public:
        explicit BorrowError(const char *msg) : std::runtime_error(msg) {}
    };

    // Forward declarations
    template <typename T> class RefCell;
    template <typename T> class RefMut;

    /**
     * @brief Immutable reference guard for RefCell
     *
     * RAII guard that represents an immutable borrow of a RefCell's value.
     * Multiple Ref instances can exist simultaneously.
     */
    template <typename T> class Ref {
      public:
        /**
         * @brief Access the borrowed value
         */
        const T &operator*() const { return *ptr_; }

        /**
         * @brief Access members of the borrowed value
         */
        const T *operator->() const { return ptr_; }

        /**
         * @brief Get raw pointer to the borrowed value
         */
        const T *get() const { return ptr_; }

        /**
         * @brief Destructor - releases the borrow
         */
        ~Ref() {
            if (borrow_count_) {
                --(*borrow_count_);
            }
        }

        /**
         * @brief Move constructor
         */
        Ref(Ref &&other) noexcept : ptr_(other.ptr_), borrow_count_(other.borrow_count_) {
            other.ptr_ = nullptr;
            other.borrow_count_ = nullptr;
        }

        /**
         * @brief Move assignment
         */
        Ref &operator=(Ref &&other) noexcept {
            if (this != &other) {
                if (borrow_count_) {
                    --(*borrow_count_);
                }
                ptr_ = other.ptr_;
                borrow_count_ = other.borrow_count_;
                other.ptr_ = nullptr;
                other.borrow_count_ = nullptr;
            }
            return *this;
        }

        // Delete copy operations
        Ref(const Ref &) = delete;
        Ref &operator=(const Ref &) = delete;

      private:
        friend class RefCell<T>;

        Ref(const T *ptr, datapod::usize *borrow_count) : ptr_(ptr), borrow_count_(borrow_count) { ++(*borrow_count_); }

        const T *ptr_;
        datapod::usize *borrow_count_;
    };

    /**
     * @brief Mutable reference guard for RefCell
     *
     * RAII guard that represents a mutable borrow of a RefCell's value.
     * Only one RefMut can exist at a time, and no Ref instances can coexist.
     */
    template <typename T> class RefMut {
      public:
        /**
         * @brief Access the borrowed value
         */
        T &operator*() { return *ptr_; }

        /**
         * @brief Access the borrowed value (const)
         */
        const T &operator*() const { return *ptr_; }

        /**
         * @brief Access members of the borrowed value
         */
        T *operator->() { return ptr_; }

        /**
         * @brief Access members of the borrowed value (const)
         */
        const T *operator->() const { return ptr_; }

        /**
         * @brief Get raw pointer to the borrowed value
         */
        T *get() { return ptr_; }

        /**
         * @brief Get raw pointer to the borrowed value (const)
         */
        const T *get() const { return ptr_; }

        /**
         * @brief Destructor - releases the borrow
         */
        ~RefMut() {
            if (is_borrowed_mut_) {
                *is_borrowed_mut_ = false;
            }
        }

        /**
         * @brief Move constructor
         */
        RefMut(RefMut &&other) noexcept : ptr_(other.ptr_), is_borrowed_mut_(other.is_borrowed_mut_) {
            other.ptr_ = nullptr;
            other.is_borrowed_mut_ = nullptr;
        }

        /**
         * @brief Move assignment
         */
        RefMut &operator=(RefMut &&other) noexcept {
            if (this != &other) {
                if (is_borrowed_mut_) {
                    *is_borrowed_mut_ = false;
                }
                ptr_ = other.ptr_;
                is_borrowed_mut_ = other.is_borrowed_mut_;
                other.ptr_ = nullptr;
                other.is_borrowed_mut_ = nullptr;
            }
            return *this;
        }

        // Delete copy operations
        RefMut(const RefMut &) = delete;
        RefMut &operator=(const RefMut &) = delete;

      private:
        friend class RefCell<T>;

        RefMut(T *ptr, bool *is_borrowed_mut) : ptr_(ptr), is_borrowed_mut_(is_borrowed_mut) {
            *is_borrowed_mut_ = true;
        }

        T *ptr_;
        bool *is_borrowed_mut_;
    };

    /**
     * @brief Cell with interior mutability and runtime borrow checking
     *
     * RefCell<T> provides interior mutability - the ability to mutate data
     * even when there are immutable references to it. Borrow rules are enforced
     * at runtime:
     * - Multiple immutable borrows (Ref) are allowed simultaneously
     * - Only one mutable borrow (RefMut) is allowed at a time
     * - Mutable and immutable borrows cannot coexist
     *
     * Violating these rules throws BorrowError.
     *
     * @tparam T The type of value stored in the cell
     */
    template <typename T> class RefCell {
      public:
        /**
         * @brief Construct with a value
         */
        explicit RefCell(T value) : value_(std::move(value)) {}

        /**
         * @brief Construct in-place
         */
        template <typename... Args> explicit RefCell(Args &&...args) : value_(std::forward<Args>(args)...) {}

        /**
         * @brief Borrow the value immutably
         *
         * @return Ref<T> guard for the borrowed value
         * @throws BorrowError if already mutably borrowed
         */
        Ref<T> borrow() const {
            if (is_borrowed_mut_) {
                throw BorrowError("Already mutably borrowed");
            }
            return Ref<T>(&value_, &borrow_count_);
        }

        /**
         * @brief Try to borrow the value immutably
         *
         * @return Ref<T> guard if successful, throws otherwise
         */
        Ref<T> try_borrow() const {
            if (is_borrowed_mut_) {
                throw BorrowError("Already mutably borrowed");
            }
            return Ref<T>(&value_, &borrow_count_);
        }

        /**
         * @brief Borrow the value mutably
         *
         * @return RefMut<T> guard for the borrowed value
         * @throws BorrowError if already borrowed (mutably or immutably)
         */
        RefMut<T> borrow_mut() {
            if (is_borrowed_mut_) {
                throw BorrowError("Already mutably borrowed");
            }
            if (borrow_count_ > 0) {
                throw BorrowError("Already immutably borrowed");
            }
            return RefMut<T>(&value_, &is_borrowed_mut_);
        }

        /**
         * @brief Try to borrow the value mutably
         *
         * @return RefMut<T> guard if successful, throws otherwise
         */
        RefMut<T> try_borrow_mut() {
            if (is_borrowed_mut_) {
                throw BorrowError("Already mutably borrowed");
            }
            if (borrow_count_ > 0) {
                throw BorrowError("Already immutably borrowed");
            }
            return RefMut<T>(&value_, &is_borrowed_mut_);
        }

        /**
         * @brief Replace the contained value and return the old value
         *
         * @param value New value
         * @return Old value
         * @throws BorrowError if currently borrowed
         */
        T replace(T value) {
            if (is_borrowed_mut_ || borrow_count_ > 0) {
                throw BorrowError("Cannot replace while borrowed");
            }
            T old = std::move(value_);
            value_ = std::move(value);
            return old;
        }

        /**
         * @brief Swap the contained value with another RefCell
         *
         * @param other Other RefCell to swap with
         * @throws BorrowError if either cell is currently borrowed
         */
        void swap(RefCell &other) {
            if (is_borrowed_mut_ || borrow_count_ > 0) {
                throw BorrowError("Cannot swap while borrowed");
            }
            if (other.is_borrowed_mut_ || other.borrow_count_ > 0) {
                throw BorrowError("Cannot swap while other is borrowed");
            }
            std::swap(value_, other.value_);
        }

        /**
         * @brief Get a copy of the contained value
         *
         * @return Copy of the value
         * @throws BorrowError if mutably borrowed
         */
        T get() const {
            if (is_borrowed_mut_) {
                throw BorrowError("Already mutably borrowed");
            }
            return value_;
        }

        /**
         * @brief Set the contained value
         *
         * @param value New value
         * @throws BorrowError if currently borrowed
         */
        void set(T value) {
            if (is_borrowed_mut_ || borrow_count_ > 0) {
                throw BorrowError("Cannot set while borrowed");
            }
            value_ = std::move(value);
        }

        /**
         * @brief Check if currently borrowed
         */
        bool is_borrowed() const { return is_borrowed_mut_ || borrow_count_ > 0; }

        /**
         * @brief Check if currently mutably borrowed
         */
        bool is_borrowed_mut() const { return is_borrowed_mut_; }

        /**
         * @brief Get the number of immutable borrows
         */
        datapod::usize borrow_count() const { return borrow_count_; }

        /**
         * @brief Take the value out of the RefCell, leaving it in a moved-from state
         *
         * @return The contained value
         * @throws BorrowError if currently borrowed
         */
        T take() {
            if (is_borrowed_mut_ || borrow_count_ > 0) {
                throw BorrowError("Cannot take while borrowed");
            }
            return std::move(value_);
        }

        // Delete copy operations (RefCell is not copyable)
        RefCell(const RefCell &) = delete;
        RefCell &operator=(const RefCell &) = delete;

        // Allow move operations
        RefCell(RefCell &&other) noexcept
            : value_(std::move(other.value_)), borrow_count_(other.borrow_count_),
              is_borrowed_mut_(other.is_borrowed_mut_) {
            other.borrow_count_ = 0;
            other.is_borrowed_mut_ = false;
        }

        RefCell &operator=(RefCell &&other) noexcept {
            if (this != &other) {
                value_ = std::move(other.value_);
                borrow_count_ = other.borrow_count_;
                is_borrowed_mut_ = other.is_borrowed_mut_;
                other.borrow_count_ = 0;
                other.is_borrowed_mut_ = false;
            }
            return *this;
        }

      private:
        mutable T value_;
        mutable datapod::usize borrow_count_ = 0;
        mutable bool is_borrowed_mut_ = false;
    };

    namespace ref_cell {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace ref_cell

} // namespace datapod
