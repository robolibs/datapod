#pragma once
#include <datapod/types/types.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <utility>

#include "datapod/core/strong.hpp"
#include "datapod/pods/memory/allocator.hpp"

namespace datapod {

    // Dynamic array container templated on pointer type
    template <typename T, typename Ptr = T *, typename Alloc = Allocator<T>, typename AccessType = datapod::usize>
    class BasicVector {
      public:
        using value_type = T;
        using allocator_type = Alloc;
        using size_type = datapod::usize;
        using access_type = AccessType;
        using difference_type = datapod::isize;
        using reference = T &;
        using const_reference = T const &;
        using pointer = Ptr;
        using const_pointer = T const *;
        using iterator = T *;
        using const_iterator = T const *;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // Constructors
        BasicVector() noexcept : data_(nullptr), size_(0), capacity_(0) {}

        explicit BasicVector(size_type count) : data_(nullptr), size_(0), capacity_(0) { resize(count); }

        BasicVector(size_type count, T const &value) : data_(nullptr), size_(0), capacity_(0) { resize(count, value); }

        // Copy constructor
        BasicVector(BasicVector const &other) : data_(nullptr), size_(0), capacity_(0) {
            reserve(other.size_);
            for (size_type i = 0; i < other.size_; ++i) {
                push_back(other.data_[i]);
            }
        }

        // Move constructor
        BasicVector(BasicVector &&other) noexcept : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }

        // Range constructor
        template <typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
        BasicVector(InputIt first, InputIt last) : data_(nullptr), size_(0), capacity_(0) {
            size_type count = std::distance(first, last);
            reserve(count);
            for (auto it = first; it != last; ++it) {
                push_back(*it);
            }
        }

        // Initializer list constructor
        BasicVector(std::initializer_list<T> init) : data_(nullptr), size_(0), capacity_(0) {
            reserve(init.size());
            for (auto const &elem : init) {
                push_back(elem);
            }
        }

        // Destructor
        ~BasicVector() {
            clear();
            if (data_ != nullptr) {
                alloc_.deallocate(data_, capacity_);
            }
        }

        // Assignment operators
        BasicVector &operator=(BasicVector const &other) {
            if (this != &other) {
                clear();
                reserve(other.size_);
                for (size_type i = 0; i < other.size_; ++i) {
                    push_back(other.data_[i]);
                }
            }
            return *this;
        }

        BasicVector &operator=(BasicVector &&other) noexcept {
            if (this != &other) {
                clear();
                if (data_ != nullptr) {
                    alloc_.deallocate(data_, capacity_);
                }

                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;

                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            }
            return *this;
        }

        // Initializer list assignment
        BasicVector &operator=(std::initializer_list<T> ilist) {
            assign(ilist);
            return *this;
        }

        // Element access
        reference operator[](size_type pos) noexcept { return data_[pos]; }

        const_reference operator[](size_type pos) const noexcept { return data_[pos]; }

        // Element access with access_type (supports Strong types via to_idx)
        // Only enabled when access_type is different from size_type to avoid ambiguity
        template <typename AT = access_type, typename = std::enable_if_t<!std::is_same_v<AT, size_type>>>
        reference operator[](access_type const &index) noexcept {
            return data_[to_idx(index)];
        }

        template <typename AT = access_type, typename = std::enable_if_t<!std::is_same_v<AT, size_type>>>
        const_reference operator[](access_type const &index) const noexcept {
            return data_[to_idx(index)];
        }

        reference at(size_type pos) {
            if (pos >= size_) {
                throw std::out_of_range("vector::at");
            }
            return data_[pos];
        }

        const_reference at(size_type pos) const {
            if (pos >= size_) {
                throw std::out_of_range("vector::at");
            }
            return data_[pos];
        }

        reference front() noexcept { return data_[0]; }

        const_reference front() const noexcept { return data_[0]; }

        reference back() noexcept { return data_[size_ - 1]; }

        const_reference back() const noexcept { return data_[size_ - 1]; }

        T *data() noexcept { return data_; }

        T const *data() const noexcept { return data_; }

        // Iterators
        iterator begin() noexcept { return data_; }

        const_iterator begin() const noexcept { return data_; }

        const_iterator cbegin() const noexcept { return data_; }

        iterator end() noexcept { return data_ + size_; }

        const_iterator end() const noexcept { return data_ + size_; }

        const_iterator cend() const noexcept { return data_ + size_; }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

        // Capacity
        bool empty() const noexcept { return size_ == 0; }

        size_type size() const noexcept { return size_; }

        size_type max_size() const noexcept { return std::numeric_limits<size_type>::max() / sizeof(T); }

        size_type capacity() const noexcept { return capacity_; }

        void reserve(size_type new_cap) {
            if (new_cap <= capacity_) {
                return;
            }

            T *new_data = alloc_.allocate(new_cap);

            // Move existing elements
            for (size_type i = 0; i < size_; ++i) {
                alloc_.construct(&new_data[i], std::move(data_[i]));
                alloc_.destroy(&data_[i]);
            }

            if (data_ != nullptr) {
                alloc_.deallocate(data_, capacity_);
            }

            data_ = new_data;
            capacity_ = new_cap;
        }

        void shrink_to_fit() {
            if (size_ == capacity_) {
                return;
            }

            if (size_ == 0) {
                if (data_ != nullptr) {
                    alloc_.deallocate(data_, capacity_);
                    data_ = nullptr;
                }
                capacity_ = 0;
                return;
            }

            T *new_data = alloc_.allocate(size_);

            for (size_type i = 0; i < size_; ++i) {
                alloc_.construct(&new_data[i], std::move(data_[i]));
                alloc_.destroy(&data_[i]);
            }

            alloc_.deallocate(data_, capacity_);
            data_ = new_data;
            capacity_ = size_;
        }

        // Modifiers
        void clear() noexcept {
            for (size_type i = 0; i < size_; ++i) {
                alloc_.destroy(&data_[i]);
            }
            size_ = 0;
        }

        void push_back(T const &value) {
            if (size_ == capacity_) {
                reserve(compute_new_capacity(capacity_, size_ + 1));
            }
            alloc_.construct(&data_[size_], value);
            ++size_;
        }

        void push_back(T &&value) {
            if (size_ == capacity_) {
                reserve(compute_new_capacity(capacity_, size_ + 1));
            }
            alloc_.construct(&data_[size_], std::move(value));
            ++size_;
        }

        template <typename... Args> reference emplace_back(Args &&...args) {
            if (size_ == capacity_) {
                reserve(compute_new_capacity(capacity_, size_ + 1));
            }
            alloc_.construct(&data_[size_], std::forward<Args>(args)...);
            ++size_;
            return data_[size_ - 1];
        }

        void pop_back() noexcept {
            if (size_ > 0) {
                --size_;
                alloc_.destroy(&data_[size_]);
            }
        }

        // Bulk append operations (Folly-style)
        template <typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
        void append(InputIt first, InputIt last) {
            size_type count = std::distance(first, last);
            if (count == 0)
                return;

            reserve(size_ + count);
            for (auto it = first; it != last; ++it) {
                alloc_.construct(&data_[size_++], *it);
            }
        }

        void append(std::initializer_list<T> ilist) { append(ilist.begin(), ilist.end()); }

        template <typename Range> void append(Range const &range) { append(range.begin(), range.end()); }

        // Insert single element at position
        iterator insert(const_iterator pos, T const &value) {
            size_type index = pos - begin();
            if (size_ == capacity_) {
                reserve(compute_new_capacity(capacity_, size_ + 1));
            }
            // Shift elements right
            for (size_type i = size_; i > index; --i) {
                alloc_.construct(&data_[i], std::move(data_[i - 1]));
                alloc_.destroy(&data_[i - 1]);
            }
            alloc_.construct(&data_[index], value);
            ++size_;
            return begin() + index;
        }

        // Insert single element (move) at position
        iterator insert(const_iterator pos, T &&value) {
            size_type index = pos - begin();
            if (size_ == capacity_) {
                reserve(compute_new_capacity(capacity_, size_ + 1));
            }
            // Shift elements right
            for (size_type i = size_; i > index; --i) {
                alloc_.construct(&data_[i], std::move(data_[i - 1]));
                alloc_.destroy(&data_[i - 1]);
            }
            alloc_.construct(&data_[index], std::move(value));
            ++size_;
            return begin() + index;
        }

        // Insert count copies at position
        iterator insert(const_iterator pos, size_type count, T const &value) {
            if (count == 0)
                return const_cast<iterator>(pos);
            size_type index = pos - begin();
            if (size_ + count > capacity_) {
                reserve(size_ + count);
            }
            // Shift elements right
            for (size_type i = size_ + count - 1; i >= index + count; --i) {
                alloc_.construct(&data_[i], std::move(data_[i - count]));
                alloc_.destroy(&data_[i - count]);
            }
            // Insert new elements
            for (size_type i = 0; i < count; ++i) {
                alloc_.construct(&data_[index + i], value);
            }
            size_ += count;
            return begin() + index;
        }

        // Insert range [first, last) at position
        // Use enable_if to prevent this from matching integral types
        template <typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
        iterator insert(const_iterator pos, InputIt first, InputIt last) {
            size_type count = std::distance(first, last);
            if (count == 0)
                return const_cast<iterator>(pos);
            size_type index = pos - begin();
            if (size_ + count > capacity_) {
                reserve(size_ + count);
            }
            // Shift elements right
            for (size_type i = size_ + count - 1; i >= index + count && i < size_ + count; --i) {
                alloc_.construct(&data_[i], std::move(data_[i - count]));
                alloc_.destroy(&data_[i - count]);
            }
            // Insert new elements
            size_type i = index;
            for (auto it = first; it != last; ++it, ++i) {
                alloc_.construct(&data_[i], *it);
            }
            size_ += count;
            return begin() + index;
        }

        // Insert initializer list at position
        iterator insert(const_iterator pos, std::initializer_list<T> ilist) {
            return insert(pos, ilist.begin(), ilist.end());
        }

        // Emplace element at position
        template <typename... Args> iterator emplace(const_iterator pos, Args &&...args) {
            size_type index = pos - begin();
            if (size_ == capacity_) {
                reserve(compute_new_capacity(capacity_, size_ + 1));
            }
            // Shift elements right
            for (size_type i = size_; i > index; --i) {
                alloc_.construct(&data_[i], std::move(data_[i - 1]));
                alloc_.destroy(&data_[i - 1]);
            }
            alloc_.construct(&data_[index], std::forward<Args>(args)...);
            ++size_;
            return begin() + index;
        }

        // Erase single element at position
        T *erase(T *pos) {
            auto const r = pos;
            T *last = end() - 1;
            while (pos < last) {
                std::swap(*pos, *(pos + 1));
                pos = pos + 1;
            }
            alloc_.destroy(pos);
            --size_;
            return r;
        }

        // Erase range [first, last)
        T *erase(T *first, T *last) {
            if (first != last) {
                auto const new_end = std::move(last, end(), first);
                for (auto it = new_end; it != end(); ++it) {
                    alloc_.destroy(it);
                }
                size_ -= static_cast<size_type>(std::distance(new_end, end()));
            }
            return end();
        }

        void resize(size_type count) {
            if (count < size_) {
                for (size_type i = count; i < size_; ++i) {
                    alloc_.destroy(&data_[i]);
                }
                size_ = count;
            } else if (count > size_) {
                reserve(count);
                for (size_type i = size_; i < count; ++i) {
                    alloc_.construct(&data_[i]);
                }
                size_ = count;
            }
        }

        void resize(size_type count, T const &value) {
            if (count < size_) {
                for (size_type i = count; i < size_; ++i) {
                    alloc_.destroy(&data_[i]);
                }
                size_ = count;
            } else if (count > size_) {
                reserve(count);
                for (size_type i = size_; i < count; ++i) {
                    alloc_.construct(&data_[i], value);
                }
                size_ = count;
            }
        }

        // Assign count copies of value
        void assign(size_type count, T const &value) {
            clear();
            reserve(count);
            for (size_type i = 0; i < count; ++i) {
                alloc_.construct(&data_[i], value);
            }
            size_ = count;
        }

        // Assign range [first, last)
        // Use enable_if to prevent this from matching integral types
        template <typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
        void assign(InputIt first, InputIt last) {
            clear();
            size_type count = std::distance(first, last);
            reserve(count);
            size_type i = 0;
            for (auto it = first; it != last; ++it, ++i) {
                alloc_.construct(&data_[i], *it);
            }
            size_ = count;
        }

        // Assign from initializer list
        void assign(std::initializer_list<T> ilist) { assign(ilist.begin(), ilist.end()); }

        void swap(BasicVector &other) noexcept {
            std::swap(data_, other.data_);
            std::swap(size_, other.size_);
            std::swap(capacity_, other.capacity_);
        }

        // Serialization support
        auto members() noexcept { return std::tie(data_, size_, capacity_); }

      private:
        // Folly-style 1.5x growth strategy for better memory reuse
        size_type compute_new_capacity(size_type current, size_type needed) const noexcept {
            // Growth factor of 1.5x (current + current/2)
            // Better than 2x for memory reuse and cache efficiency
            size_type grown = current + current / 2;
            return std::max(needed, grown);
        }

        T *data_;
        size_type size_;
        size_type capacity_;
        [[no_unique_address]] Alloc alloc_;
    };

    // Type aliases
    template <typename T> using Vector = BasicVector<T, T *, Allocator<T>>;

    // VectorMap - vector indexed by Key type (supports Strong types)
    template <typename Key, typename Value> using VectorMap = BasicVector<Value, Value *, Allocator<Value>, Key>;

    // Comparison operators
    template <typename T, typename Ptr, typename Alloc>
    bool operator==(BasicVector<T, Ptr, Alloc> const &lhs, BasicVector<T, Ptr, Alloc> const &rhs) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (typename BasicVector<T, Ptr, Alloc>::size_type i = 0; i < lhs.size(); ++i) {
            if (!(lhs[i] == rhs[i])) {
                return false;
            }
        }
        return true;
    }

    template <typename T, typename Ptr, typename Alloc>
    bool operator!=(BasicVector<T, Ptr, Alloc> const &lhs, BasicVector<T, Ptr, Alloc> const &rhs) {
        return !(lhs == rhs);
    }

    template <typename T, typename Ptr, typename Alloc>
    bool operator<(BasicVector<T, Ptr, Alloc> const &lhs, BasicVector<T, Ptr, Alloc> const &rhs) {
        auto min_size = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
        for (typename BasicVector<T, Ptr, Alloc>::size_type i = 0; i < min_size; ++i) {
            if (lhs[i] < rhs[i])
                return true;
            if (rhs[i] < lhs[i])
                return false;
        }
        return lhs.size() < rhs.size();
    }

    template <typename T, typename Ptr, typename Alloc>
    bool operator<=(BasicVector<T, Ptr, Alloc> const &lhs, BasicVector<T, Ptr, Alloc> const &rhs) {
        return !(rhs < lhs);
    }

    template <typename T, typename Ptr, typename Alloc>
    bool operator>(BasicVector<T, Ptr, Alloc> const &lhs, BasicVector<T, Ptr, Alloc> const &rhs) {
        return rhs < lhs;
    }

    template <typename T, typename Ptr, typename Alloc>
    bool operator>=(BasicVector<T, Ptr, Alloc> const &lhs, BasicVector<T, Ptr, Alloc> const &rhs) {
        return !(lhs < rhs);
    }

    namespace seq_vector {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace seq_vector

} // namespace datapod
