#pragma once

#include <utility>

namespace bitcon {

    // Simple unique_ptr implementation
    template <typename T, typename Ptr = T *> class UniquePtr {
      public:
        using pointer = Ptr;
        using element_type = T;

        // Constructors
        constexpr UniquePtr() noexcept : ptr_(nullptr) {}

        constexpr UniquePtr(std::nullptr_t) noexcept : ptr_(nullptr) {}

        explicit UniquePtr(T *ptr) noexcept : ptr_(ptr) {}

        // Move constructor
        UniquePtr(UniquePtr &&other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }

        // Destructor
        ~UniquePtr() { reset(); }

        // Move assignment
        UniquePtr &operator=(UniquePtr &&other) noexcept {
            if (this != &other) {
                reset();
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
            }
            return *this;
        }

        UniquePtr &operator=(std::nullptr_t) noexcept {
            reset();
            return *this;
        }

        // Disable copy
        UniquePtr(UniquePtr const &) = delete;
        UniquePtr &operator=(UniquePtr const &) = delete;

        // Observers
        T *get() const noexcept { return ptr_; }

        T &operator*() const noexcept { return *ptr_; }

        T *operator->() const noexcept { return ptr_; }

        explicit operator bool() const noexcept { return ptr_ != nullptr; }

        // Modifiers
        T *release() noexcept {
            T *tmp = ptr_;
            ptr_ = nullptr;
            return tmp;
        }

        void reset(T *ptr = nullptr) noexcept {
            if (ptr_ != nullptr) {
                delete ptr_;
            }
            ptr_ = ptr;
        }

        void swap(UniquePtr &other) noexcept { std::swap(ptr_, other.ptr_); }

      private:
        T *ptr_;
    };

    // make_unique helper
    template <typename T, typename... Args> UniquePtr<T> make_unique(Args &&...args) {
        return UniquePtr<T>(new T(std::forward<Args>(args)...));
    }

    // Comparison operators
    template <typename T, typename Ptr> bool operator==(UniquePtr<T, Ptr> const &lhs, UniquePtr<T, Ptr> const &rhs) {
        return lhs.get() == rhs.get();
    }

    template <typename T, typename Ptr> bool operator!=(UniquePtr<T, Ptr> const &lhs, UniquePtr<T, Ptr> const &rhs) {
        return lhs.get() != rhs.get();
    }

    template <typename T, typename Ptr> bool operator==(UniquePtr<T, Ptr> const &ptr, std::nullptr_t) {
        return ptr.get() == nullptr;
    }

    template <typename T, typename Ptr> bool operator==(std::nullptr_t, UniquePtr<T, Ptr> const &ptr) {
        return ptr.get() == nullptr;
    }

    template <typename T, typename Ptr> bool operator!=(UniquePtr<T, Ptr> const &ptr, std::nullptr_t) {
        return ptr.get() != nullptr;
    }

    template <typename T, typename Ptr> bool operator!=(std::nullptr_t, UniquePtr<T, Ptr> const &ptr) {
        return ptr.get() != nullptr;
    }

} // namespace bitcon
