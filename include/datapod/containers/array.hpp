#pragma once

#include <cstddef>
#include <stdexcept>
#include <tuple>

namespace datapod {

    // Fixed-size array (similar to std::array)
    template <typename T, std::size_t N> struct Array {
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T &;
        using const_reference = T const &;
        using pointer = T *;
        using const_pointer = T const *;
        using iterator = T *;
        using const_iterator = T const *;

        T data_[N];

        // Element access
        constexpr reference operator[](size_type pos) noexcept { return data_[pos]; }

        constexpr const_reference operator[](size_type pos) const noexcept { return data_[pos]; }

        constexpr reference at(size_type pos) {
            if (pos >= N) {
                throw std::out_of_range("array::at");
            }
            return data_[pos];
        }

        constexpr const_reference at(size_type pos) const {
            if (pos >= N) {
                throw std::out_of_range("array::at");
            }
            return data_[pos];
        }

        constexpr reference front() noexcept { return data_[0]; }

        constexpr const_reference front() const noexcept { return data_[0]; }

        constexpr reference back() noexcept { return data_[N - 1]; }

        constexpr const_reference back() const noexcept { return data_[N - 1]; }

        constexpr pointer data() noexcept { return data_; }

        constexpr const_pointer data() const noexcept { return data_; }

        // Iterators
        constexpr iterator begin() noexcept { return data_; }

        constexpr const_iterator begin() const noexcept { return data_; }

        constexpr const_iterator cbegin() const noexcept { return data_; }

        constexpr iterator end() noexcept { return data_ + N; }

        constexpr const_iterator end() const noexcept { return data_ + N; }

        constexpr const_iterator cend() const noexcept { return data_ + N; }

        // Capacity
        constexpr bool empty() const noexcept { return N == 0; }

        constexpr size_type size() const noexcept { return N; }

        constexpr size_type max_size() const noexcept { return N; }

        // Operations
        constexpr void fill(T const &value) {
            for (size_type i = 0; i < N; ++i) {
                data_[i] = value;
            }
        }

        constexpr void swap(Array &other) noexcept {
            for (size_type i = 0; i < N; ++i) {
                T tmp = data_[i];
                data_[i] = other.data_[i];
                other.data_[i] = tmp;
            }
        }

        // Serialization support
        auto members() noexcept { return std::tie(data_); }
    };

    // Specialization for zero-size array
    template <typename T> struct Array<T, 0> {
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T &;
        using const_reference = T const &;
        using pointer = T *;
        using const_pointer = T const *;
        using iterator = T *;
        using const_iterator = T const *;

        constexpr pointer data() noexcept { return nullptr; }
        constexpr const_pointer data() const noexcept { return nullptr; }

        constexpr iterator begin() noexcept { return nullptr; }
        constexpr const_iterator begin() const noexcept { return nullptr; }
        constexpr const_iterator cbegin() const noexcept { return nullptr; }

        constexpr iterator end() noexcept { return nullptr; }
        constexpr const_iterator end() const noexcept { return nullptr; }
        constexpr const_iterator cend() const noexcept { return nullptr; }

        constexpr bool empty() const noexcept { return true; }
        constexpr size_type size() const noexcept { return 0; }
        constexpr size_type max_size() const noexcept { return 0; }

        constexpr void fill(T const &) {}
        constexpr void swap(Array &) noexcept {}
    };

    // Deduction guide
    template <typename T, typename... U> Array(T, U...) -> Array<T, 1 + sizeof...(U)>;

    // Comparison operators
    template <typename T, std::size_t N> constexpr bool operator==(Array<T, N> const &lhs, Array<T, N> const &rhs) {
        for (std::size_t i = 0; i < N; ++i) {
            if (!(lhs[i] == rhs[i])) {
                return false;
            }
        }
        return true;
    }

    template <typename T, std::size_t N> constexpr bool operator!=(Array<T, N> const &lhs, Array<T, N> const &rhs) {
        return !(lhs == rhs);
    }

    template <typename T, std::size_t N> constexpr bool operator<(Array<T, N> const &lhs, Array<T, N> const &rhs) {
        for (std::size_t i = 0; i < N; ++i) {
            if (lhs[i] < rhs[i]) {
                return true;
            }
            if (rhs[i] < lhs[i]) {
                return false;
            }
        }
        return false;
    }

    template <typename T, std::size_t N> constexpr bool operator<=(Array<T, N> const &lhs, Array<T, N> const &rhs) {
        return !(rhs < lhs);
    }

    template <typename T, std::size_t N> constexpr bool operator>(Array<T, N> const &lhs, Array<T, N> const &rhs) {
        return rhs < lhs;
    }

    template <typename T, std::size_t N> constexpr bool operator>=(Array<T, N> const &lhs, Array<T, N> const &rhs) {
        return !(lhs < rhs);
    }

} // namespace datapod
