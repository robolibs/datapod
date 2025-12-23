#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace datapod {

    // Forward declaration
    template <typename T> struct scalar;

    /**
     * @brief Tensor (rank-1, 1-D only) - fixed-size numeric vector
     *
     * Mathematical tensor of order 1 - represents a vector space element.
     * NOT a container - purely for numeric/mathematical operations.
     *
     * Examples:
     *   tensor<double, 3> position{1.0, 2.0, 3.0};     // ℝ³ vector
     *   tensor<float, 6> state;                         // 6-DOF state
     *   tensor<scalar<double>, 10> features;            // Feature vector with scalars
     *
     * Design:
     * - Fixed size N (no resizing)
     * - Contiguous storage (cache-friendly)
     * - Aligned for SIMD (32-byte alignment)
     * - POD-compatible
     * - Serializable via members()
     * - NO math operations (data layer only)
     * - Bridge to Eigen via data() pointer
     * - Accepts both arithmetic types AND scalar<T>
     */
    template <typename T, size_t N> struct tensor {
        // Accept arithmetic types (and scalar<T>, but we can't check that here due to forward declaration)
        // The scalar<T> case will work because it's a POD type
        static_assert(N > 0, "tensor size must be > 0");

        using value_type = T;
        using size_type = size_t;
        using reference = T &;
        using const_reference = const T &;
        using pointer = T *;
        using const_pointer = const T *;
        using iterator = T *;
        using const_iterator = const T *;

        static constexpr size_t rank = 1;  // Rank-1 tensor
        static constexpr size_t size_ = N; // Dimension

        alignas(32) T data_[N]; // Aligned for SIMD

        // Serialization support
        auto members() noexcept { return std::tie(data_); }
        auto members() const noexcept { return std::tie(data_); }

        // Element access
        constexpr reference operator[](size_type i) noexcept { return data_[i]; }

        constexpr const_reference operator[](size_type i) const noexcept { return data_[i]; }

        constexpr reference at(size_type i) {
            if (i >= N) {
                throw std::out_of_range("tensor::at");
            }
            return data_[i];
        }

        constexpr const_reference at(size_type i) const {
            if (i >= N) {
                throw std::out_of_range("tensor::at");
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

        constexpr void swap(tensor &other) noexcept {
            for (size_type i = 0; i < N; ++i) {
                T tmp = data_[i];
                data_[i] = other.data_[i];
                other.data_[i] = tmp;
            }
        }

        // Comparison
        constexpr bool operator==(const tensor &other) const noexcept {
            for (size_type i = 0; i < N; ++i) {
                if (!(data_[i] == other.data_[i])) {
                    return false;
                }
            }
            return true;
        }

        constexpr bool operator!=(const tensor &other) const noexcept { return !(*this == other); }
    };

    // Deduction guide for aggregate initialization
    template <typename T, typename... U> tensor(T, U...) -> tensor<T, 1 + sizeof...(U)>;

    // Type traits
    template <typename T> struct is_tensor : std::false_type {};
    template <typename T, size_t N> struct is_tensor<tensor<T, N>> : std::true_type {};
    template <typename T> inline constexpr bool is_tensor_v = is_tensor<T>::value;

    // Common tensor type aliases
    template <typename T> using tensor1 = tensor<T, 1>;
    template <typename T> using tensor2 = tensor<T, 2>;
    template <typename T> using tensor3 = tensor<T, 3>;
    template <typename T> using tensor4 = tensor<T, 4>;
    template <typename T> using tensor6 = tensor<T, 6>; // 6-DOF state

    // Common numeric types
    using tensor3f = tensor<float, 3>;
    using tensor3d = tensor<double, 3>;
    using tensor4f = tensor<float, 4>;
    using tensor4d = tensor<double, 4>;
    using tensor6f = tensor<float, 6>;
    using tensor6d = tensor<double, 6>;

} // namespace datapod
