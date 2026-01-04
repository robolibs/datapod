#pragma once

#include <cstddef>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "datapod/pods/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief Sorted unique set using a Red-Black tree
     *
     * OrderedSet<T> is a sorted associative container that uses index-based
     * nodes instead of pointers, enabling full serialization via members().
     *
     * Useful for:
     * - Sorted iteration over unique elements
     * - Range queries (find all elements in [a, b))
     * - Finding min/max elements
     * - When you need O(log n) operations with ordering
     *
     * @tparam T Element type
     * @tparam Compare Comparison functor (default std::less<T>)
     *
     * Time Complexity:
     * - insert, find, erase: O(log n)
     * - min, max: O(log n)
     * - iteration: O(n) total, O(1) per step
     */
    template <typename T, typename Compare = std::less<T>> class OrderedSet {
      public:
        static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

        struct Node {
            T value;
            size_t left;
            size_t right;
            size_t parent;
            bool is_red;

            Node() : value{}, left{INVALID_INDEX}, right{INVALID_INDEX}, parent{INVALID_INDEX}, is_red{true} {}

            Node(T const &v, size_t p) : value{v}, left{INVALID_INDEX}, right{INVALID_INDEX}, parent{p}, is_red{true} {}

            Node(T &&v, size_t p)
                : value{std::move(v)}, left{INVALID_INDEX}, right{INVALID_INDEX}, parent{p}, is_red{true} {}

            auto members() noexcept { return std::tie(value, left, right, parent, is_red); }
            auto members() const noexcept { return std::tie(value, left, right, parent, is_red); }
        };

        using key_type = T;
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using key_compare = Compare;
        using value_compare = Compare;

        // ====================================================================
        // Iterator (in-order traversal)
        // ====================================================================

        class iterator {
          public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T const *;
            using reference = T const &;

            iterator() : set_{nullptr}, index_{INVALID_INDEX} {}
            iterator(OrderedSet *set, size_t index) : set_{set}, index_{index} {}

            T const &operator*() const { return set_->nodes_[index_].value; }
            T const *operator->() const { return &set_->nodes_[index_].value; }

            iterator &operator++() {
                index_ = set_->successor(index_);
                return *this;
            }

            iterator operator++(int) {
                iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            iterator &operator--() {
                if (index_ == INVALID_INDEX) {
                    index_ = set_->maximum(set_->root_);
                } else {
                    index_ = set_->predecessor(index_);
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
            friend class OrderedSet;
            OrderedSet *set_;
            size_t index_;
        };

        class const_iterator {
          public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T const *;
            using reference = T const &;

            const_iterator() : set_{nullptr}, index_{INVALID_INDEX} {}
            const_iterator(OrderedSet const *set, size_t index) : set_{set}, index_{index} {}
            const_iterator(iterator it) : set_{it.set_}, index_{it.index_} {}

            T const &operator*() const { return set_->nodes_[index_].value; }
            T const *operator->() const { return &set_->nodes_[index_].value; }

            const_iterator &operator++() {
                index_ = set_->successor(index_);
                return *this;
            }

            const_iterator operator++(int) {
                const_iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            const_iterator &operator--() {
                if (index_ == INVALID_INDEX) {
                    index_ = set_->maximum(set_->root_);
                } else {
                    index_ = set_->predecessor(index_);
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
            friend class OrderedSet;
            OrderedSet const *set_;
            size_t index_;
        };

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // ====================================================================
        // Constructors
        // ====================================================================

        OrderedSet() : nodes_{}, root_{INVALID_INDEX}, size_{0}, free_list_{}, comp_{} {}

        explicit OrderedSet(Compare const &comp)
            : nodes_{}, root_{INVALID_INDEX}, size_{0}, free_list_{}, comp_{comp} {}

        OrderedSet(std::initializer_list<T> init) : OrderedSet() {
            for (auto const &v : init) {
                insert(v);
            }
        }

        template <typename InputIt> OrderedSet(InputIt first, InputIt last) : OrderedSet() {
            for (; first != last; ++first) {
                insert(*first);
            }
        }

        OrderedSet(OrderedSet const &other)
            : nodes_{other.nodes_}, root_{other.root_}, size_{other.size_}, free_list_{other.free_list_},
              comp_{other.comp_} {}

        OrderedSet(OrderedSet &&other) noexcept
            : nodes_{std::move(other.nodes_)}, root_{other.root_}, size_{other.size_},
              free_list_{std::move(other.free_list_)}, comp_{std::move(other.comp_)} {
            other.root_ = INVALID_INDEX;
            other.size_ = 0;
        }

        OrderedSet &operator=(OrderedSet const &other) {
            if (this != &other) {
                nodes_ = other.nodes_;
                root_ = other.root_;
                size_ = other.size_;
                free_list_ = other.free_list_;
                comp_ = other.comp_;
            }
            return *this;
        }

        OrderedSet &operator=(OrderedSet &&other) noexcept {
            if (this != &other) {
                nodes_ = std::move(other.nodes_);
                root_ = other.root_;
                size_ = other.size_;
                free_list_ = std::move(other.free_list_);
                comp_ = std::move(other.comp_);
                other.root_ = INVALID_INDEX;
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
        // Lookup
        // ====================================================================

        iterator find(T const &value) {
            size_t node = root_;
            while (node != INVALID_INDEX) {
                if (comp_(value, nodes_[node].value)) {
                    node = nodes_[node].left;
                } else if (comp_(nodes_[node].value, value)) {
                    node = nodes_[node].right;
                } else {
                    return iterator(this, node);
                }
            }
            return end();
        }

        const_iterator find(T const &value) const {
            size_t node = root_;
            while (node != INVALID_INDEX) {
                if (comp_(value, nodes_[node].value)) {
                    node = nodes_[node].left;
                } else if (comp_(nodes_[node].value, value)) {
                    node = nodes_[node].right;
                } else {
                    return const_iterator(this, node);
                }
            }
            return end();
        }

        bool contains(T const &value) const { return find(value) != end(); }

        size_type count(T const &value) const { return contains(value) ? 1 : 0; }

        /// Find first element >= value
        iterator lower_bound(T const &value) {
            size_t node = root_;
            size_t result = INVALID_INDEX;

            while (node != INVALID_INDEX) {
                if (!comp_(nodes_[node].value, value)) {
                    result = node;
                    node = nodes_[node].left;
                } else {
                    node = nodes_[node].right;
                }
            }
            return iterator(this, result);
        }

        const_iterator lower_bound(T const &value) const {
            size_t node = root_;
            size_t result = INVALID_INDEX;

            while (node != INVALID_INDEX) {
                if (!comp_(nodes_[node].value, value)) {
                    result = node;
                    node = nodes_[node].left;
                } else {
                    node = nodes_[node].right;
                }
            }
            return const_iterator(this, result);
        }

        /// Find first element > value
        iterator upper_bound(T const &value) {
            size_t node = root_;
            size_t result = INVALID_INDEX;

            while (node != INVALID_INDEX) {
                if (comp_(value, nodes_[node].value)) {
                    result = node;
                    node = nodes_[node].left;
                } else {
                    node = nodes_[node].right;
                }
            }
            return iterator(this, result);
        }

        const_iterator upper_bound(T const &value) const {
            size_t node = root_;
            size_t result = INVALID_INDEX;

            while (node != INVALID_INDEX) {
                if (comp_(value, nodes_[node].value)) {
                    result = node;
                    node = nodes_[node].left;
                } else {
                    node = nodes_[node].right;
                }
            }
            return const_iterator(this, result);
        }

        /// Get range [lower, upper)
        std::pair<iterator, iterator> equal_range(T const &value) { return {lower_bound(value), upper_bound(value)}; }

        std::pair<const_iterator, const_iterator> equal_range(T const &value) const {
            return {lower_bound(value), upper_bound(value)};
        }

        /// Get minimum element
        T const &min() const {
            if (empty()) {
                throw std::out_of_range("OrderedSet::min: set is empty");
            }
            return nodes_[minimum(root_)].value;
        }

        /// Get maximum element
        T const &max() const {
            if (empty()) {
                throw std::out_of_range("OrderedSet::max: set is empty");
            }
            return nodes_[maximum(root_)].value;
        }

        // ====================================================================
        // Modifiers
        // ====================================================================

        std::pair<iterator, bool> insert(T const &value) {
            // Find insertion point
            size_t parent = INVALID_INDEX;
            size_t node = root_;
            bool go_left = false;

            while (node != INVALID_INDEX) {
                parent = node;
                if (comp_(value, nodes_[node].value)) {
                    node = nodes_[node].left;
                    go_left = true;
                } else if (comp_(nodes_[node].value, value)) {
                    node = nodes_[node].right;
                    go_left = false;
                } else {
                    // Value already exists
                    return {iterator(this, node), false};
                }
            }

            // Create new node
            size_t new_node = allocate_node(value, parent);

            if (parent == INVALID_INDEX) {
                root_ = new_node;
            } else if (go_left) {
                nodes_[parent].left = new_node;
            } else {
                nodes_[parent].right = new_node;
            }

            ++size_;
            insert_fixup(new_node);

            return {iterator(this, new_node), true};
        }

        std::pair<iterator, bool> insert(T &&value) {
            size_t parent = INVALID_INDEX;
            size_t node = root_;
            bool go_left = false;

            while (node != INVALID_INDEX) {
                parent = node;
                if (comp_(value, nodes_[node].value)) {
                    node = nodes_[node].left;
                    go_left = true;
                } else if (comp_(nodes_[node].value, value)) {
                    node = nodes_[node].right;
                    go_left = false;
                } else {
                    return {iterator(this, node), false};
                }
            }

            size_t new_node = allocate_node(std::move(value), parent);

            if (parent == INVALID_INDEX) {
                root_ = new_node;
            } else if (go_left) {
                nodes_[parent].left = new_node;
            } else {
                nodes_[parent].right = new_node;
            }

            ++size_;
            insert_fixup(new_node);

            return {iterator(this, new_node), true};
        }

        template <typename... Args> std::pair<iterator, bool> emplace(Args &&...args) {
            return insert(T(std::forward<Args>(args)...));
        }

        size_type erase(T const &value) {
            auto it = find(value);
            if (it == end()) {
                return 0;
            }
            erase(it);
            return 1;
        }

        iterator erase(iterator pos) {
            if (pos == end()) {
                return end();
            }

            size_t z = pos.index();
            iterator next_it = pos;
            ++next_it;

            size_t y = z;
            size_t x = INVALID_INDEX;
            size_t x_parent = INVALID_INDEX;
            bool y_original_red = nodes_[y].is_red;

            if (nodes_[z].left == INVALID_INDEX) {
                x = nodes_[z].right;
                x_parent = nodes_[z].parent;
                transplant(z, nodes_[z].right);
            } else if (nodes_[z].right == INVALID_INDEX) {
                x = nodes_[z].left;
                x_parent = nodes_[z].parent;
                transplant(z, nodes_[z].left);
            } else {
                y = minimum(nodes_[z].right);
                y_original_red = nodes_[y].is_red;
                x = nodes_[y].right;
                x_parent = y;

                if (nodes_[y].parent == z) {
                    if (x != INVALID_INDEX) {
                        nodes_[x].parent = y;
                    }
                } else {
                    x_parent = nodes_[y].parent;
                    transplant(y, nodes_[y].right);
                    nodes_[y].right = nodes_[z].right;
                    if (nodes_[y].right != INVALID_INDEX) {
                        nodes_[nodes_[y].right].parent = y;
                    }
                }

                transplant(z, y);
                nodes_[y].left = nodes_[z].left;
                if (nodes_[y].left != INVALID_INDEX) {
                    nodes_[nodes_[y].left].parent = y;
                }
                nodes_[y].is_red = nodes_[z].is_red;
            }

            deallocate_node(z);
            --size_;

            if (!y_original_red) {
                erase_fixup(x, x_parent);
            }

            return next_it;
        }

        void clear() {
            nodes_.clear();
            free_list_.clear();
            root_ = INVALID_INDEX;
            size_ = 0;
        }

        // ====================================================================
        // Iterators
        // ====================================================================

        iterator begin() noexcept {
            if (root_ == INVALID_INDEX) {
                return end();
            }
            return iterator(this, minimum(root_));
        }

        const_iterator begin() const noexcept {
            if (root_ == INVALID_INDEX) {
                return end();
            }
            return const_iterator(this, minimum(root_));
        }

        const_iterator cbegin() const noexcept { return begin(); }

        iterator end() noexcept { return iterator(this, INVALID_INDEX); }
        const_iterator end() const noexcept { return const_iterator(this, INVALID_INDEX); }
        const_iterator cend() const noexcept { return end(); }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

        // ====================================================================
        // Serialization support
        // ====================================================================

        auto members() noexcept { return std::tie(nodes_, root_, size_, free_list_); }
        auto members() const noexcept { return std::tie(nodes_, root_, size_, free_list_); }

        // ====================================================================
        // Comparison operators
        // ====================================================================

        bool operator==(OrderedSet const &other) const {
            if (size_ != other.size_) {
                return false;
            }
            auto it1 = begin();
            auto it2 = other.begin();
            while (it1 != end()) {
                if (!(!comp_(*it1, *it2) && !comp_(*it2, *it1))) {
                    return false;
                }
                ++it1;
                ++it2;
            }
            return true;
        }

        bool operator!=(OrderedSet const &other) const { return !(*this == other); }

      private:
        Vector<Node> nodes_;
        size_t root_;
        size_t size_;
        Vector<size_t> free_list_;
        Compare comp_;

        // ====================================================================
        // Tree navigation helpers
        // ====================================================================

        size_t minimum(size_t node) const {
            while (node != INVALID_INDEX && nodes_[node].left != INVALID_INDEX) {
                node = nodes_[node].left;
            }
            return node;
        }

        size_t maximum(size_t node) const {
            while (node != INVALID_INDEX && nodes_[node].right != INVALID_INDEX) {
                node = nodes_[node].right;
            }
            return node;
        }

        size_t successor(size_t node) const {
            if (node == INVALID_INDEX) {
                return INVALID_INDEX;
            }
            if (nodes_[node].right != INVALID_INDEX) {
                return minimum(nodes_[node].right);
            }
            size_t parent = nodes_[node].parent;
            while (parent != INVALID_INDEX && node == nodes_[parent].right) {
                node = parent;
                parent = nodes_[parent].parent;
            }
            return parent;
        }

        size_t predecessor(size_t node) const {
            if (node == INVALID_INDEX) {
                return INVALID_INDEX;
            }
            if (nodes_[node].left != INVALID_INDEX) {
                return maximum(nodes_[node].left);
            }
            size_t parent = nodes_[node].parent;
            while (parent != INVALID_INDEX && node == nodes_[parent].left) {
                node = parent;
                parent = nodes_[parent].parent;
            }
            return parent;
        }

        // ====================================================================
        // Node allocation
        // ====================================================================

        size_t allocate_node(T const &value, size_t parent) {
            size_t idx;
            if (!free_list_.empty()) {
                idx = free_list_.back();
                free_list_.pop_back();
                nodes_[idx] = Node(value, parent);
            } else {
                idx = nodes_.size();
                nodes_.push_back(Node(value, parent));
            }
            return idx;
        }

        size_t allocate_node(T &&value, size_t parent) {
            size_t idx;
            if (!free_list_.empty()) {
                idx = free_list_.back();
                free_list_.pop_back();
                nodes_[idx] = Node(std::move(value), parent);
            } else {
                idx = nodes_.size();
                nodes_.push_back(Node(std::move(value), parent));
            }
            return idx;
        }

        void deallocate_node(size_t idx) { free_list_.push_back(idx); }

        // ====================================================================
        // Red-Black tree rotations
        // ====================================================================

        void rotate_left(size_t x) {
            size_t y = nodes_[x].right;
            nodes_[x].right = nodes_[y].left;

            if (nodes_[y].left != INVALID_INDEX) {
                nodes_[nodes_[y].left].parent = x;
            }

            nodes_[y].parent = nodes_[x].parent;

            if (nodes_[x].parent == INVALID_INDEX) {
                root_ = y;
            } else if (x == nodes_[nodes_[x].parent].left) {
                nodes_[nodes_[x].parent].left = y;
            } else {
                nodes_[nodes_[x].parent].right = y;
            }

            nodes_[y].left = x;
            nodes_[x].parent = y;
        }

        void rotate_right(size_t x) {
            size_t y = nodes_[x].left;
            nodes_[x].left = nodes_[y].right;

            if (nodes_[y].right != INVALID_INDEX) {
                nodes_[nodes_[y].right].parent = x;
            }

            nodes_[y].parent = nodes_[x].parent;

            if (nodes_[x].parent == INVALID_INDEX) {
                root_ = y;
            } else if (x == nodes_[nodes_[x].parent].right) {
                nodes_[nodes_[x].parent].right = y;
            } else {
                nodes_[nodes_[x].parent].left = y;
            }

            nodes_[y].right = x;
            nodes_[x].parent = y;
        }

        // ====================================================================
        // Red-Black tree fixup after insert
        // ====================================================================

        void insert_fixup(size_t z) {
            while (z != root_ && nodes_[nodes_[z].parent].is_red) {
                size_t parent = nodes_[z].parent;
                size_t grandparent = nodes_[parent].parent;

                if (parent == nodes_[grandparent].left) {
                    size_t uncle = nodes_[grandparent].right;
                    if (uncle != INVALID_INDEX && nodes_[uncle].is_red) {
                        // Case 1: Uncle is red
                        nodes_[parent].is_red = false;
                        nodes_[uncle].is_red = false;
                        nodes_[grandparent].is_red = true;
                        z = grandparent;
                    } else {
                        if (z == nodes_[parent].right) {
                            // Case 2: z is right child
                            z = parent;
                            rotate_left(z);
                            parent = nodes_[z].parent;
                            grandparent = nodes_[parent].parent;
                        }
                        // Case 3: z is left child
                        nodes_[parent].is_red = false;
                        nodes_[grandparent].is_red = true;
                        rotate_right(grandparent);
                    }
                } else {
                    // Mirror cases
                    size_t uncle = nodes_[grandparent].left;
                    if (uncle != INVALID_INDEX && nodes_[uncle].is_red) {
                        nodes_[parent].is_red = false;
                        nodes_[uncle].is_red = false;
                        nodes_[grandparent].is_red = true;
                        z = grandparent;
                    } else {
                        if (z == nodes_[parent].left) {
                            z = parent;
                            rotate_right(z);
                            parent = nodes_[z].parent;
                            grandparent = nodes_[parent].parent;
                        }
                        nodes_[parent].is_red = false;
                        nodes_[grandparent].is_red = true;
                        rotate_left(grandparent);
                    }
                }
            }
            nodes_[root_].is_red = false;
        }

        // ====================================================================
        // Transplant helper for erase
        // ====================================================================

        void transplant(size_t u, size_t v) {
            if (nodes_[u].parent == INVALID_INDEX) {
                root_ = v;
            } else if (u == nodes_[nodes_[u].parent].left) {
                nodes_[nodes_[u].parent].left = v;
            } else {
                nodes_[nodes_[u].parent].right = v;
            }
            if (v != INVALID_INDEX) {
                nodes_[v].parent = nodes_[u].parent;
            }
        }

        // ====================================================================
        // Red-Black tree fixup after erase
        // ====================================================================

        void erase_fixup(size_t x, size_t x_parent) {
            while (x != root_ && (x == INVALID_INDEX || !nodes_[x].is_red)) {
                if (x_parent == INVALID_INDEX) {
                    break;
                }

                if (x == nodes_[x_parent].left) {
                    size_t w = nodes_[x_parent].right;
                    if (w != INVALID_INDEX && nodes_[w].is_red) {
                        // Case 1
                        nodes_[w].is_red = false;
                        nodes_[x_parent].is_red = true;
                        rotate_left(x_parent);
                        w = nodes_[x_parent].right;
                    }

                    bool w_left_black =
                        (w == INVALID_INDEX || nodes_[w].left == INVALID_INDEX || !nodes_[nodes_[w].left].is_red);
                    bool w_right_black =
                        (w == INVALID_INDEX || nodes_[w].right == INVALID_INDEX || !nodes_[nodes_[w].right].is_red);

                    if (w_left_black && w_right_black) {
                        // Case 2
                        if (w != INVALID_INDEX) {
                            nodes_[w].is_red = true;
                        }
                        x = x_parent;
                        x_parent = nodes_[x].parent;
                    } else {
                        if (w_right_black) {
                            // Case 3
                            if (w != INVALID_INDEX && nodes_[w].left != INVALID_INDEX) {
                                nodes_[nodes_[w].left].is_red = false;
                            }
                            if (w != INVALID_INDEX) {
                                nodes_[w].is_red = true;
                                rotate_right(w);
                            }
                            w = nodes_[x_parent].right;
                        }
                        // Case 4
                        if (w != INVALID_INDEX) {
                            nodes_[w].is_red = nodes_[x_parent].is_red;
                        }
                        nodes_[x_parent].is_red = false;
                        if (w != INVALID_INDEX && nodes_[w].right != INVALID_INDEX) {
                            nodes_[nodes_[w].right].is_red = false;
                        }
                        rotate_left(x_parent);
                        x = root_;
                        break;
                    }
                } else {
                    // Mirror cases
                    size_t w = nodes_[x_parent].left;
                    if (w != INVALID_INDEX && nodes_[w].is_red) {
                        nodes_[w].is_red = false;
                        nodes_[x_parent].is_red = true;
                        rotate_right(x_parent);
                        w = nodes_[x_parent].left;
                    }

                    bool w_left_black =
                        (w == INVALID_INDEX || nodes_[w].left == INVALID_INDEX || !nodes_[nodes_[w].left].is_red);
                    bool w_right_black =
                        (w == INVALID_INDEX || nodes_[w].right == INVALID_INDEX || !nodes_[nodes_[w].right].is_red);

                    if (w_left_black && w_right_black) {
                        if (w != INVALID_INDEX) {
                            nodes_[w].is_red = true;
                        }
                        x = x_parent;
                        x_parent = nodes_[x].parent;
                    } else {
                        if (w_left_black) {
                            if (w != INVALID_INDEX && nodes_[w].right != INVALID_INDEX) {
                                nodes_[nodes_[w].right].is_red = false;
                            }
                            if (w != INVALID_INDEX) {
                                nodes_[w].is_red = true;
                                rotate_left(w);
                            }
                            w = nodes_[x_parent].left;
                        }
                        if (w != INVALID_INDEX) {
                            nodes_[w].is_red = nodes_[x_parent].is_red;
                        }
                        nodes_[x_parent].is_red = false;
                        if (w != INVALID_INDEX && nodes_[w].left != INVALID_INDEX) {
                            nodes_[nodes_[w].left].is_red = false;
                        }
                        rotate_right(x_parent);
                        x = root_;
                        break;
                    }
                }
            }
            if (x != INVALID_INDEX) {
                nodes_[x].is_red = false;
            }
        }
    };

    namespace ordered_set {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace ordered_set

} // namespace datapod
