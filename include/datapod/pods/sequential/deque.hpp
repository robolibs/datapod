#pragma once

#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "datapod/pods/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief Double-ended queue with O(1) amortized push/pop at both ends
     *
     * Deque<T> is a double-ended queue that uses two vectors internally,
     * enabling full serialization via members().
     *
     * Implementation uses two vectors:
     * - front_: stores elements in reverse order (front of deque = back of vector)
     * - back_: stores elements in normal order (back of deque = back of vector)
     *
     * Useful for:
     * - BFS (breadth-first search)
     * - Sliding window algorithms
     * - Work-stealing queues
     * - Any case where O(1) operations at both ends are needed
     *
     * @tparam T Value type
     *
     * Time Complexity:
     * - push_front, push_back: O(1) amortized
     * - pop_front, pop_back: O(1) amortized
     * - front, back: O(1)
     * - operator[]: O(1)
     *
     * Memory Layout:
     * @code
     * front_: [e2, e1, e0]  <- reversed, front of deque is back of vector
     * back_:  [e3, e4, e5]  <- normal order
     *
     * Logical order: e0, e1, e2, e3, e4, e5
     * @endcode
     */
    template <typename T> class Deque {
      public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = T &;
        using const_reference = T const &;
        using pointer = T *;
        using const_pointer = T const *;

        // ====================================================================
        // Iterator
        // ====================================================================

        class iterator {
          public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T *;
            using reference = T &;

            iterator() : deque_{nullptr}, index_{0} {}
            iterator(Deque *deque, size_type index) : deque_{deque}, index_{index} {}

            reference operator*() { return (*deque_)[index_]; }
            pointer operator->() { return &(*deque_)[index_]; }
            reference operator[](difference_type n) { return (*deque_)[index_ + n]; }

            iterator &operator++() {
                ++index_;
                return *this;
            }
            iterator operator++(int) {
                iterator tmp = *this;
                ++index_;
                return tmp;
            }
            iterator &operator--() {
                --index_;
                return *this;
            }
            iterator operator--(int) {
                iterator tmp = *this;
                --index_;
                return tmp;
            }

            iterator &operator+=(difference_type n) {
                index_ += n;
                return *this;
            }
            iterator &operator-=(difference_type n) {
                index_ -= n;
                return *this;
            }

            iterator operator+(difference_type n) const { return iterator(deque_, index_ + n); }
            iterator operator-(difference_type n) const { return iterator(deque_, index_ - n); }
            difference_type operator-(iterator const &other) const { return index_ - other.index_; }

            bool operator==(iterator const &other) const { return index_ == other.index_; }
            bool operator!=(iterator const &other) const { return index_ != other.index_; }
            bool operator<(iterator const &other) const { return index_ < other.index_; }
            bool operator<=(iterator const &other) const { return index_ <= other.index_; }
            bool operator>(iterator const &other) const { return index_ > other.index_; }
            bool operator>=(iterator const &other) const { return index_ >= other.index_; }

          private:
            Deque *deque_;
            size_type index_;
        };

        class const_iterator {
          public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T const *;
            using reference = T const &;

            const_iterator() : deque_{nullptr}, index_{0} {}
            const_iterator(Deque const *deque, size_type index) : deque_{deque}, index_{index} {}
            const_iterator(iterator it) : deque_{it.deque_}, index_{it.index_} {}

            reference operator*() const { return (*deque_)[index_]; }
            pointer operator->() const { return &(*deque_)[index_]; }
            reference operator[](difference_type n) const { return (*deque_)[index_ + n]; }

            const_iterator &operator++() {
                ++index_;
                return *this;
            }
            const_iterator operator++(int) {
                const_iterator tmp = *this;
                ++index_;
                return tmp;
            }
            const_iterator &operator--() {
                --index_;
                return *this;
            }
            const_iterator operator--(int) {
                const_iterator tmp = *this;
                --index_;
                return tmp;
            }

            const_iterator &operator+=(difference_type n) {
                index_ += n;
                return *this;
            }
            const_iterator &operator-=(difference_type n) {
                index_ -= n;
                return *this;
            }

            const_iterator operator+(difference_type n) const { return const_iterator(deque_, index_ + n); }
            const_iterator operator-(difference_type n) const { return const_iterator(deque_, index_ - n); }
            difference_type operator-(const_iterator const &other) const { return index_ - other.index_; }

            bool operator==(const_iterator const &other) const { return index_ == other.index_; }
            bool operator!=(const_iterator const &other) const { return index_ != other.index_; }
            bool operator<(const_iterator const &other) const { return index_ < other.index_; }
            bool operator<=(const_iterator const &other) const { return index_ <= other.index_; }
            bool operator>(const_iterator const &other) const { return index_ > other.index_; }
            bool operator>=(const_iterator const &other) const { return index_ >= other.index_; }

          private:
            Deque const *deque_;
            size_type index_;
        };

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // ====================================================================
        // Construction
        // ====================================================================

        Deque() = default;

        explicit Deque(size_type count) { back_.resize(count); }

        Deque(size_type count, T const &value) { back_.resize(count, value); }

        Deque(std::initializer_list<T> init) {
            back_.reserve(init.size());
            for (auto const &val : init) {
                back_.push_back(val);
            }
        }

        template <typename InputIt, typename = std::enable_if_t<!std::is_integral_v<InputIt>>>
        Deque(InputIt first, InputIt last) {
            for (auto it = first; it != last; ++it) {
                back_.push_back(*it);
            }
        }

        Deque(Deque const &other) = default;
        Deque(Deque &&other) noexcept = default;
        Deque &operator=(Deque const &other) = default;
        Deque &operator=(Deque &&other) noexcept = default;

        // ====================================================================
        // Capacity
        // ====================================================================

        bool empty() const noexcept { return front_.empty() && back_.empty(); }
        size_type size() const noexcept { return front_.size() + back_.size(); }

        void reserve(size_type new_cap) {
            // Reserve space in back_ for simplicity
            back_.reserve(new_cap);
        }

        void shrink_to_fit() {
            front_.shrink_to_fit();
            back_.shrink_to_fit();
        }

        // ====================================================================
        // Element Access
        // ====================================================================

        reference operator[](size_type pos) {
            if (pos < front_.size()) {
                return front_[front_.size() - 1 - pos];
            }
            return back_[pos - front_.size()];
        }

        const_reference operator[](size_type pos) const {
            if (pos < front_.size()) {
                return front_[front_.size() - 1 - pos];
            }
            return back_[pos - front_.size()];
        }

        reference at(size_type pos) {
            if (pos >= size()) {
                throw std::out_of_range("Deque::at: index out of range");
            }
            return (*this)[pos];
        }

        const_reference at(size_type pos) const {
            if (pos >= size()) {
                throw std::out_of_range("Deque::at: index out of range");
            }
            return (*this)[pos];
        }

        reference front() {
            if (empty()) {
                throw std::out_of_range("Deque::front: deque is empty");
            }
            if (!front_.empty()) {
                return front_.back();
            }
            return back_.front();
        }

        const_reference front() const {
            if (empty()) {
                throw std::out_of_range("Deque::front: deque is empty");
            }
            if (!front_.empty()) {
                return front_.back();
            }
            return back_.front();
        }

        reference back() {
            if (empty()) {
                throw std::out_of_range("Deque::back: deque is empty");
            }
            if (!back_.empty()) {
                return back_.back();
            }
            return front_.front();
        }

        const_reference back() const {
            if (empty()) {
                throw std::out_of_range("Deque::back: deque is empty");
            }
            if (!back_.empty()) {
                return back_.back();
            }
            return front_.front();
        }

        // ====================================================================
        // Modifiers
        // ====================================================================

        void push_front(T const &value) { front_.push_back(value); }

        void push_front(T &&value) { front_.push_back(std::move(value)); }

        void push_back(T const &value) { back_.push_back(value); }

        void push_back(T &&value) { back_.push_back(std::move(value)); }

        template <typename... Args> reference emplace_front(Args &&...args) {
            front_.emplace_back(std::forward<Args>(args)...);
            return front_.back();
        }

        template <typename... Args> reference emplace_back(Args &&...args) {
            back_.emplace_back(std::forward<Args>(args)...);
            return back_.back();
        }

        void pop_front() {
            if (empty()) {
                throw std::out_of_range("Deque::pop_front: deque is empty");
            }
            if (!front_.empty()) {
                front_.pop_back();
            } else {
                // Move half of back_ to front_ (reversed)
                rebalance_to_front();
                if (!front_.empty()) {
                    front_.pop_back();
                }
            }
        }

        void pop_back() {
            if (empty()) {
                throw std::out_of_range("Deque::pop_back: deque is empty");
            }
            if (!back_.empty()) {
                back_.pop_back();
            } else {
                // Move half of front_ to back_
                rebalance_to_back();
                if (!back_.empty()) {
                    back_.pop_back();
                }
            }
        }

        void clear() noexcept {
            front_.clear();
            back_.clear();
        }

        void resize(size_type count) {
            if (count < size()) {
                // Shrink
                while (size() > count) {
                    pop_back();
                }
            } else if (count > size()) {
                // Grow
                back_.resize(back_.size() + (count - size()));
            }
        }

        void resize(size_type count, T const &value) {
            if (count < size()) {
                while (size() > count) {
                    pop_back();
                }
            } else if (count > size()) {
                back_.resize(back_.size() + (count - size()), value);
            }
        }

        // ====================================================================
        // Iterators
        // ====================================================================

        iterator begin() noexcept { return iterator(this, 0); }
        const_iterator begin() const noexcept { return const_iterator(this, 0); }
        const_iterator cbegin() const noexcept { return const_iterator(this, 0); }

        iterator end() noexcept { return iterator(this, size()); }
        const_iterator end() const noexcept { return const_iterator(this, size()); }
        const_iterator cend() const noexcept { return const_iterator(this, size()); }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

        // ====================================================================
        // Serialization
        // ====================================================================

        auto members() noexcept { return std::tie(front_, back_); }
        auto members() const noexcept { return std::tie(front_, back_); }

      private:
        void rebalance_to_front() {
            // Move first half of back_ to front_ (reversed)
            size_type half = back_.size() / 2;
            if (half == 0) {
                half = back_.size();
            }

            front_.reserve(half);
            for (size_type i = 0; i < half; ++i) {
                front_.push_back(std::move(back_[half - 1 - i]));
            }

            // Shift remaining elements in back_
            Vector<T> new_back;
            new_back.reserve(back_.size() - half);
            for (size_type i = half; i < back_.size(); ++i) {
                new_back.push_back(std::move(back_[i]));
            }
            back_ = std::move(new_back);
        }

        void rebalance_to_back() {
            // Move first half of front_ to back_
            size_type half = front_.size() / 2;
            if (half == 0) {
                half = front_.size();
            }

            back_.reserve(half);
            for (size_type i = 0; i < half; ++i) {
                back_.push_back(std::move(front_[half - 1 - i]));
            }

            // Shift remaining elements in front_
            Vector<T> new_front;
            new_front.reserve(front_.size() - half);
            for (size_type i = half; i < front_.size(); ++i) {
                new_front.push_back(std::move(front_[i]));
            }
            front_ = std::move(new_front);
        }

        Vector<T> front_; // Reversed: front of deque = back of vector
        Vector<T> back_;  // Normal: back of deque = back of vector
    };

    // Comparison operators
    template <typename T> bool operator==(Deque<T> const &lhs, Deque<T> const &rhs) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (size_t i = 0; i < lhs.size(); ++i) {
            if (!(lhs[i] == rhs[i])) {
                return false;
            }
        }
        return true;
    }

    template <typename T> bool operator!=(Deque<T> const &lhs, Deque<T> const &rhs) { return !(lhs == rhs); }

    template <typename T> bool operator<(Deque<T> const &lhs, Deque<T> const &rhs) {
        size_t min_size = lhs.size() < rhs.size() ? lhs.size() : rhs.size();
        for (size_t i = 0; i < min_size; ++i) {
            if (lhs[i] < rhs[i])
                return true;
            if (rhs[i] < lhs[i])
                return false;
        }
        return lhs.size() < rhs.size();
    }

    template <typename T> bool operator<=(Deque<T> const &lhs, Deque<T> const &rhs) { return !(rhs < lhs); }

    template <typename T> bool operator>(Deque<T> const &lhs, Deque<T> const &rhs) { return rhs < lhs; }

    template <typename T> bool operator>=(Deque<T> const &lhs, Deque<T> const &rhs) { return !(lhs < rhs); }

} // namespace datapod
