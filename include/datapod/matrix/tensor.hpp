#pragma once

#include <array>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace datapod {
    namespace mat {

        // Forward declaration
        template <typename T> struct scalar;

        /**
         * @brief Tensor (rank-N, N-dimensional) - fixed-size N-dimensional numeric array
         *
         * Mathematical tensor of arbitrary order (3D and higher).
         * NOT a container - purely for numeric/mathematical operations.
         *
         * Examples:
         *   tensor<double, 3, 3, 3> cube;                  // 3x3x3 cube
         *   tensor<float, 2, 3, 4> volume;                 // 2x3x4 volume
         *   tensor<double, 2, 2, 2, 2> rank4;              // Rank-4 tensor
         *   tensor<scalar<double>, 4, 4, 4> tagged;        // Tensor of scalars
         *
         * Design:
         * - Fixed shape (no resizing)
         * - Column-major storage (matching Eigen, BLAS, LAPACK, matrix.hpp)
         * - Contiguous storage (cache-friendly)
         * - Aligned for SIMD (32-byte alignment)
         * - POD-compatible
         * - Serializable via members()
         * - NO math operations (data layer only)
         * - Accepts both arithmetic types AND scalar<T>
         */
        template <typename T, size_t... Dims> struct tensor {
            static_assert(sizeof...(Dims) >= 3,
                          "tensor requires at least 3 dimensions (use vector for 1D, matrix for 2D)");
            static_assert(((Dims > 0) && ...), "all tensor dimensions must be > 0");

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

            alignas(32) T data_[size_]; // Column-major storage

            // Default constructor (for aggregate initialization)
            constexpr tensor() noexcept = default;

            // Composition constructor: construct tensor from slices along last dimension
            // For tensor<T, D0, D1, ..., Dn>, accepts Dn slices of shape D0 x D1 x ... x D(n-1)
            template <typename... SliceTypes, typename = std::enable_if_t<(sizeof...(SliceTypes) == dims_[rank - 1] &&
                                                                           sizeof...(SliceTypes) > 0)>>
            constexpr tensor(const SliceTypes &...slices) noexcept : data_{} {
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
                        throw std::out_of_range("tensor::at");
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
                        throw std::out_of_range("tensor::at");
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

            constexpr void swap(tensor &other) noexcept {
                for (size_type i = 0; i < size_; ++i) {
                    T tmp = data_[i];
                    data_[i] = other.data_[i];
                    other.data_[i] = tmp;
                }
            }

            // Comparison
            constexpr bool operator==(const tensor &other) const noexcept {
                for (size_type i = 0; i < size_; ++i) {
                    if (!(data_[i] == other.data_[i])) {
                        return false;
                    }
                }
                return true;
            }

            constexpr bool operator!=(const tensor &other) const noexcept { return !(*this == other); }
        };

        // Type traits
        template <typename T> struct is_tensor : std::false_type {};
        template <typename T, size_t... Dims> struct is_tensor<tensor<T, Dims...>> : std::true_type {};
        template <typename T> inline constexpr bool is_tensor_v = is_tensor<T>::value;

        // Common tensor type aliases (3D)
        template <typename T> using tensor3d_2x2x2 = tensor<T, 2, 2, 2>;
        template <typename T> using tensor3d_3x3x3 = tensor<T, 3, 3, 3>;
        template <typename T> using tensor3d_4x4x4 = tensor<T, 4, 4, 4>;

        // Common numeric types (3D cubes)
        using tensor3d_2x2x2f = tensor<float, 2, 2, 2>;
        using tensor3d_2x2x2d = tensor<double, 2, 2, 2>;
        using tensor3d_3x3x3f = tensor<float, 3, 3, 3>;
        using tensor3d_3x3x3d = tensor<double, 3, 3, 3>;
        using tensor3d_4x4x4f = tensor<float, 4, 4, 4>;
        using tensor3d_4x4x4d = tensor<double, 4, 4, 4>;

    } // namespace mat
} // namespace datapod
