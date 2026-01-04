#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "datapod/pods/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief Binary heap for priority queue operations
     *
     * Heap<T, Compare> is a binary heap that uses Vector internally,
     * enabling full serialization via members().
     *
     * By default, it's a max-heap (largest element at top).
     * Use std::greater<T> for a min-heap.
     *
     * Useful for:
     * - Priority queues
     * - Dijkstra's algorithm
     * - A* pathfinding
     * - Task scheduling
     * - Heap sort
     *
     * @tparam T Value type
     * @tparam Compare Comparison functor (default std::less<T> for max-heap)
     *
     * Time Complexity:
     * - push: O(log n)
     * - pop: O(log n)
     * - top: O(1)
     *
     * Memory Layout:
     * @code
     * data_: [root, left, right, ...]  <- Vector in heap order
     *
     * Tree structure (indices):
     *           0
     *         /   \
     *        1     2
     *       / \   / \
     *      3   4 5   6
     *
     * Parent of i: (i - 1) / 2
     * Left child of i: 2 * i + 1
     * Right child of i: 2 * i + 2
     * @endcode
     */
    template <typename T, typename Compare = std::less<T>> class Heap {
      public:
        using value_type = T;
        using size_type = std::size_t;
        using reference = T &;
        using const_reference = T const &;
        using compare_type = Compare;

        // ====================================================================
        // Construction
        // ====================================================================

        Heap() = default;

        explicit Heap(Compare const &comp) : comp_{comp} {}

        Heap(std::initializer_list<T> init) {
            data_.reserve(init.size());
            for (auto const &val : init) {
                push(val);
            }
        }

        Heap(std::initializer_list<T> init, Compare const &comp) : comp_{comp} {
            data_.reserve(init.size());
            for (auto const &val : init) {
                push(val);
            }
        }

        template <typename InputIt> Heap(InputIt first, InputIt last) {
            for (auto it = first; it != last; ++it) {
                push(*it);
            }
        }

        template <typename InputIt> Heap(InputIt first, InputIt last, Compare const &comp) : comp_{comp} {
            for (auto it = first; it != last; ++it) {
                push(*it);
            }
        }

        Heap(Heap const &other) = default;
        Heap(Heap &&other) noexcept = default;
        Heap &operator=(Heap const &other) = default;
        Heap &operator=(Heap &&other) noexcept = default;

        // ====================================================================
        // Capacity
        // ====================================================================

        bool empty() const noexcept { return data_.empty(); }
        size_type size() const noexcept { return data_.size(); }

        void reserve(size_type new_cap) { data_.reserve(new_cap); }

        // ====================================================================
        // Element Access
        // ====================================================================

        const_reference top() const {
            if (empty()) {
                throw std::out_of_range("Heap::top: heap is empty");
            }
            return data_[0];
        }

        // ====================================================================
        // Modifiers
        // ====================================================================

        void push(T const &value) {
            data_.push_back(value);
            sift_up(data_.size() - 1);
        }

        void push(T &&value) {
            data_.push_back(std::move(value));
            sift_up(data_.size() - 1);
        }

        template <typename... Args> void emplace(Args &&...args) {
            data_.emplace_back(std::forward<Args>(args)...);
            sift_up(data_.size() - 1);
        }

        void pop() {
            if (empty()) {
                throw std::out_of_range("Heap::pop: heap is empty");
            }

            if (data_.size() == 1) {
                data_.pop_back();
                return;
            }

            // Move last element to root and sift down
            data_[0] = std::move(data_.back());
            data_.pop_back();
            sift_down(0);
        }

        T pop_top() {
            if (empty()) {
                throw std::out_of_range("Heap::pop_top: heap is empty");
            }

            T result = std::move(data_[0]);

            if (data_.size() > 1) {
                data_[0] = std::move(data_.back());
                data_.pop_back();
                sift_down(0);
            } else {
                data_.pop_back();
            }

            return result;
        }

        void clear() noexcept { data_.clear(); }

        // ====================================================================
        // Bulk Operations
        // ====================================================================

        // Build heap from unsorted data (O(n) instead of O(n log n))
        static Heap from_unsorted(Vector<T> data, Compare const &comp = Compare{}) {
            Heap heap;
            heap.data_ = std::move(data);
            heap.comp_ = comp;
            heap.heapify();
            return heap;
        }

        // ====================================================================
        // Serialization
        // ====================================================================

        auto members() noexcept { return std::tie(data_); }
        auto members() const noexcept { return std::tie(data_); }

        // After deserialization, call this if the heap property might be violated
        void heapify() {
            if (data_.size() <= 1) {
                return;
            }
            // Start from last non-leaf node and sift down
            for (size_type i = data_.size() / 2; i > 0; --i) {
                sift_down(i - 1);
            }
        }

      private:
        static size_type parent(size_type i) noexcept { return (i - 1) / 2; }
        static size_type left_child(size_type i) noexcept { return 2 * i + 1; }
        static size_type right_child(size_type i) noexcept { return 2 * i + 2; }

        void sift_up(size_type i) {
            while (i > 0) {
                size_type p = parent(i);
                // For max-heap with std::less: if parent < current, swap
                if (comp_(data_[p], data_[i])) {
                    std::swap(data_[p], data_[i]);
                    i = p;
                } else {
                    break;
                }
            }
        }

        void sift_down(size_type i) {
            size_type n = data_.size();
            while (true) {
                size_type largest = i;
                size_type left = left_child(i);
                size_type right = right_child(i);

                // For max-heap with std::less: find largest among i, left, right
                if (left < n && comp_(data_[largest], data_[left])) {
                    largest = left;
                }
                if (right < n && comp_(data_[largest], data_[right])) {
                    largest = right;
                }

                if (largest != i) {
                    std::swap(data_[i], data_[largest]);
                    i = largest;
                } else {
                    break;
                }
            }
        }

        Vector<T> data_;
        [[no_unique_address]] Compare comp_{};
    };

    // Type aliases
    template <typename T> using MaxHeap = Heap<T, std::less<T>>;
    template <typename T> using MinHeap = Heap<T, std::greater<T>>;
    template <typename T> using PriorityQueue = Heap<T>;

    // Comparison operators
    template <typename T, typename Compare> bool operator==(Heap<T, Compare> const &lhs, Heap<T, Compare> const &rhs) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        // Note: This compares the internal representation, not logical equality
        // Two heaps with the same elements might have different internal orders
        Heap<T, Compare> lhs_copy = lhs;
        Heap<T, Compare> rhs_copy = rhs;
        while (!lhs_copy.empty()) {
            if (!(lhs_copy.top() == rhs_copy.top())) {
                return false;
            }
            lhs_copy.pop();
            rhs_copy.pop();
        }
        return true;
    }

    template <typename T, typename Compare> bool operator!=(Heap<T, Compare> const &lhs, Heap<T, Compare> const &rhs) {
        return !(lhs == rhs);
    }

    namespace heap {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace heap

} // namespace datapod
