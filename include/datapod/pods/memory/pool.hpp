#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <tuple>
#include <utility>

namespace datapod {

    /// Pool allocator with segregated free list for fixed-size allocations.
    /// Provides O(1) allocation and deallocation by maintaining a linked list of free blocks.
    /// All allocations are the same size (sizeof(T)), making it extremely efficient
    /// for containers that allocate many objects of the same type.
    template <typename T> class Pool {
      public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using const_pointer = const T *;
        using reference = T &;
        using const_reference = const T &;

        template <typename U> struct rebind {
            using other = Pool<U>;
        };

      private:
        // Free list node - stored in unused blocks
        struct FreeNode {
            FreeNode *next;
        };

        // Memory chunk - large blocks that we subdivide
        struct Chunk {
            void *memory;
            Chunk *next;
            size_type capacity; // Number of blocks in this chunk
        };

        static constexpr size_type default_chunk_size = 64; // Blocks per chunk

      public:
        /// Default constructor - creates pool with default chunk size
        Pool() noexcept : free_list_(nullptr), chunks_(nullptr), chunk_size_(default_chunk_size), allocated_count_(0) {}

        /// Constructor with custom chunk size (number of blocks per chunk)
        explicit Pool(size_type chunk_size) noexcept
            : free_list_(nullptr), chunks_(nullptr), chunk_size_(chunk_size), allocated_count_(0) {}

        /// Copy constructor - creates independent pool (does NOT share memory)
        Pool(Pool const &other) noexcept
            : free_list_(nullptr), chunks_(nullptr), chunk_size_(other.chunk_size_), allocated_count_(0) {}

        /// Move constructor
        Pool(Pool &&other) noexcept
            : free_list_(other.free_list_), chunks_(other.chunks_), chunk_size_(other.chunk_size_),
              allocated_count_(other.allocated_count_) {
            other.free_list_ = nullptr;
            other.chunks_ = nullptr;
            other.allocated_count_ = 0;
        }

        /// Destructor - frees all allocated chunks
        ~Pool() { clear(); }

        /// Copy assignment - resets this pool (does NOT share memory)
        Pool &operator=(Pool const &other) {
            if (this != &other) {
                clear();
                chunk_size_ = other.chunk_size_;
            }
            return *this;
        }

        /// Move assignment
        Pool &operator=(Pool &&other) noexcept {
            if (this != &other) {
                clear();

                free_list_ = other.free_list_;
                chunks_ = other.chunks_;
                chunk_size_ = other.chunk_size_;
                allocated_count_ = other.allocated_count_;

                other.free_list_ = nullptr;
                other.chunks_ = nullptr;
                other.allocated_count_ = 0;
            }
            return *this;
        }

        /// Allocate memory for n objects of type T
        /// Note: For efficiency, pool allocators ignore n and always allocate one block.
        /// If you need n > 1, call allocate() n times.
        T *allocate(std::size_t n = 1) {
            if (n == 0) {
                return nullptr;
            }

            if (n > max_size()) {
                throw std::bad_alloc();
            }

            if (n > 1) {
                // Pool allocators work best with single-object allocations
                // For multiple objects, fall back to malloc
                void *ptr = std::malloc(n * sizeof(T));
                if (ptr == nullptr) {
                    throw std::bad_alloc();
                }
                return static_cast<T *>(ptr);
            }

            // If free list is empty, allocate a new chunk
            if (free_list_ == nullptr) {
                allocate_chunk();
            }

            // Pop from free list
            FreeNode *node = free_list_;
            free_list_ = node->next;
            ++allocated_count_;

            return reinterpret_cast<T *>(node);
        }

        /// Deallocate memory - returns block to free list
        void deallocate(T *ptr, std::size_t n = 1) noexcept {
            if (ptr == nullptr) {
                return;
            }

            if (n > 1) {
                // This was allocated with malloc (see allocate)
                std::free(ptr);
                return;
            }

            // Push to free list
            FreeNode *node = reinterpret_cast<FreeNode *>(ptr);
            node->next = free_list_;
            free_list_ = node;

            if (allocated_count_ > 0) {
                --allocated_count_;
            }
        }

        /// Maximum number of objects that can be allocated
        std::size_t max_size() const noexcept { return std::numeric_limits<std::size_t>::max() / sizeof(T); }

        /// Construct object at given location
        template <typename U, typename... Args> void construct(U *ptr, Args &&...args) {
            new (ptr) U(std::forward<Args>(args)...);
        }

        /// Destroy object at given location
        template <typename U> void destroy(U *ptr) { ptr->~U(); }

        /// Clear all memory and reset to initial state
        void clear() noexcept {
            // Free all chunks
            Chunk *chunk = chunks_;
            while (chunk != nullptr) {
                Chunk *next = chunk->next;
                std::free(chunk->memory);
                std::free(chunk);
                chunk = next;
            }

            chunks_ = nullptr;
            free_list_ = nullptr;
            allocated_count_ = 0;
        }

        /// Get number of currently allocated blocks
        size_type allocated_count() const noexcept { return allocated_count_; }

        /// Get chunk size (blocks per chunk)
        size_type chunk_size() const noexcept { return chunk_size_; }

        /// Count total number of chunks allocated
        size_type chunk_count() const noexcept {
            size_type count = 0;
            Chunk *chunk = chunks_;
            while (chunk != nullptr) {
                ++count;
                chunk = chunk->next;
            }
            return count;
        }

        /// Get total capacity (number of blocks across all chunks)
        size_type capacity() const noexcept {
            size_type total = 0;
            Chunk *chunk = chunks_;
            while (chunk != nullptr) {
                total += chunk->capacity;
                chunk = chunk->next;
            }
            return total;
        }

        /// Get number of free blocks available
        size_type free_count() const noexcept {
            size_type count = 0;
            FreeNode *node = free_list_;
            while (node != nullptr) {
                ++count;
                node = node->next;
            }
            return count;
        }

        /// Serialization support - returns internal state
        /// Note: Serializing the actual pointers is not portable; this is for introspection
        auto members() noexcept { return std::tie(chunk_size_, allocated_count_); }

        auto members() const noexcept { return std::tie(chunk_size_, allocated_count_); }

      private:
        /// Calculate block size with proper alignment
        static constexpr size_type block_size() noexcept {
            // Block must fit at least a FreeNode and have proper alignment
            size_type size = sizeof(T) > sizeof(FreeNode) ? sizeof(T) : sizeof(FreeNode);
            size_type align = alignof(T) > alignof(FreeNode) ? alignof(T) : alignof(FreeNode);

            // Align size up to alignment boundary
            return (size + align - 1) & ~(align - 1);
        }

        /// Allocate a new chunk and add its blocks to the free list
        void allocate_chunk() {
            size_type const block_sz = block_size();
            size_type const chunk_bytes = chunk_size_ * block_sz;

            // Allocate memory for the chunk
            void *memory = std::malloc(chunk_bytes);
            if (memory == nullptr) {
                throw std::bad_alloc();
            }

            // Create chunk descriptor
            Chunk *chunk = static_cast<Chunk *>(std::malloc(sizeof(Chunk)));
            if (chunk == nullptr) {
                std::free(memory);
                throw std::bad_alloc();
            }

            chunk->memory = memory;
            chunk->capacity = chunk_size_;
            chunk->next = chunks_;
            chunks_ = chunk;

            // Link all blocks in this chunk into the free list
            char *block = static_cast<char *>(memory);
            for (size_type i = 0; i < chunk_size_; ++i) {
                FreeNode *node = reinterpret_cast<FreeNode *>(block);
                node->next = free_list_;
                free_list_ = node;
                block += block_sz;
            }
        }

        FreeNode *free_list_;       // Head of free list
        Chunk *chunks_;             // Linked list of memory chunks
        size_type chunk_size_;      // Number of blocks per chunk
        size_type allocated_count_; // Number of blocks currently allocated
    };

    /// Comparison operators - pools are never equal (they manage independent memory)
    template <typename T, typename U> bool operator==(Pool<T> const &, Pool<U> const &) noexcept { return false; }

    template <typename T, typename U> bool operator!=(Pool<T> const &, Pool<U> const &) noexcept { return true; }

    namespace pool {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace pool

} // namespace datapod
