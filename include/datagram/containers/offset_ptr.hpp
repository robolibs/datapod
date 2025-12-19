#pragma once

#include <cassert>
#include <cstddef>
#include <type_traits>

#include "datagram/core/offset_t.hpp"

namespace datagram {

    // Self-relative pointer - stores offset from its own address
    // This allows the pointer to remain valid after memory relocation
    template <typename T> class OffsetPtr {
      public:
        using value_type = T;

        OffsetPtr() = default;

        OffsetPtr(std::nullptr_t) noexcept : offset_(NULLPTR_OFFSET) {}

        explicit OffsetPtr(T *ptr) noexcept { set(ptr); }

        template <typename U, typename = std::enable_if_t<std::is_convertible_v<U *, T *>>>
        OffsetPtr(OffsetPtr<U> const &other) noexcept {
            if (other.offset_ == NULLPTR_OFFSET) {
                offset_ = NULLPTR_OFFSET;
            } else {
                set(other.get());
            }
        }

        OffsetPtr(OffsetPtr const &o) noexcept {
            if (o.offset_ == NULLPTR_OFFSET) {
                offset_ = NULLPTR_OFFSET;
            } else {
                set(o.get());
            }
        }

        OffsetPtr &operator=(OffsetPtr const &o) noexcept {
            if (o.offset_ == NULLPTR_OFFSET) {
                offset_ = NULLPTR_OFFSET;
            } else {
                set(o.get());
            }
            return *this;
        }

        OffsetPtr(OffsetPtr &&) noexcept = default;
        OffsetPtr &operator=(OffsetPtr &&) noexcept = default;

        OffsetPtr &operator=(std::nullptr_t) noexcept {
            offset_ = NULLPTR_OFFSET;
            return *this;
        }

        OffsetPtr &operator=(T *ptr) noexcept {
            set(ptr);
            return *this;
        }

        T *get() const noexcept {
            if (offset_ == NULLPTR_OFFSET) {
                return nullptr;
            }
            return reinterpret_cast<T *>(reinterpret_cast<std::uintptr_t>(this) + static_cast<std::intptr_t>(offset_));
        }

        T *operator->() const noexcept {
            assert(offset_ != NULLPTR_OFFSET);
            return get();
        }

        T &operator*() const noexcept {
            assert(offset_ != NULLPTR_OFFSET);
            return *get();
        }

        explicit operator bool() const noexcept { return offset_ != NULLPTR_OFFSET; }

        bool operator==(std::nullptr_t) const noexcept { return offset_ == NULLPTR_OFFSET; }
        bool operator!=(std::nullptr_t) const noexcept { return offset_ != NULLPTR_OFFSET; }

        bool operator==(OffsetPtr const &other) const noexcept { return get() == other.get(); }
        bool operator!=(OffsetPtr const &other) const noexcept { return get() != other.get(); }

        bool operator==(T const *ptr) const noexcept { return get() == ptr; }
        bool operator!=(T const *ptr) const noexcept { return get() != ptr; }

        bool operator<(OffsetPtr const &other) const noexcept { return get() < other.get(); }
        bool operator<=(OffsetPtr const &other) const noexcept { return get() <= other.get(); }
        bool operator>(OffsetPtr const &other) const noexcept { return get() > other.get(); }
        bool operator>=(OffsetPtr const &other) const noexcept { return get() >= other.get(); }

        // Pointer arithmetic
        OffsetPtr &operator++() noexcept {
            set(get() + 1);
            return *this;
        }

        OffsetPtr operator++(int) noexcept {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        OffsetPtr &operator--() noexcept {
            set(get() - 1);
            return *this;
        }

        OffsetPtr operator--(int) noexcept {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        OffsetPtr operator+(std::ptrdiff_t n) const noexcept {
            OffsetPtr tmp;
            tmp.set(get() + n);
            return tmp;
        }

        OffsetPtr operator-(std::ptrdiff_t n) const noexcept {
            OffsetPtr tmp;
            tmp.set(get() - n);
            return tmp;
        }

        std::ptrdiff_t operator-(OffsetPtr const &other) const noexcept { return get() - other.get(); }

        OffsetPtr &operator+=(std::ptrdiff_t n) noexcept {
            set(get() + n);
            return *this;
        }

        OffsetPtr &operator-=(std::ptrdiff_t n) noexcept {
            set(get() - n);
            return *this;
        }

        T &operator[](std::ptrdiff_t n) const noexcept { return *(get() + n); }

        // Access to raw offset (for serialization)
        offset_t offset() const noexcept { return offset_; }

        void set_offset(offset_t offset) noexcept { offset_ = offset; }

      private:
        void set(T *ptr) noexcept {
            if (ptr == nullptr) {
                offset_ = NULLPTR_OFFSET;
            } else {
                offset_ = static_cast<offset_t>(reinterpret_cast<std::uintptr_t>(ptr) -
                                                reinterpret_cast<std::uintptr_t>(this));
            }
        }

        offset_t offset_{NULLPTR_OFFSET};
    };

    // Type trait to detect offset_ptr
    template <typename T> struct IsOffsetPtr : std::false_type {};

    template <typename T> struct IsOffsetPtr<OffsetPtr<T>> : std::true_type {};

    template <typename T> inline constexpr bool is_offset_ptr_v = IsOffsetPtr<T>::value;

} // namespace datagram
