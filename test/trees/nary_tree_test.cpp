#include "datapod/datapod.hpp"
#include <doctest/doctest.h>
#include <string>
#include <vector>

using namespace datapod;

TEST_SUITE("NaryTree") {

    TEST_CASE("Default construction") {
        NaryTree<int> tree;
        CHECK(tree.empty());
        CHECK(tree.size() == 0);
        CHECK_FALSE(tree.has_root());
        CHECK(tree.root() == NaryTree<int>::INVALID_INDEX);
    }

    TEST_CASE("Set root") {
        NaryTree<int> tree;
        auto root = tree.set_root(42);

        CHECK_FALSE(tree.empty());
        CHECK(tree.size() == 1);
        CHECK(tree.has_root());
        CHECK(tree.root() == root);
        CHECK(tree.get(root) == 42);
    }

    TEST_CASE("Replace root value") {
        NaryTree<int> tree;
        auto root = tree.set_root(42);
        tree.set_root(100);

        CHECK(tree.size() == 1);
        CHECK(tree.get(root) == 100);
    }

    TEST_CASE("Add single child") {
        NaryTree<int> tree;
        auto root = tree.set_root(1);
        auto child = tree.add_child(root, 2);

        CHECK(tree.size() == 2);
        CHECK(tree.get(child) == 2);
        CHECK(tree.parent(child) == root);
        CHECK(tree.first_child(root) == child);
        CHECK(tree.num_children(root) == 1);
    }

    TEST_CASE("Add multiple children") {
        NaryTree<int> tree;
        auto root = tree.set_root(1);
        auto c1 = tree.add_child(root, 2);
        auto c2 = tree.add_child(root, 3);
        auto c3 = tree.add_child(root, 4);

        CHECK(tree.size() == 4);
        CHECK(tree.num_children(root) == 3);
        CHECK(tree.first_child(root) == c1);
        CHECK(tree.next_sibling(c1) == c2);
        CHECK(tree.next_sibling(c2) == c3);
        CHECK(tree.next_sibling(c3) == NaryTree<int>::INVALID_INDEX);
    }

    TEST_CASE("Children iterator") {
        NaryTree<int> tree;
        auto root = tree.set_root(1);
        tree.add_child(root, 2);
        tree.add_child(root, 3);
        tree.add_child(root, 4);

        std::vector<int> values;
        for (auto child_id : tree.children(root)) {
            values.push_back(tree.get(child_id));
        }

        CHECK(values.size() == 3);
        CHECK(values[0] == 2);
        CHECK(values[1] == 3);
        CHECK(values[2] == 4);
    }

    TEST_CASE("Build multi-level tree") {
        //        1
        //      / | \
        //     2  3  4
        //    /|     |
        //   5 6     7
        NaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_child(n1, 2);
        auto n3 = tree.add_child(n1, 3);
        auto n4 = tree.add_child(n1, 4);
        tree.add_child(n2, 5);
        tree.add_child(n2, 6);
        tree.add_child(n4, 7);

        CHECK(tree.size() == 7);
        CHECK(tree.num_children(n1) == 3);
        CHECK(tree.num_children(n2) == 2);
        CHECK(tree.num_children(n3) == 0);
        CHECK(tree.num_children(n4) == 1);
    }

    TEST_CASE("Node validation") {
        NaryTree<int> tree;
        auto root = tree.set_root(1);

        CHECK(tree.valid(root));
        CHECK_FALSE(tree.valid(NaryTree<int>::INVALID_INDEX));
        CHECK_FALSE(tree.valid(999));
    }

    TEST_CASE("Is leaf and is root") {
        NaryTree<int> tree;
        auto root = tree.set_root(1);
        auto child = tree.add_child(root, 2);

        CHECK(tree.is_root(root));
        CHECK_FALSE(tree.is_root(child));
        CHECK_FALSE(tree.is_leaf(root));
        CHECK(tree.is_leaf(child));
    }

    TEST_CASE("Get and set values") {
        NaryTree<int> tree;
        auto root = tree.set_root(1);

        CHECK(tree.get(root) == 1);
        tree.set(root, 100);
        CHECK(tree.get(root) == 100);

        tree[root] = 200;
        CHECK(tree[root] == 200);
    }

    TEST_CASE("Remove leaf node") {
        NaryTree<int> tree;
        auto root = tree.set_root(1);
        auto c1 = tree.add_child(root, 2);
        auto c2 = tree.add_child(root, 3);

        tree.remove(c1);

        CHECK(tree.size() == 2);
        CHECK(tree.num_children(root) == 1);
        CHECK(tree.first_child(root) == c2);
        CHECK_FALSE(tree.valid(c1));
    }

    TEST_CASE("Remove middle sibling") {
        NaryTree<int> tree;
        auto root = tree.set_root(1);
        auto c1 = tree.add_child(root, 2);
        auto c2 = tree.add_child(root, 3);
        auto c3 = tree.add_child(root, 4);

        tree.remove(c2);

        CHECK(tree.size() == 3);
        CHECK(tree.num_children(root) == 2);
        CHECK(tree.first_child(root) == c1);
        CHECK(tree.next_sibling(c1) == c3);
    }

    TEST_CASE("Remove subtree") {
        //        1
        //      / | \
        //     2  3  4
        //    /|
        //   5 6
        NaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_child(n1, 2);
        tree.add_child(n1, 3);
        tree.add_child(n1, 4);
        tree.add_child(n2, 5);
        tree.add_child(n2, 6);

        CHECK(tree.size() == 6);

        // Remove node 2 and its subtree (5, 6)
        tree.remove(n2);

        CHECK(tree.size() == 3);
        CHECK(tree.num_children(n1) == 2);
    }

    TEST_CASE("Remove root clears tree") {
        NaryTree<int> tree;
        auto root = tree.set_root(1);
        tree.add_child(root, 2);
        tree.add_child(root, 3);

        tree.remove(root);

        CHECK(tree.empty());
        CHECK_FALSE(tree.has_root());
    }

    TEST_CASE("Clear tree") {
        NaryTree<int> tree;
        auto root = tree.set_root(1);
        tree.add_child(root, 2);
        tree.add_child(root, 3);

        tree.clear();

        CHECK(tree.empty());
        CHECK(tree.size() == 0);
        CHECK_FALSE(tree.has_root());
    }

    TEST_CASE("Depth calculation") {
        NaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_child(n1, 2);
        auto n3 = tree.add_child(n2, 3);

        CHECK(tree.depth(n1) == 0);
        CHECK(tree.depth(n2) == 1);
        CHECK(tree.depth(n3) == 2);
    }

    TEST_CASE("Height calculation") {
        //        1
        //      / | \
        //     2  3  4
        //    /
        //   5
        NaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_child(n1, 2);
        auto n3 = tree.add_child(n1, 3);
        auto n4 = tree.add_child(n1, 4);
        tree.add_child(n2, 5);

        CHECK(tree.height() == 2);
        CHECK(tree.height(n1) == 2);
        CHECK(tree.height(n2) == 1);
        CHECK(tree.height(n3) == 0);
        CHECK(tree.height(n4) == 0);
    }

    TEST_CASE("Subtree size") {
        //        1
        //      / | \
        //     2  3  4
        //    /|
        //   5 6
        NaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_child(n1, 2);
        auto n3 = tree.add_child(n1, 3);
        auto n4 = tree.add_child(n1, 4);
        tree.add_child(n2, 5);
        tree.add_child(n2, 6);

        CHECK(tree.subtree_size(n1) == 6);
        CHECK(tree.subtree_size(n2) == 3);
        CHECK(tree.subtree_size(n3) == 1);
        CHECK(tree.subtree_size(n4) == 1);
    }

    TEST_CASE("Preorder traversal") {
        //        1
        //      / | \
        //     2  3  4
        //    /|
        //   5 6
        NaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_child(n1, 2);
        tree.add_child(n1, 3);
        tree.add_child(n1, 4);
        tree.add_child(n2, 5);
        tree.add_child(n2, 6);

        auto result = tree.to_preorder();
        CHECK(result.size() == 6);
        CHECK(result[0] == 1);
        CHECK(result[1] == 2);
        CHECK(result[2] == 5);
        CHECK(result[3] == 6);
        CHECK(result[4] == 3);
        CHECK(result[5] == 4);
    }

    TEST_CASE("Postorder traversal") {
        //        1
        //      / | \
        //     2  3  4
        //    /|
        //   5 6
        NaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_child(n1, 2);
        tree.add_child(n1, 3);
        tree.add_child(n1, 4);
        tree.add_child(n2, 5);
        tree.add_child(n2, 6);

        auto result = tree.to_postorder();
        CHECK(result.size() == 6);
        CHECK(result[0] == 5);
        CHECK(result[1] == 6);
        CHECK(result[2] == 2);
        CHECK(result[3] == 3);
        CHECK(result[4] == 4);
        CHECK(result[5] == 1);
    }

    TEST_CASE("Levelorder traversal") {
        //        1
        //      / | \
        //     2  3  4
        //    /|
        //   5 6
        NaryTree<int> tree;
        auto n1 = tree.set_root(1);
        auto n2 = tree.add_child(n1, 2);
        tree.add_child(n1, 3);
        tree.add_child(n1, 4);
        tree.add_child(n2, 5);
        tree.add_child(n2, 6);

        auto result = tree.to_levelorder();
        CHECK(result.size() == 6);
        CHECK(result[0] == 1);
        CHECK(result[1] == 2);
        CHECK(result[2] == 3);
        CHECK(result[3] == 4);
        CHECK(result[4] == 5);
        CHECK(result[5] == 6);
    }

    TEST_CASE("Traversal with callback") {
        NaryTree<int> tree;
        auto root = tree.set_root(1);
        tree.add_child(root, 2);
        tree.add_child(root, 3);

        int sum = 0;
        tree.preorder([&sum](int const &val, NaryTree<int>::NodeId) { sum += val; });
        CHECK(sum == 6);
    }

    TEST_CASE("String values") {
        NaryTree<String> tree;
        auto root = tree.set_root(String("root"));
        tree.add_child(root, String("child1"));
        tree.add_child(root, String("child2"));

        CHECK(tree.get(root).view() == "root");
        CHECK(tree.size() == 3);
    }

    TEST_CASE("Copy construction") {
        NaryTree<int> original;
        auto root = original.set_root(1);
        original.add_child(root, 2);
        original.add_child(root, 3);

        NaryTree<int> copy(original);

        CHECK(copy.size() == 3);
        CHECK(copy.get(copy.root()) == 1);

        original.set(root, 100);
        CHECK(copy.get(copy.root()) == 1);
    }

    TEST_CASE("Move construction") {
        NaryTree<int> original;
        auto root = original.set_root(1);
        original.add_child(root, 2);

        NaryTree<int> moved(std::move(original));

        CHECK(moved.size() == 2);
        CHECK(moved.get(moved.root()) == 1);
    }

    TEST_CASE("Serialization roundtrip") {
        NaryTree<int> original;
        auto n1 = original.set_root(1);
        auto n2 = original.add_child(n1, 2);
        original.add_child(n1, 3);
        original.add_child(n2, 4);
        original.add_child(n2, 5);

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, NaryTree<int>>(buffer);

        CHECK(restored.size() == original.size());

        auto orig_pre = original.to_preorder();
        auto rest_pre = restored.to_preorder();
        CHECK(orig_pre.size() == rest_pre.size());
        for (size_t i = 0; i < orig_pre.size(); ++i) {
            CHECK(orig_pre[i] == rest_pre[i]);
        }
    }

    TEST_CASE("Serialization with strings") {
        NaryTree<String> original;
        auto root = original.set_root(String("root"));
        original.add_child(root, String("child1"));
        original.add_child(root, String("child2"));

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, NaryTree<String>>(buffer);

        CHECK(restored.size() == 3);
        CHECK(restored.get(restored.root()).view() == "root");
    }

    TEST_CASE("Node reuse after removal") {
        NaryTree<int> tree;
        auto root = tree.set_root(1);
        auto child = tree.add_child(root, 2);

        tree.remove(child);
        CHECK(tree.size() == 1);

        auto new_child = tree.add_child(root, 10);
        CHECK(tree.size() == 2);
        CHECK(tree.get(new_child) == 10);
    }

    TEST_CASE("Error handling - invalid operations") {
        NaryTree<int> tree;

        CHECK_THROWS_AS(tree.get(0), std::out_of_range);
        CHECK_THROWS_AS(tree.add_child(0, 1), std::out_of_range);
    }

    TEST_CASE("File system tree example") {
        // /
        // ├── home
        // │   ├── user
        // │   └── admin
        // ├── etc
        // └── var
        //     └── log
        NaryTree<String> fs;
        auto root = fs.set_root(String("/"));
        auto home = fs.add_child(root, String("home"));
        auto etc = fs.add_child(root, String("etc"));
        auto var = fs.add_child(root, String("var"));
        fs.add_child(home, String("user"));
        fs.add_child(home, String("admin"));
        fs.add_child(var, String("log"));

        CHECK(fs.size() == 7);
        CHECK(fs.num_children(root) == 3);
        CHECK(fs.num_children(home) == 2);
        CHECK(fs.is_leaf(etc));

        // Find path depth
        auto log_id = fs.first_child(var);
        CHECK(fs.depth(log_id) == 2);
    }
}
