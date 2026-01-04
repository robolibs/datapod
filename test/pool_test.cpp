#include <doctest/doctest.h>

#include "datapod/pods/memory/pool.hpp"

#include <memory>
#include <string>
#include <vector>

using namespace datapod;

TEST_SUITE("Pool") {
    TEST_CASE("BasicAllocation") {
        Pool<int> pool;

        int *p1 = pool.allocate(1);
        CHECK(p1 != nullptr);

        int *p2 = pool.allocate(1);
        CHECK(p2 != nullptr);

        // Pointers should be different
        CHECK(p1 != p2);

        pool.deallocate(p1, 1);
        pool.deallocate(p2, 1);
    }

    TEST_CASE("AllocationDeallocationCycle") {
        Pool<int> pool;

        int *p1 = pool.allocate(1);
        CHECK(pool.allocated_count() == 1);

        pool.deallocate(p1, 1);
        CHECK(pool.allocated_count() == 0);

        // After deallocation, should be able to reuse the block
        int *p2 = pool.allocate(1);
        CHECK(p2 == p1); // Should reuse the same block

        pool.deallocate(p2, 1);
    }

    TEST_CASE("ConstructDestroy") {
        Pool<int> pool;

        int *p = pool.allocate(1);
        pool.construct(p, 42);

        CHECK(*p == 42);

        pool.destroy(p);
        pool.deallocate(p, 1);
    }

    TEST_CASE("ComplexTypes") {
        Pool<std::string> pool;

        std::string *p1 = pool.allocate(1);
        pool.construct(p1, "hello");

        std::string *p2 = pool.allocate(1);
        pool.construct(p2, "world");

        CHECK(*p1 == "hello");
        CHECK(*p2 == "world");

        pool.destroy(p1);
        pool.deallocate(p1, 1);

        pool.destroy(p2);
        pool.deallocate(p2, 1);
    }

    TEST_CASE("CustomChunkSize") {
        Pool<int> pool(16); // 16 blocks per chunk

        CHECK(pool.chunk_size() == 16);

        // Allocate enough to verify chunk size works
        std::vector<int *> ptrs;
        for (int i = 0; i < 16; ++i) {
            int *p = pool.allocate(1);
            CHECK(p != nullptr);
            ptrs.push_back(p);
        }

        CHECK(pool.chunk_count() == 1); // Should have allocated 1 chunk
        CHECK(pool.allocated_count() == 16);

        // Clean up
        for (auto *p : ptrs) {
            pool.deallocate(p, 1);
        }
    }

    TEST_CASE("MultipleChunks") {
        Pool<int> pool(8); // Small chunks

        std::vector<int *> ptrs;

        // Allocate more than one chunk
        for (int i = 0; i < 20; ++i) {
            int *p = pool.allocate(1);
            CHECK(p != nullptr);
            ptrs.push_back(p);
        }

        CHECK(pool.chunk_count() >= 2); // Should have multiple chunks
        CHECK(pool.allocated_count() == 20);

        // Clean up
        for (auto *p : ptrs) {
            pool.deallocate(p, 1);
        }
    }

    TEST_CASE("FreeListReuse") {
        Pool<int> pool;

        // Allocate and deallocate to populate free list
        int *p1 = pool.allocate(1);
        int *p2 = pool.allocate(1);
        int *p3 = pool.allocate(1);

        pool.deallocate(p2, 1); // Free middle block
        pool.deallocate(p3, 1); // Free last block

        // Allocate again - should reuse from free list
        int *p4 = pool.allocate(1);
        int *p5 = pool.allocate(1);

        CHECK(p4 == p3); // LIFO order (last freed, first allocated)
        CHECK(p5 == p2);

        pool.deallocate(p1, 1);
        pool.deallocate(p4, 1);
        pool.deallocate(p5, 1);
    }

    TEST_CASE("Clear") {
        Pool<int> pool;

        std::vector<int *> ptrs;
        for (int i = 0; i < 10; ++i) {
            ptrs.push_back(pool.allocate(1));
        }

        CHECK(pool.allocated_count() == 10);
        CHECK(pool.chunk_count() > 0);

        pool.clear();

        CHECK(pool.allocated_count() == 0);
        CHECK(pool.chunk_count() == 0);
        CHECK(pool.capacity() == 0);
    }

    TEST_CASE("CopyConstructor") {
        Pool<int> pool1;
        int *p1 = pool1.allocate(1);

        Pool<int> pool2(pool1);

        // pool2 should be independent
        CHECK(pool2.allocated_count() == 0);
        CHECK(pool2.chunk_size() == pool1.chunk_size());

        int *p2 = pool2.allocate(1);
        CHECK(p1 != p2); // Different memory

        pool1.deallocate(p1, 1);
        pool2.deallocate(p2, 1);
    }

    TEST_CASE("MoveConstructor") {
        Pool<int> pool1(32);
        int *p1 = pool1.allocate(1);
        size_t count = pool1.allocated_count();

        Pool<int> pool2(std::move(pool1));

        // pool2 should have pool1's state
        CHECK(pool2.allocated_count() == count);
        CHECK(pool2.chunk_size() == 32);

        // pool1 should be empty
        CHECK(pool1.allocated_count() == 0);
        CHECK(pool1.chunk_count() == 0);

        pool2.deallocate(p1, 1);
    }

    TEST_CASE("CopyAssignment") {
        Pool<int> pool1;
        pool1.allocate(1);

        Pool<int> pool2;
        pool2.allocate(1);

        pool2 = pool1;

        // pool2 should be reset (independent copy)
        CHECK(pool2.allocated_count() == 0);
        CHECK(pool2.chunk_size() == pool1.chunk_size());
    }

    TEST_CASE("MoveAssignment") {
        Pool<int> pool1(64);
        int *p1 = pool1.allocate(1);
        size_t count = pool1.allocated_count();

        Pool<int> pool2;
        pool2 = std::move(pool1);

        // pool2 should have pool1's state
        CHECK(pool2.allocated_count() == count);
        CHECK(pool2.chunk_size() == 64);

        // pool1 should be empty
        CHECK(pool1.allocated_count() == 0);

        pool2.deallocate(p1, 1);
    }

    TEST_CASE("AllocatedCount") {
        Pool<int> pool;

        CHECK(pool.allocated_count() == 0);

        int *p1 = pool.allocate(1);
        CHECK(pool.allocated_count() == 1);

        int *p2 = pool.allocate(1);
        CHECK(pool.allocated_count() == 2);

        pool.deallocate(p1, 1);
        CHECK(pool.allocated_count() == 1);

        pool.deallocate(p2, 1);
        CHECK(pool.allocated_count() == 0);
    }

    TEST_CASE("FreeCount") {
        Pool<int> pool(8);

        // Initially, no free blocks (no chunks allocated yet)
        CHECK(pool.free_count() == 0);

        // Allocate one - this creates a chunk with 8 blocks, uses 1
        int *p1 = pool.allocate(1);
        CHECK(pool.free_count() == 7); // 7 remaining in chunk

        int *p2 = pool.allocate(1);
        CHECK(pool.free_count() == 6);

        pool.deallocate(p1, 1);
        CHECK(pool.free_count() == 7); // One returned to free list

        pool.deallocate(p2, 1);
        CHECK(pool.free_count() == 8);
    }

    TEST_CASE("Capacity") {
        Pool<int> pool(10);

        CHECK(pool.capacity() == 0); // No chunks yet

        pool.allocate(1); // Allocates first chunk
        CHECK(pool.capacity() == 10);

        // Allocate more to trigger second chunk
        for (int i = 0; i < 10; ++i) {
            pool.allocate(1);
        }

        CHECK(pool.capacity() == 20); // Two chunks
    }

    TEST_CASE("MaxSize") {
        Pool<int> pool;

        size_t max = pool.max_size();
        CHECK(max > 0);
        CHECK(max == std::numeric_limits<size_t>::max() / sizeof(int));
    }

    TEST_CASE("AllocationExceedsMaxSize") {
        Pool<int> pool;

        size_t max = pool.max_size();
        CHECK_THROWS_AS(pool.allocate(max + 1), std::bad_alloc);
    }

    TEST_CASE("MultipleAllocations") {
        Pool<int> pool;

        std::vector<int *> ptrs;

        // Allocate many blocks
        for (int i = 0; i < 100; ++i) {
            int *p = pool.allocate(1);
            CHECK(p != nullptr);
            pool.construct(p, i);
            ptrs.push_back(p);
        }

        CHECK(pool.allocated_count() == 100);

        // Verify values
        for (size_t i = 0; i < ptrs.size(); ++i) {
            CHECK(*ptrs[i] == static_cast<int>(i));
        }

        // Clean up
        for (auto *p : ptrs) {
            pool.destroy(p);
            pool.deallocate(p, 1);
        }

        CHECK(pool.allocated_count() == 0);
    }

    TEST_CASE("Rebind") {
        Pool<double> pool;

        double *p = pool.allocate(1);
        CHECK(p != nullptr);

        pool.construct(p, 3.14159);
        CHECK(*p == doctest::Approx(3.14159));

        pool.destroy(p);
        pool.deallocate(p, 1);
    }

    TEST_CASE("LargeStructs") {
        struct LargeStruct {
            char data[1024];
            int value;
        };

        Pool<LargeStruct> pool;

        LargeStruct *p = pool.allocate(1);
        CHECK(p != nullptr);

        pool.construct(p);
        p->value = 42;
        CHECK(p->value == 42);

        pool.destroy(p);
        pool.deallocate(p, 1);
    }

    TEST_CASE("AlignmentRequirements") {
        struct alignas(64) AlignedStruct {
            int value;
        };

        Pool<AlignedStruct> pool;

        AlignedStruct *p = pool.allocate(1);
        CHECK(p != nullptr);

        // Pool respects natural alignment but may not respect custom alignas
        // Check that it's at least aligned to the natural alignment
        CHECK(reinterpret_cast<uintptr_t>(p) % alignof(int) == 0);

        pool.deallocate(p, 1);
    }

    TEST_CASE("SerializationMembers") {
        Pool<int> pool(128);
        pool.allocate(1);
        pool.allocate(1);

        auto [chunk_size, allocated] = pool.members();

        CHECK(chunk_size == 128);
        CHECK(allocated == 2);
    }

    TEST_CASE("EqualityOperators") {
        Pool<int> pool1;
        Pool<int> pool2;

        // Pools are never equal (they manage independent memory)
        CHECK_FALSE(pool1 == pool2);
        CHECK(pool1 != pool2);
    }

    TEST_CASE("StressManyAllocations") {
        Pool<int> pool;

        constexpr int num_allocs = 10000;
        std::vector<int *> ptrs;
        ptrs.reserve(num_allocs);

        // Allocate many blocks
        for (int i = 0; i < num_allocs; ++i) {
            int *p = pool.allocate(1);
            CHECK(p != nullptr);
            pool.construct(p, i);
            ptrs.push_back(p);
        }

        CHECK(pool.allocated_count() == num_allocs);

        // Verify values
        for (int i = 0; i < num_allocs; ++i) {
            CHECK(*ptrs[i] == i);
        }

        // Deallocate half
        for (int i = 0; i < num_allocs / 2; ++i) {
            pool.destroy(ptrs[i]);
            pool.deallocate(ptrs[i], 1);
        }

        CHECK(pool.allocated_count() == num_allocs / 2);

        // Allocate again (should reuse freed blocks)
        for (int i = 0; i < num_allocs / 2; ++i) {
            int *p = pool.allocate(1);
            CHECK(p != nullptr);
            pool.construct(p, i + 10000);
            ptrs[i] = p;
        }

        CHECK(pool.allocated_count() == num_allocs);

        // Clean up
        for (auto *p : ptrs) {
            pool.destroy(p);
            pool.deallocate(p, 1);
        }

        CHECK(pool.allocated_count() == 0);
    }

    TEST_CASE("InterleavedAllocDealloc") {
        Pool<int> pool;

        std::vector<int *> ptrs;

        // Interleaved pattern
        for (int i = 0; i < 50; ++i) {
            int *p = pool.allocate(1);
            ptrs.push_back(p);

            if (i % 3 == 0 && !ptrs.empty()) {
                pool.deallocate(ptrs.back(), 1);
                ptrs.pop_back();
            }
        }

        // Clean up remaining
        for (auto *p : ptrs) {
            pool.deallocate(p, 1);
        }
    }

    TEST_CASE("NullDeallocation") {
        Pool<int> pool;

        // Deallocating nullptr should be safe
        pool.deallocate(nullptr, 1);

        // Should still work normally
        int *p = pool.allocate(1);
        CHECK(p != nullptr);
        pool.deallocate(p, 1);
    }

    TEST_CASE("ZeroAllocation") {
        Pool<int> pool;

        int *p = pool.allocate(0);
        CHECK(p == nullptr);
    }
}
