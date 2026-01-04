#pragma once

/**
 * @file trees.hpp
 * @brief All tree-based data structures
 *
 * This header includes all tree-based containers for hierarchical
 * and ordered data storage.
 *
 * Includes:
 * - OrderedMap<K,V> - Sorted key-value map (tree-based)
 * - OrderedSet<T> - Sorted unique elements (tree-based)
 * - BinaryTree<T> - General binary tree
 * - NaryTree<T> - N-children tree
 * - Trie<T> - Prefix tree
 */

#include "pods/trees/binary_tree.hpp"
#include "pods/trees/nary_tree.hpp"
#include "pods/trees/ordered_map.hpp"
#include "pods/trees/ordered_set.hpp"
#include "pods/trees/trie.hpp"
