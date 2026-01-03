#pragma once

#include <array>
#include <cstddef>
#include <initializer_list>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include "datapod/core/aligned_alloc.hpp"
#include "datapod/matrix/vector.hpp" // For HEAP_THRESHOLD

namespace datapod {
    namespace mat {

        // Forward declaration
        template <typename T> struct Scalar;

        /**
         * @brief Tensor (rank-N, N-dimensional) - fixed-size N-dimensional numeric array
         *
         * Mathematical Tensor of arbitrary order (3D and higher).
         * NOT a container - purely for numeric/mathematical operations.
         *
         * Examples:
         *   Tensor<double, 3, 3, 3> cube;                  // 3x3x3 cube (stack)
         *   Tensor<float, 2, 3, 4> volume;                 // 2x3x4 volume (stack)
         *   Tensor<float, 256, 256, 256> big_vol;          // Large volume (heap, automatic)
         *   Tensor<double, 2, 2, 2, 2> rank4;              // Rank-4 Tensor (stack)
         *   Tensor<scalar<double>, 4, 4, 4> tagged;        // Tensor of scalars
         *
         * Design:
         * - Fixed shape (no resizing)
         * - Column-major storage (matching Eigen, BLAS, LAPACK, matrix.hpp)
         * - Contiguous storage (cache-friendly)
         * - Aligned for SIMD (32-byte alignment)
         * - Small tensors (total elements <= HEAP_THRESHOLD): POD-compatible, stack-allocated
         * - Large tensors (total elements > HEAP_THRESHOLD): Heap-allocated, SIMD-aligned
         * - Serializable via members() or explicit serialize/deserialize
         * - NO math operations (data layer only)
         * - Accepts both arithmetic types AND scalar<T>
         */

        // =============================================================================
        // STACK-ALLOCATED TENSOR (small tensors, POD, zero-copy)
        // =============================================================================
        template <typename T, size_t... Dims> struct Tensor {
            static_assert(sizeof...(Dims) >= 3,
                          "Tensor requires at least 3 dimensions (use vector for 1D, matrix for 2D)");
            static_assert(((Dims > 0) && ...), "all Tensor dimensions must be > 0");

            using value_type = T;
            using size_type = size_t;
            using reference = T &;
            using const_reference = const T &;
            using pointer = T *;
            using const_pointer = const T *;
            using iterator = T *;
            using const_iterator = const T *;

            static constexpr size_t rank = sizeof...(Dims);
            static constexpr std::array<size_t, rank> dims_ = {Dims...};
            static constexpr size_t size_ = (Dims * ...);
            static constexpr bool is_pod = true; // POD for zero-copy serialization
            static constexpr bool uses_heap = false;

            alignas(32) T data_[size_]; // Column-major storage (stack)

            // Default constructor (for aggregate initialization)
            constexpr Tensor() noexcept = default;

            // Brace initialization from flat list (column-major order)
            // Usage: Tensor<double, 2, 2, 2> t = {1, 2, 3, 4, 5, 6, 7, 8};
            constexpr Tensor(std::initializer_list<T> init) noexcept : data_{} {
                size_t i = 0;
                for (const auto &val : init) {
                    if (i >= size_)
                        break;
                    data_[i++] = val;
                }
            }

            // Composition constructor: construct Tensor from slices along last dimension
            // For Tensor<T, D0, D1, ..., Dn>, accepts Dn slices of shape D0 x D1 x ... x D(n-1)
            template <typename... SliceTypes, typename = std::enable_if_t<(sizeof...(SliceTypes) == dims_[rank - 1] &&
                                                                           sizeof...(SliceTypes) > 0)>>
            constexpr Tensor(const SliceTypes &...slices) noexcept : data_{} {
                fill_from_slices<0>(slices...);
            }

          private:
            // Calculate slice size (product of all dims except last)
            static constexpr size_t slice_size() noexcept {
                size_t s = 1;
                for (size_t i = 0; i < rank - 1; ++i) {
                    s *= dims_[i];
                }
                return s;
            }

            // Recursive helper to fill slices
            template <size_t SliceIdx> constexpr void fill_from_slices() noexcept {}

            template <size_t SliceIdx, typename SliceType, typename... Rest>
            constexpr void fill_from_slices(const SliceType &slice, const Rest &...rest) noexcept {
                constexpr size_t sz = slice_size();
                const T *slice_data = slice.data();
                for (size_t i = 0; i < sz; ++i) {
                    data_[i + SliceIdx * sz] = slice_data[i];
                }
                fill_from_slices<SliceIdx + 1>(rest...);
            }

          public:
            // Serialization support
            auto members() noexcept { return std::tie(data_); }
            auto members() const noexcept { return std::tie(data_); }

            // Helper to compute linear index from multi-dimensional indices (column-major)
          private:
            template <size_t I = 0, typename... Indices>
            static constexpr size_type compute_index(size_type idx, Indices... rest) noexcept {
                constexpr size_type stride = []() {
                    constexpr std::array<size_t, rank> d = {Dims...};
                    size_type s = 1;
                    for (size_t i = 0; i < I; ++i) {
                        s *= d[i];
                    }
                    return s;
                }();

                if constexpr (sizeof...(rest) == 0) {
                    return idx * stride;
                } else {
                    return idx * stride + compute_index<I + 1>(rest...);
                }
            }

          public:
            // Multi-dimensional indexing
            template <typename... Indices,
                      typename = std::enable_if_t<sizeof...(Indices) == rank &&
                                                  (std::is_convertible_v<Indices, size_type> && ...)>>
            constexpr reference operator()(Indices... indices) noexcept {
                return data_[compute_index(static_cast<size_type>(indices)...)];
            }

            template <typename... Indices,
                      typename = std::enable_if_t<sizeof...(Indices) == rank &&
                                                  (std::is_convertible_v<Indices, size_type> && ...)>>
            constexpr const_reference operator()(Indices... indices) const noexcept {
                return data_[compute_index(static_cast<size_type>(indices)...)];
            }

            // Checked multi-dimensional access
            template <typename... Indices,
                      typename = std::enable_if_t<sizeof...(Indices) == rank &&
                                                  (std::is_convertible_v<Indices, size_type> && ...)>>
            constexpr reference at(Indices... indices) {
                std::array<size_type, rank> idx_arr = {static_cast<size_type>(indices)...};
                for (size_t i = 0; i < rank; ++i) {
                    if (idx_arr[i] >= dims_[i]) {
                        throw std::out_of_range("Tensor::at");
                    }
                }
                return data_[compute_index(static_cast<size_type>(indices)...)];
            }

            template <typename... Indices,
                      typename = std::enable_if_t<sizeof...(Indices) == rank &&
                                                  (std::is_convertible_v<Indices, size_type> && ...)>>
            constexpr const_reference at(Indices... indices) const {
                std::array<size_type, rank> idx_arr = {static_cast<size_type>(indices)...};
                for (size_t i = 0; i < rank; ++i) {
                    if (idx_arr[i] >= dims_[i]) {
                        throw std::out_of_range("Tensor::at");
                    }
                }
                return data_[compute_index(static_cast<size_type>(indices)...)];
            }

            // 1D indexing (linear access in column-major order)
            constexpr reference operator[](size_type i) noexcept { return data_[i]; }
            constexpr const_reference operator[](size_type i) const noexcept { return data_[i]; }

            // Raw data access
            constexpr pointer data() noexcept { return data_; }
            constexpr const_pointer data() const noexcept { return data_; }

            // Dimensions
            constexpr size_type size() const noexcept { return size_; }
            constexpr bool empty() const noexcept { return false; } // Always false for valid dimensions
            static constexpr std::array<size_t, rank> shape() noexcept { return dims_; }
            static constexpr size_type dim(size_t i) noexcept { return dims_[i]; }

            // Iterators (linear iteration in column-major order)
            constexpr iterator begin() noexcept { return data_; }
            constexpr const_iterator begin() const noexcept { return data_; }
            constexpr const_iterator cbegin() const noexcept { return data_; }

            constexpr iterator end() noexcept { return data_ + size_; }
            constexpr const_iterator end() const noexcept { return data_ + size_; }
            constexpr const_iterator cend() const noexcept { return data_ + size_; }

            // Operations
            constexpr void fill(const T &value) noexcept {
                for (size_type i = 0; i < size_; ++i) {
                    data_[i] = value;
                }
            }

            constexpr void swap(Tensor &other) noexcept {
                for (size_type i = 0; i < size_; ++i) {
                    T tmp = data_[i];
                    data_[i] = other.data_[i];
                    other.data_[i] = tmp;
                }
            }

            // Comparison
            constexpr bool operator==(const Tensor &other) const noexcept {
                for (size_type i = 0; i < size_; ++i) {
                    if (!(data_[i] == other.data_[i])) {
                        return false;
                    }
                }
                return true;
            }

            constexpr bool operator!=(const Tensor &other) const noexcept { return !(*this == other); }
        };

        // =============================================================================
        // HEAP-ALLOCATED TENSOR (large tensors, NOT POD, SIMD-aligned)
        // =============================================================================
        template <typename T, size_t... Dims> struct HeapTensor {
            static_assert(sizeof...(Dims) >= 3,
                          "HeapTensor requires at least 3 dimensions (use vector for 1D, matrix for 2D)");
            static_assert(((Dims > 0) && ...), "all Tensor dimensions must be > 0");

            using value_type = T;
            using size_type = size_t;
            using reference = T &;
            using const_reference = const T &;
            using pointer = T *;
            using const_pointer = const T *;
            using iterator = T *;
            using const_iterator = const T *;

            static constexpr size_t rank = sizeof...(Dims);
            static constexpr std::array<size_t, rank> dims_ = {Dims...};
            static constexpr size_t size_ = (Dims * ...);
            static constexpr bool is_pod = false; // NOT POD (has destructor)
            static constexpr bool uses_heap = true;

            T *data_; // Heap-allocated, SIMD-aligned, column-major

            // Default constructor - allocate aligned heap memory
            HeapTensor() : data_(static_cast<T *>(aligned_alloc(32, sizeof(T) * size_))) {
                for (size_t i = 0; i < size_; ++i) {
                    new (&data_[i]) T{};
                }
            }

            // Destructor - free heap memory
            ~HeapTensor() {
                if (data_) {
                    for (size_t i = 0; i < size_; ++i) {
                        data_[i].~T();
                    }
                    aligned_free(32, data_);
                }
            }

            // Copy constructor
            HeapTensor(const HeapTensor &other) : data_(static_cast<T *>(aligned_alloc(32, sizeof(T) * size_))) {
                for (size_t i = 0; i < size_; ++i) {
                    new (&data_[i]) T(other.data_[i]);
                }
            }

            // Copy assignment
            HeapTensor &operator=(const HeapTensor &other) {
                if (this != &other) {
                    for (size_t i = 0; i < size_; ++i) {
                        data_[i] = other.data_[i];
                    }
                }
                return *this;
            }

            // Move constructor
            HeapTensor(HeapTensor &&other) noexcept : data_(other.data_) { other.data_ = nullptr; }

            // Move assignment
            HeapTensor &operator=(HeapTensor &&other) noexcept {
                if (this != &other) {
                    if (data_) {
                        for (size_t i = 0; i < size_; ++i) {
                            data_[i].~T();
                        }
                        aligned_free(32, data_);
                    }
                    data_ = other.data_;
                    other.data_ = nullptr;
                }
                return *this;
            }

            // Brace initialization from flat list (column-major order)
            // Usage: HeapTensor<double, 10, 10, 10> t = {1, 2, 3, ...};
            HeapTensor(std::initializer_list<T> init) : data_(static_cast<T *>(aligned_alloc(32, sizeof(T) * size_))) {
                size_t i = 0;
                for (const auto &val : init) {
                    if (i >= size_)
                        break;
                    new (&data_[i++]) T(val);
                }
                // Zero-initialize remaining elements
                for (; i < size_; ++i) {
                    new (&data_[i]) T{};
                }
            }

            // Serialization support
            auto members() noexcept { return std::tie(data_); }
            auto members() const noexcept { return std::tie(data_); }

            // Helper to compute linear index from multi-dimensional indices (column-major)
          private:
            template <size_t I = 0, typename... Indices>
            static constexpr size_type compute_index(size_type idx, Indices... rest) noexcept {
                constexpr size_type stride = []() {
                    constexpr std::array<size_t, rank> d = {Dims...};
                    size_type s = 1;
                    for (size_t i = 0; i < I; ++i) {
                        s *= d[i];
                    }
                    return s;
                }();

                if constexpr (sizeof...(rest) == 0) {
                    return idx * stride;
                } else {
                    return idx * stride + compute_index<I + 1>(rest...);
                }
            }

          public:
            // Multi-dimensional indexing - SAME API as stack Tensor
            template <typename... Indices,
                      typename = std::enable_if_t<sizeof...(Indices) == rank &&
                                                  (std::is_convertible_v<Indices, size_type> && ...)>>
            reference operator()(Indices... indices) noexcept {
                return data_[compute_index(static_cast<size_type>(indices)...)];
            }

            template <typename... Indices,
                      typename = std::enable_if_t<sizeof...(Indices) == rank &&
                                                  (std::is_convertible_v<Indices, size_type> && ...)>>
            const_reference operator()(Indices... indices) const noexcept {
                return data_[compute_index(static_cast<size_type>(indices)...)];
            }

            // Checked multi-dimensional access
            template <typename... Indices,
                      typename = std::enable_if_t<sizeof...(Indices) == rank &&
                                                  (std::is_convertible_v<Indices, size_type> && ...)>>
            reference at(Indices... indices) {
                std::array<size_type, rank> idx_arr = {static_cast<size_type>(indices)...};
                for (size_t i = 0; i < rank; ++i) {
                    if (idx_arr[i] >= dims_[i]) {
                        throw std::out_of_range("HeapTensor::at");
                    }
                }
                return data_[compute_index(static_cast<size_type>(indices)...)];
            }

            template <typename... Indices,
                      typename = std::enable_if_t<sizeof...(Indices) == rank &&
                                                  (std::is_convertible_v<Indices, size_type> && ...)>>
            const_reference at(Indices... indices) const {
                std::array<size_type, rank> idx_arr = {static_cast<size_type>(indices)...};
                for (size_t i = 0; i < rank; ++i) {
                    if (idx_arr[i] >= dims_[i]) {
                        throw std::out_of_range("HeapTensor::at");
                    }
                }
                return data_[compute_index(static_cast<size_type>(indices)...)];
            }

            // 1D indexing (linear access in column-major order)
            reference operator[](size_type i) noexcept { return data_[i]; }
            const_reference operator[](size_type i) const noexcept { return data_[i]; }

            // Raw data access
            pointer data() noexcept { return data_; }
            const_pointer data() const noexcept { return data_; }

            // Dimensions
            constexpr size_type size() const noexcept { return size_; }
            constexpr bool empty() const noexcept { return false; }
            static constexpr std::array<size_t, rank> shape() noexcept { return dims_; }
            static constexpr size_type dim(size_t i) noexcept { return dims_[i]; }

            // Iterators (linear iteration in column-major order)
            iterator begin() noexcept { return data_; }
            const_iterator begin() const noexcept { return data_; }
            const_iterator cbegin() const noexcept { return data_; }

            iterator end() noexcept { return data_ + size_; }
            const_iterator end() const noexcept { return data_ + size_; }
            const_iterator cend() const noexcept { return data_ + size_; }

            // Operations
            void fill(const T &value) noexcept {
                for (size_type i = 0; i < size_; ++i) {
                    data_[i] = value;
                }
            }

            void swap(HeapTensor &other) noexcept { std::swap(data_, other.data_); }

            // Comparison
            bool operator==(const HeapTensor &other) const noexcept {
                for (size_type i = 0; i < size_; ++i) {
                    if (!(data_[i] == other.data_[i])) {
                        return false;
                    }
                }
                return true;
            }

            bool operator!=(const HeapTensor &other) const noexcept { return !(*this == other); }
        };

        // =============================================================================
        // HELPER: Check if any dimension is Dynamic
        // =============================================================================
        template <size_t... Dims> struct has_dynamic_dim : std::bool_constant<((Dims == Dynamic) || ...)> {};

        template <size_t... Dims> inline constexpr bool has_dynamic_dim_v = has_dynamic_dim<Dims...>::value;

        // Count fixed dimensions (non-Dynamic)
        template <size_t... Dims> struct count_fixed_dims {
            static constexpr size_t value = ((Dims != Dynamic ? 1 : 0) + ...);
        };

        // Count dynamic dimensions
        template <size_t... Dims> struct count_dynamic_dims {
            static constexpr size_t value = ((Dims == Dynamic ? 1 : 0) + ...);
        };

        // =============================================================================
        // DYNAMIC TENSOR: When any dimension is Dynamic
        // =============================================================================
        /**
         * @brief Tensor with some or all dimensions determined at runtime
         *
         * When any template dimension is Dynamic, this specialization is used.
         * Fixed dimensions are known at compile-time, dynamic ones at runtime.
         *
         * Examples:
         *   Tensor<double, Dynamic, 4, 5> batch(32);      // 32x4x5 - batch of 4x5 matrices
         *   Tensor<float, Dynamic, 224, 224, 3> imgs(16); // 16x224x224x3 - batch of images
         *   Tensor<double, Dynamic, Dynamic, Dynamic> t(2,3,4); // fully dynamic 2x3x4
         *
         * Constructor takes runtime sizes for Dynamic dimensions only, in order.
         */
        template <typename T, size_t... Dims>
        requires(has_dynamic_dim_v<Dims...> && sizeof...(Dims) >= 3)
        struct Tensor<T, Dims...> {
            using value_type = T;
            using size_type = size_t;
            using reference = T &;
            using const_reference = const T &;
            using pointer = T *;
            using const_pointer = const T *;
            using iterator = T *;
            using const_iterator = const T *;

            static constexpr size_t rank = sizeof...(Dims);
            static constexpr std::array<size_t, rank> template_dims_ = {Dims...};
            static constexpr size_t num_dynamic = count_dynamic_dims<Dims...>::value;
            static constexpr bool is_pod = false;
            static constexpr bool uses_heap = true;
            static constexpr bool is_dynamic = true;

          private:
            std::array<size_t, rank> dims_;    // Actual dimensions (fixed or runtime)
            std::array<size_t, rank> strides_; // Strides for indexing
            size_t size_;
            T *data_;

            void compute_strides() noexcept {
                // Column-major strides
                strides_[0] = 1;
                for (size_t i = 1; i < rank; ++i) {
                    strides_[i] = strides_[i - 1] * dims_[i - 1];
                }
            }

            void compute_size() noexcept {
                size_ = 1;
                for (size_t i = 0; i < rank; ++i) {
                    size_ *= dims_[i];
                }
            }

            void allocate() {
                if (size_ > 0) {
                    data_ = static_cast<T *>(aligned_alloc(32, sizeof(T) * size_));
                    for (size_t i = 0; i < size_; ++i) {
                        new (&data_[i]) T{};
                    }
                } else {
                    data_ = nullptr;
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

            // Initialize dims_ from template (fixed) and runtime (dynamic) values
            template <size_t I = 0, size_t DynIdx = 0>
            void init_dims(const std::array<size_t, num_dynamic> &dyn_sizes) noexcept {
                if constexpr (I < rank) {
                    if constexpr (template_dims_[I] == Dynamic) {
                        dims_[I] = dyn_sizes[DynIdx];
                        init_dims<I + 1, DynIdx + 1>(dyn_sizes);
                    } else {
                        dims_[I] = template_dims_[I];
                        init_dims<I + 1, DynIdx>(dyn_sizes);
                    }
                }
            }

          public:
            // Constructor taking runtime sizes for dynamic dimensions only
            template <typename... DynSizes>
            requires(sizeof...(DynSizes) == num_dynamic && (std::is_convertible_v<DynSizes, size_t> && ...))
            explicit Tensor(DynSizes... dyn_sizes) : dims_{}, strides_{}, size_(0), data_(nullptr) {
                std::array<size_t, num_dynamic> dyn_arr = {static_cast<size_t>(dyn_sizes)...};
                init_dims(dyn_arr);
                compute_strides();
                compute_size();
                allocate();
            }

            // Default constructor (only valid if no dynamic dims - but this specialization requires dynamic)
            // For fully dynamic, creates empty Tensor
            Tensor() : dims_{}, strides_{}, size_(0), data_(nullptr) {
                // Initialize fixed dims from template
                for (size_t i = 0; i < rank; ++i) {
                    dims_[i] = (template_dims_[i] == Dynamic) ? 0 : template_dims_[i];
                }
                compute_strides();
                compute_size();
                // Don't allocate - size is 0 due to dynamic dims being 0
            }

            // Copy constructor
            Tensor(const Tensor &other)
                : dims_(other.dims_), strides_(other.strides_), size_(other.size_), data_(nullptr) {
                if (size_ > 0) {
                    data_ = static_cast<T *>(aligned_alloc(32, sizeof(T) * size_));
                    for (size_t i = 0; i < size_; ++i) {
                        new (&data_[i]) T(other.data_[i]);
                    }
                }
            }

            // Move constructor
            Tensor(Tensor &&other) noexcept
                : dims_(other.dims_), strides_(other.strides_), size_(other.size_), data_(other.data_) {
                other.size_ = 0;
                other.data_ = nullptr;
            }

            // Destructor
            ~Tensor() { deallocate(); }

            // Copy assignment
            Tensor &operator=(const Tensor &other) {
                if (this != &other) {
                    deallocate();
                    dims_ = other.dims_;
                    strides_ = other.strides_;
                    size_ = other.size_;
                    if (size_ > 0) {
                        data_ = static_cast<T *>(aligned_alloc(32, sizeof(T) * size_));
                        for (size_t i = 0; i < size_; ++i) {
                            new (&data_[i]) T(other.data_[i]);
                        }
                    }
                }
                return *this;
            }

            // Move assignment
            Tensor &operator=(Tensor &&other) noexcept {
                if (this != &other) {
                    deallocate();
                    dims_ = other.dims_;
                    strides_ = other.strides_;
                    size_ = other.size_;
                    data_ = other.data_;
                    other.size_ = 0;
                    other.data_ = nullptr;
                }
                return *this;
            }

            // Resize (only dynamic dimensions can change)
            template <typename... DynSizes>
            requires(sizeof...(DynSizes) == num_dynamic && (std::is_convertible_v<DynSizes, size_t> && ...))
            void resize(DynSizes... dyn_sizes) {
                deallocate();
                std::array<size_t, num_dynamic> dyn_arr = {static_cast<size_t>(dyn_sizes)...};
                init_dims(dyn_arr);
                compute_strides();
                compute_size();
                allocate();
            }

            // Multi-dimensional indexing
            template <typename... Indices>
            requires(sizeof...(Indices) == rank && (std::is_convertible_v<Indices, size_type> && ...))
            reference operator()(Indices... indices) noexcept {
                std::array<size_type, rank> idx_arr = {static_cast<size_type>(indices)...};
                size_type linear = 0;
                for (size_t i = 0; i < rank; ++i) {
                    linear += idx_arr[i] * strides_[i];
                }
                return data_[linear];
            }

            template <typename... Indices>
            requires(sizeof...(Indices) == rank && (std::is_convertible_v<Indices, size_type> && ...))
            const_reference operator()(Indices... indices) const noexcept {
                std::array<size_type, rank> idx_arr = {static_cast<size_type>(indices)...};
                size_type linear = 0;
                for (size_t i = 0; i < rank; ++i) {
                    linear += idx_arr[i] * strides_[i];
                }
                return data_[linear];
            }

            // Bounds-checked access
            template <typename... Indices>
            requires(sizeof...(Indices) == rank && (std::is_convertible_v<Indices, size_type> && ...))
            reference at(Indices... indices) {
                std::array<size_type, rank> idx_arr = {static_cast<size_type>(indices)...};
                for (size_t i = 0; i < rank; ++i) {
                    if (idx_arr[i] >= dims_[i]) {
                        throw std::out_of_range("Tensor::at");
                    }
                }
                size_type linear = 0;
                for (size_t i = 0; i < rank; ++i) {
                    linear += idx_arr[i] * strides_[i];
                }
                return data_[linear];
            }

            template <typename... Indices>
            requires(sizeof...(Indices) == rank && (std::is_convertible_v<Indices, size_type> && ...))
            const_reference at(Indices... indices) const {
                std::array<size_type, rank> idx_arr = {static_cast<size_type>(indices)...};
                for (size_t i = 0; i < rank; ++i) {
                    if (idx_arr[i] >= dims_[i]) {
                        throw std::out_of_range("Tensor::at");
                    }
                }
                size_type linear = 0;
                for (size_t i = 0; i < rank; ++i) {
                    linear += idx_arr[i] * strides_[i];
                }
                return data_[linear];
            }

            // Linear indexing
            reference operator[](size_type i) noexcept { return data_[i]; }
            const_reference operator[](size_type i) const noexcept { return data_[i]; }

            // Raw data access
            pointer data() noexcept { return data_; }
            const_pointer data() const noexcept { return data_; }

            // Dimensions
            size_type size() const noexcept { return size_; }
            bool empty() const noexcept { return size_ == 0; }
            const std::array<size_t, rank> &shape() const noexcept { return dims_; }
            size_type dim(size_t i) const noexcept { return dims_[i]; }
            const std::array<size_t, rank> &strides() const noexcept { return strides_; }

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

            void setZero() { fill(T{}); }

            void swap(Tensor &other) noexcept {
                std::swap(dims_, other.dims_);
                std::swap(strides_, other.strides_);
                std::swap(size_, other.size_);
                std::swap(data_, other.data_);
            }

            // Comparison
            bool operator==(const Tensor &other) const noexcept {
                if (dims_ != other.dims_)
                    return false;
                for (size_t i = 0; i < size_; ++i) {
                    if (!(data_[i] == other.data_[i]))
                        return false;
                }
                return true;
            }

            bool operator!=(const Tensor &other) const noexcept { return !(*this == other); }
        };

        // Type traits
        template <typename T> struct is_tensor : std::false_type {};
        template <typename T, size_t... Dims> struct is_tensor<Tensor<T, Dims...>> : std::true_type {};
        template <typename T, size_t... Dims> struct is_tensor<HeapTensor<T, Dims...>> : std::true_type {};
        template <typename T> inline constexpr bool is_tensor_v = is_tensor<T>::value;

        // Type trait to check if Tensor uses heap
        template <typename T> struct is_heap_tensor : std::false_type {};
        template <typename T, size_t... Dims> struct is_heap_tensor<HeapTensor<T, Dims...>> : std::true_type {};
        template <typename T> inline constexpr bool is_heap_tensor_v = is_heap_tensor<T>::value;

        // Type trait to check if Tensor has any dynamic dimensions (partial dynamic)
        // Note: This is different from dynamic_tensor<T> in dynamic.hpp which has runtime rank
        template <typename T> struct is_partially_dynamic_tensor : std::false_type {};
        template <typename T, size_t... Dims>
        requires(has_dynamic_dim_v<Dims...>)
        struct is_partially_dynamic_tensor<Tensor<T, Dims...>> : std::true_type {};
        template <typename T>
        inline constexpr bool is_partially_dynamic_tensor_v = is_partially_dynamic_tensor<T>::value;

        // Common Tensor type aliases (3D)
        template <typename T> using tensor3d_2x2x2 = Tensor<T, 2, 2, 2>;
        template <typename T> using tensor3d_3x3x3 = Tensor<T, 3, 3, 3>;
        template <typename T> using tensor3d_4x4x4 = Tensor<T, 4, 4, 4>;

        // Common numeric types (3D cubes)
        using tensor3d_2x2x2f = Tensor<float, 2, 2, 2>;
        using tensor3d_2x2x2d = Tensor<double, 2, 2, 2>;
        using tensor3d_3x3x3f = Tensor<float, 3, 3, 3>;
        using tensor3d_3x3x3d = Tensor<double, 3, 3, 3>;
        using tensor3d_4x4x4f = Tensor<float, 4, 4, 4>;
        using tensor3d_4x4x4d = Tensor<double, 4, 4, 4>;

        // Fully dynamic Tensor aliases (all dimensions runtime)
        template <typename T> using Tensor3Xd = Tensor<T, Dynamic, Dynamic, Dynamic>;
        template <typename T> using Tensor4Xd = Tensor<T, Dynamic, Dynamic, Dynamic, Dynamic>;

        using Tensor3Xf = Tensor<float, Dynamic, Dynamic, Dynamic>;
        using Tensor3Xdd = Tensor<double, Dynamic, Dynamic, Dynamic>;
        using Tensor4Xf = Tensor<float, Dynamic, Dynamic, Dynamic, Dynamic>;
        using Tensor4Xdd = Tensor<double, Dynamic, Dynamic, Dynamic, Dynamic>;

    } // namespace mat
} // namespace datapod
