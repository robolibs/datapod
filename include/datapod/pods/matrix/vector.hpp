#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "datapod/core/aligned_alloc.hpp"

namespace datapod {
    namespace mat {

        // Forward declaration
        template <typename T> struct scalar;

        // =============================================================================
        // DYNAMIC SIZE SENTINEL (Eigen-style)
        // =============================================================================
        // Use this as a template parameter to indicate runtime-sized dimensions.
        // Example: Vector<double, Dynamic> is a runtime-sized Vector
        inline constexpr size_t Dynamic = static_cast<size_t>(-1);

        // =============================================================================
        // HEAP ALLOCATION THRESHOLD
        // =============================================================================
        // Vectors with more than this many elements will use heap allocation.
        // Default: 1024 elements (e.g., 4KB for float, 8KB for double)
        // Small vectors (N <= 1024) stay POD for zero-copy serialization.
        // Large vectors (N > 1024) use heap with SIMD alignment.
        inline constexpr size_t HEAP_THRESHOLD = 1024;

        /**
         * @brief Vector (rank-1, 1-D only) - fixed-size numeric Vector
         *
         * Mathematical tensor of order 1 - represents a Vector space element.
         * NOT a container - purely for numeric/mathematical operations.
         *
         * Examples:
         *   Vector<double, 3> position{1.0, 2.0, 3.0};     // ℝ³ Vector (stack)
         *   Vector<float, 6> state;                         // 6-DOF state (stack)
         *   Vector<float, 100000> embeddings;               // ML embeddings (heap)
         *   Vector<scalar<double>, 10> features;            // Feature Vector with scalars
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
        // Note: When N == Dynamic, we want to use the dynamic specialization (false flag)
        // otherwise use heap allocation for large vectors (N > HEAP_THRESHOLD)
        template <typename T, size_t N, bool UseHeap = (N != Dynamic && N > HEAP_THRESHOLD)> struct Vector {
            // Accept arithmetic types (and scalar<T>, but we can't check that here due to forward declaration)
            // The scalar<T> case will work because it's a POD type
            static_assert(N > 0, "Vector size must be > 0");

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
                    throw std::out_of_range("Vector::at");
                }
                return data_[i];
            }

            constexpr const_reference at(size_type i) const {
                if (i >= N) {
                    throw std::out_of_range("Vector::at");
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

            constexpr void swap(Vector &other) noexcept {
                for (size_type i = 0; i < N; ++i) {
                    T tmp = data_[i];
                    data_[i] = other.data_[i];
                    other.data_[i] = tmp;
                }
            }

            // Comparison
            constexpr bool operator==(const Vector &other) const noexcept {
                for (size_type i = 0; i < N; ++i) {
                    if (!(data_[i] == other.data_[i])) {
                        return false;
                    }
                }
                return true;
            }

            constexpr bool operator!=(const Vector &other) const noexcept { return !(*this == other); }
        };

        // =============================================================================
        // SPECIALIZATION: Large vectors (heap-allocated, NOT POD, SIMD-aligned)
        // =============================================================================
        template <typename T, size_t N> struct Vector<T, N, true> {
            static_assert(N > 0, "Vector size must be > 0");

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
            Vector() : data_(static_cast<T *>(aligned_alloc(32, sizeof(T) * N))) {
                for (size_t i = 0; i < N; ++i) {
                    new (&data_[i]) T{};
                }
            }

            // Destructor - free heap memory
            ~Vector() {
                if (data_) {
                    for (size_t i = 0; i < N; ++i) {
                        data_[i].~T();
                    }
                    aligned_free(32, data_);
                }
            }

            // Copy constructor
            Vector(const Vector &other) : data_(static_cast<T *>(aligned_alloc(32, sizeof(T) * N))) {
                for (size_t i = 0; i < N; ++i) {
                    new (&data_[i]) T(other.data_[i]);
                }
            }

            // Copy assignment
            Vector &operator=(const Vector &other) {
                if (this != &other) {
                    for (size_t i = 0; i < N; ++i) {
                        data_[i] = other.data_[i];
                    }
                }
                return *this;
            }

            // Move constructor
            Vector(Vector &&other) noexcept : data_(other.data_) { other.data_ = nullptr; }

            // Move assignment
            Vector &operator=(Vector &&other) noexcept {
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
                    throw std::out_of_range("Vector::at");
                }
                return data_[i];
            }

            const_reference at(size_type i) const {
                if (i >= N) {
                    throw std::out_of_range("Vector::at");
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

            void swap(Vector &other) noexcept { std::swap(data_, other.data_); }

            // Comparison
            bool operator==(const Vector &other) const noexcept {
                for (size_type i = 0; i < N; ++i) {
                    if (!(data_[i] == other.data_[i])) {
                        return false;
                    }
                }
                return true;
            }

            bool operator!=(const Vector &other) const noexcept { return !(*this == other); }
        };

        // Deduction guide for aggregate initialization
        template <typename T, typename... U> Vector(T, U...) -> Vector<T, 1 + sizeof...(U)>;

        // Type traits
        template <typename T> struct is_vector : std::false_type {};
        template <typename T, size_t N, bool UseHeap> struct is_vector<Vector<T, N, UseHeap>> : std::true_type {};
        template <typename T> inline constexpr bool is_vector_v = is_vector<T>::value;

        // Type trait to check if Vector uses heap
        template <typename T> struct is_heap_vector : std::false_type {};
        template <typename T, size_t N> struct is_heap_vector<Vector<T, N, true>> : std::true_type {};
        template <typename T> inline constexpr bool is_heap_vector_v = is_heap_vector<T>::value;

        // Common Vector type aliases
        template <typename T> using Vector1 = Vector<T, 1>;
        template <typename T> using Vector2 = Vector<T, 2>;
        template <typename T> using Vector3 = Vector<T, 3>;
        template <typename T> using Vector4 = Vector<T, 4>;
        template <typename T> using Vector6 = Vector<T, 6>; // 6-DOF state

        // Common numeric types
        using Vector3f = Vector<float, 3>;
        using Vector3d = Vector<double, 3>;
        using Vector4f = Vector<float, 4>;
        using Vector4d = Vector<double, 4>;
        using Vector6f = Vector<float, 6>;
        using Vector6d = Vector<double, 6>;

        // =============================================================================
        // SPECIALIZATION: Dynamic vectors (runtime-sized, heap-allocated)
        // =============================================================================
        /**
         * @brief Runtime-sized numeric Vector
         *
         * Specialization for Vector<T, Dynamic> - size determined at runtime.
         * Always heap-allocated with SIMD alignment.
         *
         * Examples:
         *   Vector<double, Dynamic> v(100);           // 100-element Vector
         *   Vector<float, Dynamic> w = {1, 2, 3, 4};  // From initializer list
         *   v.resize(200);                            // Resize to 200 elements
         */
        template <typename T> struct Vector<T, Dynamic, false> {
            using value_type = T;
            using size_type = size_t;
            using reference = T &;
            using const_reference = const T &;
            using pointer = T *;
            using const_pointer = const T *;
            using iterator = T *;
            using const_iterator = const T *;

            static constexpr size_t rank = 1;
            static constexpr bool is_pod = false;
            static constexpr bool uses_heap = true;
            static constexpr bool is_dynamic = true;

          private:
            size_t size_;
            size_t capacity_;
            T *data_;

            void allocate(size_t cap) {
                if (cap > 0) {
                    data_ = static_cast<T *>(aligned_alloc(32, sizeof(T) * cap));
                    capacity_ = cap;
                } else {
                    data_ = nullptr;
                    capacity_ = 0;
                }
            }

            void deallocate() {
                if (data_) {
                    for (size_t i = 0; i < size_; ++i) {
                        data_[i].~T();
                    }
                    aligned_free(32, data_);
                    data_ = nullptr;
                }
            }

          public:
            // Default constructor - empty Vector
            Vector() noexcept : size_(0), capacity_(0), data_(nullptr) {}

            // Size constructor - allocate with given size, zero-initialized
            explicit Vector(size_t size) : size_(size), capacity_(0), data_(nullptr) {
                allocate(size);
                for (size_t i = 0; i < size_; ++i) {
                    new (&data_[i]) T{};
                }
            }

            // Size + value constructor
            Vector(size_t size, const T &value) : size_(size), capacity_(0), data_(nullptr) {
                allocate(size);
                for (size_t i = 0; i < size_; ++i) {
                    new (&data_[i]) T(value);
                }
            }

            // Initializer list constructor
            Vector(std::initializer_list<T> init) : size_(init.size()), capacity_(0), data_(nullptr) {
                allocate(size_);
                size_t i = 0;
                for (const auto &val : init) {
                    new (&data_[i++]) T(val);
                }
            }

            // Copy constructor
            Vector(const Vector &other) : size_(other.size_), capacity_(0), data_(nullptr) {
                allocate(size_);
                for (size_t i = 0; i < size_; ++i) {
                    new (&data_[i]) T(other.data_[i]);
                }
            }

            // Move constructor
            Vector(Vector &&other) noexcept : size_(other.size_), capacity_(other.capacity_), data_(other.data_) {
                other.size_ = 0;
                other.capacity_ = 0;
                other.data_ = nullptr;
            }

            // Destructor
            ~Vector() { deallocate(); }

            // Copy assignment
            Vector &operator=(const Vector &other) {
                if (this != &other) {
                    deallocate();
                    size_ = other.size_;
                    allocate(size_);
                    for (size_t i = 0; i < size_; ++i) {
                        new (&data_[i]) T(other.data_[i]);
                    }
                }
                return *this;
            }

            // Move assignment
            Vector &operator=(Vector &&other) noexcept {
                if (this != &other) {
                    deallocate();
                    size_ = other.size_;
                    capacity_ = other.capacity_;
                    data_ = other.data_;
                    other.size_ = 0;
                    other.capacity_ = 0;
                    other.data_ = nullptr;
                }
                return *this;
            }

            // Initializer list assignment
            Vector &operator=(std::initializer_list<T> init) {
                deallocate();
                size_ = init.size();
                allocate(size_);
                size_t i = 0;
                for (const auto &val : init) {
                    new (&data_[i++]) T(val);
                }
                return *this;
            }

            // Element access
            reference operator[](size_type i) noexcept { return data_[i]; }
            const_reference operator[](size_type i) const noexcept { return data_[i]; }

            reference at(size_type i) {
                if (i >= size_) {
                    throw std::out_of_range("Vector::at");
                }
                return data_[i];
            }

            const_reference at(size_type i) const {
                if (i >= size_) {
                    throw std::out_of_range("Vector::at");
                }
                return data_[i];
            }

            reference front() noexcept { return data_[0]; }
            const_reference front() const noexcept { return data_[0]; }

            reference back() noexcept { return data_[size_ - 1]; }
            const_reference back() const noexcept { return data_[size_ - 1]; }

            // Raw data access
            pointer data() noexcept { return data_; }
            const_pointer data() const noexcept { return data_; }

            // Capacity
            size_type size() const noexcept { return size_; }
            size_type length() const noexcept { return size_; }
            size_type capacity() const noexcept { return capacity_; }
            bool empty() const noexcept { return size_ == 0; }

            // Resize
            void resize(size_t new_size) {
                if (new_size == size_)
                    return;

                if (new_size <= capacity_) {
                    if (new_size < size_) {
                        for (size_t i = new_size; i < size_; ++i) {
                            data_[i].~T();
                        }
                    } else {
                        for (size_t i = size_; i < new_size; ++i) {
                            new (&data_[i]) T{};
                        }
                    }
                    size_ = new_size;
                } else {
                    T *new_data = static_cast<T *>(aligned_alloc(32, sizeof(T) * new_size));
                    for (size_t i = 0; i < size_; ++i) {
                        new (&new_data[i]) T(std::move(data_[i]));
                        data_[i].~T();
                    }
                    for (size_t i = size_; i < new_size; ++i) {
                        new (&new_data[i]) T{};
                    }
                    if (data_) {
                        aligned_free(32, data_);
                    }
                    data_ = new_data;
                    capacity_ = new_size;
                    size_ = new_size;
                }
            }

            void resize(size_t new_size, const T &value) {
                size_t old_size = size_;
                resize(new_size);
                for (size_t i = old_size; i < size_; ++i) {
                    data_[i] = value;
                }
            }

            void reserve(size_t new_cap) {
                if (new_cap <= capacity_)
                    return;

                T *new_data = static_cast<T *>(aligned_alloc(32, sizeof(T) * new_cap));
                for (size_t i = 0; i < size_; ++i) {
                    new (&new_data[i]) T(std::move(data_[i]));
                    data_[i].~T();
                }
                if (data_) {
                    aligned_free(32, data_);
                }
                data_ = new_data;
                capacity_ = new_cap;
            }

            void clear() {
                for (size_t i = 0; i < size_; ++i) {
                    data_[i].~T();
                }
                size_ = 0;
            }

            void push_back(const T &value) {
                if (size_ >= capacity_) {
                    reserve(capacity_ == 0 ? 8 : capacity_ * 2);
                }
                new (&data_[size_++]) T(value);
            }

            void push_back(T &&value) {
                if (size_ >= capacity_) {
                    reserve(capacity_ == 0 ? 8 : capacity_ * 2);
                }
                new (&data_[size_++]) T(std::move(value));
            }

            void pop_back() {
                if (size_ > 0) {
                    data_[--size_].~T();
                }
            }

            // Iterators
            iterator begin() noexcept { return data_; }
            const_iterator begin() const noexcept { return data_; }
            const_iterator cbegin() const noexcept { return data_; }

            iterator end() noexcept { return data_ + size_; }
            const_iterator end() const noexcept { return data_ + size_; }
            const_iterator cend() const noexcept { return data_ + size_; }

            // Operations
            void fill(const T &value) noexcept {
                for (size_t i = 0; i < size_; ++i) {
                    data_[i] = value;
                }
            }

            void swap(Vector &other) noexcept {
                std::swap(size_, other.size_);
                std::swap(capacity_, other.capacity_);
                std::swap(data_, other.data_);
            }

            // Comparison
            bool operator==(const Vector &other) const noexcept {
                if (size_ != other.size_)
                    return false;
                for (size_t i = 0; i < size_; ++i) {
                    if (!(data_[i] == other.data_[i]))
                        return false;
                }
                return true;
            }

            bool operator!=(const Vector &other) const noexcept { return !(*this == other); }
        };

        // Type trait for dynamic Vector
        template <typename T> struct is_dynamic_vector : std::false_type {};
        template <typename T> struct is_dynamic_vector<Vector<T, Dynamic, false>> : std::true_type {};
        template <typename T> inline constexpr bool is_dynamic_vector_v = is_dynamic_vector<T>::value;

        // Eigen-style aliases for dynamic vectors
        using VectorXf = Vector<float, Dynamic>;
        using VectorXd = Vector<double, Dynamic>;
        using VectorXi = Vector<int, Dynamic>;

    } // namespace mat

    namespace mat_vector {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace mat_vector

} // namespace datapod
