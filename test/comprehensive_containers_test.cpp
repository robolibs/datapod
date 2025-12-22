#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "datapod/containers/map.hpp"
#include "datapod/containers/mmap_vec.hpp"
#include "datapod/containers/mutable_fws_multimap.hpp"
#include "datapod/containers/nvec.hpp"
#include "datapod/containers/offset_ptr.hpp"
#include "datapod/containers/ptr.hpp"
#include "datapod/containers/rtree.hpp"
#include "datapod/containers/set.hpp"
#include "datapod/core/mmap.hpp"

using namespace datapod;

// Helper template for Rtree - wraps BasicVector for use as template template parameter
template <typename Key, typename Value> using VectorMap = BasicVector<Value, Value *, Allocator<Value>>;

#define TEST(name)                                                                                                     \
    void name();                                                                                                       \
    struct name##_runner {                                                                                             \
        name##_runner() {                                                                                              \
            std::cout << "Running " #name "..." << std::endl;                                                          \
            name();                                                                                                    \
            std::cout << "  ✓ " #name " passed" << std::endl;                                                          \
        }                                                                                                              \
    } name##_instance;                                                                                                 \
    void name()

#define ASSERT(cond)                                                                                                   \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::cerr << "Assertion failed: " #cond " at " << __FILE__ << ":" << __LINE__ << std::endl;                \
            std::exit(1);                                                                                              \
        }                                                                                                              \
    } while (0)

#define ASSERT_EQ(a, b)                                                                                                \
    do {                                                                                                               \
        if ((a) != (b)) {                                                                                              \
            std::cerr << "Assertion failed: " #a " == " #b << " (" << (a) << " vs " << (b) << ") at " << __FILE__      \
                      << ":" << __LINE__ << std::endl;                                                                 \
            std::exit(1);                                                                                              \
        }                                                                                                              \
    } while (0)

// ============================================================================
// Map Tests
// ============================================================================

TEST(test_map_basic) {
    Map<int, String> map;

    ASSERT(map.empty());
    ASSERT_EQ(map.size(), 0);

    map[1] = String("one");
    map[2] = String("two");
    map[3] = String("three");

    ASSERT(!map.empty());
    ASSERT_EQ(map.size(), 3);

    ASSERT_EQ(map[1].view(), "one");
    ASSERT_EQ(map[2].view(), "two");
    ASSERT_EQ(map[3].view(), "three");
}

TEST(test_map_find) {
    Map<int, int> map;
    map[10] = 100;
    map[20] = 200;

    auto it1 = map.find(10);
    ASSERT(it1 != map.end());
    ASSERT_EQ(it1->first, 10);
    ASSERT_EQ(it1->second, 100);

    auto it2 = map.find(999);
    ASSERT(it2 == map.end());
}

TEST(test_map_erase) {
    Map<int, int> map;
    map[1] = 10;
    map[2] = 20;
    map[3] = 30;

    ASSERT_EQ(map.size(), 3);

    auto it = map.find(2);
    map.erase(it);

    ASSERT_EQ(map.size(), 2);
    ASSERT(map.find(2) == map.end());
    ASSERT(map.find(1) != map.end());
    ASSERT(map.find(3) != map.end());
}

TEST(test_map_clear) {
    Map<int, int> map;
    map[1] = 10;
    map[2] = 20;

    map.clear();
    ASSERT(map.empty());
    ASSERT_EQ(map.size(), 0);
}

TEST(test_map_iteration) {
    Map<int, int> map;
    map[1] = 10;
    map[2] = 20;
    map[3] = 30;

    int sum = 0;
    for (auto const &kv : map) {
        sum += kv.second;
    }
    ASSERT_EQ(sum, 60);
}

TEST(test_map_overwrite) {
    Map<int, int> map;
    map[1] = 10;

    map[1] = 100; // overwrite
    ASSERT_EQ(map[1], 100);
    ASSERT_EQ(map.size(), 1);
}

TEST(test_map_rehash) {
    Map<int, int> map;

    // Insert many elements to trigger rehashing
    for (int i = 0; i < 100; ++i) {
        map[i] = i * 2;
    }

    ASSERT_EQ(map.size(), 100);

    // Verify all elements are still accessible
    for (int i = 0; i < 100; ++i) {
        ASSERT_EQ(map[i], i * 2);
    }
}

// ============================================================================
// Set Tests
// ============================================================================

TEST(test_set_basic) {
    Set<int> set;

    ASSERT(set.empty());
    ASSERT_EQ(set.size(), 0);

    set.insert(1);
    set.insert(2);
    set.insert(3);

    ASSERT(!set.empty());
    ASSERT_EQ(set.size(), 3);

    ASSERT(set.find(1) != set.end());
    ASSERT(set.find(2) != set.end());
    ASSERT(set.find(3) != set.end());
    ASSERT(set.find(4) == set.end());
}

TEST(test_set_find) {
    Set<int> set;
    set.insert(10);
    set.insert(20);

    auto it1 = set.find(10);
    ASSERT(it1 != set.end());
    ASSERT_EQ(*it1, 10);

    auto it2 = set.find(999);
    ASSERT(it2 == set.end());
}

TEST(test_set_erase) {
    Set<int> set;
    set.insert(1);
    set.insert(2);
    set.insert(3);

    ASSERT_EQ(set.size(), 3);

    auto it = set.find(2);
    set.erase(it);

    ASSERT_EQ(set.size(), 2);
    ASSERT(set.find(2) == set.end());
    ASSERT(set.find(1) != set.end());
    ASSERT(set.find(3) != set.end());
}

TEST(test_set_duplicate_insert) {
    Set<int> set;
    set.insert(42);
    set.insert(42); // duplicate

    ASSERT_EQ(set.size(), 1);
}

TEST(test_set_iteration) {
    Set<int> set;
    set.insert(1);
    set.insert(2);
    set.insert(3);

    int sum = 0;
    for (auto val : set) {
        sum += val;
    }
    ASSERT_EQ(sum, 6);
}

TEST(test_set_clear) {
    Set<int> set;
    set.insert(1);
    set.insert(2);

    set.clear();
    ASSERT(set.empty());
    ASSERT_EQ(set.size(), 0);
}

TEST(test_set_with_strings) {
    Set<String> set;
    set.insert(String("hello"));
    set.insert(String("world"));
    set.insert(String("test"));

    ASSERT_EQ(set.size(), 3);
    ASSERT(set.find(String("hello")) != set.end());
    ASSERT(set.find(String("xyz")) == set.end());
}

// ============================================================================
// Nvec Tests (N-dimensional nested vectors)
// ============================================================================

TEST(test_nvec_1d_emplace_back) {
    using NvecType = BasicNvec<std::size_t, Vector<int>, Vector<std::size_t>, 1>;
    NvecType vec;

    vec.emplace_back(std::vector<int>{1, 2, 3});
    vec.emplace_back(std::vector<int>{4, 5});

    ASSERT_EQ(vec.size(), 2);
    ASSERT_EQ(vec.size(0), 3);
    ASSERT_EQ(vec.size(1), 2);
}

TEST(test_nvec_2d_emplace_back) {
    using NvecType = BasicNvec<std::size_t, Vector<int>, Vector<std::size_t>, 2>;
    NvecType mat;

    mat.emplace_back(std::vector<std::vector<int>>{{1, 2}, {3, 4, 5}});
    mat.emplace_back(std::vector<std::vector<int>>{{6, 7, 8}, {9}});

    ASSERT_EQ(mat.size(), 2);
    ASSERT_EQ(mat.size(0), 2); // First element has 2 sub-vectors
    ASSERT_EQ(mat.size(1), 2); // Second element has 2 sub-vectors
}

TEST(test_nvec_3d_emplace_back) {
    using NvecType = BasicNvec<std::size_t, Vector<int>, Vector<std::size_t>, 3>;
    NvecType tensor;

    tensor.emplace_back(std::vector<std::vector<std::vector<int>>>{{{1, 2}, {3}}, {{4}}});

    ASSERT_EQ(tensor.size(), 1);
    ASSERT_EQ(tensor.size(0), 2); // First element has 2 sub-vectors
}

TEST(test_nvec_access) {
    using NvecType = BasicNvec<std::size_t, Vector<int>, Vector<std::size_t>, 2>;
    NvecType mat;

    mat.emplace_back(std::vector<std::vector<int>>{{10, 20, 30}, {40, 50}});

    // Access using at() with variadic indices
    ASSERT_EQ(mat.at(0, 0).at(0), 10);
    ASSERT_EQ(mat.at(0, 0).at(2), 30);
    ASSERT_EQ(mat.at(0, 1).at(1), 50);
}

// ============================================================================
// Rtree Tests (R-Tree spatial index)
// ============================================================================

TEST(test_rtree_basic_insert_2d) {
    Rtree<std::size_t, 2> tree;

    typename Rtree<std::size_t, 2>::coord_t min1 = {0.0f, 0.0f};
    typename Rtree<std::size_t, 2>::coord_t max1 = {1.0f, 1.0f};
    tree.insert(min1, max1, 100);

    typename Rtree<std::size_t, 2>::coord_t min2 = {2.0f, 2.0f};
    typename Rtree<std::size_t, 2>::coord_t max2 = {3.0f, 3.0f};
    tree.insert(min2, max2, 200);

    ASSERT(tree.nodes_.size() > 0);
}

TEST(test_rtree_search_2d) {
    Rtree<std::size_t, 2> tree;

    // Insert some rectangles
    typename Rtree<std::size_t, 2>::coord_t min1 = {0.0f, 0.0f};
    typename Rtree<std::size_t, 2>::coord_t max1 = {1.0f, 1.0f};
    tree.insert(min1, max1, 100);

    typename Rtree<std::size_t, 2>::coord_t min2 = {0.5f, 0.5f};
    typename Rtree<std::size_t, 2>::coord_t max2 = {1.5f, 1.5f};
    tree.insert(min2, max2, 200);

    typename Rtree<std::size_t, 2>::coord_t min3 = {5.0f, 5.0f};
    typename Rtree<std::size_t, 2>::coord_t max3 = {6.0f, 6.0f};
    tree.insert(min3, max3, 300);

    // Search overlapping region
    Vector<std::size_t> results;
    typename Rtree<std::size_t, 2>::coord_t search_min = {0.0f, 0.0f};
    typename Rtree<std::size_t, 2>::coord_t search_max = {2.0f, 2.0f};

    tree.search(search_min, search_max, [&](auto const &min, auto const &max, std::size_t const &val) {
        (void)min;
        (void)max;
        results.push_back(val);
        return true;
    });

    // Should find at least the first two rectangles
    ASSERT(results.size() >= 2);
}

TEST(test_rtree_3d) {
    Rtree<std::size_t, 3> tree;

    typename Rtree<std::size_t, 3>::coord_t min1 = {0.0f, 0.0f, 0.0f};
    typename Rtree<std::size_t, 3>::coord_t max1 = {1.0f, 1.0f, 1.0f};
    tree.insert(min1, max1, 1);

    typename Rtree<std::size_t, 3>::coord_t min2 = {2.0f, 2.0f, 2.0f};
    typename Rtree<std::size_t, 3>::coord_t max2 = {3.0f, 3.0f, 3.0f};
    tree.insert(min2, max2, 2);

    ASSERT(tree.nodes_.size() > 0);
}

TEST(test_rtree_bulk_insert) {
    Rtree<std::size_t, 2> tree;

    // Insert 50 rectangles
    for (std::size_t i = 0; i < 50; ++i) {
        float x = static_cast<float>(i);
        typename Rtree<std::size_t, 2>::coord_t min_c = {x, x};
        typename Rtree<std::size_t, 2>::coord_t max_c = {x + 1.0f, x + 1.0f};
        tree.insert(min_c, max_c, i);
    }

    // Search a region
    Vector<std::size_t> results;
    typename Rtree<std::size_t, 2>::coord_t search_min = {10.0f, 10.0f};
    typename Rtree<std::size_t, 2>::coord_t search_max = {20.0f, 20.0f};

    tree.search(search_min, search_max, [&](auto const &min, auto const &max, std::size_t const &val) {
        (void)min;
        (void)max;
        results.push_back(val);
        return true;
    });

    ASSERT(results.size() > 0);
}

// ============================================================================
// MutableFwsMultimap Tests
// ============================================================================

TEST(test_mutable_fws_multimap_basic) {
    MutableFwsMultimap<std::uint32_t, int> mm;

    ASSERT_EQ(mm.element_count(), 0);
    ASSERT_EQ(mm.size(), 0);

    mm[0].push_back(42);
    ASSERT_EQ(mm.element_count(), 1);
    ASSERT_EQ(mm.size(), 1);
    ASSERT_EQ(mm[0].size(), 1);
    ASSERT_EQ(mm[0][0], 42);
}

TEST(test_mutable_fws_multimap_multiple_buckets) {
    MutableFwsMultimap<std::uint32_t, int> mm;

    mm[0].push_back(4);
    mm[0].push_back(8);

    mm[1].push_back(15);
    mm[1].push_back(16);
    mm[1].push_back(23);
    mm[1].push_back(42);

    mm[2].push_back(100);
    mm[2].push_back(200);

    ASSERT_EQ(mm.size(), 3);
    ASSERT_EQ(mm[0].size(), 2);
    ASSERT_EQ(mm[1].size(), 4);
    ASSERT_EQ(mm[2].size(), 2);

    ASSERT_EQ(mm[0][0], 4);
    ASSERT_EQ(mm[0][1], 8);
    ASSERT_EQ(mm[1][3], 42);
    ASSERT_EQ(mm[2][1], 200);
}

TEST(test_mutable_fws_multimap_iteration) {
    MutableFwsMultimap<std::uint32_t, int> mm;

    mm[0].push_back(10);
    mm[0].push_back(20);
    mm[1].push_back(30);

    int sum = 0;
    for (auto const &bucket : mm) {
        for (auto val : bucket) {
            sum += val;
        }
    }
    ASSERT_EQ(sum, 60);
}

TEST(test_mutable_fws_multimap_clear) {
    MutableFwsMultimap<std::uint32_t, int> mm;

    mm[0].push_back(1);
    mm[1].push_back(2);

    mm[0].clear();
    ASSERT_EQ(mm[0].size(), 0);
    ASSERT_EQ(mm[1].size(), 1);
}

// ============================================================================
// OffsetPtr Tests
// ============================================================================

TEST(test_offset_ptr_default) {
    OffsetPtr<int> ptr;
    ASSERT(ptr.get() == nullptr);
    ASSERT(!ptr);
}

TEST(test_offset_ptr_constructor) {
    int value = 42;
    OffsetPtr<int> ptr(&value);

    ASSERT(ptr.get() != nullptr);
    ASSERT(ptr);
    ASSERT_EQ(*ptr, 42);
}

TEST(test_offset_ptr_assignment) {
    int value = 99;
    OffsetPtr<int> ptr;
    ptr = &value;

    ASSERT_EQ(*ptr, 99);
}

TEST(test_offset_ptr_arrow) {
    struct Point {
        int x, y;
    };

    Point p{10, 20};
    OffsetPtr<Point> ptr(&p);

    ASSERT_EQ(ptr->x, 10);
    ASSERT_EQ(ptr->y, 20);
}

TEST(test_offset_ptr_array_access) {
    int arr[] = {1, 2, 3, 4, 5};
    OffsetPtr<int> ptr(arr);

    ASSERT_EQ(ptr[0], 1);
    ASSERT_EQ(ptr[2], 3);
    ASSERT_EQ(ptr[4], 5);
}

TEST(test_offset_ptr_comparison) {
    int a = 1, b = 2;
    OffsetPtr<int> ptr1(&a);
    OffsetPtr<int> ptr2(&a);
    OffsetPtr<int> ptr3(&b);

    ASSERT(ptr1 == ptr2);
    ASSERT(ptr1 != ptr3);
}

TEST(test_offset_ptr_arithmetic) {
    int arr[] = {10, 20, 30, 40};
    OffsetPtr<int> ptr(arr);

    auto ptr1 = ptr + 1;
    ASSERT_EQ(*ptr1, 20);

    auto ptr2 = ptr + 3;
    ASSERT_EQ(*ptr2, 40);
}

TEST(test_offset_ptr_nullptr) {
    OffsetPtr<int> ptr(nullptr);
    ASSERT(!ptr);
    ASSERT(ptr == nullptr);
    ASSERT(ptr.get() == nullptr);
}

// ============================================================================
// Raw Ptr Tests (using raw::ptr)
// ============================================================================

TEST(test_raw_ptr_basic) {
    int value = 99;
    raw::ptr<int> ptr = &value;

    ASSERT(ptr != nullptr);
    ASSERT_EQ(*ptr, 99);
}

TEST(test_raw_ptr_arrow) {
    struct Data {
        int x;
        String name;
    };

    Data d{42, String("test")};
    raw::ptr<Data> ptr = &d;

    ASSERT_EQ(ptr->x, 42);
    ASSERT_EQ(ptr->name.view(), "test");
}

TEST(test_raw_ptr_comparison) {
    int a = 1, b = 2;
    raw::ptr<int> ptr1 = &a;
    raw::ptr<int> ptr2 = &a;
    raw::ptr<int> ptr3 = &b;

    ASSERT(ptr1 == ptr2);
    ASSERT(ptr1 != ptr3);
}

TEST(test_raw_ptr_nullptr) {
    raw::ptr<int> ptr = nullptr;
    ASSERT(ptr == nullptr);
}

// ============================================================================
// MmapVec Tests
// ============================================================================

TEST(test_mmap_vec_write_mode) {
    const char *test_file = "/tmp/datagram_mmap_test.bin";

    {
        Mmap m(test_file, Mmap::Protection::WRITE);
        m.resize(10 * sizeof(int));
        MmapVec<int> vec(std::move(m));

        for (std::size_t i = 0; i < vec.size(); ++i) {
            vec[i] = static_cast<int>(i * 2);
        }

        ASSERT_EQ(vec.size(), 10);
        ASSERT_EQ(vec[5], 10);
    }

    // Clean up
    std::remove(test_file);
}

TEST(test_mmap_vec_push_back) {
    const char *test_file = "/tmp/datagram_mmap_test2.bin";

    {
        Mmap m(test_file, Mmap::Protection::WRITE);
        m.resize(sizeof(int));
        MmapVec<int> vec(std::move(m));

        vec.push_back(1);
        vec.push_back(2);
        vec.push_back(3);

        ASSERT_EQ(vec.size(), 4); // 1 initial + 3 pushed
        ASSERT_EQ(vec[1], 1);
        ASSERT_EQ(vec[2], 2);
        ASSERT_EQ(vec[3], 3);
    }

    // Clean up
    std::remove(test_file);
}

TEST(test_mmap_vec_iteration) {
    const char *test_file = "/tmp/datagram_mmap_test3.bin";

    {
        Mmap m(test_file, Mmap::Protection::WRITE);
        m.resize(5 * sizeof(int));
        MmapVec<int> vec(std::move(m));

        for (std::size_t i = 0; i < vec.size(); ++i) {
            vec[i] = static_cast<int>(i + 1);
        }

        int sum = 0;
        for (auto val : vec) {
            sum += val;
        }
        ASSERT_EQ(sum, 15); // 1+2+3+4+5
    }

    // Clean up
    std::remove(test_file);
}

int main() {
    std::cout << "\n=== Comprehensive Container Tests ===" << std::endl;
    std::cout << "\nAll comprehensive tests passed! ✓\n" << std::endl;
    return 0;
}
