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
     * @brief General-purpose binary tree
     *
     * BinaryTree<T> is a structural binary tree (NOT a search tree) that uses
     * index-based nodes instead of pointers, enabling full serialization via members().
     *
     * Useful for:
     * - Expression trees
     * - Decision trees
     * - Parse trees
     * - Huffman coding trees
     * - Any hierarchical structure with at most 2 children per node
     *
     * @tparam T Value type stored in each node
     *
     * Time Complexity:
     * - add_left, add_right: O(1)
     * - get, set: O(1)
     * - navigation (left, right, parent): O(1)
     * - remove subtree: O(subtree size)
     * - traversals: O(n)
     */
    template <typename T> class BinaryTree {
      public:
        static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

        /// Node handle for external reference
        using NodeId = size_t;

        struct Node {
            T value;
            size_t left;
            size_t right;
            size_t parent;

            Node() : value{}, left{INVALID_INDEX}, right{INVALID_INDEX}, parent{INVALID_INDEX} {}

            explicit Node(T const &v, size_t p = INVALID_INDEX)
                : value{v}, left{INVALID_INDEX}, right{INVALID_INDEX}, parent{p} {}

            explicit Node(T &&v, size_t p = INVALID_INDEX)
                : value{std::move(v)}, left{INVALID_INDEX}, right{INVALID_INDEX}, parent{p} {}

            auto members() noexcept { return std::tie(value, left, right, parent); }
            auto members() const noexcept { return std::tie(value, left, right, parent); }
        };

        using value_type = T;
        using size_type = std::size_t;
        using reference = T &;
        using const_reference = T const &;

        // ====================================================================
        // Construction
        // ====================================================================

        BinaryTree() : nodes_{}, root_{INVALID_INDEX}, size_{0}, free_list_{} {}

        BinaryTree(BinaryTree const &other) = default;
        BinaryTree(BinaryTree &&other) noexcept = default;
        BinaryTree &operator=(BinaryTree const &other) = default;
        BinaryTree &operator=(BinaryTree &&other) noexcept = default;

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
                // Replace existing root value
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
                throw std::out_of_range("BinaryTree::get: invalid node ID");
            }
            return nodes_[id].value;
        }

        T const &get(NodeId id) const {
            if (!valid(id)) {
                throw std::out_of_range("BinaryTree::get: invalid node ID");
            }
            return nodes_[id].value;
        }

        /// Set node value
        void set(NodeId id, T const &value) {
            if (!valid(id)) {
                throw std::out_of_range("BinaryTree::set: invalid node ID");
            }
            nodes_[id].value = value;
        }

        void set(NodeId id, T &&value) {
            if (!valid(id)) {
                throw std::out_of_range("BinaryTree::set: invalid node ID");
            }
            nodes_[id].value = std::move(value);
        }

        /// Operator[] for convenient access
        T &operator[](NodeId id) { return get(id); }
        T const &operator[](NodeId id) const { return get(id); }

        // ====================================================================
        // Navigation
        // ====================================================================

        /// Get left child ID (INVALID_INDEX if none)
        NodeId left(NodeId id) const {
            if (!valid(id)) {
                return INVALID_INDEX;
            }
            return nodes_[id].left;
        }

        /// Get right child ID (INVALID_INDEX if none)
        NodeId right(NodeId id) const {
            if (!valid(id)) {
                return INVALID_INDEX;
            }
            return nodes_[id].right;
        }

        /// Get parent ID (INVALID_INDEX if root or invalid)
        NodeId parent(NodeId id) const {
            if (!valid(id)) {
                return INVALID_INDEX;
            }
            return nodes_[id].parent;
        }

        /// Check if node is a leaf (no children)
        bool is_leaf(NodeId id) const {
            if (!valid(id)) {
                return false;
            }
            return nodes_[id].left == INVALID_INDEX && nodes_[id].right == INVALID_INDEX;
        }

        /// Check if node has left child
        bool has_left(NodeId id) const {
            if (!valid(id)) {
                return false;
            }
            return nodes_[id].left != INVALID_INDEX;
        }

        /// Check if node has right child
        bool has_right(NodeId id) const {
            if (!valid(id)) {
                return false;
            }
            return nodes_[id].right != INVALID_INDEX;
        }

        /// Check if node is root
        bool is_root(NodeId id) const { return id == root_ && valid(id); }

        // ====================================================================
        // Modifiers
        // ====================================================================

        /// Add left child to a node, returns new node ID
        NodeId add_left(NodeId parent_id, T const &value) {
            if (!valid(parent_id)) {
                throw std::out_of_range("BinaryTree::add_left: invalid parent ID");
            }
            if (nodes_[parent_id].left != INVALID_INDEX) {
                throw std::logic_error("BinaryTree::add_left: node already has left child");
            }

            NodeId new_id = allocate_node(value, parent_id);
            nodes_[parent_id].left = new_id;
            return new_id;
        }

        NodeId add_left(NodeId parent_id, T &&value) {
            if (!valid(parent_id)) {
                throw std::out_of_range("BinaryTree::add_left: invalid parent ID");
            }
            if (nodes_[parent_id].left != INVALID_INDEX) {
                throw std::logic_error("BinaryTree::add_left: node already has left child");
            }

            NodeId new_id = allocate_node(std::move(value), parent_id);
            nodes_[parent_id].left = new_id;
            return new_id;
        }

        /// Add right child to a node, returns new node ID
        NodeId add_right(NodeId parent_id, T const &value) {
            if (!valid(parent_id)) {
                throw std::out_of_range("BinaryTree::add_right: invalid parent ID");
            }
            if (nodes_[parent_id].right != INVALID_INDEX) {
                throw std::logic_error("BinaryTree::add_right: node already has right child");
            }

            NodeId new_id = allocate_node(value, parent_id);
            nodes_[parent_id].right = new_id;
            return new_id;
        }

        NodeId add_right(NodeId parent_id, T &&value) {
            if (!valid(parent_id)) {
                throw std::out_of_range("BinaryTree::add_right: invalid parent ID");
            }
            if (nodes_[parent_id].right != INVALID_INDEX) {
                throw std::logic_error("BinaryTree::add_right: node already has right child");
            }

            NodeId new_id = allocate_node(std::move(value), parent_id);
            nodes_[parent_id].right = new_id;
            return new_id;
        }

        /// Remove a node and its entire subtree
        void remove(NodeId id) {
            if (!valid(id)) {
                return;
            }

            // Recursively remove children first
            if (nodes_[id].left != INVALID_INDEX) {
                remove(nodes_[id].left);
            }
            if (nodes_[id].right != INVALID_INDEX) {
                remove(nodes_[id].right);
            }

            // Update parent's child pointer
            NodeId parent_id = nodes_[id].parent;
            if (parent_id != INVALID_INDEX) {
                if (nodes_[parent_id].left == id) {
                    nodes_[parent_id].left = INVALID_INDEX;
                } else if (nodes_[parent_id].right == id) {
                    nodes_[parent_id].right = INVALID_INDEX;
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

        /// Calculate height of subtree rooted at id (0 for leaf, -1 for invalid)
        int height(NodeId id) const {
            if (!valid(id)) {
                return -1;
            }
            int left_height = height(nodes_[id].left);
            int right_height = height(nodes_[id].right);
            return 1 + (left_height > right_height ? left_height : right_height);
        }

        /// Calculate height of entire tree
        int height() const { return height(root_); }

        /// Count nodes in subtree rooted at id
        size_type subtree_size(NodeId id) const {
            if (!valid(id)) {
                return 0;
            }
            return 1 + subtree_size(nodes_[id].left) + subtree_size(nodes_[id].right);
        }

        // ====================================================================
        // Traversals
        // ====================================================================

        /// Pre-order traversal: visit node, then left, then right
        template <typename Func> void preorder(Func &&func) const { preorder_impl(root_, std::forward<Func>(func)); }

        template <typename Func> void preorder(NodeId start, Func &&func) const {
            preorder_impl(start, std::forward<Func>(func));
        }

        /// In-order traversal: visit left, then node, then right
        template <typename Func> void inorder(Func &&func) const { inorder_impl(root_, std::forward<Func>(func)); }

        template <typename Func> void inorder(NodeId start, Func &&func) const {
            inorder_impl(start, std::forward<Func>(func));
        }

        /// Post-order traversal: visit left, then right, then node
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

                if (nodes_[current].left != INVALID_INDEX) {
                    queue.push_back(nodes_[current].left);
                }
                if (nodes_[current].right != INVALID_INDEX) {
                    queue.push_back(nodes_[current].right);
                }
            }
        }

        /// Collect values in pre-order
        Vector<T> to_preorder() const {
            Vector<T> result;
            preorder([&result](T const &val, NodeId) { result.push_back(val); });
            return result;
        }

        /// Collect values in in-order
        Vector<T> to_inorder() const {
            Vector<T> result;
            inorder([&result](T const &val, NodeId) { result.push_back(val); });
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
            preorder_impl(nodes_[id].left, std::forward<Func>(func));
            preorder_impl(nodes_[id].right, std::forward<Func>(func));
        }

        template <typename Func> void inorder_impl(NodeId id, Func &&func) const {
            if (!valid(id)) {
                return;
            }
            inorder_impl(nodes_[id].left, std::forward<Func>(func));
            func(nodes_[id].value, id);
            inorder_impl(nodes_[id].right, std::forward<Func>(func));
        }

        template <typename Func> void postorder_impl(NodeId id, Func &&func) const {
            if (!valid(id)) {
                return;
            }
            postorder_impl(nodes_[id].left, std::forward<Func>(func));
            postorder_impl(nodes_[id].right, std::forward<Func>(func));
            func(nodes_[id].value, id);
        }
    };

} // namespace datapod
