#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "datapod/core/aligned_alloc.hpp"

namespace datapod {
    namespace mat {

        // Forward declaration
        template <typename T> struct scalar;

        // =============================================================================
        // HEAP ALLOCATION THRESHOLD
        // =============================================================================
        // Vectors with more than this many elements will use heap allocation.
        // Default: 1024 elements (e.g., 4KB for float, 8KB for double)
        // Small vectors (N <= 1024) stay POD for zero-copy serialization.
        // Large vectors (N > 1024) use heap with SIMD alignment.
        inline constexpr size_t HEAP_THRESHOLD = 1024;

        /**
         * @brief Vector (rank-1, 1-D only) - fixed-size numeric vector
         *
         * Mathematical tensor of order 1 - represents a vector space element.
         * NOT a container - purely for numeric/mathematical operations.
         *
         * Examples:
         *   vector<double, 3> position{1.0, 2.0, 3.0};     // ℝ³ vector (stack)
         *   vector<float, 6> state;                         // 6-DOF state (stack)
         *   vector<float, 100000> embeddings;               // ML embeddings (heap)
         *   vector<scalar<double>, 10> features;            // Feature vector with scalars
         *
         * Design:
         * - Fixed size N (no resizing)
         * - Contiguous storage (cache-friendly)
         * - Aligned for SIMD (32-byte alignment)
         * - Small vectors (N <= HEAP_THRESHOLD): POD-compatible, stack-allocated
         * - Large vectors (N > HEAP_THRESHOLD): Heap-allocated, SIMD-aligned
         * - Serializable via members() or explicit serialize/deserialize
         * - NO math operations (data layer only)
         * - Bridge to Eigen via data() pointer
         * - Accepts both arithmetic types AND scalar<T>
         */

        // =============================================================================
        // PRIMARY TEMPLATE: Small vectors (stack-allocated, POD, zero-copy)
        // =============================================================================
        template <typename T, size_t N, bool UseHeap = (N > HEAP_THRESHOLD)> struct vector {
            // Accept arithmetic types (and scalar<T>, but we can't check that here due to forward declaration)
            // The scalar<T> case will work because it's a POD type
            static_assert(N > 0, "vector size must be > 0");

            using value_type = T;
            using size_type = size_t;
            using reference = T &;
            using const_reference = const T &;
            using pointer = T *;
            using const_pointer = const T *;
            using iterator = T *;
            using const_iterator = const T *;

            static constexpr size_t rank = 1;    // Rank-1 tensor
            static constexpr size_t size_ = N;   // Dimension
            static constexpr bool is_pod = true; // POD for zero-copy serialization
            static constexpr bool uses_heap = false;

            alignas(32) T data_[N]; // Aligned for SIMD (stack)

            // Serialization support
            auto members() noexcept { return std::tie(data_); }
            auto members() const noexcept { return std::tie(data_); }

            // Element access
            constexpr reference operator[](size_type i) noexcept { return data_[i]; }

            constexpr const_reference operator[](size_type i) const noexcept { return data_[i]; }

            constexpr reference at(size_type i) {
                if (i >= N) {
                    throw std::out_of_range("vector::at");
                }
                return data_[i];
            }

            constexpr const_reference at(size_type i) const {
                if (i >= N) {
                    throw std::out_of_range("vector::at");
                }
                return data_[i];
            }

            constexpr reference front() noexcept { return data_[0]; }
            constexpr const_reference front() const noexcept { return data_[0]; }

            constexpr reference back() noexcept { return data_[N - 1]; }
            constexpr const_reference back() const noexcept { return data_[N - 1]; }

            // Raw data access (for Eigen mapping)
            constexpr pointer data() noexcept { return data_; }
            constexpr const_pointer data() const noexcept { return data_; }

            // Capacity
            constexpr size_type size() const noexcept { return N; }
            constexpr size_type length() const noexcept { return N; } // Mathematical alias
            constexpr bool empty() const noexcept { return false; }   // Always false for N>0

            // Iterators
            constexpr iterator begin() noexcept { return data_; }
            constexpr const_iterator begin() const noexcept { return data_; }
            constexpr const_iterator cbegin() const noexcept { return data_; }

            constexpr iterator end() noexcept { return data_ + N; }
            constexpr const_iterator end() const noexcept { return data_ + N; }
            constexpr const_iterator cend() const noexcept { return data_ + N; }

            // Operations
            constexpr void fill(const T &value) noexcept {
                for (size_type i = 0; i < N; ++i) {
                    data_[i] = value;
                }
            }

            constexpr void swap(vector &other) noexcept {
                for (size_type i = 0; i < N; ++i) {
                    T tmp = data_[i];
                    data_[i] = other.data_[i];
                    other.data_[i] = tmp;
                }
            }

            // Comparison
            constexpr bool operator==(const vector &other) const noexcept {
                for (size_type i = 0; i < N; ++i) {
                    if (!(data_[i] == other.data_[i])) {
                        return false;
                    }
                }
                return true;
            }

            constexpr bool operator!=(const vector &other) const noexcept { return !(*this == other); }
        };

        // =============================================================================
        // SPECIALIZATION: Large vectors (heap-allocated, NOT POD, SIMD-aligned)
        // =============================================================================
        template <typename T, size_t N> struct vector<T, N, true> {
            static_assert(N > 0, "vector size must be > 0");

            using value_type = T;
            using size_type = size_t;
            using reference = T &;
            using const_reference = const T &;
            using pointer = T *;
            using const_pointer = const T *;
            using iterator = T *;
            using const_iterator = const T *;

            static constexpr size_t rank = 1;     // Rank-1 tensor
            static constexpr size_t size_ = N;    // Dimension
            static constexpr bool is_pod = false; // NOT POD (has destructor)
            static constexpr bool uses_heap = true;

            T *data_; // Heap-allocated, SIMD-aligned

            // Default constructor - allocate aligned heap memory
            vector() : data_(static_cast<T *>(aligned_alloc(32, sizeof(T) * N))) {
                for (size_t i = 0; i < N; ++i) {
                    new (&data_[i]) T{};
                }
            }

            // Destructor - free heap memory
            ~vector() {
                if (data_) {
                    for (size_t i = 0; i < N; ++i) {
                        data_[i].~T();
                    }
                    aligned_free(32, data_);
                }
            }

            // Copy constructor
            vector(const vector &other) : data_(static_cast<T *>(aligned_alloc(32, sizeof(T) * N))) {
                for (size_t i = 0; i < N; ++i) {
                    new (&data_[i]) T(other.data_[i]);
                }
            }

            // Copy assignment
            vector &operator=(const vector &other) {
                if (this != &other) {
                    for (size_t i = 0; i < N; ++i) {
                        data_[i] = other.data_[i];
                    }
                }
                return *this;
            }

            // Move constructor
            vector(vector &&other) noexcept : data_(other.data_) { other.data_ = nullptr; }

            // Move assignment
            vector &operator=(vector &&other) noexcept {
                if (this != &other) {
                    if (data_) {
                        for (size_t i = 0; i < N; ++i) {
                            data_[i].~T();
                        }
                        aligned_free(32, data_);
                    }
                    data_ = other.data_;
                    other.data_ = nullptr;
                }
                return *this;
            }

            // Serialization support - returns pointer and size for custom serialization
            // Note: Heap vectors need explicit serialize/deserialize overloads
            auto members() noexcept { return std::tie(data_); }
            auto members() const noexcept { return std::tie(data_); }

            // Element access - SAME API as stack version (transparent to users)
            reference operator[](size_type i) noexcept { return data_[i]; }
            const_reference operator[](size_type i) const noexcept { return data_[i]; }

            reference at(size_type i) {
                if (i >= N) {
                    throw std::out_of_range("vector::at");
                }
                return data_[i];
            }

            const_reference at(size_type i) const {
                if (i >= N) {
                    throw std::out_of_range("vector::at");
                }
                return data_[i];
            }

            reference front() noexcept { return data_[0]; }
            const_reference front() const noexcept { return data_[0]; }

            reference back() noexcept { return data_[N - 1]; }
            const_reference back() const noexcept { return data_[N - 1]; }

            // Raw data access (for Eigen mapping)
            pointer data() noexcept { return data_; }
            const_pointer data() const noexcept { return data_; }

            // Capacity
            constexpr size_type size() const noexcept { return N; }
            constexpr size_type length() const noexcept { return N; }
            constexpr bool empty() const noexcept { return false; }

            // Iterators
            iterator begin() noexcept { return data_; }
            const_iterator begin() const noexcept { return data_; }
            const_iterator cbegin() const noexcept { return data_; }

            iterator end() noexcept { return data_ + N; }
            const_iterator end() const noexcept { return data_ + N; }
            const_iterator cend() const noexcept { return data_ + N; }

            // Operations
            void fill(const T &value) noexcept {
                for (size_type i = 0; i < N; ++i) {
                    data_[i] = value;
                }
            }

            void swap(vector &other) noexcept { std::swap(data_, other.data_); }

            // Comparison
            bool operator==(const vector &other) const noexcept {
                for (size_type i = 0; i < N; ++i) {
                    if (!(data_[i] == other.data_[i])) {
                        return false;
                    }
                }
                return true;
            }

            bool operator!=(const vector &other) const noexcept { return !(*this == other); }
        };

        // Deduction guide for aggregate initialization
        template <typename T, typename... U> vector(T, U...) -> vector<T, 1 + sizeof...(U)>;

        // Type traits
        template <typename T> struct is_vector : std::false_type {};
        template <typename T, size_t N, bool UseHeap> struct is_vector<vector<T, N, UseHeap>> : std::true_type {};
        template <typename T> inline constexpr bool is_vector_v = is_vector<T>::value;

        // Type trait to check if vector uses heap
        template <typename T> struct is_heap_vector : std::false_type {};
        template <typename T, size_t N> struct is_heap_vector<vector<T, N, true>> : std::true_type {};
        template <typename T> inline constexpr bool is_heap_vector_v = is_heap_vector<T>::value;

        // Common vector type aliases
        template <typename T> using vector1 = vector<T, 1>;
        template <typename T> using vector2 = vector<T, 2>;
        template <typename T> using vector3 = vector<T, 3>;
        template <typename T> using vector4 = vector<T, 4>;
        template <typename T> using vector6 = vector<T, 6>; // 6-DOF state

        // Common numeric types
        using vector3f = vector<float, 3>;
        using vector3d = vector<double, 3>;
        using vector4f = vector<float, 4>;
        using vector4d = vector<double, 4>;
        using vector6f = vector<float, 6>;
        using vector6d = vector<double, 6>;

    } // namespace mat
} // namespace datapod
