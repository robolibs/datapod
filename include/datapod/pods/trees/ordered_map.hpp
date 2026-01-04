#pragma once
#include <datapod/types/types.hpp>

#include <cstddef>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "datapod/pods/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief Sorted key-value map using a Red-Black tree
     *
     * OrderedMap<K, V> is a sorted associative container that uses index-based
     * nodes instead of pointers, enabling full serialization via members().
     *
     * Useful for:
     * - Sorted iteration over key-value pairs
     * - Range queries (find all keys in [a, b))
     * - Finding min/max keys
     * - When you need O(log n) operations with ordering
     *
     * @tparam K Key type
     * @tparam V Value type
     * @tparam Compare Comparison functor (default std::less<K>)
     *
     * Time Complexity:
     * - insert, find, erase: O(log n)
     * - min, max: O(log n)
     * - iteration: O(n) total, O(1) per step
     */
    template <typename K, typename V, typename Compare = std::less<K>> class OrderedMap {
      public:
        static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

        struct Node {
            K key;
            V value;
            size_t left;
            size_t right;
            size_t parent;
            bool is_red;

            Node() : key{}, value{}, left{INVALID_INDEX}, right{INVALID_INDEX}, parent{INVALID_INDEX}, is_red{true} {}

            Node(K const &k, V const &v, size_t p)
                : key{k}, value{v}, left{INVALID_INDEX}, right{INVALID_INDEX}, parent{p}, is_red{true} {}

            Node(K &&k, V &&v, size_t p)
                : key{std::move(k)}, value{std::move(v)}, left{INVALID_INDEX}, right{INVALID_INDEX}, parent{p},
                  is_red{true} {}

            auto members() noexcept { return std::tie(key, value, left, right, parent, is_red); }
            auto members() const noexcept { return std::tie(key, value, left, right, parent, is_red); }
        };

        using key_type = K;
        using mapped_type = V;
        using value_type = std::pair<K const, V>;
        using size_type = datapod::usize;
        using difference_type = datapod::isize;
        using key_compare = Compare;

        // ====================================================================
        // Iterator (in-order traversal)
        // ====================================================================

        class iterator {
          public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = std::pair<K const &, V &>;
            using difference_type = datapod::isize;
            using pointer = void;
            using reference = value_type;

            iterator() : map_{nullptr}, index_{INVALID_INDEX} {}
            iterator(OrderedMap *map, size_t index) : map_{map}, index_{index} {}

            std::pair<K const &, V &> operator*() { return {map_->nodes_[index_].key, map_->nodes_[index_].value}; }

            K const &key() const { return map_->nodes_[index_].key; }
            V &value() { return map_->nodes_[index_].value; }

            iterator &operator++() {
                index_ = map_->successor(index_);
                return *this;
            }

            iterator operator++(int) {
                iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            iterator &operator--() {
                if (index_ == INVALID_INDEX) {
                    index_ = map_->maximum(map_->root_);
                } else {
                    index_ = map_->predecessor(index_);
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
            friend class OrderedMap;
            OrderedMap *map_;
            size_t index_;
        };

        class const_iterator {
          public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = std::pair<K const &, V const &>;
            using difference_type = datapod::isize;
            using pointer = void;
            using reference = value_type;

            const_iterator() : map_{nullptr}, index_{INVALID_INDEX} {}
            const_iterator(OrderedMap const *map, size_t index) : map_{map}, index_{index} {}
            const_iterator(iterator it) : map_{it.map_}, index_{it.index_} {}

            std::pair<K const &, V const &> operator*() const {
                return {map_->nodes_[index_].key, map_->nodes_[index_].value};
            }

            K const &key() const { return map_->nodes_[index_].key; }
            V const &value() const { return map_->nodes_[index_].value; }

            const_iterator &operator++() {
                index_ = map_->successor(index_);
                return *this;
            }

            const_iterator operator++(int) {
                const_iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            const_iterator &operator--() {
                if (index_ == INVALID_INDEX) {
                    index_ = map_->maximum(map_->root_);
                } else {
                    index_ = map_->predecessor(index_);
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
            OrderedMap const *map_;
            size_t index_;
        };

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // ====================================================================
        // Construction
        // ====================================================================

        OrderedMap() : root_{INVALID_INDEX}, size_{0} {}

        explicit OrderedMap(Compare const &comp) : root_{INVALID_INDEX}, size_{0}, comp_{comp} {}

        OrderedMap(std::initializer_list<std::pair<K, V>> init) : root_{INVALID_INDEX}, size_{0} {
            for (auto const &p : init) {
                insert(p.first, p.second);
            }
        }

        OrderedMap(OrderedMap const &other)
            : nodes_{other.nodes_}, root_{other.root_}, size_{other.size_}, free_list_{other.free_list_},
              comp_{other.comp_} {}

        OrderedMap(OrderedMap &&other) noexcept
            : nodes_{std::move(other.nodes_)}, root_{other.root_}, size_{other.size_},
              free_list_{std::move(other.free_list_)}, comp_{std::move(other.comp_)} {
            other.root_ = INVALID_INDEX;
            other.size_ = 0;
        }

        OrderedMap &operator=(OrderedMap const &other) {
            if (this != &other) {
                nodes_ = other.nodes_;
                root_ = other.root_;
                size_ = other.size_;
                free_list_ = other.free_list_;
                comp_ = other.comp_;
            }
            return *this;
        }

        OrderedMap &operator=(OrderedMap &&other) noexcept {
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
        // Element Access
        // ====================================================================

        V &operator[](K const &key) {
            auto it = find(key);
            if (it != end()) {
                return it.value();
            }
            auto [inserted_it, _] = insert(key, V{});
            return inserted_it.value();
        }

        V &at(K const &key) {
            auto it = find(key);
            if (it == end()) {
                throw std::out_of_range("OrderedMap::at: key not found");
            }
            return it.value();
        }

        V const &at(K const &key) const {
            auto it = find(key);
            if (it == end()) {
                throw std::out_of_range("OrderedMap::at: key not found");
            }
            return it.value();
        }

        // ====================================================================
        // Lookup
        // ====================================================================

        iterator find(K const &key) {
            size_t node = root_;
            while (node != INVALID_INDEX) {
                if (comp_(key, nodes_[node].key)) {
                    node = nodes_[node].left;
                } else if (comp_(nodes_[node].key, key)) {
                    node = nodes_[node].right;
                } else {
                    return iterator(this, node);
                }
            }
            return end();
        }

        const_iterator find(K const &key) const {
            size_t node = root_;
            while (node != INVALID_INDEX) {
                if (comp_(key, nodes_[node].key)) {
                    node = nodes_[node].left;
                } else if (comp_(nodes_[node].key, key)) {
                    node = nodes_[node].right;
                } else {
                    return const_iterator(this, node);
                }
            }
            return end();
        }

        bool contains(K const &key) const { return find(key) != end(); }

        size_type count(K const &key) const { return contains(key) ? 1 : 0; }

        iterator lower_bound(K const &key) {
            size_t node = root_;
            size_t result = INVALID_INDEX;
            while (node != INVALID_INDEX) {
                if (!comp_(nodes_[node].key, key)) {
                    result = node;
                    node = nodes_[node].left;
                } else {
                    node = nodes_[node].right;
                }
            }
            return iterator(this, result);
        }

        const_iterator lower_bound(K const &key) const {
            size_t node = root_;
            size_t result = INVALID_INDEX;
            while (node != INVALID_INDEX) {
                if (!comp_(nodes_[node].key, key)) {
                    result = node;
                    node = nodes_[node].left;
                } else {
                    node = nodes_[node].right;
                }
            }
            return const_iterator(this, result);
        }

        iterator upper_bound(K const &key) {
            size_t node = root_;
            size_t result = INVALID_INDEX;
            while (node != INVALID_INDEX) {
                if (comp_(key, nodes_[node].key)) {
                    result = node;
                    node = nodes_[node].left;
                } else {
                    node = nodes_[node].right;
                }
            }
            return iterator(this, result);
        }

        const_iterator upper_bound(K const &key) const {
            size_t node = root_;
            size_t result = INVALID_INDEX;
            while (node != INVALID_INDEX) {
                if (comp_(key, nodes_[node].key)) {
                    result = node;
                    node = nodes_[node].left;
                } else {
                    node = nodes_[node].right;
                }
            }
            return const_iterator(this, result);
        }

        // ====================================================================
        // Min/Max
        // ====================================================================

        K const &min_key() const {
            if (empty()) {
                throw std::out_of_range("OrderedMap::min_key: map is empty");
            }
            return nodes_[minimum(root_)].key;
        }

        K const &max_key() const {
            if (empty()) {
                throw std::out_of_range("OrderedMap::max_key: map is empty");
            }
            return nodes_[maximum(root_)].key;
        }

        // ====================================================================
        // Modifiers
        // ====================================================================

        std::pair<iterator, bool> insert(K const &key, V const &value) {
            // Find insertion point
            size_t parent = INVALID_INDEX;
            size_t node = root_;
            bool go_left = false;

            while (node != INVALID_INDEX) {
                parent = node;
                if (comp_(key, nodes_[node].key)) {
                    node = nodes_[node].left;
                    go_left = true;
                } else if (comp_(nodes_[node].key, key)) {
                    node = nodes_[node].right;
                    go_left = false;
                } else {
                    // Key already exists
                    return {iterator(this, node), false};
                }
            }

            // Create new node
            size_t new_node = allocate_node(key, value, parent);

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

        std::pair<iterator, bool> insert(K &&key, V &&value) {
            size_t parent = INVALID_INDEX;
            size_t node = root_;
            bool go_left = false;

            while (node != INVALID_INDEX) {
                parent = node;
                if (comp_(key, nodes_[node].key)) {
                    node = nodes_[node].left;
                    go_left = true;
                } else if (comp_(nodes_[node].key, key)) {
                    node = nodes_[node].right;
                    go_left = false;
                } else {
                    return {iterator(this, node), false};
                }
            }

            size_t new_node = allocate_node(std::move(key), std::move(value), parent);

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

        template <typename... Args> std::pair<iterator, bool> emplace(K const &key, Args &&...args) {
            return insert(key, V(std::forward<Args>(args)...));
        }

        size_type erase(K const &key) {
            auto it = find(key);
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
            iterator next_it(this, successor(z));

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

                if (nodes_[y].parent == z) {
                    x_parent = y;
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

        void clear() noexcept {
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

        auto members() noexcept { return std::tie(nodes_, root_, size_, free_list_); }
        auto members() const noexcept { return std::tie(nodes_, root_, size_, free_list_); }

      private:
        // ====================================================================
        // Node allocation
        // ====================================================================

        size_t allocate_node(K const &key, V const &value, size_t parent) {
            if (!free_list_.empty()) {
                size_t index = free_list_.back();
                free_list_.pop_back();
                nodes_[index] = Node(key, value, parent);
                return index;
            }
            nodes_.push_back(Node(key, value, parent));
            return nodes_.size() - 1;
        }

        size_t allocate_node(K &&key, V &&value, size_t parent) {
            if (!free_list_.empty()) {
                size_t index = free_list_.back();
                free_list_.pop_back();
                nodes_[index] = Node(std::move(key), std::move(value), parent);
                return index;
            }
            nodes_.push_back(Node(std::move(key), std::move(value), parent));
            return nodes_.size() - 1;
        }

        void deallocate_node(size_t index) { free_list_.push_back(index); }

        // ====================================================================
        // Tree navigation
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
        // Red-Black tree operations
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

        void insert_fixup(size_t z) {
            while (z != root_ && nodes_[nodes_[z].parent].is_red) {
                size_t parent = nodes_[z].parent;
                size_t grandparent = nodes_[parent].parent;

                if (parent == nodes_[grandparent].left) {
                    size_t uncle = nodes_[grandparent].right;

                    if (uncle != INVALID_INDEX && nodes_[uncle].is_red) {
                        nodes_[parent].is_red = false;
                        nodes_[uncle].is_red = false;
                        nodes_[grandparent].is_red = true;
                        z = grandparent;
                    } else {
                        if (z == nodes_[parent].right) {
                            z = parent;
                            rotate_left(z);
                            parent = nodes_[z].parent;
                            grandparent = nodes_[parent].parent;
                        }
                        nodes_[parent].is_red = false;
                        nodes_[grandparent].is_red = true;
                        rotate_right(grandparent);
                    }
                } else {
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

        void erase_fixup(size_t x, size_t x_parent) {
            while (x != root_ && (x == INVALID_INDEX || !nodes_[x].is_red)) {
                if (x == nodes_[x_parent].left) {
                    size_t w = nodes_[x_parent].right;

                    if (w != INVALID_INDEX && nodes_[w].is_red) {
                        nodes_[w].is_red = false;
                        nodes_[x_parent].is_red = true;
                        rotate_left(x_parent);
                        w = nodes_[x_parent].right;
                    }

                    bool left_black = (nodes_[w].left == INVALID_INDEX || !nodes_[nodes_[w].left].is_red);
                    bool right_black = (nodes_[w].right == INVALID_INDEX || !nodes_[nodes_[w].right].is_red);

                    if (left_black && right_black) {
                        if (w != INVALID_INDEX) {
                            nodes_[w].is_red = true;
                        }
                        x = x_parent;
                        x_parent = nodes_[x].parent;
                    } else {
                        if (right_black) {
                            if (nodes_[w].left != INVALID_INDEX) {
                                nodes_[nodes_[w].left].is_red = false;
                            }
                            nodes_[w].is_red = true;
                            rotate_right(w);
                            w = nodes_[x_parent].right;
                        }

                        nodes_[w].is_red = nodes_[x_parent].is_red;
                        nodes_[x_parent].is_red = false;
                        if (nodes_[w].right != INVALID_INDEX) {
                            nodes_[nodes_[w].right].is_red = false;
                        }
                        rotate_left(x_parent);
                        x = root_;
                    }
                } else {
                    size_t w = nodes_[x_parent].left;

                    if (w != INVALID_INDEX && nodes_[w].is_red) {
                        nodes_[w].is_red = false;
                        nodes_[x_parent].is_red = true;
                        rotate_right(x_parent);
                        w = nodes_[x_parent].left;
                    }

                    bool left_black = (nodes_[w].left == INVALID_INDEX || !nodes_[nodes_[w].left].is_red);
                    bool right_black = (nodes_[w].right == INVALID_INDEX || !nodes_[nodes_[w].right].is_red);

                    if (left_black && right_black) {
                        if (w != INVALID_INDEX) {
                            nodes_[w].is_red = true;
                        }
                        x = x_parent;
                        x_parent = nodes_[x].parent;
                    } else {
                        if (left_black) {
                            if (nodes_[w].right != INVALID_INDEX) {
                                nodes_[nodes_[w].right].is_red = false;
                            }
                            nodes_[w].is_red = true;
                            rotate_left(w);
                            w = nodes_[x_parent].left;
                        }

                        nodes_[w].is_red = nodes_[x_parent].is_red;
                        nodes_[x_parent].is_red = false;
                        if (nodes_[w].left != INVALID_INDEX) {
                            nodes_[nodes_[w].left].is_red = false;
                        }
                        rotate_right(x_parent);
                        x = root_;
                    }
                }
            }

            if (x != INVALID_INDEX) {
                nodes_[x].is_red = false;
            }
        }

        Vector<Node> nodes_;
        size_t root_;
        size_t size_;
        Vector<size_t> free_list_;
        [[no_unique_address]] Compare comp_{};
    };

    // Comparison operators
    template <typename K, typename V, typename Compare>
    bool operator==(OrderedMap<K, V, Compare> const &lhs, OrderedMap<K, V, Compare> const &rhs) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        auto it1 = lhs.begin();
        auto it2 = rhs.begin();
        while (it1 != lhs.end()) {
            auto [k1, v1] = *it1;
            auto [k2, v2] = *it2;
            if (!(k1 == k2) || !(v1 == v2)) {
                return false;
            }
            ++it1;
            ++it2;
        }
        return true;
    }

    template <typename K, typename V, typename Compare>
    bool operator!=(OrderedMap<K, V, Compare> const &lhs, OrderedMap<K, V, Compare> const &rhs) {
        return !(lhs == rhs);
    }

    namespace ordered_map {
        /// Placeholder for template/container type (no useful make() function)
        inline void unimplemented() {}
    } // namespace ordered_map

} // namespace datapod
