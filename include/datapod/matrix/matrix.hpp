#pragma once

#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "datapod/core/aligned_alloc.hpp"
#include "datapod/matrix/vector.hpp" // For HEAP_THRESHOLD

namespace datapod {
    namespace mat {

        // Forward declaration
        template <typename T> struct scalar;

        /**
         * @brief Matrix (rank-2, 2-D only) - fixed-size numeric matrix
         *
         * Mathematical tensor of order 2 - represents a linear operator.
         * NOT a container - purely for numeric/mathematical operations.
         *
         * Examples:
         *   matrix<double, 3, 3> rotation;              // SO(3) rotation matrix (stack)
         *   matrix<double, 6, 6> covariance;            // 6x6 covariance matrix (stack)
         *   matrix<float, 4, 4> transform;              // SE(3) transformation (stack)
         *   matrix<float, 1024, 1024> big_mat;          // Large matrix (heap)
         *   matrix<scalar<double>, 3, 3> tagged_mat;    // Matrix of scalars
         *
         * Design:
         * - Fixed shape R×C (no resizing)
         * - Column-major storage (matching Eigen, BLAS, LAPACK)
         * - Contiguous storage (cache-friendly)
         * - Aligned for SIMD (32-byte alignment)
         * - Small matrices (R*C <= HEAP_THRESHOLD): POD-compatible, stack-allocated
         * - Large matrices (R*C > HEAP_THRESHOLD): Heap-allocated, SIMD-aligned
         * - Serializable via members() or explicit serialize/deserialize
         * - NO math operations (data layer only)
         * - Bridge to Eigen via data() pointer
         * - Accepts both arithmetic types AND scalar<T>
         */

        // =============================================================================
        // PRIMARY TEMPLATE: Small matrices (stack-allocated, POD, zero-copy)
        // =============================================================================
        // Note: When R == Dynamic or C == Dynamic, we want to use the dynamic specialization (false flag)
        // otherwise use heap allocation for large matrices (R * C > HEAP_THRESHOLD)
        template <typename T, size_t R, size_t C,
                  bool UseHeap = (R != Dynamic && C != Dynamic && R * C > HEAP_THRESHOLD)>
        struct matrix {
            // Accept arithmetic types (and scalar<T>, but we can't check that here due to forward declaration)
            // The scalar<T> case will work because it's a POD type
            static_assert(R > 0, "matrix rows must be > 0");
            static_assert(C > 0, "matrix cols must be > 0");

            using value_type = T;
            using size_type = size_t;
            using reference = T &;
            using const_reference = const T &;
            using pointer = T *;
            using const_pointer = const T *;
            using iterator = T *;
            using const_iterator = const T *;

            static constexpr size_t rank = 2;      // Rank-2 tensor
            static constexpr size_t rows_ = R;     // Number of rows
            static constexpr size_t cols_ = C;     // Number of columns
            static constexpr size_t size_ = R * C; // Total elements
            static constexpr bool is_pod = true;   // POD for zero-copy serialization
            static constexpr bool uses_heap = false;

            alignas(32) T data_[R * C]; // Column-major: data_[col * R + row] (stack)

            // Default constructor (zero-initialized)
            constexpr matrix() noexcept : data_{} {}

            // Brace initialization from flat list (column-major order)
            // Usage: matrix<double, 2, 2> m = {1, 2, 3, 4}; // col0={1,2}, col1={3,4}
            constexpr matrix(std::initializer_list<T> init) noexcept : data_{} {
                size_t i = 0;
                for (const auto &val : init) {
                    if (i >= R * C)
                        break;
                    data_[i++] = val;
                }
            }

            // Composition constructor: construct matrix from C column vectors
            // Usage: matrix<T, R, C> m(vec1, vec2, ..., vecC);
            template <typename... VecTypes,
                      typename = std::enable_if_t<(sizeof...(VecTypes) == C && sizeof...(VecTypes) > 0)>>
            constexpr matrix(const VecTypes &...vecs) noexcept : data_{} {
                fill_from_vectors<0>(vecs...);
            }

          private:
            // Recursive helper to fill columns from vectors
            template <size_t Col> constexpr void fill_from_vectors() noexcept {}

            template <size_t Col, typename VecType, typename... Rest>
            constexpr void fill_from_vectors(const VecType &vec, const Rest &...rest) noexcept {
                for (size_t i = 0; i < R; ++i) {
                    (*this)(i, Col) = vec[i];
                }
                fill_from_vectors<Col + 1>(rest...);
            }

          public:
            // Serialization support
            auto members() noexcept { return std::tie(data_); }
            auto members() const noexcept { return std::tie(data_); }

            // 2D indexing - column-major layout
            constexpr reference operator()(size_type row, size_type col) noexcept {
                return data_[col * R + row]; // Column-major
            }

            constexpr const_reference operator()(size_type row, size_type col) const noexcept {
                return data_[col * R + row]; // Column-major
            }

            constexpr reference at(size_type row, size_type col) {
                if (row >= R || col >= C) {
                    throw std::out_of_range("matrix::at");
                }
                return data_[col * R + row];
            }

            constexpr const_reference at(size_type row, size_type col) const {
                if (row >= R || col >= C) {
                    throw std::out_of_range("matrix::at");
                }
                return data_[col * R + row];
            }

            // 1D indexing (linear access in column-major order)
            constexpr reference operator[](size_type i) noexcept { return data_[i]; }
            constexpr const_reference operator[](size_type i) const noexcept { return data_[i]; }

            // Raw data access (for Eigen mapping)
            constexpr pointer data() noexcept { return data_; }
            constexpr const_pointer data() const noexcept { return data_; }

            // Dimensions
            constexpr size_type rows() const noexcept { return R; }
            constexpr size_type cols() const noexcept { return C; }
            constexpr size_type size() const noexcept { return R * C; }
            constexpr bool empty() const noexcept { return false; } // Always false for R,C>0

            // Iterators (linear iteration in column-major order)
            constexpr iterator begin() noexcept { return data_; }
            constexpr const_iterator begin() const noexcept { return data_; }
            constexpr const_iterator cbegin() const noexcept { return data_; }

            constexpr iterator end() noexcept { return data_ + R * C; }
            constexpr const_iterator end() const noexcept { return data_ + R * C; }
            constexpr const_iterator cend() const noexcept { return data_ + R * C; }

            // Operations
            constexpr void fill(const T &value) noexcept {
                for (size_type i = 0; i < R * C; ++i) {
                    data_[i] = value;
                }
            }

            constexpr void swap(matrix &other) noexcept {
                for (size_type i = 0; i < R * C; ++i) {
                    T tmp = data_[i];
                    data_[i] = other.data_[i];
                    other.data_[i] = tmp;
                }
            }

            // Set to identity (only for square matrices)
            template <size_t R_ = R, size_t C_ = C> constexpr std::enable_if_t<R_ == C_, void> set_identity() noexcept {
                fill(T{});
                for (size_type i = 0; i < R; ++i) {
                    (*this)(i, i) = T{1};
                }
            }

            // Comparison
            constexpr bool operator==(const matrix &other) const noexcept {
                for (size_type i = 0; i < R * C; ++i) {
                    if (!(data_[i] == other.data_[i])) {
                        return false;
                    }
                }
                return true;
            }

            constexpr bool operator!=(const matrix &other) const noexcept { return !(*this == other); }
        };

        // =============================================================================
        // SPECIALIZATION: Large matrices (heap-allocated, NOT POD, SIMD-aligned)
        // =============================================================================
        template <typename T, size_t R, size_t C> struct matrix<T, R, C, true> {
            static_assert(R > 0, "matrix rows must be > 0");
            static_assert(C > 0, "matrix cols must be > 0");

            using value_type = T;
            using size_type = size_t;
            using reference = T &;
            using const_reference = const T &;
            using pointer = T *;
            using const_pointer = const T *;
            using iterator = T *;
            using const_iterator = const T *;

            static constexpr size_t rank = 2;      // Rank-2 tensor
            static constexpr size_t rows_ = R;     // Number of rows
            static constexpr size_t cols_ = C;     // Number of columns
            static constexpr size_t size_ = R * C; // Total elements
            static constexpr bool is_pod = false;  // NOT POD (has destructor)
            static constexpr bool uses_heap = true;

            T *data_; // Heap-allocated, SIMD-aligned, column-major

            // Default constructor - allocate aligned heap memory
            matrix() : data_(static_cast<T *>(aligned_alloc(32, sizeof(T) * R * C))) {
                for (size_t i = 0; i < R * C; ++i) {
                    new (&data_[i]) T{};
                }
            }

            // Destructor - free heap memory
            ~matrix() {
                if (data_) {
                    for (size_t i = 0; i < R * C; ++i) {
                        data_[i].~T();
                    }
                    aligned_free(32, data_);
                }
            }

            // Copy constructor
            matrix(const matrix &other) : data_(static_cast<T *>(aligned_alloc(32, sizeof(T) * R * C))) {
                for (size_t i = 0; i < R * C; ++i) {
                    new (&data_[i]) T(other.data_[i]);
                }
            }

            // Copy assignment
            matrix &operator=(const matrix &other) {
                if (this != &other) {
                    for (size_t i = 0; i < R * C; ++i) {
                        data_[i] = other.data_[i];
                    }
                }
                return *this;
            }

            // Move constructor
            matrix(matrix &&other) noexcept : data_(other.data_) { other.data_ = nullptr; }

            // Move assignment
            matrix &operator=(matrix &&other) noexcept {
                if (this != &other) {
                    if (data_) {
                        for (size_t i = 0; i < R * C; ++i) {
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
            // Usage: matrix<double, 100, 100> m = {1, 2, 3, ...};
            matrix(std::initializer_list<T> init) : data_(static_cast<T *>(aligned_alloc(32, sizeof(T) * R * C))) {
                size_t i = 0;
                for (const auto &val : init) {
                    if (i >= R * C)
                        break;
                    new (&data_[i++]) T(val);
                }
                // Zero-initialize remaining elements
                for (; i < R * C; ++i) {
                    new (&data_[i]) T{};
                }
            }

            // Serialization support
            auto members() noexcept { return std::tie(data_); }
            auto members() const noexcept { return std::tie(data_); }

            // 2D indexing - column-major layout (SAME API as stack version)
            reference operator()(size_type row, size_type col) noexcept {
                return data_[col * R + row]; // Column-major
            }

            const_reference operator()(size_type row, size_type col) const noexcept {
                return data_[col * R + row]; // Column-major
            }

            reference at(size_type row, size_type col) {
                if (row >= R || col >= C) {
                    throw std::out_of_range("matrix::at");
                }
                return data_[col * R + row];
            }

            const_reference at(size_type row, size_type col) const {
                if (row >= R || col >= C) {
                    throw std::out_of_range("matrix::at");
                }
                return data_[col * R + row];
            }

            // 1D indexing (linear access in column-major order)
            reference operator[](size_type i) noexcept { return data_[i]; }
            const_reference operator[](size_type i) const noexcept { return data_[i]; }

            // Raw data access (for Eigen mapping)
            pointer data() noexcept { return data_; }
            const_pointer data() const noexcept { return data_; }

            // Dimensions
            constexpr size_type rows() const noexcept { return R; }
            constexpr size_type cols() const noexcept { return C; }
            constexpr size_type size() const noexcept { return R * C; }
            constexpr bool empty() const noexcept { return false; }

            // Iterators (linear iteration in column-major order)
            iterator begin() noexcept { return data_; }
            const_iterator begin() const noexcept { return data_; }
            const_iterator cbegin() const noexcept { return data_; }

            iterator end() noexcept { return data_ + R * C; }
            const_iterator end() const noexcept { return data_ + R * C; }
            const_iterator cend() const noexcept { return data_ + R * C; }

            // Operations
            void fill(const T &value) noexcept {
                for (size_type i = 0; i < R * C; ++i) {
                    data_[i] = value;
                }
            }

            void swap(matrix &other) noexcept { std::swap(data_, other.data_); }

            // Set to identity (only for square matrices)
            template <size_t R_ = R, size_t C_ = C> std::enable_if_t<R_ == C_, void> set_identity() noexcept {
                fill(T{});
                for (size_type i = 0; i < R; ++i) {
                    (*this)(i, i) = T{1};
                }
            }

            // Comparison
            bool operator==(const matrix &other) const noexcept {
                for (size_type i = 0; i < R * C; ++i) {
                    if (!(data_[i] == other.data_[i])) {
                        return false;
                    }
                }
                return true;
            }

            bool operator!=(const matrix &other) const noexcept { return !(*this == other); }
        };

        // Type traits
        template <typename T> struct is_matrix : std::false_type {};
        template <typename T, size_t R, size_t C, bool UseHeap>
        struct is_matrix<matrix<T, R, C, UseHeap>> : std::true_type {};
        template <typename T> inline constexpr bool is_matrix_v = is_matrix<T>::value;

        // Type trait to check if matrix uses heap
        template <typename T> struct is_heap_matrix : std::false_type {};
        template <typename T, size_t R, size_t C> struct is_heap_matrix<matrix<T, R, C, true>> : std::true_type {};
        template <typename T> inline constexpr bool is_heap_matrix_v = is_heap_matrix<T>::value;

        // Common matrix type aliases
        template <typename T> using matrix2x2 = matrix<T, 2, 2>;
        template <typename T> using matrix3x3 = matrix<T, 3, 3>;
        template <typename T> using matrix4x4 = matrix<T, 4, 4>;
        template <typename T> using matrix6x6 = matrix<T, 6, 6>;

        // Common numeric types
        using matrix2x2f = matrix<float, 2, 2>;
        using matrix2x2d = matrix<double, 2, 2>;
        using matrix3x3f = matrix<float, 3, 3>;
        using matrix3x3d = matrix<double, 3, 3>;
        using matrix4x4f = matrix<float, 4, 4>;
        using matrix4x4d = matrix<double, 4, 4>;
        using matrix6x6f = matrix<float, 6, 6>;
        using matrix6x6d = matrix<double, 6, 6>;

        // =============================================================================
        // SPECIALIZATION: Dynamic matrices (runtime-sized, heap-allocated)
        // =============================================================================
        /**
         * @brief Runtime-sized numeric matrix
         *
         * Specialization for matrix<T, Dynamic, Dynamic> - dimensions at runtime.
         * Always heap-allocated with SIMD alignment. Column-major storage.
         *
         * Examples:
         *   matrix<double, Dynamic, Dynamic> m(100, 100);   // 100x100 matrix
         *   matrix<float, Dynamic, Dynamic> A(3, 4);        // 3x4 matrix
         *   m.resize(200, 200);                             // Resize to 200x200
         */
        template <typename T> struct matrix<T, Dynamic, Dynamic, false> {
            using value_type = T;
            using size_type = size_t;
            using reference = T &;
            using const_reference = const T &;
            using pointer = T *;
            using const_pointer = const T *;
            using iterator = T *;
            using const_iterator = const T *;

            static constexpr size_t rank = 2;
            static constexpr bool is_pod = false;
            static constexpr bool uses_heap = true;
            static constexpr bool is_dynamic = true;

          private:
            size_t rows_;
            size_t cols_;
            T *data_;

            void allocate(size_t total) {
                if (total > 0) {
                    data_ = static_cast<T *>(aligned_alloc(32, sizeof(T) * total));
                } else {
                    data_ = nullptr;
                }
            }

            void deallocate() {
                if (data_) {
                    size_t total = rows_ * cols_;
                    for (size_t i = 0; i < total; ++i) {
                        data_[i].~T();
                    }
                    aligned_free(32, data_);
                    data_ = nullptr;
                }
            }

          public:
            // Default constructor - empty matrix
            matrix() noexcept : rows_(0), cols_(0), data_(nullptr) {}

            // Size constructor
            matrix(size_t rows, size_t cols) : rows_(rows), cols_(cols), data_(nullptr) {
                size_t total = rows_ * cols_;
                allocate(total);
                for (size_t i = 0; i < total; ++i) {
                    new (&data_[i]) T{};
                }
            }

            // Size + value constructor
            matrix(size_t rows, size_t cols, const T &value) : rows_(rows), cols_(cols), data_(nullptr) {
                size_t total = rows_ * cols_;
                allocate(total);
                for (size_t i = 0; i < total; ++i) {
                    new (&data_[i]) T(value);
                }
            }

            // Initializer list constructor (column-major order)
            matrix(size_t rows, size_t cols, std::initializer_list<T> init) : rows_(rows), cols_(cols), data_(nullptr) {
                size_t total = rows_ * cols_;
                allocate(total);
                for (size_t i = 0; i < total; ++i) {
                    new (&data_[i]) T{};
                }
                size_t i = 0;
                for (const auto &val : init) {
                    if (i >= total)
                        break;
                    data_[i++] = val;
                }
            }

            // Copy constructor
            matrix(const matrix &other) : rows_(other.rows_), cols_(other.cols_), data_(nullptr) {
                size_t total = rows_ * cols_;
                allocate(total);
                for (size_t i = 0; i < total; ++i) {
                    new (&data_[i]) T(other.data_[i]);
                }
            }

            // Move constructor
            matrix(matrix &&other) noexcept : rows_(other.rows_), cols_(other.cols_), data_(other.data_) {
                other.rows_ = 0;
                other.cols_ = 0;
                other.data_ = nullptr;
            }

            // Destructor
            ~matrix() { deallocate(); }

            // Copy assignment
            matrix &operator=(const matrix &other) {
                if (this != &other) {
                    deallocate();
                    rows_ = other.rows_;
                    cols_ = other.cols_;
                    size_t total = rows_ * cols_;
                    allocate(total);
                    for (size_t i = 0; i < total; ++i) {
                        new (&data_[i]) T(other.data_[i]);
                    }
                }
                return *this;
            }

            // Move assignment
            matrix &operator=(matrix &&other) noexcept {
                if (this != &other) {
                    deallocate();
                    rows_ = other.rows_;
                    cols_ = other.cols_;
                    data_ = other.data_;
                    other.rows_ = 0;
                    other.cols_ = 0;
                    other.data_ = nullptr;
                }
                return *this;
            }

            // 2D indexing - column-major layout
            reference operator()(size_type row, size_type col) noexcept { return data_[col * rows_ + row]; }
            const_reference operator()(size_type row, size_type col) const noexcept { return data_[col * rows_ + row]; }

            reference at(size_type row, size_type col) {
                if (row >= rows_ || col >= cols_) {
                    throw std::out_of_range("matrix::at");
                }
                return data_[col * rows_ + row];
            }

            const_reference at(size_type row, size_type col) const {
                if (row >= rows_ || col >= cols_) {
                    throw std::out_of_range("matrix::at");
                }
                return data_[col * rows_ + row];
            }

            // 1D indexing (linear access in column-major order)
            reference operator[](size_type i) noexcept { return data_[i]; }
            const_reference operator[](size_type i) const noexcept { return data_[i]; }

            // Raw data access
            pointer data() noexcept { return data_; }
            const_pointer data() const noexcept { return data_; }

            // Dimensions
            size_type rows() const noexcept { return rows_; }
            size_type cols() const noexcept { return cols_; }
            size_type size() const noexcept { return rows_ * cols_; }
            bool empty() const noexcept { return rows_ == 0 || cols_ == 0; }

            // Resize (destructive)
            void resize(size_t new_rows, size_t new_cols) {
                if (new_rows == rows_ && new_cols == cols_)
                    return;

                deallocate();
                rows_ = new_rows;
                cols_ = new_cols;
                size_t total = rows_ * cols_;
                allocate(total);
                for (size_t i = 0; i < total; ++i) {
                    new (&data_[i]) T{};
                }
            }

            void resize(size_t new_rows, size_t new_cols, const T &value) {
                resize(new_rows, new_cols);
                fill(value);
            }

            // Conservative resize (preserves data where possible)
            void conservativeResize(size_t new_rows, size_t new_cols) {
                if (new_rows == rows_ && new_cols == cols_)
                    return;

                size_t new_total = new_rows * new_cols;
                T *new_data = static_cast<T *>(aligned_alloc(32, sizeof(T) * new_total));

                for (size_t i = 0; i < new_total; ++i) {
                    new (&new_data[i]) T{};
                }

                size_t min_rows = rows_ < new_rows ? rows_ : new_rows;
                size_t min_cols = cols_ < new_cols ? cols_ : new_cols;
                for (size_t c = 0; c < min_cols; ++c) {
                    for (size_t r = 0; r < min_rows; ++r) {
                        new_data[c * new_rows + r] = data_[c * rows_ + r];
                    }
                }

                deallocate();
                rows_ = new_rows;
                cols_ = new_cols;
                data_ = new_data;
            }

            // Iterators
            iterator begin() noexcept { return data_; }
            const_iterator begin() const noexcept { return data_; }
            const_iterator cbegin() const noexcept { return data_; }

            iterator end() noexcept { return data_ + rows_ * cols_; }
            const_iterator end() const noexcept { return data_ + rows_ * cols_; }
            const_iterator cend() const noexcept { return data_ + rows_ * cols_; }

            // Operations
            void fill(const T &value) noexcept {
                size_t total = rows_ * cols_;
                for (size_t i = 0; i < total; ++i) {
                    data_[i] = value;
                }
            }

            void swap(matrix &other) noexcept {
                std::swap(rows_, other.rows_);
                std::swap(cols_, other.cols_);
                std::swap(data_, other.data_);
            }

            void setIdentity() {
                if (rows_ != cols_) {
                    throw std::logic_error("setIdentity requires square matrix");
                }
                fill(T{});
                for (size_t i = 0; i < rows_; ++i) {
                    (*this)(i, i) = T{1};
                }
            }

            void setZero() { fill(T{}); }

            // Comparison
            bool operator==(const matrix &other) const noexcept {
                if (rows_ != other.rows_ || cols_ != other.cols_)
                    return false;
                size_t total = rows_ * cols_;
                for (size_t i = 0; i < total; ++i) {
                    if (!(data_[i] == other.data_[i]))
                        return false;
                }
                return true;
            }

            bool operator!=(const matrix &other) const noexcept { return !(*this == other); }
        };

        // Type trait for dynamic matrix
        template <typename T> struct is_dynamic_matrix : std::false_type {};
        template <typename T> struct is_dynamic_matrix<matrix<T, Dynamic, Dynamic, false>> : std::true_type {};
        template <typename T> inline constexpr bool is_dynamic_matrix_v = is_dynamic_matrix<T>::value;

        // Eigen-style aliases for dynamic matrices
        using MatrixXf = matrix<float, Dynamic, Dynamic>;
        using MatrixXd = matrix<double, Dynamic, Dynamic>;
        using MatrixXi = matrix<int, Dynamic, Dynamic>;

    } // namespace mat
} // namespace datapod
