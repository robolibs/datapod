#pragma once

#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "datapod/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief Singly linked list with O(1) prepend
     *
     * ForwardList<T> is a singly linked list that uses index-based nodes
     * instead of pointers, enabling full serialization via members().
     *
     * Useful for:
     * - Adjacency lists in graphs
     * - Free lists and object pools
     * - Any case where O(1) front insertion is needed
     *
     * @tparam T Value type
     *
     * Memory Layout:
     * @code
     * nodes_:     [Node0, Node1, Node2, ...]  <- Vector of nodes
     * head_:      index of first node (INVALID_INDEX if empty)
     * size_:      number of elements
     * free_list_: [idx1, idx2, ...]  <- recycled node indices
     * @endcode
     */
    template <typename T> class ForwardList {
      public:
        static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

        struct Node {
            T value;
            size_t next;

            Node() : value{}, next{INVALID_INDEX} {}
            Node(T const &v, size_t n) : value{v}, next{n} {}
            Node(T &&v, size_t n) : value{std::move(v)}, next{n} {}

            auto members() noexcept { return std::tie(value, next); }
            auto members() const noexcept { return std::tie(value, next); }
        };

        using value_type = T;
        using size_type = std::size_t;
        using reference = T &;
        using const_reference = T const &;

        // ====================================================================
        // Iterator
        // ====================================================================

        class iterator {
          public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T *;
            using reference = T &;

            iterator() : list_{nullptr}, index_{INVALID_INDEX} {}
            iterator(ForwardList *list, size_t index) : list_{list}, index_{index} {}

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

            bool operator==(iterator const &other) const { return index_ == other.index_; }
            bool operator!=(iterator const &other) const { return index_ != other.index_; }

            size_t index() const { return index_; }

          private:
            ForwardList *list_;
            size_t index_;
        };

        class const_iterator {
          public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T const *;
            using reference = T const &;

            const_iterator() : list_{nullptr}, index_{INVALID_INDEX} {}
            const_iterator(ForwardList const *list, size_t index) : list_{list}, index_{index} {}
            const_iterator(iterator it) : list_{it.list_}, index_{it.index()} {}

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

            bool operator==(const_iterator const &other) const { return index_ == other.index_; }
            bool operator!=(const_iterator const &other) const { return index_ != other.index_; }

            size_t index() const { return index_; }

          private:
            ForwardList const *list_;
            size_t index_;
        };

        // ====================================================================
        // Construction
        // ====================================================================

        ForwardList() : head_{INVALID_INDEX}, size_{0} {}

        ForwardList(std::initializer_list<T> init) : head_{INVALID_INDEX}, size_{0} {
            // Insert in reverse order to maintain order
            Vector<T> temp(init);
            for (size_t i = temp.size(); i > 0; --i) {
                push_front(temp[i - 1]);
            }
        }

        ForwardList(ForwardList const &other)
            : nodes_{other.nodes_}, head_{other.head_}, size_{other.size_}, free_list_{other.free_list_} {}

        ForwardList(ForwardList &&other) noexcept
            : nodes_{std::move(other.nodes_)}, head_{other.head_}, size_{other.size_},
              free_list_{std::move(other.free_list_)} {
            other.head_ = INVALID_INDEX;
            other.size_ = 0;
        }

        ForwardList &operator=(ForwardList const &other) {
            if (this != &other) {
                nodes_ = other.nodes_;
                head_ = other.head_;
                size_ = other.size_;
                free_list_ = other.free_list_;
            }
            return *this;
        }

        ForwardList &operator=(ForwardList &&other) noexcept {
            if (this != &other) {
                nodes_ = std::move(other.nodes_);
                head_ = other.head_;
                size_ = other.size_;
                free_list_ = std::move(other.free_list_);
                other.head_ = INVALID_INDEX;
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
                throw std::out_of_range("ForwardList::front: list is empty");
            }
            return nodes_[head_].value;
        }

        const_reference front() const {
            if (empty()) {
                throw std::out_of_range("ForwardList::front: list is empty");
            }
            return nodes_[head_].value;
        }

        // ====================================================================
        // Modifiers
        // ====================================================================

        void push_front(T const &value) {
            size_t new_index = allocate_node(value, head_);
            head_ = new_index;
            ++size_;
        }

        void push_front(T &&value) {
            size_t new_index = allocate_node(std::move(value), head_);
            head_ = new_index;
            ++size_;
        }

        template <typename... Args> reference emplace_front(Args &&...args) {
            size_t new_index = allocate_node_emplace(head_, std::forward<Args>(args)...);
            head_ = new_index;
            ++size_;
            return nodes_[new_index].value;
        }

        void pop_front() {
            if (empty()) {
                throw std::out_of_range("ForwardList::pop_front: list is empty");
            }
            size_t old_head = head_;
            head_ = nodes_[head_].next;
            deallocate_node(old_head);
            --size_;
        }

        iterator insert_after(iterator pos, T const &value) {
            size_t pos_index = pos.index();
            if (pos_index == INVALID_INDEX) {
                throw std::out_of_range("ForwardList::insert_after: invalid iterator");
            }
            size_t new_index = allocate_node(value, nodes_[pos_index].next);
            nodes_[pos_index].next = new_index;
            ++size_;
            return iterator(this, new_index);
        }

        iterator insert_after(iterator pos, T &&value) {
            size_t pos_index = pos.index();
            if (pos_index == INVALID_INDEX) {
                throw std::out_of_range("ForwardList::insert_after: invalid iterator");
            }
            size_t new_index = allocate_node(std::move(value), nodes_[pos_index].next);
            nodes_[pos_index].next = new_index;
            ++size_;
            return iterator(this, new_index);
        }

        iterator erase_after(iterator pos) {
            size_t pos_index = pos.index();
            if (pos_index == INVALID_INDEX) {
                throw std::out_of_range("ForwardList::erase_after: invalid iterator");
            }
            size_t to_erase = nodes_[pos_index].next;
            if (to_erase == INVALID_INDEX) {
                throw std::out_of_range("ForwardList::erase_after: nothing to erase");
            }
            nodes_[pos_index].next = nodes_[to_erase].next;
            deallocate_node(to_erase);
            --size_;
            return iterator(this, nodes_[pos_index].next);
        }

        void clear() noexcept {
            nodes_.clear();
            free_list_.clear();
            head_ = INVALID_INDEX;
            size_ = 0;
        }

        void reverse() noexcept {
            size_t prev = INVALID_INDEX;
            size_t current = head_;
            while (current != INVALID_INDEX) {
                size_t next = nodes_[current].next;
                nodes_[current].next = prev;
                prev = current;
                current = next;
            }
            head_ = prev;
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

        iterator before_begin() noexcept { return iterator(this, INVALID_INDEX); }
        const_iterator before_begin() const noexcept { return const_iterator(this, INVALID_INDEX); }

        // ====================================================================
        // Serialization
        // ====================================================================

        auto members() noexcept { return std::tie(nodes_, head_, size_, free_list_); }
        auto members() const noexcept { return std::tie(nodes_, head_, size_, free_list_); }

      private:
        size_t allocate_node(T const &value, size_t next) {
            if (!free_list_.empty()) {
                size_t index = free_list_.back();
                free_list_.pop_back();
                nodes_[index] = Node(value, next);
                return index;
            }
            nodes_.push_back(Node(value, next));
            return nodes_.size() - 1;
        }

        size_t allocate_node(T &&value, size_t next) {
            if (!free_list_.empty()) {
                size_t index = free_list_.back();
                free_list_.pop_back();
                nodes_[index] = Node(std::move(value), next);
                return index;
            }
            nodes_.push_back(Node(std::move(value), next));
            return nodes_.size() - 1;
        }

        template <typename... Args> size_t allocate_node_emplace(size_t next, Args &&...args) {
            if (!free_list_.empty()) {
                size_t index = free_list_.back();
                free_list_.pop_back();
                nodes_[index].value = T(std::forward<Args>(args)...);
                nodes_[index].next = next;
                return index;
            }
            nodes_.emplace_back(T(std::forward<Args>(args)...), next);
            return nodes_.size() - 1;
        }

        void deallocate_node(size_t index) { free_list_.push_back(index); }

        Vector<Node> nodes_;
        size_t head_;
        size_t size_;
        Vector<size_t> free_list_;
    };

    // Comparison operators
    template <typename T> bool operator==(ForwardList<T> const &lhs, ForwardList<T> const &rhs) {
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

    template <typename T> bool operator!=(ForwardList<T> const &lhs, ForwardList<T> const &rhs) {
        return !(lhs == rhs);
    }

} // namespace datapod
