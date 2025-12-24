#include <doctest/doctest.h>

#include <datapod/spatial/quadtree.hpp>

using namespace datapod;

// ============================================================================
// QuadTree Construction Tests
// ============================================================================

TEST_CASE("QuadTree - Default construction with boundary") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);
    CHECK(tree.empty());
    CHECK(tree.size() == 0);
}

TEST_CASE("QuadTree - Boundary access") {
    AABB boundary{Point{-10.0, -10.0, 0.0}, Point{10.0, 10.0, 0.0}};
    QuadTree<int> tree(boundary);
    CHECK(tree.boundary() == boundary);
}

// ============================================================================
// Insert Tests
// ============================================================================

TEST_CASE("QuadTree - Insert single point") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    bool inserted = tree.insert(Point{50.0, 50.0, 0.0}, 42);
    CHECK(inserted);
    CHECK_FALSE(tree.empty());
    CHECK(tree.size() == 1);
}

TEST_CASE("QuadTree - Insert multiple points") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{10.0, 10.0, 0.0}, 1);
    tree.insert(Point{20.0, 20.0, 0.0}, 2);
    tree.insert(Point{30.0, 30.0, 0.0}, 3);

    CHECK(tree.size() == 3);
}

TEST_CASE("QuadTree - Insert point outside boundary") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    bool inserted = tree.insert(Point{150.0, 150.0, 0.0}, 42);
    CHECK_FALSE(inserted);
    CHECK(tree.size() == 0);
}

TEST_CASE("QuadTree - Insert at boundary edges") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    // Test corners
    CHECK(tree.insert(Point{0.0, 0.0, 0.0}, 1));
    CHECK(tree.insert(Point{100.0, 100.0, 0.0}, 2));
    CHECK(tree.insert(Point{0.0, 100.0, 0.0}, 3));
    CHECK(tree.insert(Point{100.0, 0.0, 0.0}, 4));

    CHECK(tree.size() == 4);
}

TEST_CASE("QuadTree - Insert triggers subdivision") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int, 4> tree(boundary); // Small capacity to force subdivision

    // Insert more than capacity
    for (int i = 0; i < 20; ++i) {
        double x = 10.0 + i * 2.0;
        double y = 10.0 + i * 2.0;
        tree.insert(Point{x, y, 0.0}, i);
    }

    CHECK(tree.size() == 20);
}

TEST_CASE("QuadTree - Insert using Entry struct") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    QuadTree<int>::Entry entry{Point{50.0, 50.0, 0.0}, 42};
    bool inserted = tree.insert(entry);

    CHECK(inserted);
    CHECK(tree.size() == 1);
}

// ============================================================================
// Query (Range) Tests
// ============================================================================

TEST_CASE("QuadTree - query - empty tree") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    AABB query_range{Point{10.0, 10.0, 0.0}, Point{20.0, 20.0, 0.0}};
    auto results = tree.query(query_range);

    CHECK(results.empty());
}

TEST_CASE("QuadTree - query - exact match") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{15.0, 15.0, 0.0}, 42);

    AABB query_range{Point{10.0, 10.0, 0.0}, Point{20.0, 20.0, 0.0}};
    auto results = tree.query(query_range);

    CHECK(results.size() == 1);
    CHECK(results[0].data == 42);
}

TEST_CASE("QuadTree - query - multiple points in range") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{10.0, 10.0, 0.0}, 1);
    tree.insert(Point{15.0, 15.0, 0.0}, 2);
    tree.insert(Point{20.0, 20.0, 0.0}, 3);
    tree.insert(Point{50.0, 50.0, 0.0}, 4); // Outside range

    AABB query_range{Point{5.0, 5.0, 0.0}, Point{25.0, 25.0, 0.0}};
    auto results = tree.query(query_range);

    CHECK(results.size() == 3);
}

TEST_CASE("QuadTree - query - no points in range") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{10.0, 10.0, 0.0}, 1);
    tree.insert(Point{15.0, 15.0, 0.0}, 2);

    AABB query_range{Point{50.0, 50.0, 0.0}, Point{60.0, 60.0, 0.0}};
    auto results = tree.query(query_range);

    CHECK(results.empty());
}

TEST_CASE("QuadTree - query - across quadrants") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int, 2> tree(boundary); // Force subdivision

    // Insert in all quadrants
    tree.insert(Point{25.0, 75.0, 0.0}, 1); // NW
    tree.insert(Point{75.0, 75.0, 0.0}, 2); // NE
    tree.insert(Point{25.0, 25.0, 0.0}, 3); // SW
    tree.insert(Point{75.0, 25.0, 0.0}, 4); // SE

    // Query spanning all quadrants
    AABB query_range{Point{20.0, 20.0, 0.0}, Point{80.0, 80.0, 0.0}};
    auto results = tree.query(query_range);

    CHECK(results.size() == 4);
}

// ============================================================================
// Query Radius Tests
// ============================================================================

TEST_CASE("QuadTree - query_radius - empty tree") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    auto results = tree.query_radius(Point{50.0, 50.0, 0.0}, 10.0);
    CHECK(results.empty());
}

TEST_CASE("QuadTree - query_radius - single point within radius") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{50.0, 50.0, 0.0}, 42);

    auto results = tree.query_radius(Point{50.0, 50.0, 0.0}, 10.0);
    CHECK(results.size() == 1);
    CHECK(results[0].data == 42);
}

TEST_CASE("QuadTree - query_radius - multiple points within radius") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{50.0, 50.0, 0.0}, 1);
    tree.insert(Point{51.0, 51.0, 0.0}, 2);
    tree.insert(Point{52.0, 52.0, 0.0}, 3);
    tree.insert(Point{80.0, 80.0, 0.0}, 4); // Outside radius

    auto results = tree.query_radius(Point{50.0, 50.0, 0.0}, 5.0);
    CHECK(results.size() == 3);
}

TEST_CASE("QuadTree - query_radius - point exactly at radius") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{55.0, 50.0, 0.0}, 42);

    // Distance is exactly 5.0
    auto results = tree.query_radius(Point{50.0, 50.0, 0.0}, 5.0);
    CHECK(results.size() == 1);
}

TEST_CASE("QuadTree - query_radius - no points within radius") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{10.0, 10.0, 0.0}, 1);
    tree.insert(Point{90.0, 90.0, 0.0}, 2);

    auto results = tree.query_radius(Point{50.0, 50.0, 0.0}, 5.0);
    CHECK(results.empty());
}

// ============================================================================
// k-Nearest Neighbor Tests
// ============================================================================

TEST_CASE("QuadTree - k_nearest - empty tree") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    auto results = tree.k_nearest(Point{50.0, 50.0, 0.0}, 3);
    CHECK(results.empty());
}

TEST_CASE("QuadTree - k_nearest - k=1") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{10.0, 10.0, 0.0}, 1);
    tree.insert(Point{50.0, 50.0, 0.0}, 2);
    tree.insert(Point{90.0, 90.0, 0.0}, 3);

    auto results = tree.k_nearest(Point{12.0, 12.0, 0.0}, 1);
    CHECK(results.size() == 1);
    CHECK(results[0].data == 1); // Closest to (10, 10)
}

TEST_CASE("QuadTree - k_nearest - k=3") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{10.0, 10.0, 0.0}, 1);
    tree.insert(Point{20.0, 20.0, 0.0}, 2);
    tree.insert(Point{30.0, 30.0, 0.0}, 3);
    tree.insert(Point{90.0, 90.0, 0.0}, 4);

    auto results = tree.k_nearest(Point{0.0, 0.0, 0.0}, 3);
    CHECK(results.size() == 3);
    // Results should be sorted by distance
    CHECK(results[0].data == 1);
    CHECK(results[1].data == 2);
    CHECK(results[2].data == 3);
}

TEST_CASE("QuadTree - k_nearest - k larger than size") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{10.0, 10.0, 0.0}, 1);
    tree.insert(Point{20.0, 20.0, 0.0}, 2);

    auto results = tree.k_nearest(Point{0.0, 0.0, 0.0}, 10);
    // Should return all entries
    CHECK(results.size() == 2);
}

TEST_CASE("QuadTree - k_nearest - query at exact point") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{50.0, 50.0, 0.0}, 42);
    tree.insert(Point{60.0, 60.0, 0.0}, 99);

    auto results = tree.k_nearest(Point{50.0, 50.0, 0.0}, 1);
    CHECK(results.size() == 1);
    CHECK(results[0].data == 42); // Distance = 0
}

// ============================================================================
// Remove Tests
// ============================================================================

TEST_CASE("QuadTree - remove - existing entry") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{50.0, 50.0, 0.0}, 42);
    CHECK(tree.size() == 1);

    bool removed = tree.remove(Point{50.0, 50.0, 0.0}, 42);
    CHECK(removed);
    CHECK(tree.size() == 0);
}

TEST_CASE("QuadTree - remove - non-existing entry") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{50.0, 50.0, 0.0}, 42);

    bool removed = tree.remove(Point{60.0, 60.0, 0.0}, 99);
    CHECK_FALSE(removed);
    CHECK(tree.size() == 1);
}

TEST_CASE("QuadTree - remove - same point different data") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{50.0, 50.0, 0.0}, 42);

    bool removed = tree.remove(Point{50.0, 50.0, 0.0}, 99);
    CHECK_FALSE(removed);
    CHECK(tree.size() == 1);
}

TEST_CASE("QuadTree - remove - from subdivided tree") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int, 2> tree(boundary); // Force subdivision

    tree.insert(Point{25.0, 75.0, 0.0}, 1); // NW
    tree.insert(Point{75.0, 75.0, 0.0}, 2); // NE
    tree.insert(Point{25.0, 25.0, 0.0}, 3); // SW
    tree.insert(Point{75.0, 25.0, 0.0}, 4); // SE

    CHECK(tree.size() == 4);

    bool removed = tree.remove(Point{75.0, 75.0, 0.0}, 2);
    CHECK(removed);
    CHECK(tree.size() == 3);
}

TEST_CASE("QuadTree - remove - using Entry struct") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    QuadTree<int>::Entry entry{Point{50.0, 50.0, 0.0}, 42};
    tree.insert(entry);

    bool removed = tree.remove(entry);
    CHECK(removed);
    CHECK(tree.empty());
}

// ============================================================================
// Clear Test
// ============================================================================

TEST_CASE("QuadTree - clear") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{10.0, 10.0, 0.0}, 1);
    tree.insert(Point{20.0, 20.0, 0.0}, 2);
    tree.insert(Point{30.0, 30.0, 0.0}, 3);

    CHECK(tree.size() == 3);

    tree.clear();
    CHECK(tree.empty());
    CHECK(tree.size() == 0);
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST_CASE("QuadTree - iterators - empty tree") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    int count = 0;
    for (const auto &entry : tree) {
        (void)entry;
        count++;
    }

    CHECK(count == 0);
}

TEST_CASE("QuadTree - iterators - non-empty tree") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{10.0, 10.0, 0.0}, 1);
    tree.insert(Point{20.0, 20.0, 0.0}, 2);
    tree.insert(Point{30.0, 30.0, 0.0}, 3);

    int count = 0;
    for (const auto &entry : tree) {
        (void)entry;
        count++;
    }

    CHECK(count == 3);
}

TEST_CASE("QuadTree - begin/end") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{50.0, 50.0, 0.0}, 42);

    auto it = tree.begin();
    CHECK(it != tree.end());
    CHECK(it->data == 42);
}

TEST_CASE("QuadTree - iterator traversal") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{10.0, 10.0, 0.0}, 1);
    tree.insert(Point{20.0, 20.0, 0.0}, 2);
    tree.insert(Point{30.0, 30.0, 0.0}, 3);

    Vector<int> collected_data;
    for (const auto &entry : tree) {
        collected_data.push_back(entry.data);
    }

    CHECK(collected_data.size() == 3);
}

// ============================================================================
// Serialization Test
// ============================================================================

TEST_CASE("QuadTree - members for serialization") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{50.0, 50.0, 0.0}, 42);

    // Test that members() is callable
    auto members = tree.members();
    (void)members; // Suppress unused warning

    // Test Entry members
    QuadTree<int>::Entry entry{Point{1.0, 2.0, 3.0}, 99};
    auto entry_members = entry.members();
    (void)entry_members;
}

// ============================================================================
// Entry Comparison Tests
// ============================================================================

TEST_CASE("QuadTree::Entry - equality operator") {
    QuadTree<int>::Entry e1{Point{1.0, 2.0, 3.0}, 42};
    QuadTree<int>::Entry e2{Point{1.0, 2.0, 3.0}, 42};
    QuadTree<int>::Entry e3{Point{1.0, 2.0, 3.0}, 99};
    QuadTree<int>::Entry e4{Point{4.0, 5.0, 6.0}, 42};

    CHECK(e1 == e2);
    CHECK_FALSE(e1 == e3); // Different data
    CHECK_FALSE(e1 == e4); // Different point
}

TEST_CASE("QuadTree::Entry - less-than operator") {
    QuadTree<int>::Entry e1{Point{1.0, 2.0, 3.0}, 42};
    QuadTree<int>::Entry e2{Point{2.0, 2.0, 3.0}, 42};
    QuadTree<int>::Entry e3{Point{1.0, 3.0, 3.0}, 42};
    QuadTree<int>::Entry e4{Point{1.0, 2.0, 4.0}, 42};

    CHECK(e1 < e2); // x differs
    CHECK(e1 < e3); // y differs
    CHECK(e1 < e4); // z differs
}

// ============================================================================
// Edge Cases and Stress Tests
// ============================================================================

TEST_CASE("QuadTree - large number of insertions") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{1000.0, 1000.0, 0.0}};
    QuadTree<int> tree(boundary);

    for (int i = 0; i < 100; ++i) {
        double x = (i % 10) * 100.0 + 50.0;
        double y = (i / 10) * 100.0 + 50.0;
        tree.insert(Point{x, y, 0.0}, i);
    }

    CHECK(tree.size() == 100);
}

TEST_CASE("QuadTree - duplicate points different data") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    tree.insert(Point{50.0, 50.0, 0.0}, 1);
    tree.insert(Point{50.0, 50.0, 0.0}, 2);

    CHECK(tree.size() == 2); // Both should be stored
}

TEST_CASE("QuadTree - query after multiple operations") {
    AABB boundary{Point{0.0, 0.0, 0.0}, Point{100.0, 100.0, 0.0}};
    QuadTree<int> tree(boundary);

    // Insert
    tree.insert(Point{10.0, 10.0, 0.0}, 1);
    tree.insert(Point{20.0, 20.0, 0.0}, 2);
    tree.insert(Point{30.0, 30.0, 0.0}, 3);

    // Remove one
    tree.remove(Point{20.0, 20.0, 0.0}, 2);

    // Insert more
    tree.insert(Point{40.0, 40.0, 0.0}, 4);

    // Query
    AABB query_range{Point{0.0, 0.0, 0.0}, Point{50.0, 50.0, 0.0}};
    auto results = tree.query(query_range);

    CHECK(results.size() == 3); // 1, 3, 4 (2 was removed)
}
