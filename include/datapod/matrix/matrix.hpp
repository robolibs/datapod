#pragma once

#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace datapod {

    // Forward declaration
    template <typename T> struct scalar;

    /**
     * @brief Matrix (rank-2, 2-D only) - fixed-size numeric matrix
     *
     * Mathematical tensor of order 2 - represents a linear operator.
     * NOT a container - purely for numeric/mathematical operations.
     *
     * Examples:
     *   matrix<double, 3, 3> rotation;              // SO(3) rotation matrix
     *   matrix<double, 6, 6> covariance;            // 6x6 covariance matrix
     *   matrix<float, 4, 4> transform;              // SE(3) transformation
     *   matrix<scalar<double>, 3, 3> tagged_mat;    // Matrix of scalars
     *
     * Design:
     * - Fixed shape R×C (no resizing)
     * - Column-major storage (matching Eigen, BLAS, LAPACK)
     * - Contiguous storage (cache-friendly)
     * - Aligned for SIMD (32-byte alignment)
     * - POD-compatible
     * - Serializable via members()
     * - NO math operations (data layer only)
     * - Bridge to Eigen via data() pointer
     * - Accepts both arithmetic types AND scalar<T>
     */
    template <typename T, size_t R, size_t C> struct matrix {
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

        alignas(32) T data_[R * C]; // Column-major: data_[col * R + row]

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

    // Type traits
    template <typename T> struct is_matrix : std::false_type {};
    template <typename T, size_t R, size_t C> struct is_matrix<matrix<T, R, C>> : std::true_type {};
    template <typename T> inline constexpr bool is_matrix_v = is_matrix<T>::value;

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

} // namespace datapod
