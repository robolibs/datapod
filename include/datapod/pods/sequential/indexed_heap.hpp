#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "datapod/pods/associative/map.hpp"
#include "datapod/pods/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief Binary heap with decrease-key operation support
     *
     * IndexedHeap<Key, Priority> is a binary heap that maintains a mapping
     * from keys to their positions, enabling O(log n) decrease-key operations.
     *
     * Essential for graph algorithms like Dijkstra's and A*.
     *
     * By default, it's a min-heap (smallest priority at top).
     * Use std::greater<Priority> for a max-heap.
     *
     * @tparam Key Identifier type (must be hashable for Map)
     * @tparam Priority Priority value type
     * @tparam Compare Comparison functor (default std::less<Priority> for min-heap)
     *
     * Time Complexity:
     * - push: O(log n)
     * - pop: O(log n)
     * - top: O(1)
     * - decrease_key: O(log n)
     * - contains: O(1)
     *
     * Memory Layout:
     * @code
     * data_: [(key, priority), ...]  <- Vector in heap order
     * index_: {key -> position}      <- Map for O(1) lookup
     *
     * Tree structure (indices):
     *           0
     *         /   \
     *        1     2
     *       / \   / \
     *      3   4 5   6
     * @endcode
     */
    template <typename Key, typename Priority, typename Compare = std::less<Priority>> class IndexedHeap {
      public:
        struct Entry {
            Key key;
            Priority priority;

            auto members() noexcept { return std::tie(key, priority); }
            auto members() const noexcept { return std::tie(key, priority); }
        };

        using key_type = Key;
        using priority_type = Priority;
        using value_type = Entry;
        using size_type = std::size_t;
        using compare_type = Compare;

        // ====================================================================
        // Construction
        // ====================================================================

        IndexedHeap() = default;

        explicit IndexedHeap(Compare const &comp) : comp_{comp} {}

        IndexedHeap(IndexedHeap const &other) = default;
        IndexedHeap(IndexedHeap &&other) noexcept = default;
        IndexedHeap &operator=(IndexedHeap const &other) = default;
        IndexedHeap &operator=(IndexedHeap &&other) noexcept = default;

        // ====================================================================
        // Capacity
        // ====================================================================

        bool empty() const noexcept { return data_.empty(); }
        size_type size() const noexcept { return data_.size(); }

        void reserve(size_type capacity) { data_.reserve(capacity); }

        // ====================================================================
        // Element access
        // ====================================================================

        /// Get the top element (min or max depending on Compare)
        Entry const &top() const {
            if (empty()) {
                throw std::out_of_range("IndexedHeap::top: heap is empty");
            }
            return data_[0];
        }

        /// Get the priority of a key
        Priority const &priority(Key const &key) const {
            auto it = index_.find(key);
            if (it == index_.end()) {
                throw std::out_of_range("IndexedHeap::priority: key not found");
            }
            return data_[it->second].priority;
        }

        /// Check if key exists in heap
        bool contains(Key const &key) const { return index_.contains(key); }

        // ====================================================================
        // Modifiers
        // ====================================================================

        /// Insert or update a key with given priority
        void push(Key const &key, Priority const &priority) {
            auto it = index_.find(key);
            if (it != index_.end()) {
                // Key exists - update priority
                size_t pos = it->second;
                Priority old_priority = data_[pos].priority;
                data_[pos].priority = priority;

                // Restore heap property
                if (comp_(priority, old_priority)) {
                    sift_up(pos);
                } else {
                    sift_down(pos);
                }
            } else {
                // New key - insert at end and sift up
                size_t pos = data_.size();
                data_.push_back(Entry{key, priority});
                index_[key] = pos;
                sift_up(pos);
            }
        }

        void push(Key &&key, Priority &&priority) {
            auto it = index_.find(key);
            if (it != index_.end()) {
                size_t pos = it->second;
                Priority old_priority = data_[pos].priority;
                data_[pos].priority = std::move(priority);

                if (comp_(data_[pos].priority, old_priority)) {
                    sift_up(pos);
                } else {
                    sift_down(pos);
                }
            } else {
                size_t pos = data_.size();
                Key key_copy = key; // Need copy for index
                data_.push_back(Entry{std::move(key), std::move(priority)});
                index_[std::move(key_copy)] = pos;
                sift_up(pos);
            }
        }

        /// Remove and return the top element
        Entry pop() {
            if (empty()) {
                throw std::out_of_range("IndexedHeap::pop: heap is empty");
            }

            Entry result = std::move(data_[0]);
            index_.erase(result.key);

            if (data_.size() > 1) {
                // Move last element to root
                data_[0] = std::move(data_.back());
                data_.pop_back();
                index_[data_[0].key] = 0;
                sift_down(0);
            } else {
                data_.pop_back();
            }

            return result;
        }

        /// Decrease the priority of a key (for min-heap, this moves it up)
        /// For max-heap with std::greater, this would be "increase_key"
        void decrease_key(Key const &key, Priority const &new_priority) {
            auto it = index_.find(key);
            if (it == index_.end()) {
                throw std::out_of_range("IndexedHeap::decrease_key: key not found");
            }

            size_t pos = it->second;
            if (!comp_(new_priority, data_[pos].priority)) {
                throw std::invalid_argument("IndexedHeap::decrease_key: new priority is not less than current");
            }

            data_[pos].priority = new_priority;
            sift_up(pos);
        }

        /// Update priority of a key (can increase or decrease)
        void update_priority(Key const &key, Priority const &new_priority) {
            auto it = index_.find(key);
            if (it == index_.end()) {
                throw std::out_of_range("IndexedHeap::update_priority: key not found");
            }

            size_t pos = it->second;
            Priority old_priority = data_[pos].priority;
            data_[pos].priority = new_priority;

            if (comp_(new_priority, old_priority)) {
                sift_up(pos);
            } else {
                sift_down(pos);
            }
        }

        /// Remove a specific key from the heap
        bool erase(Key const &key) {
            auto it = index_.find(key);
            if (it == index_.end()) {
                return false;
            }

            size_t pos = it->second;
            index_.erase(key);

            if (pos == data_.size() - 1) {
                // Removing last element
                data_.pop_back();
            } else {
                // Move last element to this position
                data_[pos] = std::move(data_.back());
                data_.pop_back();
                index_[data_[pos].key] = pos;

                // Restore heap property
                if (pos > 0 && comp_(data_[pos].priority, data_[parent(pos)].priority)) {
                    sift_up(pos);
                } else {
                    sift_down(pos);
                }
            }

            return true;
        }

        void clear() {
            data_.clear();
            index_.clear();
        }

        // ====================================================================
        // Iteration (not in heap order, just for inspection)
        // ====================================================================

        auto begin() { return data_.begin(); }
        auto end() { return data_.end(); }
        auto begin() const { return data_.begin(); }
        auto end() const { return data_.end(); }

        // ====================================================================
        // Serialization support
        // ====================================================================

        auto members() noexcept { return std::tie(data_, index_); }
        auto members() const noexcept { return std::tie(data_, index_); }

      private:
        Vector<Entry> data_;
        Map<Key, size_t> index_;
        Compare comp_;

        // ====================================================================
        // Heap helpers
        // ====================================================================

        static size_t parent(size_t i) { return (i - 1) / 2; }
        static size_t left_child(size_t i) { return 2 * i + 1; }
        static size_t right_child(size_t i) { return 2 * i + 2; }

        void swap_entries(size_t i, size_t j) {
            std::swap(data_[i], data_[j]);
            index_[data_[i].key] = i;
            index_[data_[j].key] = j;
        }

        void sift_up(size_t pos) {
            while (pos > 0) {
                size_t p = parent(pos);
                if (comp_(data_[pos].priority, data_[p].priority)) {
                    swap_entries(pos, p);
                    pos = p;
                } else {
                    break;
                }
            }
        }

        void sift_down(size_t pos) {
            size_t n = data_.size();
            while (true) {
                size_t smallest = pos;
                size_t left = left_child(pos);
                size_t right = right_child(pos);

                if (left < n && comp_(data_[left].priority, data_[smallest].priority)) {
                    smallest = left;
                }
                if (right < n && comp_(data_[right].priority, data_[smallest].priority)) {
                    smallest = right;
                }

                if (smallest != pos) {
                    swap_entries(pos, smallest);
                    pos = smallest;
                } else {
                    break;
                }
            }
        }
    };

    // Convenience aliases
    template <typename Key, typename Priority> using MinIndexedHeap = IndexedHeap<Key, Priority, std::less<Priority>>;

    template <typename Key, typename Priority>
    using MaxIndexedHeap = IndexedHeap<Key, Priority, std::greater<Priority>>;

    namespace indexed_heap {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace indexed_heap

} // namespace datapod
