#include <doctest/doctest.h>

#include <datapod/spatial/rtree.hpp>

using namespace datapod;

// ============================================================================
// RTree (AABB-based) Tests
// ============================================================================

TEST_CASE("RTree - Default construction") {
    RTree<int> tree;
    CHECK(tree.empty());
    CHECK(tree.size() == 0);
}

TEST_CASE("RTree - Insert single entry") {
    RTree<int> tree;
    AABB bounds{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}};
    tree.insert(bounds, 42);

    CHECK_FALSE(tree.empty());
    CHECK(tree.size() == 1);
}

TEST_CASE("RTree - Insert multiple entries") {
    RTree<int> tree;

    tree.insert(AABB{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}}, 1);
    tree.insert(AABB{Point{2.0, 2.0, 2.0}, Point{3.0, 3.0, 3.0}}, 2);
    tree.insert(AABB{Point{4.0, 4.0, 4.0}, Point{5.0, 5.0, 5.0}}, 3);

    CHECK(tree.size() == 3);
}

TEST_CASE("RTree - query_intersects - exact match") {
    RTree<int> tree;
    AABB bounds{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}};
    tree.insert(bounds, 42);

    auto results = tree.query_intersects(bounds);
    CHECK(results.size() == 1);
    CHECK(results[0].data == 42);
}

TEST_CASE("RTree - query_intersects - overlapping boxes") {
    RTree<int> tree;

    tree.insert(AABB{Point{0.0, 0.0, 0.0}, Point{2.0, 2.0, 2.0}}, 1);
    tree.insert(AABB{Point{1.0, 1.0, 1.0}, Point{3.0, 3.0, 3.0}}, 2);
    tree.insert(AABB{Point{5.0, 5.0, 5.0}, Point{6.0, 6.0, 6.0}}, 3);

    // Query box overlaps first two
    AABB query{Point{1.5, 1.5, 1.5}, Point{2.5, 2.5, 2.5}};
    auto results = tree.query_intersects(query);

    CHECK(results.size() == 2);
}

TEST_CASE("RTree - query_intersects - no overlap") {
    RTree<int> tree;

    tree.insert(AABB{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}}, 1);
    tree.insert(AABB{Point{10.0, 10.0, 10.0}, Point{11.0, 11.0, 11.0}}, 2);

    // Query box far away
    AABB query{Point{50.0, 50.0, 50.0}, Point{51.0, 51.0, 51.0}};
    auto results = tree.query_intersects(query);

    CHECK(results.empty());
}

TEST_CASE("RTree - search alias for query_intersects") {
    RTree<int> tree;
    tree.insert(AABB{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}}, 42);

    auto results = tree.search(AABB{Point{0.5, 0.5, 0.5}, Point{1.5, 1.5, 1.5}});
    CHECK(results.size() == 1);
}

TEST_CASE("RTree - query_nearest - single point") {
    RTree<int> tree;

    tree.insert(AABB{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}}, 1);
    tree.insert(AABB{Point{10.0, 10.0, 10.0}, Point{11.0, 11.0, 11.0}}, 2);
    tree.insert(AABB{Point{5.0, 5.0, 5.0}, Point{6.0, 6.0, 6.0}}, 3);

    Point query{0.5, 0.5, 0.5};
    auto results = tree.query_nearest(query, 1);

    CHECK(results.size() == 1);
    CHECK(results[0].data == 1); // Closest to query point
}

TEST_CASE("RTree - query_nearest - k=3") {
    RTree<int> tree;

    tree.insert(AABB{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}}, 1);
    tree.insert(AABB{Point{2.0, 2.0, 2.0}, Point{3.0, 3.0, 3.0}}, 2);
    tree.insert(AABB{Point{4.0, 4.0, 4.0}, Point{5.0, 5.0, 5.0}}, 3);
    tree.insert(AABB{Point{10.0, 10.0, 10.0}, Point{11.0, 11.0, 11.0}}, 4);

    Point query{0.0, 0.0, 0.0};
    auto results = tree.query_nearest(query, 3);

    CHECK(results.size() == 3);
    // Results should be sorted by distance
    CHECK(results[0].data == 1);
    CHECK(results[1].data == 2);
    CHECK(results[2].data == 3);
}

TEST_CASE("RTree - query_nearest - k larger than size") {
    RTree<int> tree;

    tree.insert(AABB{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}}, 1);
    tree.insert(AABB{Point{2.0, 2.0, 2.0}, Point{3.0, 3.0, 3.0}}, 2);

    Point query{0.0, 0.0, 0.0};
    auto results = tree.query_nearest(query, 10);

    // Should return all entries
    CHECK(results.size() == 2);
}

TEST_CASE("RTree - query_radius - within radius") {
    RTree<int> tree;

    tree.insert(AABB{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}}, 1);
    tree.insert(AABB{Point{2.0, 2.0, 2.0}, Point{3.0, 3.0, 3.0}}, 2);
    tree.insert(AABB{Point{10.0, 10.0, 10.0}, Point{11.0, 11.0, 11.0}}, 3);

    Point center{1.0, 1.0, 1.0};
    double radius = 5.0;
    auto results = tree.query_radius(center, radius);

    // First two should be within radius
    CHECK(results.size() == 2);
}

TEST_CASE("RTree - query_radius - nothing within radius") {
    RTree<int> tree;

    tree.insert(AABB{Point{10.0, 10.0, 10.0}, Point{11.0, 11.0, 11.0}}, 1);

    Point center{0.0, 0.0, 0.0};
    double radius = 1.0;
    auto results = tree.query_radius(center, radius);

    CHECK(results.empty());
}

TEST_CASE("RTree - remove - existing entry") {
    RTree<int> tree;
    AABB bounds{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}};
    tree.insert(bounds, 42);

    CHECK(tree.size() == 1);

    bool removed = tree.remove(bounds, 42);
    CHECK(removed);
    CHECK(tree.size() == 0);
}

TEST_CASE("RTree - remove - non-existing entry") {
    RTree<int> tree;
    tree.insert(AABB{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}}, 42);

    AABB different_bounds{Point{10.0, 10.0, 10.0}, Point{11.0, 11.0, 11.0}};
    bool removed = tree.remove(different_bounds, 99);

    CHECK_FALSE(removed);
    CHECK(tree.size() == 1); // Original entry still there
}

TEST_CASE("RTree - clear") {
    RTree<int> tree;

    tree.insert(AABB{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}}, 1);
    tree.insert(AABB{Point{2.0, 2.0, 2.0}, Point{3.0, 3.0, 3.0}}, 2);

    CHECK(tree.size() == 2);

    tree.clear();
    CHECK(tree.empty());
    CHECK(tree.size() == 0);
}

TEST_CASE("RTree - iterators - empty tree") {
    RTree<int> tree;

    int count = 0;
    for (const auto &entry : tree) {
        (void)entry;
        count++;
    }

    CHECK(count == 0);
}

TEST_CASE("RTree - iterators - non-empty tree") {
    RTree<int> tree;

    tree.insert(AABB{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}}, 1);
    tree.insert(AABB{Point{2.0, 2.0, 2.0}, Point{3.0, 3.0, 3.0}}, 2);
    tree.insert(AABB{Point{4.0, 4.0, 4.0}, Point{5.0, 5.0, 5.0}}, 3);

    int count = 0;
    for (const auto &entry : tree) {
        (void)entry;
        count++;
    }

    CHECK(count == 3);
}

TEST_CASE("RTree - begin/end") {
    RTree<int> tree;
    tree.insert(AABB{Point{0.0, 0.0, 0.0}, Point{1.0, 1.0, 1.0}}, 42);

    auto it = tree.begin();
    CHECK(it != tree.end());
    CHECK(it->data == 42);
}

// ============================================================================
// PointRTree Tests
// ============================================================================

TEST_CASE("PointRTree - Default construction") {
    PointRTree<int> tree;
    CHECK(tree.empty());
    CHECK(tree.size() == 0);
}

TEST_CASE("PointRTree - Insert single point") {
    PointRTree<int> tree;
    tree.insert(Point{1.0, 2.0, 3.0}, 42);

    CHECK_FALSE(tree.empty());
    CHECK(tree.size() == 1);
}

TEST_CASE("PointRTree - Insert multiple points") {
    PointRTree<int> tree;

    tree.insert(Point{0.0, 0.0, 0.0}, 1);
    tree.insert(Point{1.0, 1.0, 1.0}, 2);
    tree.insert(Point{2.0, 2.0, 2.0}, 3);

    CHECK(tree.size() == 3);
}

TEST_CASE("PointRTree - query_intersects - points in box") {
    PointRTree<int> tree;

    tree.insert(Point{0.5, 0.5, 0.5}, 1);
    tree.insert(Point{1.5, 1.5, 1.5}, 2);
    tree.insert(Point{10.0, 10.0, 10.0}, 3);

    AABB query{Point{0.0, 0.0, 0.0}, Point{2.0, 2.0, 2.0}};
    auto results = tree.query_intersects(query);

    CHECK(results.size() == 2); // First two points in box
}

TEST_CASE("PointRTree - query_nearest - k=1") {
    PointRTree<int> tree;

    tree.insert(Point{0.0, 0.0, 0.0}, 1);
    tree.insert(Point{10.0, 10.0, 10.0}, 2);
    tree.insert(Point{5.0, 5.0, 5.0}, 3);

    Point query{0.1, 0.1, 0.1};
    auto results = tree.query_nearest(query, 1);

    CHECK(results.size() == 1);
    CHECK(results[0].data == 1); // Closest point
}

TEST_CASE("PointRTree - query_nearest - k=2") {
    PointRTree<int> tree;

    tree.insert(Point{0.0, 0.0, 0.0}, 1);
    tree.insert(Point{1.0, 1.0, 1.0}, 2);
    tree.insert(Point{2.0, 2.0, 2.0}, 3);
    tree.insert(Point{10.0, 10.0, 10.0}, 4);

    Point query{0.0, 0.0, 0.0};
    auto results = tree.query_nearest(query, 2);

    CHECK(results.size() == 2);
    CHECK(results[0].data == 1); // Closest
    CHECK(results[1].data == 2); // Second closest
}

TEST_CASE("PointRTree - query_radius - within radius") {
    PointRTree<int> tree;

    tree.insert(Point{0.0, 0.0, 0.0}, 1);
    tree.insert(Point{1.0, 1.0, 1.0}, 2);
    tree.insert(Point{10.0, 10.0, 10.0}, 3);

    Point center{0.0, 0.0, 0.0};
    double radius = 5.0;
    auto results = tree.query_radius(center, radius);

    // First two points within radius
    CHECK(results.size() == 2);
}

TEST_CASE("PointRTree - remove - existing point") {
    PointRTree<int> tree;
    Point pt{1.0, 2.0, 3.0};
    tree.insert(pt, 42);

    CHECK(tree.size() == 1);

    bool removed = tree.remove(pt, 42);
    CHECK(removed);
    CHECK(tree.size() == 0);
}

TEST_CASE("PointRTree - remove - non-existing point") {
    PointRTree<int> tree;
    tree.insert(Point{0.0, 0.0, 0.0}, 42);

    bool removed = tree.remove(Point{10.0, 10.0, 10.0}, 99);
    CHECK_FALSE(removed);
    CHECK(tree.size() == 1);
}

TEST_CASE("PointRTree - clear") {
    PointRTree<int> tree;

    tree.insert(Point{0.0, 0.0, 0.0}, 1);
    tree.insert(Point{1.0, 1.0, 1.0}, 2);

    CHECK(tree.size() == 2);

    tree.clear();
    CHECK(tree.empty());
    CHECK(tree.size() == 0);
}

TEST_CASE("PointRTree - iterators") {
    PointRTree<int> tree;

    tree.insert(Point{0.0, 0.0, 0.0}, 1);
    tree.insert(Point{1.0, 1.0, 1.0}, 2);
    tree.insert(Point{2.0, 2.0, 2.0}, 3);

    int count = 0;
    for (const auto &entry : tree) {
        (void)entry;
        count++;
    }

    CHECK(count == 3);
}

// ============================================================================
// FarmTrax-style Usage Tests
// ============================================================================

TEST_CASE("RTree - FarmTrax swath indexing pattern") {
    // Simulate FarmTrax usage: std::pair<BBox, std::size_t>
    RTree<std::size_t> swath_tree;

    // Insert swaths with indices
    swath_tree.insert(AABB{Point{0.0, 0.0, 0.0}, Point{1.0, 0.5, 0.0}}, 0);
    swath_tree.insert(AABB{Point{0.0, 1.0, 0.0}, Point{1.0, 1.5, 0.0}}, 1);
    swath_tree.insert(AABB{Point{0.0, 2.0, 0.0}, Point{1.0, 2.5, 0.0}}, 2);

    // Find swaths intersecting a search box
    AABB search_box{Point{0.5, 0.5, 0.0}, Point{1.5, 1.5, 0.0}};
    auto results = swath_tree.query_intersects(search_box);

    CHECK(results.size() == 2); // Should find swaths 0 and 1
}

TEST_CASE("PointRTree - FarmTrax endpoint pattern") {
    // Simulate FarmTrax usage: std::pair<Point, std::size_t>
    PointRTree<std::size_t> endpoint_tree;

    // Insert swath endpoints
    endpoint_tree.insert(Point{0.0, 0.0, 0.0}, 0); // Swath 0 start
    endpoint_tree.insert(Point{1.0, 0.0, 0.0}, 1); // Swath 0 end
    endpoint_tree.insert(Point{0.0, 1.0, 0.0}, 2); // Swath 1 start
    endpoint_tree.insert(Point{1.0, 1.0, 0.0}, 3); // Swath 1 end

    // Find k nearest endpoints
    Point query_point{0.1, 0.1, 0.0};
    auto nearest = endpoint_tree.query_nearest(query_point, 2);

    CHECK(nearest.size() == 2);
    CHECK(nearest[0].data == 0); // Closest to (0,0)
}

TEST_CASE("RTree - FarmTrax spatial division pattern") {
    RTree<std::size_t> tree;

    // Insert all swaths
    for (std::size_t i = 0; i < 10; ++i) {
        double y = static_cast<double>(i);
        tree.insert(AABB{Point{0.0, y, 0.0}, Point{10.0, y + 0.5, 0.0}}, i);
    }

    // Simulate removing swaths one by one (like FarmTrax division algorithm)
    auto it = tree.begin();
    CHECK(it != tree.end());

    std::size_t first_idx = it->data;
    tree.remove(it->bounds, it->data);

    CHECK(tree.size() == 9);
}
