#pragma once
#include <datapod/types/types.hpp>

#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "datapod/pods/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief Doubly linked list with O(1) insert/remove anywhere
     *
     * List<T> is a doubly linked list that uses index-based nodes
     * instead of pointers, enabling full serialization via members().
     *
     * Useful for:
     * - LRU caches (move recently used to front)
     * - Ordered collections with frequent insertions/deletions
     * - Any case where O(1) insert/remove at arbitrary positions is needed
     *
     * @tparam T Value type
     *
     * Memory Layout:
     * @code
     * nodes_:     [Node0, Node1, Node2, ...]  <- Vector of nodes
     * head_:      index of first node (INVALID_INDEX if empty)
     * tail_:      index of last node (INVALID_INDEX if empty)
     * size_:      number of elements
     * free_list_: [idx1, idx2, ...]  <- recycled node indices
     * @endcode
     */
    template <typename T> class List {
      public:
        static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

        struct Node {
            T value;
            size_t prev;
            size_t next;

            Node() : value{}, prev{INVALID_INDEX}, next{INVALID_INDEX} {}
            Node(T const &v, size_t p, size_t n) : value{v}, prev{p}, next{n} {}
            Node(T &&v, size_t p, size_t n) : value{std::move(v)}, prev{p}, next{n} {}

            auto members() noexcept { return std::tie(value, prev, next); }
            auto members() const noexcept { return std::tie(value, prev, next); }
        };

        using value_type = T;
        using size_type = datapod::usize;
        using reference = T &;
        using const_reference = T const &;

        // ====================================================================
        // Forward Iterator
        // ====================================================================

        class iterator {
          public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = T;
            using difference_type = datapod::isize;
            using pointer = T *;
            using reference = T &;

            iterator() : list_{nullptr}, index_{INVALID_INDEX} {}
            iterator(List *list, size_t index) : list_{list}, index_{index} {}

            reference operator*() { return list_->nodes_[index_].value; }
            pointer operator->() { return &list_->nodes_[index_].value; }

            iterator &operator++() {
                index_ = list_->nodes_[index_].next;
                return *this;
            }

            iterator operator++(int) {
                iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            iterator &operator--() {
                if (index_ == INVALID_INDEX) {
                    index_ = list_->tail_;
                } else {
                    index_ = list_->nodes_[index_].prev;
                }
                return *this;
            }

            iterator operator--(int) {
                iterator tmp = *this;
                --(*this);
                return tmp;
            }

            bool operator==(iterator const &other) const { return index_ == other.index_; }
            bool operator!=(iterator const &other) const { return index_ != other.index_; }

            size_t index() const { return index_; }

          private:
            friend class List;
            List *list_;
            size_t index_;
        };

        class const_iterator {
          public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = T;
            using difference_type = datapod::isize;
            using pointer = T const *;
            using reference = T const &;

            const_iterator() : list_{nullptr}, index_{INVALID_INDEX} {}
            const_iterator(List const *list, size_t index) : list_{list}, index_{index} {}
            const_iterator(iterator it) : list_{it.list_}, index_{it.index_} {}

            reference operator*() const { return list_->nodes_[index_].value; }
            pointer operator->() const { return &list_->nodes_[index_].value; }

            const_iterator &operator++() {
                index_ = list_->nodes_[index_].next;
                return *this;
            }

            const_iterator operator++(int) {
                const_iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            const_iterator &operator--() {
                if (index_ == INVALID_INDEX) {
                    index_ = list_->tail_;
                } else {
                    index_ = list_->nodes_[index_].prev;
                }
                return *this;
            }

            const_iterator operator--(int) {
                const_iterator tmp = *this;
                --(*this);
                return tmp;
            }

            bool operator==(const_iterator const &other) const { return index_ == other.index_; }
            bool operator!=(const_iterator const &other) const { return index_ != other.index_; }

            size_t index() const { return index_; }

          private:
            List const *list_;
            size_t index_;
        };

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // ====================================================================
        // Construction
        // ====================================================================

        List() : head_{INVALID_INDEX}, tail_{INVALID_INDEX}, size_{0} {}

        List(std::initializer_list<T> init) : head_{INVALID_INDEX}, tail_{INVALID_INDEX}, size_{0} {
            for (auto const &val : init) {
                push_back(val);
            }
        }

        List(List const &other)
            : nodes_{other.nodes_}, head_{other.head_}, tail_{other.tail_}, size_{other.size_},
              free_list_{other.free_list_} {}

        List(List &&other) noexcept
            : nodes_{std::move(other.nodes_)}, head_{other.head_}, tail_{other.tail_}, size_{other.size_},
              free_list_{std::move(other.free_list_)} {
            other.head_ = INVALID_INDEX;
            other.tail_ = INVALID_INDEX;
            other.size_ = 0;
        }

        List &operator=(List const &other) {
            if (this != &other) {
                nodes_ = other.nodes_;
                head_ = other.head_;
                tail_ = other.tail_;
                size_ = other.size_;
                free_list_ = other.free_list_;
            }
            return *this;
        }

        List &operator=(List &&other) noexcept {
            if (this != &other) {
                nodes_ = std::move(other.nodes_);
                head_ = other.head_;
                tail_ = other.tail_;
                size_ = other.size_;
                free_list_ = std::move(other.free_list_);
                other.head_ = INVALID_INDEX;
                other.tail_ = INVALID_INDEX;
                other.size_ = 0;
            }
            return *this;
        }

        // ====================================================================
        // Capacity
        // ====================================================================

        bool empty() const noexcept { return size_ == 0; }
        size_type size() const noexcept { return size_; }

        // ====================================================================
        // Element Access
        // ====================================================================

        reference front() {
            if (empty()) {
                throw std::out_of_range("List::front: list is empty");
            }
            return nodes_[head_].value;
        }

        const_reference front() const {
            if (empty()) {
                throw std::out_of_range("List::front: list is empty");
            }
            return nodes_[head_].value;
        }

        reference back() {
            if (empty()) {
                throw std::out_of_range("List::back: list is empty");
            }
            return nodes_[tail_].value;
        }

        const_reference back() const {
            if (empty()) {
                throw std::out_of_range("List::back: list is empty");
            }
            return nodes_[tail_].value;
        }

        // ====================================================================
        // Modifiers
        // ====================================================================

        void push_front(T const &value) {
            size_t new_index = allocate_node(value, INVALID_INDEX, head_);

            if (head_ != INVALID_INDEX) {
                nodes_[head_].prev = new_index;
            }
            head_ = new_index;

            if (tail_ == INVALID_INDEX) {
                tail_ = new_index;
            }
            ++size_;
        }

        void push_front(T &&value) {
            size_t new_index = allocate_node(std::move(value), INVALID_INDEX, head_);

            if (head_ != INVALID_INDEX) {
                nodes_[head_].prev = new_index;
            }
            head_ = new_index;

            if (tail_ == INVALID_INDEX) {
                tail_ = new_index;
            }
            ++size_;
        }

        void push_back(T const &value) {
            size_t new_index = allocate_node(value, tail_, INVALID_INDEX);

            if (tail_ != INVALID_INDEX) {
                nodes_[tail_].next = new_index;
            }
            tail_ = new_index;

            if (head_ == INVALID_INDEX) {
                head_ = new_index;
            }
            ++size_;
        }

        void push_back(T &&value) {
            size_t new_index = allocate_node(std::move(value), tail_, INVALID_INDEX);

            if (tail_ != INVALID_INDEX) {
                nodes_[tail_].next = new_index;
            }
            tail_ = new_index;

            if (head_ == INVALID_INDEX) {
                head_ = new_index;
            }
            ++size_;
        }

        template <typename... Args> reference emplace_front(Args &&...args) {
            size_t new_index = allocate_node_emplace(INVALID_INDEX, head_, std::forward<Args>(args)...);

            if (head_ != INVALID_INDEX) {
                nodes_[head_].prev = new_index;
            }
            head_ = new_index;

            if (tail_ == INVALID_INDEX) {
                tail_ = new_index;
            }
            ++size_;
            return nodes_[new_index].value;
        }

        template <typename... Args> reference emplace_back(Args &&...args) {
            size_t new_index = allocate_node_emplace(tail_, INVALID_INDEX, std::forward<Args>(args)...);

            if (tail_ != INVALID_INDEX) {
                nodes_[tail_].next = new_index;
            }
            tail_ = new_index;

            if (head_ == INVALID_INDEX) {
                head_ = new_index;
            }
            ++size_;
            return nodes_[new_index].value;
        }

        void pop_front() {
            if (empty()) {
                throw std::out_of_range("List::pop_front: list is empty");
            }

            size_t old_head = head_;
            head_ = nodes_[head_].next;

            if (head_ != INVALID_INDEX) {
                nodes_[head_].prev = INVALID_INDEX;
            } else {
                tail_ = INVALID_INDEX;
            }

            deallocate_node(old_head);
            --size_;
        }

        void pop_back() {
            if (empty()) {
                throw std::out_of_range("List::pop_back: list is empty");
            }

            size_t old_tail = tail_;
            tail_ = nodes_[tail_].prev;

            if (tail_ != INVALID_INDEX) {
                nodes_[tail_].next = INVALID_INDEX;
            } else {
                head_ = INVALID_INDEX;
            }

            deallocate_node(old_tail);
            --size_;
        }

        iterator insert(iterator pos, T const &value) {
            if (pos.index_ == INVALID_INDEX) {
                // Insert at end
                push_back(value);
                return iterator(this, tail_);
            }

            if (pos.index_ == head_) {
                // Insert at beginning
                push_front(value);
                return iterator(this, head_);
            }

            size_t pos_index = pos.index_;
            size_t prev_index = nodes_[pos_index].prev;
            size_t new_index = allocate_node(value, prev_index, pos_index);

            nodes_[prev_index].next = new_index;
            nodes_[pos_index].prev = new_index;
            ++size_;

            return iterator(this, new_index);
        }

        iterator insert(iterator pos, T &&value) {
            if (pos.index_ == INVALID_INDEX) {
                push_back(std::move(value));
                return iterator(this, tail_);
            }

            if (pos.index_ == head_) {
                push_front(std::move(value));
                return iterator(this, head_);
            }

            size_t pos_index = pos.index_;
            size_t prev_index = nodes_[pos_index].prev;
            size_t new_index = allocate_node(std::move(value), prev_index, pos_index);

            nodes_[prev_index].next = new_index;
            nodes_[pos_index].prev = new_index;
            ++size_;

            return iterator(this, new_index);
        }

        iterator erase(iterator pos) {
            if (pos.index_ == INVALID_INDEX) {
                throw std::out_of_range("List::erase: invalid iterator");
            }

            size_t pos_index = pos.index_;
            size_t prev_index = nodes_[pos_index].prev;
            size_t next_index = nodes_[pos_index].next;

            if (prev_index != INVALID_INDEX) {
                nodes_[prev_index].next = next_index;
            } else {
                head_ = next_index;
            }

            if (next_index != INVALID_INDEX) {
                nodes_[next_index].prev = prev_index;
            } else {
                tail_ = prev_index;
            }

            deallocate_node(pos_index);
            --size_;

            return iterator(this, next_index);
        }

        void clear() noexcept {
            nodes_.clear();
            free_list_.clear();
            head_ = INVALID_INDEX;
            tail_ = INVALID_INDEX;
            size_ = 0;
        }

        void reverse() noexcept {
            size_t current = head_;
            while (current != INVALID_INDEX) {
                std::swap(nodes_[current].prev, nodes_[current].next);
                current = nodes_[current].prev; // Was next before swap
            }
            std::swap(head_, tail_);
        }

        // Move element to front (useful for LRU cache)
        void move_to_front(iterator pos) {
            if (pos.index_ == INVALID_INDEX || pos.index_ == head_) {
                return;
            }

            size_t pos_index = pos.index_;
            size_t prev_index = nodes_[pos_index].prev;
            size_t next_index = nodes_[pos_index].next;

            // Unlink from current position
            if (prev_index != INVALID_INDEX) {
                nodes_[prev_index].next = next_index;
            }
            if (next_index != INVALID_INDEX) {
                nodes_[next_index].prev = prev_index;
            } else {
                tail_ = prev_index;
            }

            // Link at front
            nodes_[pos_index].prev = INVALID_INDEX;
            nodes_[pos_index].next = head_;
            nodes_[head_].prev = pos_index;
            head_ = pos_index;
        }

        // ====================================================================
        // Iterators
        // ====================================================================

        iterator begin() noexcept { return iterator(this, head_); }
        const_iterator begin() const noexcept { return const_iterator(this, head_); }
        const_iterator cbegin() const noexcept { return const_iterator(this, head_); }

        iterator end() noexcept { return iterator(this, INVALID_INDEX); }
        const_iterator end() const noexcept { return const_iterator(this, INVALID_INDEX); }
        const_iterator cend() const noexcept { return const_iterator(this, INVALID_INDEX); }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

        // ====================================================================
        // Serialization
        // ====================================================================

        auto members() noexcept { return std::tie(nodes_, head_, tail_, size_, free_list_); }
        auto members() const noexcept { return std::tie(nodes_, head_, tail_, size_, free_list_); }

      private:
        size_t allocate_node(T const &value, size_t prev, size_t next) {
            if (!free_list_.empty()) {
                size_t index = free_list_.back();
                free_list_.pop_back();
                nodes_[index] = Node(value, prev, next);
                return index;
            }
            nodes_.push_back(Node(value, prev, next));
            return nodes_.size() - 1;
        }

        size_t allocate_node(T &&value, size_t prev, size_t next) {
            if (!free_list_.empty()) {
                size_t index = free_list_.back();
                free_list_.pop_back();
                nodes_[index] = Node(std::move(value), prev, next);
                return index;
            }
            nodes_.push_back(Node(std::move(value), prev, next));
            return nodes_.size() - 1;
        }

        template <typename... Args> size_t allocate_node_emplace(size_t prev, size_t next, Args &&...args) {
            if (!free_list_.empty()) {
                size_t index = free_list_.back();
                free_list_.pop_back();
                nodes_[index].value = T(std::forward<Args>(args)...);
                nodes_[index].prev = prev;
                nodes_[index].next = next;
                return index;
            }
            nodes_.emplace_back(T(std::forward<Args>(args)...), prev, next);
            return nodes_.size() - 1;
        }

        void deallocate_node(size_t index) { free_list_.push_back(index); }

        Vector<Node> nodes_;
        size_t head_;
        size_t tail_;
        size_t size_;
        Vector<size_t> free_list_;
    };

    // Comparison operators
    template <typename T> bool operator==(List<T> const &lhs, List<T> const &rhs) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        auto it1 = lhs.begin();
        auto it2 = rhs.begin();
        while (it1 != lhs.end()) {
            if (!(*it1 == *it2)) {
                return false;
            }
            ++it1;
            ++it2;
        }
        return true;
    }

    template <typename T> bool operator!=(List<T> const &lhs, List<T> const &rhs) { return !(lhs == rhs); }

    namespace list {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace list

} // namespace datapod
