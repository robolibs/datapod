#include <doctest/doctest.h>

#include "datapod/pods/memory/arena.hpp"
#include "datapod/pods/sequential/vector.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace datapod;

TEST_SUITE("Arena") {
    TEST_CASE("BasicAllocation") {
        Arena<int> arena;

        int *p1 = arena.allocate(1);
        CHECK(p1 != nullptr);

        int *p2 = arena.allocate(10);
        CHECK(p2 != nullptr);

        // Pointers should be different
        CHECK(p1 != p2);
    }

    TEST_CASE("ConstructDestroy") {
        Arena<int> arena;

        int *p = arena.allocate(5);
        for (int i = 0; i < 5; ++i) {
            arena.construct(&p[i], i * 10);
        }

        // Verify values
        for (int i = 0; i < 5; ++i) {
            CHECK(p[i] == i * 10);
        }

        // Destroy
        for (int i = 0; i < 5; ++i) {
            arena.destroy(&p[i]);
        }
    }

    TEST_CASE("ComplexTypes") {
        Arena<std::string> arena;

        std::string *p = arena.allocate(3);
        arena.construct(&p[0], "hello");
        arena.construct(&p[1], "world");
        arena.construct(&p[2], "arena");

        CHECK(p[0] == "hello");
        CHECK(p[1] == "world");
        CHECK(p[2] == "arena");

        // Destroy
        for (int i = 0; i < 3; ++i) {
            arena.destroy(&p[i]);
        }
    }

    TEST_CASE("Reset") {
        Arena<int> arena;

        int *p1 = arena.allocate(100);
        size_t used_before = arena.bytes_used();
        CHECK(used_before > 0);

        arena.reset();
        CHECK(arena.bytes_used() == 0);

        // Should be able to allocate again from the beginning
        int *p2 = arena.allocate(100);
        CHECK(p2 != nullptr);
        // After reset, new allocation should reuse the same buffer
        CHECK(p1 == p2); // Same address since we reset
    }

    TEST_CASE("Clear") {
        Arena<int> arena;

        arena.allocate(100);
        CHECK(arena.bytes_capacity() > 0);

        arena.clear();
        CHECK(arena.bytes_used() == 0);
        CHECK(arena.bytes_capacity() == 0);
    }

    TEST_CASE("CustomBlockSize") {
        Arena<char> arena(1024); // 1KB blocks

        CHECK(arena.block_size() == 1024);

        // Allocate enough to trigger growth
        char *p1 = arena.allocate(512);
        char *p2 = arena.allocate(512);
        char *p3 = arena.allocate(512); // Should trigger growth

        CHECK(p1 != nullptr);
        CHECK(p2 != nullptr);
        CHECK(p3 != nullptr);

        CHECK(arena.bytes_capacity() >= 1536); // At least 1.5KB
    }

    TEST_CASE("Alignment") {
        Arena<std::max_align_t> arena;

        void *p1 = arena.allocate(1);
        void *p2 = arena.allocate(1);

        // Check alignment for max_align_t
        CHECK(reinterpret_cast<uintptr_t>(p1) % alignof(std::max_align_t) == 0);
        CHECK(reinterpret_cast<uintptr_t>(p2) % alignof(std::max_align_t) == 0);
    }

    TEST_CASE("MixedAlignments") {
        Arena<char> arena_char;

        // Allocate chars (align 1)
        char *c1 = arena_char.allocate(1);
        char *c2 = arena_char.allocate(1);
        char *c3 = arena_char.allocate(1);

        CHECK(c1 != nullptr);
        CHECK(c2 != nullptr);
        CHECK(c3 != nullptr);

        Arena<char> arena_mixed;

        // Allocate one char
        char *c = arena_mixed.allocate(1);
        CHECK(c != nullptr);

        // Now test alignment with int arena
        Arena<int> arena_int;
        int *i = arena_int.allocate(1);
        CHECK(i != nullptr);

        // Int should be aligned
        CHECK(reinterpret_cast<uintptr_t>(i) % alignof(int) == 0);
    }

    TEST_CASE("CopyConstructor") {
        Arena<int> arena1;
        int *p1 = arena1.allocate(10);

        Arena<int> arena2(arena1);

        // arena2 should be independent
        CHECK(arena2.bytes_used() == 0); // Hasn't allocated yet
        CHECK(arena2.block_size() == arena1.block_size());

        int *p2 = arena2.allocate(10);
        CHECK(p1 != p2); // Different memory
    }

    TEST_CASE("MoveConstructor") {
        Arena<int> arena1(2048);
        int *p1 = arena1.allocate(10);
        size_t used = arena1.bytes_used();

        Arena<int> arena2(std::move(arena1));

        // arena2 should have arena1's state
        CHECK(arena2.bytes_used() == used);
        CHECK(arena2.block_size() == 2048);

        // arena1 should be empty
        CHECK(arena1.bytes_used() == 0);
        CHECK(arena1.bytes_capacity() == 0);
    }

    TEST_CASE("CopyAssignment") {
        Arena<int> arena1;
        arena1.allocate(100);

        Arena<int> arena2;
        arena2.allocate(50);

        arena2 = arena1;

        // arena2 should be reset (independent copy)
        CHECK(arena2.bytes_used() == 0);
        CHECK(arena2.block_size() == arena1.block_size());
    }

    TEST_CASE("MoveAssignment") {
        Arena<int> arena1(4096);
        int *p1 = arena1.allocate(100);
        size_t used = arena1.bytes_used();

        Arena<int> arena2;
        arena2 = std::move(arena1);

        // arena2 should have arena1's state
        CHECK(arena2.bytes_used() == used);
        CHECK(arena2.block_size() == 4096);

        // arena1 should be empty
        CHECK(arena1.bytes_used() == 0);
        CHECK(arena1.bytes_capacity() == 0);
    }

    TEST_CASE("Rebind") {
        Arena<double> arena_double;

        double *p = arena_double.allocate(5);
        CHECK(p != nullptr);

        arena_double.construct(&p[0], 3.14);
        CHECK(p[0] == doctest::Approx(3.14));

        arena_double.destroy(&p[0]);
    }

    TEST_CASE("LargeAllocations") {
        Arena<char> arena(1024); // Small block size

        // Allocate larger than block size
        char *p = arena.allocate(10000);
        CHECK(p != nullptr);

        CHECK(arena.bytes_capacity() >= 10000);
    }

    TEST_CASE("MaxSize") {
        Arena<int> arena;

        size_t max = arena.max_size();
        CHECK(max > 0);
        CHECK(max == std::numeric_limits<size_t>::max() / sizeof(int));
    }

    TEST_CASE("AllocationExceedsMaxSize") {
        Arena<int> arena;

        size_t max = arena.max_size();
        CHECK_THROWS_AS(arena.allocate(max + 1), std::bad_alloc);
    }

    TEST_CASE("MultipleAllocationsGrowth") {
        Arena<int> arena(128); // Small block for testing growth

        std::vector<int *> ptrs;

        // Allocate many small chunks to trigger multiple growths
        for (int i = 0; i < 100; ++i) {
            int *p = arena.allocate(10);
            CHECK(p != nullptr);
            ptrs.push_back(p);
        }

        CHECK(arena.bytes_capacity() > 128); // Should have grown
    }

    TEST_CASE("BytesTracking") {
        Arena<int> arena;

        CHECK(arena.bytes_used() == 0);
        CHECK(arena.bytes_capacity() == 0);

        arena.allocate(10);

        CHECK(arena.bytes_used() >= 10 * sizeof(int));
        CHECK(arena.bytes_capacity() > 0);

        size_t used1 = arena.bytes_used();

        arena.allocate(5);
        size_t used2 = arena.bytes_used();

        CHECK(used2 > used1);
    }

    TEST_CASE("SerializationMembers") {
        Arena<int> arena(2048);
        arena.allocate(100);

        auto [buffer, offset, capacity, block_size] = arena.members();

        CHECK(buffer != nullptr);
        CHECK(offset > 0);
        CHECK(capacity > 0);
        CHECK(block_size == 2048);
    }

    TEST_CASE("EqualityOperators") {
        Arena<int> arena1;
        Arena<int> arena2;

        // Arenas are never equal (they manage independent memory)
        CHECK_FALSE(arena1 == arena2);
        CHECK(arena1 != arena2);
    }

    TEST_CASE("StressManySmallAllocations") {
        Arena<int> arena;

        constexpr int num_allocs = 10000;
        std::vector<int *> ptrs;
        ptrs.reserve(num_allocs);

        for (int i = 0; i < num_allocs; ++i) {
            int *p = arena.allocate(1);
            CHECK(p != nullptr);
            arena.construct(p, i);
            ptrs.push_back(p);
        }

        // Verify all values
        for (int i = 0; i < num_allocs; ++i) {
            CHECK(*ptrs[i] == i);
        }

        // Cleanup
        for (int i = 0; i < num_allocs; ++i) {
            arena.destroy(ptrs[i]);
        }

        arena.clear();
    }
}
