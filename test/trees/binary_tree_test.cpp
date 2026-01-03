#include "datapod/datapod.hpp"
#include <doctest/doctest.h>
#include <string>
#include <vector>

using namespace datapod;

TEST_SUITE("BinaryTree") {

    TEST_CASE("Default construction") {
        BinaryTree<int> tree;
        CHECK(tree.empty());
        CHECK(tree.size() == 0);
        CHECK_FALSE(tree.has_root());
        CHECK(tree.root() == BinaryTree<int>::INVALID_INDEX);
    }

    TEST_CASE("Set root") {
        BinaryTree<int> tree;
        auto root = tree.set_root(42);

        CHECK_FALSE(tree.empty());
        CHECK(tree.size() == 1);
        CHECK(tree.has_root());
        CHECK(tree.root() == root);
        CHECK(tree.get(root) == 42);
    }

    TEST_CASE("Replace root value") {
        BinaryTree<int> tree;
        auto root = tree.set_root(42);
        tree.set_root(100);

        CHECK(tree.size() == 1);
        CHECK(tree.get(root) == 100);
    }

    TEST_CASE("Add left child") {
        BinaryTree<int> tree;
        auto root = tree.set_root(1);
        auto left = tree.add_left(root, 2);

        CHECK(tree.size() == 2);
        CHECK(tree.get(left) == 2);
        CHECK(tree.left(root) == left);
        CHECK(tree.parent(left) == root);
        CHECK(tree.has_left(root));
        CHECK_FALSE(tree.has_right(root));
    }

    TEST_CASE("Add right child") {
        BinaryTree<int> tree;
        auto root = tree.set_root(1);
        auto right = tree.add_right(root, 3);

        CHECK(tree.size() == 2);
        CHECK(tree.get(right) == 3);
        CHECK(tree.right(root) == right);
        CHECK(tree.parent(right) == root);
        CHECK_FALSE(tree.has_left(root));
        CHECK(tree.has_right(root));
    }

    TEST_CASE("Add both children") {
        BinaryTree<int> tree;
        auto root = tree.set_root(1);
        auto left = tree.add_left(root, 2);
        auto right = tree.add_right(root, 3);

        CHECK(tree.size() == 3);
        CHECK(tree.left(root) == left);
        CHECK(tree.right(root) == right);
        CHECK(tree.is_leaf(left));
        CHECK(tree.is_leaf(right));
        CHECK_FALSE(tree.is_leaf(root));
    }

    TEST_CASE("Build complete binary tree") {
        //       1
        //      / \
        //     2   3
        //    / \ / \
        //   4  5 6  7
        BinaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_left(n1, 2);
        auto n3 = tree.add_right(n1, 3);
        auto n4 = tree.add_left(n2, 4);
        auto n5 = tree.add_right(n2, 5);
        auto n6 = tree.add_left(n3, 6);
        auto n7 = tree.add_right(n3, 7);

        CHECK(tree.size() == 7);
        CHECK(tree.height() == 2);
        CHECK(tree.is_leaf(n4));
        CHECK(tree.is_leaf(n5));
        CHECK(tree.is_leaf(n6));
        CHECK(tree.is_leaf(n7));
    }

    TEST_CASE("Node validation") {
        BinaryTree<int> tree;
        auto root = tree.set_root(1);

        CHECK(tree.valid(root));
        CHECK_FALSE(tree.valid(BinaryTree<int>::INVALID_INDEX));
        CHECK_FALSE(tree.valid(999));
    }

    TEST_CASE("Is root check") {
        BinaryTree<int> tree;
        auto root = tree.set_root(1);
        auto left = tree.add_left(root, 2);

        CHECK(tree.is_root(root));
        CHECK_FALSE(tree.is_root(left));
    }

    TEST_CASE("Get and set values") {
        BinaryTree<int> tree;
        auto root = tree.set_root(1);

        CHECK(tree.get(root) == 1);
        tree.set(root, 100);
        CHECK(tree.get(root) == 100);

        // Operator[]
        tree[root] = 200;
        CHECK(tree[root] == 200);
    }

    TEST_CASE("Remove leaf node") {
        BinaryTree<int> tree;
        auto root = tree.set_root(1);
        auto left = tree.add_left(root, 2);
        auto right = tree.add_right(root, 3);

        tree.remove(left);

        CHECK(tree.size() == 2);
        CHECK_FALSE(tree.has_left(root));
        CHECK(tree.has_right(root));
        CHECK_FALSE(tree.valid(left));
    }

    TEST_CASE("Remove subtree") {
        //       1
        //      / \
        //     2   3
        //    / \
        //   4   5
        BinaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_left(n1, 2);
        tree.add_right(n1, 3);
        tree.add_left(n2, 4);
        tree.add_right(n2, 5);

        CHECK(tree.size() == 5);

        // Remove node 2 and its subtree (4, 5)
        tree.remove(n2);

        CHECK(tree.size() == 2);
        CHECK_FALSE(tree.has_left(n1));
        CHECK(tree.has_right(n1));
    }

    TEST_CASE("Remove root clears tree") {
        BinaryTree<int> tree;
        auto root = tree.set_root(1);
        tree.add_left(root, 2);
        tree.add_right(root, 3);

        tree.remove(root);

        CHECK(tree.empty());
        CHECK_FALSE(tree.has_root());
    }

    TEST_CASE("Clear tree") {
        BinaryTree<int> tree;
        auto root = tree.set_root(1);
        tree.add_left(root, 2);
        tree.add_right(root, 3);

        tree.clear();

        CHECK(tree.empty());
        CHECK(tree.size() == 0);
        CHECK_FALSE(tree.has_root());
    }

    TEST_CASE("Height calculation") {
        BinaryTree<int> tree;

        // Empty tree
        CHECK(tree.height() == -1);

        // Single node
        auto root = tree.set_root(1);
        CHECK(tree.height() == 0);

        // One level
        tree.add_left(root, 2);
        CHECK(tree.height() == 1);

        // Two levels (unbalanced)
        auto left = tree.left(root);
        tree.add_left(left, 3);
        CHECK(tree.height() == 2);
    }

    TEST_CASE("Subtree size") {
        //       1
        //      / \
        //     2   3
        //    /
        //   4
        BinaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_left(n1, 2);
        auto n3 = tree.add_right(n1, 3);
        tree.add_left(n2, 4);

        CHECK(tree.subtree_size(n1) == 4);
        CHECK(tree.subtree_size(n2) == 2);
        CHECK(tree.subtree_size(n3) == 1);
    }

    TEST_CASE("Preorder traversal") {
        //       1
        //      / \
        //     2   3
        //    / \
        //   4   5
        BinaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_left(n1, 2);
        tree.add_right(n1, 3);
        tree.add_left(n2, 4);
        tree.add_right(n2, 5);

        auto result = tree.to_preorder();
        CHECK(result.size() == 5);
        CHECK(result[0] == 1);
        CHECK(result[1] == 2);
        CHECK(result[2] == 4);
        CHECK(result[3] == 5);
        CHECK(result[4] == 3);
    }

    TEST_CASE("Inorder traversal") {
        //       1
        //      / \
        //     2   3
        //    / \
        //   4   5
        BinaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_left(n1, 2);
        tree.add_right(n1, 3);
        tree.add_left(n2, 4);
        tree.add_right(n2, 5);

        auto result = tree.to_inorder();
        CHECK(result.size() == 5);
        CHECK(result[0] == 4);
        CHECK(result[1] == 2);
        CHECK(result[2] == 5);
        CHECK(result[3] == 1);
        CHECK(result[4] == 3);
    }

    TEST_CASE("Postorder traversal") {
        //       1
        //      / \
        //     2   3
        //    / \
        //   4   5
        BinaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_left(n1, 2);
        tree.add_right(n1, 3);
        tree.add_left(n2, 4);
        tree.add_right(n2, 5);

        auto result = tree.to_postorder();
        CHECK(result.size() == 5);
        CHECK(result[0] == 4);
        CHECK(result[1] == 5);
        CHECK(result[2] == 2);
        CHECK(result[3] == 3);
        CHECK(result[4] == 1);
    }

    TEST_CASE("Levelorder traversal") {
        //       1
        //      / \
        //     2   3
        //    / \
        //   4   5
        BinaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_left(n1, 2);
        tree.add_right(n1, 3);
        tree.add_left(n2, 4);
        tree.add_right(n2, 5);

        auto result = tree.to_levelorder();
        CHECK(result.size() == 5);
        CHECK(result[0] == 1);
        CHECK(result[1] == 2);
        CHECK(result[2] == 3);
        CHECK(result[3] == 4);
        CHECK(result[4] == 5);
    }

    TEST_CASE("Traversal with callback") {
        BinaryTree<int> tree;
        auto root = tree.set_root(1);
        tree.add_left(root, 2);
        tree.add_right(root, 3);

        int sum = 0;
        tree.preorder([&sum](int const &val, BinaryTree<int>::NodeId) { sum += val; });
        CHECK(sum == 6);
    }

    TEST_CASE("String values") {
        BinaryTree<String> tree;
        auto root = tree.set_root(String("root"));
        tree.add_left(root, String("left"));
        tree.add_right(root, String("right"));

        CHECK(tree.get(root).view() == "root");
        CHECK(tree.size() == 3);
    }

    TEST_CASE("Copy construction") {
        BinaryTree<int> original;
        auto root = original.set_root(1);
        original.add_left(root, 2);
        original.add_right(root, 3);

        BinaryTree<int> copy(original);

        CHECK(copy.size() == 3);
        CHECK(copy.get(copy.root()) == 1);

        // Modify original
        original.set(root, 100);
        CHECK(copy.get(copy.root()) == 1); // Copy unchanged
    }

    TEST_CASE("Move construction") {
        BinaryTree<int> original;
        auto root = original.set_root(1);
        original.add_left(root, 2);

        BinaryTree<int> moved(std::move(original));

        CHECK(moved.size() == 2);
        CHECK(moved.get(moved.root()) == 1);
    }

    TEST_CASE("Serialization roundtrip") {
        BinaryTree<int> original;
        auto n1 = original.set_root(1);
        auto n2 = original.add_left(n1, 2);
        original.add_right(n1, 3);
        original.add_left(n2, 4);
        original.add_right(n2, 5);

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, BinaryTree<int>>(buffer);

        CHECK(restored.size() == original.size());

        auto orig_pre = original.to_preorder();
        auto rest_pre = restored.to_preorder();
        CHECK(orig_pre.size() == rest_pre.size());
        for (size_t i = 0; i < orig_pre.size(); ++i) {
            CHECK(orig_pre[i] == rest_pre[i]);
        }
    }

    TEST_CASE("Serialization with strings") {
        BinaryTree<String> original;
        auto root = original.set_root(String("root"));
        original.add_left(root, String("left"));
        original.add_right(root, String("right"));

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, BinaryTree<String>>(buffer);

        CHECK(restored.size() == 3);
        CHECK(restored.get(restored.root()).view() == "root");
    }

    TEST_CASE("Node reuse after removal") {
        BinaryTree<int> tree;
        auto root = tree.set_root(1);
        auto left = tree.add_left(root, 2);

        tree.remove(left);
        CHECK(tree.size() == 1);

        // Add new node - should reuse freed slot
        auto new_left = tree.add_left(root, 10);
        CHECK(tree.size() == 2);
        CHECK(tree.get(new_left) == 10);
    }

    TEST_CASE("Error handling - invalid operations") {
        BinaryTree<int> tree;

        // Get on invalid ID
        CHECK_THROWS_AS(tree.get(0), std::out_of_range);

        // Add child to invalid parent
        CHECK_THROWS_AS(tree.add_left(0, 1), std::out_of_range);

        auto root = tree.set_root(1);
        tree.add_left(root, 2);

        // Add left child when one exists
        CHECK_THROWS_AS(tree.add_left(root, 3), std::logic_error);
    }

    TEST_CASE("Expression tree example") {
        // Expression: (3 + 4) * 2
        //       *
        //      / \
        //     +   2
        //    / \
        //   3   4
        BinaryTree<String> expr;
        auto mult = expr.set_root(String("*"));
        auto plus = expr.add_left(mult, String("+"));
        expr.add_right(mult, String("2"));
        expr.add_left(plus, String("3"));
        expr.add_right(plus, String("4"));

        // Postfix notation (postorder): 3 4 + 2 *
        auto postfix = expr.to_postorder();
        CHECK(postfix[0].view() == "3");
        CHECK(postfix[1].view() == "4");
        CHECK(postfix[2].view() == "+");
        CHECK(postfix[3].view() == "2");
        CHECK(postfix[4].view() == "*");
    }
}
