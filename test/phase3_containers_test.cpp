#include <iostream>
#include <string>
#include <vector>

#include "datagram/containers/fws_multimap.hpp"
#include "datagram/containers/paged.hpp"
#include "datagram/containers/paged_vecvec.hpp"

using namespace datagram;

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
            std::cerr << "Assertion failed: " #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl;             \
            std::exit(1);                                                                                              \
        }                                                                                                              \
    } while (0)

// ============================================================================
// FwsMultimap Tests
// ============================================================================

TEST(FwsMultimapBasicBuildAndQuery) {
    FwsMultimapVec<std::size_t, int> map;

    // Build multimap: key 0 -> {1, 2, 3}, key 1 -> {4, 5}, key 2 -> {}
    map.push_back(1);
    map.push_back(2);
    map.push_back(3);
    map.finish_key();

    map.push_back(4);
    map.push_back(5);
    map.finish_key();

    map.finish_key(); // Empty key

    map.finish_map();

    ASSERT(map.finished());
    ASSERT(map.data_size() == 5);
    ASSERT(map.index_size() == 4); // 3 keys + 1 sentinel

    // Query key 0
    auto entry0 = map[0];
    ASSERT(entry0.size() == 3);
    ASSERT(!entry0.empty());
    ASSERT(entry0[0] == 1);
    ASSERT(entry0[1] == 2);
    ASSERT(entry0[2] == 3);

    // Query key 1
    auto entry1 = map[1];
    ASSERT(entry1.size() == 2);
    ASSERT(entry1[0] == 4);
    ASSERT(entry1[1] == 5);

    // Query key 2 (empty)
    auto entry2 = map[2];
    ASSERT(entry2.size() == 0);
    ASSERT(entry2.empty());
}

TEST(FwsMultimapEntryIteration) {
    FwsMultimapVec<std::size_t, int> map;

    map.push_back(10);
    map.push_back(20);
    map.push_back(30);
    map.finish_key();
    map.finish_map();

    auto entry = map[0];

    // Test iteration
    std::vector<int> values;
    for (auto val : entry) {
        values.push_back(val);
    }

    ASSERT(values.size() == 3);
    ASSERT(values[0] == 10);
    ASSERT(values[1] == 20);
    ASSERT(values[2] == 30);

    // Test begin/end
    ASSERT(std::distance(entry.begin(), entry.end()) == 3);
}

TEST(FwsMultimapMapIteration) {
    FwsMultimapVec<std::size_t, int> map;

    // Key 0: {1, 2}
    map.push_back(1);
    map.push_back(2);
    map.finish_key();

    // Key 1: {3}
    map.push_back(3);
    map.finish_key();

    map.finish_map();

    // Iterate over all keys
    int key_count = 0;
    for (auto entry : map) {
        if (key_count == 0) {
            ASSERT(entry.size() == 2);
            ASSERT(entry[0] == 1);
            ASSERT(entry[1] == 2);
        } else if (key_count == 1) {
            ASSERT(entry.size() == 1);
            ASSERT(entry[0] == 3);
        }
        ++key_count;
    }

    ASSERT(key_count == 2);
}

TEST(FwsMultimapEmplaceBack) {
    FwsMultimapVec<std::size_t, std::string> map;

    map.emplace_back("hello");
    map.emplace_back("world");
    map.finish_key();
    map.finish_map();

    auto entry = map[0];
    ASSERT(entry.size() == 2);
    ASSERT(entry[0] == "hello");
    ASSERT(entry[1] == "world");
}

TEST(FwsMultimapCurrentKey) {
    FwsMultimapVec<std::size_t, int> map;

    ASSERT(map.current_key() == 0);

    map.push_back(1);
    map.finish_key();
    ASSERT(map.current_key() == 1);

    map.push_back(2);
    map.finish_key();
    ASSERT(map.current_key() == 2);
}

TEST(FwsMultimapReserveIndex) {
    FwsMultimapVec<std::size_t, int> map;

    map.reserve_index(100);
    // Should not crash, just pre-allocate
    ASSERT(map.index_size() == 0);
}

TEST(FwsMultimapEmptyMap) {
    FwsMultimapVec<std::size_t, int> map;
    map.finish_map();

    ASSERT(map.finished());
    ASSERT(map.data_size() == 0);
    ASSERT(map.index_size() == 1); // Just the sentinel

    // Iteration should be empty
    int count = 0;
    for (auto entry : map) {
        (void)entry;
        ++count;
    }
    ASSERT(count == 0);
}

// ============================================================================
// Paged Tests
// ============================================================================

TEST(PagedCreatePage) {
    using PagedInt = Paged<Vector<int>>;
    PagedInt paged;

    auto page1 = paged.create_page(10);
    ASSERT(page1.valid());
    ASSERT(page1.size() == 10);
    ASSERT(page1.capacity_ >= 10);

    // Write some data
    for (std::size_t i = 0; i < page1.size(); ++i) {
        paged.data(page1)[i] = static_cast<int>(i * 2);
    }

    // Verify data
    for (std::size_t i = 0; i < page1.size(); ++i) {
        ASSERT(paged.data(page1)[i] == static_cast<int>(i * 2));
    }
}

TEST(PagedResizePage) {
    using PagedInt = Paged<Vector<int>>;
    PagedInt paged;

    auto page = paged.create_page(5);
    for (std::size_t i = 0; i < 5; ++i) {
        paged.data(page)[i] = static_cast<int>(i);
    }

    // Resize larger
    page = paged.resize_page(page, 10);
    ASSERT(page.size() == 10);

    // Original data should be preserved
    for (std::size_t i = 0; i < 5; ++i) {
        ASSERT(paged.data(page)[i] == static_cast<int>(i));
    }

    // Resize smaller
    page = paged.resize_page(page, 3);
    ASSERT(page.size() == 3);
}

TEST(PagedFreeAndReuse) {
    using PagedInt = Paged<Vector<int>>;
    PagedInt paged;

    auto page1 = paged.create_page(8);
    auto start1 = page1.start_;

    paged.free_page(page1);

    // Create another page with same size - should reuse freed space
    auto page2 = paged.create_page(8);
    ASSERT(page2.start_ == start1); // Should reuse same memory
}

TEST(PagedReadWrite) {
    using PagedInt = Paged<Vector<int>>;
    PagedInt paged;

    auto page = paged.create_page(10);

    // Write using write() method
    paged.write(page.start_ + 0, 42);
    paged.write(page.start_ + 1, 99);

    // Read back
    ASSERT(paged.read<int>(page.start_ + 0) == 42);
    ASSERT(paged.read<int>(page.start_ + 1) == 99);
}

TEST(PagedClear) {
    using PagedInt = Paged<Vector<int>>;
    PagedInt paged;

    paged.create_page(10);
    paged.create_page(20);

    paged.clear();
    ASSERT(paged.data_.size() == 0);
}

// ============================================================================
// PagedVecvec Tests
// ============================================================================

TEST(PagedVecvecBasic) {
    using MyKey = Strong<std::size_t, struct KeyTag>;
    using MyPagedVecvec = PagedVecvec<Vector<Page<std::size_t, std::uint16_t>>, Paged<Vector<int>>, MyKey>;

    MyPagedVecvec pvv;

    // Add buckets
    std::vector<int> bucket1 = {1, 2, 3};
    pvv.emplace_back(bucket1);

    std::vector<int> bucket2 = {4, 5};
    pvv.emplace_back(bucket2);

    ASSERT(pvv.size() == 2);
    ASSERT(!pvv.empty());

    // Access buckets
    auto b0 = pvv[MyKey{0}];
    ASSERT(b0.size() == 3);
    ASSERT(b0[0] == 1);
    ASSERT(b0[1] == 2);
    ASSERT(b0[2] == 3);

    auto b1 = pvv[MyKey{1}];
    ASSERT(b1.size() == 2);
    ASSERT(b1[0] == 4);
    ASSERT(b1[1] == 5);
}

TEST(PagedVecvecBucketPushBack) {
    using MyKey = Strong<std::size_t, struct KeyTag2>;
    using MyPagedVecvec = PagedVecvec<Vector<Page<std::size_t, std::uint16_t>>, Paged<Vector<int>>, MyKey>;

    MyPagedVecvec pvv;

    std::vector<int> bucket1 = {1, 2};
    pvv.emplace_back(bucket1);

    // Get bucket and push_back
    auto b = pvv[MyKey{0}];
    b.push_back(3);
    b.push_back(4);

    // Verify
    auto b_verify = pvv[MyKey{0}];
    ASSERT(b_verify.size() == 4);
    ASSERT(b_verify[0] == 1);
    ASSERT(b_verify[1] == 2);
    ASSERT(b_verify[2] == 3);
    ASSERT(b_verify[3] == 4);
}

TEST(PagedVecvecIteration) {
    using MyKey = Strong<std::size_t, struct KeyTag3>;
    using MyPagedVecvec = PagedVecvec<Vector<Page<std::size_t, std::uint16_t>>, Paged<Vector<int>>, MyKey>;

    MyPagedVecvec pvv;

    pvv.emplace_back(std::vector<int>{10, 20});
    pvv.emplace_back(std::vector<int>{30, 40, 50});

    int bucket_count = 0;
    for (auto bucket : pvv) {
        if (bucket_count == 0) {
            ASSERT(bucket.size() == 2);
        } else if (bucket_count == 1) {
            ASSERT(bucket.size() == 3);
        }
        ++bucket_count;
    }
    ASSERT(bucket_count == 2);
}

TEST(PagedVecvecEmptyBucket) {
    using MyKey = Strong<std::size_t, struct KeyTag4>;
    using MyPagedVecvec = PagedVecvec<Vector<Page<std::size_t, std::uint16_t>>, Paged<Vector<int>>, MyKey>;

    MyPagedVecvec pvv;

    pvv.emplace_back_empty();
    ASSERT(pvv.size() == 1);

    auto b = pvv[MyKey{0}];
    ASSERT(b.empty());
    ASSERT(b.size() == 0);
}

TEST(PagedVecvecClear) {
    using MyKey = Strong<std::size_t, struct KeyTag5>;
    using MyPagedVecvec = PagedVecvec<Vector<Page<std::size_t, std::uint16_t>>, Paged<Vector<int>>, MyKey>;

    MyPagedVecvec pvv;

    pvv.emplace_back(std::vector<int>{1, 2, 3});
    pvv.emplace_back(std::vector<int>{4, 5});

    pvv.clear();
    ASSERT(pvv.empty());
    ASSERT(pvv.size() == 0);
}

int main() {
    std::cout << "\nAll Phase 3 tests passed!\n";
    return 0;
}
