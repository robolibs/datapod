#pragma once

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <new>
#include <utility>

namespace datapod {

    // Simple allocator template compatible with std::allocator interface
    template <typename T> struct Allocator {
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using const_pointer = const T *;
        using reference = T &;
        using const_reference = const T &;

        template <typename U> struct rebind {
            using other = Allocator<U>;
        };

        Allocator() noexcept = default;

        template <typename U> Allocator(Allocator<U> const &) noexcept {}

        T *allocate(std::size_t n) {
            if (n > max_size()) {
                throw std::bad_alloc();
            }
            void *ptr = std::malloc(n * sizeof(T));
            if (ptr == nullptr) {
                throw std::bad_alloc();
            }
            return static_cast<T *>(ptr);
        }

        void deallocate(T *ptr, std::size_t) noexcept { std::free(ptr); }

        std::size_t max_size() const noexcept { return std::numeric_limits<std::size_t>::max() / sizeof(T); }

        template <typename U, typename... Args> void construct(U *ptr, Args &&...args) {
            new (ptr) U(std::forward<Args>(args)...);
        }

        template <typename U> void destroy(U *ptr) { ptr->~U(); }
    };

    template <typename T, typename U> bool operator==(Allocator<T> const &, Allocator<U> const &) noexcept {
        return true;
    }

    template <typename T, typename U> bool operator!=(Allocator<T> const &, Allocator<U> const &) noexcept {
        return false;
    }

    namespace allocator {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace allocator

} // namespace datapod
