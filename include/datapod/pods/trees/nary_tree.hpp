#pragma once

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "datapod/pods/sequential/deque.hpp"
#include "datapod/pods/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief General N-ary tree (each node can have any number of children)
     *
     * NaryTree<T> is a general tree structure that uses index-based nodes
     * instead of pointers, enabling full serialization via members().
     *
     * Uses first-child/next-sibling representation for memory efficiency.
     *
     * Useful for:
     * - File system hierarchies
     * - Scene graphs
     * - DOM trees
     * - Organization charts
     * - Any hierarchical structure with variable children
     *
     * @tparam T Value type stored in each node
     *
     * Time Complexity:
     * - add_child: O(k) where k is number of existing children
     * - get, set: O(1)
     * - navigation (parent, first_child): O(1)
     * - children iteration: O(k)
     * - remove subtree: O(subtree size)
     * - traversals: O(n)
     */
    template <typename T> class NaryTree {
      public:
        static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

        /// Node handle for external reference
        using NodeId = size_t;

        struct Node {
            T value;
            size_t parent;
            size_t first_child;  // First child (linked list head)
            size_t next_sibling; // Next sibling (linked list)

            Node() : value{}, parent{INVALID_INDEX}, first_child{INVALID_INDEX}, next_sibling{INVALID_INDEX} {}

            explicit Node(T const &v, size_t p = INVALID_INDEX)
                : value{v}, parent{p}, first_child{INVALID_INDEX}, next_sibling{INVALID_INDEX} {}

            explicit Node(T &&v, size_t p = INVALID_INDEX)
                : value{std::move(v)}, parent{p}, first_child{INVALID_INDEX}, next_sibling{INVALID_INDEX} {}

            auto members() noexcept { return std::tie(value, parent, first_child, next_sibling); }
            auto members() const noexcept { return std::tie(value, parent, first_child, next_sibling); }
        };

        using value_type = T;
        using size_type = std::size_t;
        using reference = T &;
        using const_reference = T const &;

        // ====================================================================
        // Children iterator
        // ====================================================================

        class ChildIterator {
          public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = NodeId;
            using difference_type = std::ptrdiff_t;
            using pointer = NodeId const *;
            using reference = NodeId;

            ChildIterator() : tree_{nullptr}, current_{INVALID_INDEX} {}
            ChildIterator(NaryTree const *tree, NodeId current) : tree_{tree}, current_{current} {}

            NodeId operator*() const { return current_; }

            ChildIterator &operator++() {
                if (current_ != INVALID_INDEX) {
                    current_ = tree_->nodes_[current_].next_sibling;
                }
                return *this;
            }

            ChildIterator operator++(int) {
                ChildIterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(ChildIterator const &other) const { return current_ == other.current_; }
            bool operator!=(ChildIterator const &other) const { return current_ != other.current_; }

          private:
            NaryTree const *tree_;
            NodeId current_;
        };

        class ChildRange {
          public:
            ChildRange(NaryTree const *tree, NodeId first) : tree_{tree}, first_{first} {}

            ChildIterator begin() const { return ChildIterator(tree_, first_); }
            ChildIterator end() const { return ChildIterator(tree_, INVALID_INDEX); }

          private:
            NaryTree const *tree_;
            NodeId first_;
        };

        // ====================================================================
        // Construction
        // ====================================================================

        NaryTree() : nodes_{}, root_{INVALID_INDEX}, size_{0}, free_list_{} {}

        NaryTree(NaryTree const &other) = default;
        NaryTree(NaryTree &&other) noexcept = default;
        NaryTree &operator=(NaryTree const &other) = default;
        NaryTree &operator=(NaryTree &&other) noexcept = default;

        // ====================================================================
        // Capacity
        // ====================================================================

        bool empty() const noexcept { return size_ == 0; }
        size_type size() const noexcept { return size_; }

        /// Check if a node ID is valid
        bool valid(NodeId id) const noexcept {
            if (id == INVALID_INDEX || id >= nodes_.size()) {
                return false;
            }
            // Check if node is in free list (invalid)
            for (size_t i = 0; i < free_list_.size(); ++i) {
                if (free_list_[i] == id) {
                    return false;
                }
            }
            return true;
        }

        // ====================================================================
        // Root operations
        // ====================================================================

        /// Get root node ID (INVALID_INDEX if empty)
        NodeId root() const noexcept { return root_; }

        /// Check if tree has a root
        bool has_root() const noexcept { return root_ != INVALID_INDEX; }

        /// Set/create root node, returns root ID
        NodeId set_root(T const &value) {
            if (root_ != INVALID_INDEX) {
                nodes_[root_].value = value;
                return root_;
            }
            root_ = allocate_node(value, INVALID_INDEX);
            return root_;
        }

        NodeId set_root(T &&value) {
            if (root_ != INVALID_INDEX) {
                nodes_[root_].value = std::move(value);
                return root_;
            }
            root_ = allocate_node(std::move(value), INVALID_INDEX);
            return root_;
        }

        // ====================================================================
        // Node access
        // ====================================================================

        /// Get node value
        T &get(NodeId id) {
            if (!valid(id)) {
                throw std::out_of_range("NaryTree::get: invalid node ID");
            }
            return nodes_[id].value;
        }

        T const &get(NodeId id) const {
            if (!valid(id)) {
                throw std::out_of_range("NaryTree::get: invalid node ID");
            }
            return nodes_[id].value;
        }

        /// Set node value
        void set(NodeId id, T const &value) {
            if (!valid(id)) {
                throw std::out_of_range("NaryTree::set: invalid node ID");
            }
            nodes_[id].value = value;
        }

        void set(NodeId id, T &&value) {
            if (!valid(id)) {
                throw std::out_of_range("NaryTree::set: invalid node ID");
            }
            nodes_[id].value = std::move(value);
        }

        /// Operator[] for convenient access
        T &operator[](NodeId id) { return get(id); }
        T const &operator[](NodeId id) const { return get(id); }

        // ====================================================================
        // Navigation
        // ====================================================================

        /// Get parent ID (INVALID_INDEX if root or invalid)
        NodeId parent(NodeId id) const {
            if (!valid(id)) {
                return INVALID_INDEX;
            }
            return nodes_[id].parent;
        }

        /// Get first child ID (INVALID_INDEX if none)
        NodeId first_child(NodeId id) const {
            if (!valid(id)) {
                return INVALID_INDEX;
            }
            return nodes_[id].first_child;
        }

        /// Get next sibling ID (INVALID_INDEX if none)
        NodeId next_sibling(NodeId id) const {
            if (!valid(id)) {
                return INVALID_INDEX;
            }
            return nodes_[id].next_sibling;
        }

        /// Get iterable range of children
        ChildRange children(NodeId id) const {
            if (!valid(id)) {
                return ChildRange(this, INVALID_INDEX);
            }
            return ChildRange(this, nodes_[id].first_child);
        }

        /// Count number of children
        size_type num_children(NodeId id) const {
            if (!valid(id)) {
                return 0;
            }
            size_type count = 0;
            NodeId child = nodes_[id].first_child;
            while (child != INVALID_INDEX) {
                ++count;
                child = nodes_[child].next_sibling;
            }
            return count;
        }

        /// Check if node is a leaf (no children)
        bool is_leaf(NodeId id) const {
            if (!valid(id)) {
                return false;
            }
            return nodes_[id].first_child == INVALID_INDEX;
        }

        /// Check if node is root
        bool is_root(NodeId id) const { return id == root_ && valid(id); }

        // ====================================================================
        // Modifiers
        // ====================================================================

        /// Add child to a node, returns new node ID
        NodeId add_child(NodeId parent_id, T const &value) {
            if (!valid(parent_id)) {
                throw std::out_of_range("NaryTree::add_child: invalid parent ID");
            }

            NodeId new_id = allocate_node(value, parent_id);

            // Add to end of sibling list
            if (nodes_[parent_id].first_child == INVALID_INDEX) {
                nodes_[parent_id].first_child = new_id;
            } else {
                NodeId last = nodes_[parent_id].first_child;
                while (nodes_[last].next_sibling != INVALID_INDEX) {
                    last = nodes_[last].next_sibling;
                }
                nodes_[last].next_sibling = new_id;
            }

            return new_id;
        }

        NodeId add_child(NodeId parent_id, T &&value) {
            if (!valid(parent_id)) {
                throw std::out_of_range("NaryTree::add_child: invalid parent ID");
            }

            NodeId new_id = allocate_node(std::move(value), parent_id);

            if (nodes_[parent_id].first_child == INVALID_INDEX) {
                nodes_[parent_id].first_child = new_id;
            } else {
                NodeId last = nodes_[parent_id].first_child;
                while (nodes_[last].next_sibling != INVALID_INDEX) {
                    last = nodes_[last].next_sibling;
                }
                nodes_[last].next_sibling = new_id;
            }

            return new_id;
        }

        /// Remove a node and its entire subtree
        void remove(NodeId id) {
            if (!valid(id)) {
                return;
            }

            // Recursively remove all children first
            NodeId child = nodes_[id].first_child;
            while (child != INVALID_INDEX) {
                NodeId next = nodes_[child].next_sibling;
                remove(child);
                child = next;
            }

            // Update parent's child list
            NodeId parent_id = nodes_[id].parent;
            if (parent_id != INVALID_INDEX) {
                if (nodes_[parent_id].first_child == id) {
                    nodes_[parent_id].first_child = nodes_[id].next_sibling;
                } else {
                    // Find previous sibling
                    NodeId prev = nodes_[parent_id].first_child;
                    while (prev != INVALID_INDEX && nodes_[prev].next_sibling != id) {
                        prev = nodes_[prev].next_sibling;
                    }
                    if (prev != INVALID_INDEX) {
                        nodes_[prev].next_sibling = nodes_[id].next_sibling;
                    }
                }
            } else {
                // Removing root
                root_ = INVALID_INDEX;
            }

            deallocate_node(id);
        }

        /// Clear the entire tree
        void clear() {
            nodes_.clear();
            free_list_.clear();
            root_ = INVALID_INDEX;
            size_ = 0;
        }

        // ====================================================================
        // Tree metrics
        // ====================================================================

        /// Calculate depth of a node (0 for root)
        int depth(NodeId id) const {
            if (!valid(id)) {
                return -1;
            }
            int d = 0;
            NodeId current = nodes_[id].parent;
            while (current != INVALID_INDEX) {
                ++d;
                current = nodes_[current].parent;
            }
            return d;
        }

        /// Calculate height of subtree rooted at id (0 for leaf)
        int height(NodeId id) const {
            if (!valid(id)) {
                return -1;
            }
            if (nodes_[id].first_child == INVALID_INDEX) {
                return 0;
            }

            int max_child_height = -1;
            NodeId child = nodes_[id].first_child;
            while (child != INVALID_INDEX) {
                int h = height(child);
                if (h > max_child_height) {
                    max_child_height = h;
                }
                child = nodes_[child].next_sibling;
            }
            return 1 + max_child_height;
        }

        /// Calculate height of entire tree
        int height() const { return height(root_); }

        /// Count nodes in subtree rooted at id
        size_type subtree_size(NodeId id) const {
            if (!valid(id)) {
                return 0;
            }
            size_type count = 1;
            NodeId child = nodes_[id].first_child;
            while (child != INVALID_INDEX) {
                count += subtree_size(child);
                child = nodes_[child].next_sibling;
            }
            return count;
        }

        // ====================================================================
        // Traversals
        // ====================================================================

        /// Pre-order (depth-first) traversal: visit node, then children
        template <typename Func> void preorder(Func &&func) const { preorder_impl(root_, std::forward<Func>(func)); }

        template <typename Func> void preorder(NodeId start, Func &&func) const {
            preorder_impl(start, std::forward<Func>(func));
        }

        /// Post-order traversal: visit children, then node
        template <typename Func> void postorder(Func &&func) const { postorder_impl(root_, std::forward<Func>(func)); }

        template <typename Func> void postorder(NodeId start, Func &&func) const {
            postorder_impl(start, std::forward<Func>(func));
        }

        /// Level-order (breadth-first) traversal
        template <typename Func> void levelorder(Func &&func) const {
            if (root_ == INVALID_INDEX) {
                return;
            }

            Deque<NodeId> queue;
            queue.push_back(root_);

            while (!queue.empty()) {
                NodeId current = queue.front();
                queue.pop_front();

                func(nodes_[current].value, current);

                NodeId child = nodes_[current].first_child;
                while (child != INVALID_INDEX) {
                    queue.push_back(child);
                    child = nodes_[child].next_sibling;
                }
            }
        }

        /// Collect values in pre-order
        Vector<T> to_preorder() const {
            Vector<T> result;
            preorder([&result](T const &val, NodeId) { result.push_back(val); });
            return result;
        }

        /// Collect values in post-order
        Vector<T> to_postorder() const {
            Vector<T> result;
            postorder([&result](T const &val, NodeId) { result.push_back(val); });
            return result;
        }

        /// Collect values in level-order
        Vector<T> to_levelorder() const {
            Vector<T> result;
            levelorder([&result](T const &val, NodeId) { result.push_back(val); });
            return result;
        }

        // ====================================================================
        // Serialization support
        // ====================================================================

        auto members() noexcept { return std::tie(nodes_, root_, size_, free_list_); }
        auto members() const noexcept { return std::tie(nodes_, root_, size_, free_list_); }

      private:
        Vector<Node> nodes_;
        size_t root_;
        size_t size_;
        Vector<size_t> free_list_;

        // ====================================================================
        // Node allocation
        // ====================================================================

        NodeId allocate_node(T const &value, size_t parent) {
            NodeId idx;
            if (!free_list_.empty()) {
                idx = free_list_.back();
                free_list_.pop_back();
                nodes_[idx] = Node(value, parent);
            } else {
                idx = nodes_.size();
                nodes_.push_back(Node(value, parent));
            }
            ++size_;
            return idx;
        }

        NodeId allocate_node(T &&value, size_t parent) {
            NodeId idx;
            if (!free_list_.empty()) {
                idx = free_list_.back();
                free_list_.pop_back();
                nodes_[idx] = Node(std::move(value), parent);
            } else {
                idx = nodes_.size();
                nodes_.push_back(Node(std::move(value), parent));
            }
            ++size_;
            return idx;
        }

        void deallocate_node(NodeId idx) {
            free_list_.push_back(idx);
            --size_;
        }

        // ====================================================================
        // Traversal implementations
        // ====================================================================

        template <typename Func> void preorder_impl(NodeId id, Func &&func) const {
            if (!valid(id)) {
                return;
            }
            func(nodes_[id].value, id);

            NodeId child = nodes_[id].first_child;
            while (child != INVALID_INDEX) {
                preorder_impl(child, std::forward<Func>(func));
                child = nodes_[child].next_sibling;
            }
        }

        template <typename Func> void postorder_impl(NodeId id, Func &&func) const {
            if (!valid(id)) {
                return;
            }

            NodeId child = nodes_[id].first_child;
            while (child != INVALID_INDEX) {
                postorder_impl(child, std::forward<Func>(func));
                child = nodes_[child].next_sibling;
            }

            func(nodes_[id].value, id);
        }
    };

} // namespace datapod
