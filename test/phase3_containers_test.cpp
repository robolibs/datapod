#include <iostream>
#include <string>
#include <vector>

#include "datagram/containers/fws_multimap.hpp"
#include "datagram/containers/paged.hpp"

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

int main() {
    std::cout << "\nAll Phase 3 tests passed!\n";
    return 0;
}
