#pragma once

#include <cstddef>
#include <string_view>
#include <tuple>
#include <utility>

#include "datapod/pods/adapters/optional.hpp"
#include "datapod/pods/associative/map.hpp"
#include "datapod/pods/sequential/string.hpp"
#include "datapod/pods/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief Prefix tree (Trie) for string/sequence operations
     *
     * Trie<T> is a prefix tree that uses index-based nodes instead of pointers,
     * enabling full serialization via members().
     *
     * Useful for:
     * - Autocomplete/typeahead
     * - Spell checking
     * - IP routing tables
     * - Dictionary implementations
     * - Prefix matching
     *
     * @tparam T Value type associated with each key (use bool for set-like behavior)
     *
     * Time Complexity (where k = key length):
     * - insert: O(k)
     * - find: O(k)
     * - contains: O(k)
     * - erase: O(k)
     * - starts_with: O(prefix length)
     * - autocomplete: O(prefix length + result size)
     */
    template <typename T> class Trie {
      public:
        static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

        using NodeId = size_t;

        struct Node {
            Map<char, size_t> children; // char -> child node index
            bool is_end;                // marks end of a word
            Optional<T> value;          // associated value

            Node() : children{}, is_end{false}, value{} {}

            auto members() noexcept { return std::tie(children, is_end, value); }
            auto members() const noexcept { return std::tie(children, is_end, value); }
        };

        using value_type = T;
        using size_type = std::size_t;

        // ====================================================================
        // Construction
        // ====================================================================

        Trie() : nodes_{}, root_{INVALID_INDEX}, size_{0} {
            // Always create root node
            root_ = allocate_node();
        }

        Trie(Trie const &other) = default;
        Trie(Trie &&other) noexcept = default;
        Trie &operator=(Trie const &other) = default;
        Trie &operator=(Trie &&other) noexcept = default;

        // ====================================================================
        // Capacity
        // ====================================================================

        bool empty() const noexcept { return size_ == 0; }
        size_type size() const noexcept { return size_; }

        // ====================================================================
        // Modifiers
        // ====================================================================

        /// Insert a key-value pair
        void insert(std::string_view key, T const &value) {
            NodeId node = root_;

            for (char c : key) {
                auto it = nodes_[node].children.find(c);
                if (it == nodes_[node].children.end()) {
                    NodeId new_node = allocate_node();
                    nodes_[node].children[c] = new_node;
                    node = new_node;
                } else {
                    node = it->second;
                }
            }

            if (!nodes_[node].is_end) {
                ++size_;
            }
            nodes_[node].is_end = true;
            nodes_[node].value = value;
        }

        void insert(std::string_view key, T &&value) {
            NodeId node = root_;

            for (char c : key) {
                auto it = nodes_[node].children.find(c);
                if (it == nodes_[node].children.end()) {
                    NodeId new_node = allocate_node();
                    nodes_[node].children[c] = new_node;
                    node = new_node;
                } else {
                    node = it->second;
                }
            }

            if (!nodes_[node].is_end) {
                ++size_;
            }
            nodes_[node].is_end = true;
            nodes_[node].value = std::move(value);
        }

        /// Insert a key (for set-like behavior, value defaults to T{})
        void insert(std::string_view key) { insert(key, T{}); }

        /// Erase a key, returns true if key existed
        bool erase(std::string_view key) {
            NodeId node = root_;

            for (char c : key) {
                auto it = nodes_[node].children.find(c);
                if (it == nodes_[node].children.end()) {
                    return false;
                }
                node = it->second;
            }

            if (!nodes_[node].is_end) {
                return false;
            }

            nodes_[node].is_end = false;
            nodes_[node].value.reset();
            --size_;
            return true;
        }

        /// Clear all entries
        void clear() {
            nodes_.clear();
            size_ = 0;
            root_ = allocate_node();
        }

        // ====================================================================
        // Lookup
        // ====================================================================

        /// Check if key exists
        bool contains(std::string_view key) const {
            NodeId node = find_node(key);
            return node != INVALID_INDEX && nodes_[node].is_end;
        }

        /// Find value associated with key
        Optional<T> find(std::string_view key) const {
            NodeId node = find_node(key);
            if (node == INVALID_INDEX || !nodes_[node].is_end) {
                return Optional<T>{};
            }
            return nodes_[node].value;
        }

        /// Get value reference (throws if not found)
        T &at(std::string_view key) {
            NodeId node = find_node(key);
            if (node == INVALID_INDEX || !nodes_[node].is_end) {
                throw std::out_of_range("Trie::at: key not found");
            }
            return nodes_[node].value.value();
        }

        T const &at(std::string_view key) const {
            NodeId node = find_node(key);
            if (node == INVALID_INDEX || !nodes_[node].is_end) {
                throw std::out_of_range("Trie::at: key not found");
            }
            return nodes_[node].value.value();
        }

        /// Check if any key starts with prefix
        bool starts_with(std::string_view prefix) const { return find_node(prefix) != INVALID_INDEX; }

        /// Get all keys that start with prefix
        Vector<String> autocomplete(std::string_view prefix) const {
            Vector<String> results;
            NodeId node = find_node(prefix);

            if (node == INVALID_INDEX) {
                return results;
            }

            String current_prefix;
            for (char c : prefix) {
                current_prefix.push_back(c);
            }

            collect_keys(node, current_prefix, results);
            return results;
        }

        /// Get all keys in the trie
        Vector<String> keys() const { return autocomplete(""); }

        // ====================================================================
        // Serialization support
        // ====================================================================

        auto members() noexcept { return std::tie(nodes_, root_, size_); }
        auto members() const noexcept { return std::tie(nodes_, root_, size_); }

      private:
        Vector<Node> nodes_;
        size_t root_;
        size_t size_;

        // ====================================================================
        // Helpers
        // ====================================================================

        NodeId allocate_node() {
            NodeId idx = nodes_.size();
            nodes_.push_back(Node{});
            return idx;
        }

        /// Find node for a key/prefix, returns INVALID_INDEX if not found
        NodeId find_node(std::string_view key) const {
            if (root_ == INVALID_INDEX) {
                return INVALID_INDEX;
            }

            NodeId node = root_;
            for (char c : key) {
                auto it = nodes_[node].children.find(c);
                if (it == nodes_[node].children.end()) {
                    return INVALID_INDEX;
                }
                node = it->second;
            }
            return node;
        }

        /// Collect all keys from a node (DFS)
        void collect_keys(NodeId node, String &current, Vector<String> &results) const {
            if (nodes_[node].is_end) {
                results.push_back(current);
            }

            for (auto it = nodes_[node].children.begin(); it != nodes_[node].children.end(); ++it) {
                auto [c, child] = *it;
                current.push_back(c);
                collect_keys(child, current, results);
                current.pop_back();
            }
        }
    };

    /// Convenience alias for set-like trie (just stores keys, no values)
    using TrieSet = Trie<bool>;

} // namespace datapod
