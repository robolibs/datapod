#pragma once
#include <datapod/types/types.hpp>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <tuple>
#include <utility>

namespace datapod {

    /// Arena allocator with bump-pointer allocation strategy.
    /// Extremely fast allocation (just pointer increment), but no individual deallocation.
    /// All memory is freed when the arena is destroyed or reset.
    template <typename T> class Arena {
      public:
        using value_type = T;
        using size_type = datapod::usize;
        using difference_type = datapod::isize;
        using pointer = T *;
        using const_pointer = const T *;
        using reference = T &;
        using const_reference = const T &;

        template <typename U> struct rebind {
            using other = Arena<U>;
        };

        /// Default constructor - creates arena with default block size (64KB)
        Arena() noexcept : buffer_(nullptr), offset_(0), capacity_(0), block_size_(default_block_size) {}

        /// Constructor with custom block size
        explicit Arena(size_type block_size) noexcept
            : buffer_(nullptr), offset_(0), capacity_(0), block_size_(block_size) {}

        /// Copy constructor - creates independent arena (does NOT share memory)
        Arena(Arena const &other) noexcept
            : buffer_(nullptr), offset_(0), capacity_(0), block_size_(other.block_size_) {}

        /// Move constructor
        Arena(Arena &&other) noexcept
            : buffer_(other.buffer_), offset_(other.offset_), capacity_(other.capacity_),
              block_size_(other.block_size_) {
            other.buffer_ = nullptr;
            other.offset_ = 0;
            other.capacity_ = 0;
        }

        /// Destructor - frees all allocated memory
        ~Arena() {
            if (buffer_ != nullptr) {
                std::free(buffer_);
            }
        }

        /// Copy assignment - resets this arena (does NOT share memory)
        Arena &operator=(Arena const &other) {
            if (this != &other) {
                reset();
                block_size_ = other.block_size_;
            }
            return *this;
        }

        /// Move assignment
        Arena &operator=(Arena &&other) noexcept {
            if (this != &other) {
                if (buffer_ != nullptr) {
                    std::free(buffer_);
                }

                buffer_ = other.buffer_;
                offset_ = other.offset_;
                capacity_ = other.capacity_;
                block_size_ = other.block_size_;

                other.buffer_ = nullptr;
                other.offset_ = 0;
                other.capacity_ = 0;
            }
            return *this;
        }

        /// Allocate memory for n objects of type T
        /// Allocations are aligned to alignof(T)
        T *allocate(datapod::usize n) {
            if (n > max_size()) {
                throw std::bad_alloc();
            }

            size_type const bytes_needed = n * sizeof(T);
            size_type const alignment = alignof(T);

            // Align the current offset
            size_type const aligned_offset = align_up(offset_, alignment);
            size_type const new_offset = aligned_offset + bytes_needed;

            // Check if we need to grow the buffer
            if (new_offset > capacity_) {
                grow(new_offset);
            }

            // Bump the pointer
            void *ptr = static_cast<char *>(buffer_) + aligned_offset;
            offset_ = new_offset;

            return static_cast<T *>(ptr);
        }

        /// Deallocate - NO-OP for arena allocators
        /// Individual deallocations are not supported; use reset() to free all memory
        void deallocate(T *, datapod::usize) noexcept {
            // Arena allocators don't support individual deallocation
        }

        /// Maximum number of objects that can be allocated
        datapod::usize max_size() const noexcept { return std::numeric_limits<datapod::usize>::max() / sizeof(T); }

        /// Construct object at given location
        template <typename U, typename... Args> void construct(U *ptr, Args &&...args) {
            new (ptr) U(std::forward<Args>(args)...);
        }

        /// Destroy object at given location
        template <typename U> void destroy(U *ptr) { ptr->~U(); }

        /// Reset the arena - deallocates all memory and starts fresh
        void reset() noexcept {
            offset_ = 0;
            // Keep the buffer allocated for reuse
        }

        /// Free all memory and reset to initial state
        void clear() noexcept {
            if (buffer_ != nullptr) {
                std::free(buffer_);
                buffer_ = nullptr;
            }
            offset_ = 0;
            capacity_ = 0;
        }

        /// Get current memory usage in bytes
        size_type bytes_used() const noexcept { return offset_; }

        /// Get total capacity in bytes
        size_type bytes_capacity() const noexcept { return capacity_; }

        /// Get block size used for growth
        size_type block_size() const noexcept { return block_size_; }

        /// Serialization support - returns internal state
        auto members() noexcept { return std::tie(buffer_, offset_, capacity_, block_size_); }

        auto members() const noexcept { return std::tie(buffer_, offset_, capacity_, block_size_); }

      private:
        static constexpr size_type default_block_size = 65536; // 64KB

        /// Align value up to the given alignment
        static size_type align_up(size_type value, size_type alignment) noexcept {
            return (value + alignment - 1) & ~(alignment - 1);
        }

        /// Grow the buffer to accommodate at least min_capacity bytes
        void grow(size_type min_capacity) {
            // Calculate new capacity: max of (min_capacity, current_capacity + block_size)
            size_type new_capacity = capacity_ + block_size_;
            if (new_capacity < min_capacity) {
                new_capacity = min_capacity;
            }

            // Allocate new buffer
            void *new_buffer = std::malloc(new_capacity);
            if (new_buffer == nullptr) {
                throw std::bad_alloc();
            }

            // Copy existing data if any
            if (buffer_ != nullptr && offset_ > 0) {
                std::memcpy(new_buffer, buffer_, offset_);
                std::free(buffer_);
            } else if (buffer_ != nullptr) {
                std::free(buffer_);
            }

            buffer_ = new_buffer;
            capacity_ = new_capacity;
        }

        void *buffer_;         // Pointer to allocated memory block
        size_type offset_;     // Current offset into buffer (bump pointer)
        size_type capacity_;   // Total capacity of buffer in bytes
        size_type block_size_; // Growth increment size
    };

    /// Comparison operators - arenas are never equal (they manage independent memory)
    template <typename T, typename U> bool operator==(Arena<T> const &, Arena<U> const &) noexcept { return false; }

    template <typename T, typename U> bool operator!=(Arena<T> const &, Arena<U> const &) noexcept { return true; }

    namespace arena {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace arena

} // namespace datapod
