#pragma once

#include <atomic>
#include <memory>
#include <utility>

namespace datapod {

    // Forward declarations
    template <typename T> class SharedPtr;
    template <typename T> class WeakPtr;

    namespace detail {
        /// Control block for reference counting
        template <typename T> struct ControlBlock {
            T *ptr;
            std::atomic<std::size_t> strong_count;
            std::atomic<std::size_t> weak_count;

            ControlBlock(T *p) : ptr(p), strong_count(1), weak_count(0) {}

            ~ControlBlock() {
                // ptr should already be deleted by the time we get here
            }

            void add_strong() noexcept { strong_count.fetch_add(1, std::memory_order_relaxed); }

            void add_weak() noexcept { weak_count.fetch_add(1, std::memory_order_relaxed); }

            bool release_strong() noexcept {
                if (strong_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    // Last strong reference - delete the object
                    delete ptr;
                    ptr = nullptr;
                    return true;
                }
                return false;
            }

            void release_weak() noexcept { weak_count.fetch_sub(1, std::memory_order_acq_rel); }

            std::size_t strong() const noexcept { return strong_count.load(std::memory_order_relaxed); }

            std::size_t weak() const noexcept { return weak_count.load(std::memory_order_relaxed); }
        };
    } // namespace detail

    /// SharedPtr<T> - Reference-counted smart pointer
    /// Similar to std::shared_ptr and Rust's Rc/Arc
    /// Provides shared ownership with automatic cleanup
    template <typename T> class SharedPtr {
        template <typename U> friend class SharedPtr;
        template <typename U> friend class WeakPtr;

      public:
        using element_type = T;

        /// Construct empty SharedPtr
        constexpr SharedPtr() noexcept : control_(nullptr) {}

        /// Construct from nullptr
        constexpr SharedPtr(std::nullptr_t) noexcept : control_(nullptr) {}

        /// Construct from raw pointer (takes ownership)
        explicit SharedPtr(T *ptr) : control_(ptr ? new detail::ControlBlock<T>(ptr) : nullptr) {}

        /// Copy constructor
        SharedPtr(SharedPtr const &other) noexcept : control_(other.control_) {
            if (control_) {
                control_->add_strong();
            }
        }

        /// Move constructor
        SharedPtr(SharedPtr &&other) noexcept : control_(other.control_) { other.control_ = nullptr; }

        /// Copy constructor from compatible type
        template <typename U> SharedPtr(SharedPtr<U> const &other) noexcept : control_(other.control_) {
            if (control_) {
                control_->add_strong();
            }
        }

        /// Copy assignment
        SharedPtr &operator=(SharedPtr const &other) noexcept {
            if (this != &other) {
                release();
                control_ = other.control_;
                if (control_) {
                    control_->add_strong();
                }
            }
            return *this;
        }

        /// Move assignment
        SharedPtr &operator=(SharedPtr &&other) noexcept {
            if (this != &other) {
                release();
                control_ = other.control_;
                other.control_ = nullptr;
            }
            return *this;
        }

        /// Destructor
        ~SharedPtr() { release(); }

        /// Create SharedPtr with in-place construction
        template <typename... Args> static SharedPtr make(Args &&...args) {
            return SharedPtr(new T(std::forward<Args>(args)...));
        }

        /// Dereference
        T &operator*() const noexcept { return *control_->ptr; }

        /// Arrow operator
        T *operator->() const noexcept { return control_->ptr; }

        /// Get raw pointer
        T *get() const noexcept { return control_ ? control_->ptr : nullptr; }

        /// Check if not null
        explicit operator bool() const noexcept { return control_ != nullptr && control_->ptr != nullptr; }

        /// Get strong reference count
        std::size_t use_count() const noexcept { return control_ ? control_->strong() : 0; }

        /// Get weak reference count
        std::size_t weak_count() const noexcept { return control_ ? control_->weak() : 0; }

        /// Check if this is the only owner
        bool unique() const noexcept { return use_count() == 1; }

        /// Reset to empty
        void reset() noexcept {
            release();
            control_ = nullptr;
        }

        /// Reset with new pointer
        void reset(T *ptr) {
            release();
            control_ = ptr ? new detail::ControlBlock<T>(ptr) : nullptr;
        }

        /// Swap with another SharedPtr
        void swap(SharedPtr &other) noexcept {
            auto tmp = control_;
            control_ = other.control_;
            other.control_ = tmp;
        }

        /// Comparison operators
        friend bool operator==(SharedPtr const &a, SharedPtr const &b) noexcept { return a.get() == b.get(); }

        friend bool operator!=(SharedPtr const &a, SharedPtr const &b) noexcept { return a.get() != b.get(); }

        friend bool operator==(SharedPtr const &a, std::nullptr_t) noexcept { return a.get() == nullptr; }

        friend bool operator==(std::nullptr_t, SharedPtr const &a) noexcept { return a.get() == nullptr; }

        friend bool operator!=(SharedPtr const &a, std::nullptr_t) noexcept { return a.get() != nullptr; }

        friend bool operator!=(std::nullptr_t, SharedPtr const &a) noexcept { return a.get() != nullptr; }

        friend bool operator<(SharedPtr const &a, SharedPtr const &b) noexcept { return a.get() < b.get(); }

        friend bool operator<=(SharedPtr const &a, SharedPtr const &b) noexcept { return a.get() <= b.get(); }

        friend bool operator>(SharedPtr const &a, SharedPtr const &b) noexcept { return a.get() > b.get(); }

        friend bool operator>=(SharedPtr const &a, SharedPtr const &b) noexcept { return a.get() >= b.get(); }

      private:
        void release() noexcept {
            if (control_) {
                bool last_strong = control_->release_strong();
                if (last_strong && control_->weak() == 0) {
                    // Last strong reference and no weak references
                    delete control_;
                }
            }
        }

        detail::ControlBlock<T> *control_;
    };

    /// WeakPtr<T> - Non-owning reference to SharedPtr
    /// Similar to std::weak_ptr and Rust's Weak
    /// Does not prevent object destruction
    /// Used to break reference cycles
    template <typename T> class WeakPtr {
        template <typename U> friend class WeakPtr;

      public:
        using element_type = T;

        /// Construct empty WeakPtr
        constexpr WeakPtr() noexcept : control_(nullptr) {}

        /// Construct from SharedPtr
        WeakPtr(SharedPtr<T> const &shared) noexcept : control_(shared.control_) {
            if (control_) {
                control_->add_weak();
            }
        }

        /// Copy constructor
        WeakPtr(WeakPtr const &other) noexcept : control_(other.control_) {
            if (control_) {
                control_->add_weak();
            }
        }

        /// Move constructor
        WeakPtr(WeakPtr &&other) noexcept : control_(other.control_) { other.control_ = nullptr; }

        /// Copy assignment
        WeakPtr &operator=(WeakPtr const &other) noexcept {
            if (this != &other) {
                release();
                control_ = other.control_;
                if (control_) {
                    control_->add_weak();
                }
            }
            return *this;
        }

        /// Move assignment
        WeakPtr &operator=(WeakPtr &&other) noexcept {
            if (this != &other) {
                release();
                control_ = other.control_;
                other.control_ = nullptr;
            }
            return *this;
        }

        /// Assign from SharedPtr
        WeakPtr &operator=(SharedPtr<T> const &shared) noexcept {
            release();
            control_ = shared.control_;
            if (control_) {
                control_->add_weak();
            }
            return *this;
        }

        /// Destructor
        ~WeakPtr() { release(); }

        /// Get strong reference count
        std::size_t use_count() const noexcept { return control_ ? control_->strong() : 0; }

        /// Check if the referenced object has been destroyed
        bool expired() const noexcept { return use_count() == 0; }

        /// Attempt to create a SharedPtr (returns empty if expired)
        SharedPtr<T> lock() const noexcept {
            if (control_ && control_->strong() > 0) {
                SharedPtr<T> result;
                result.control_ = control_;
                control_->add_strong();
                return result;
            }
            return SharedPtr<T>();
        }

        /// Reset to empty
        void reset() noexcept {
            release();
            control_ = nullptr;
        }

        /// Swap with another WeakPtr
        void swap(WeakPtr &other) noexcept {
            auto tmp = control_;
            control_ = other.control_;
            other.control_ = tmp;
        }

      private:
        void release() noexcept {
            if (control_) {
                control_->release_weak();
                // Delete control block if no more weak refs and no strong refs
                if (control_->weak() == 0 && control_->strong() == 0) {
                    delete control_;
                }
            }
        }

        detail::ControlBlock<T> *control_;
    };

    /// Non-member swap for SharedPtr
    template <typename T> void swap(SharedPtr<T> &a, SharedPtr<T> &b) noexcept { a.swap(b); }

    /// Non-member swap for WeakPtr
    template <typename T> void swap(WeakPtr<T> &a, WeakPtr<T> &b) noexcept { a.swap(b); }

    /// Helper function to create SharedPtr
    template <typename T, typename... Args> SharedPtr<T> make_shared(Args &&...args) {
        return SharedPtr<T>::make(std::forward<Args>(args)...);
    }

} // namespace datapod
