#include <doctest/doctest.h>

#include <datapod/pods/associative/mutable_fws_multimap.hpp>
#include <datapod/pods/sequential/vector.hpp>

using namespace datapod;

// Type alias for easier testing
template <typename T> using MutableMultimapVec = DynamicFwsMultimapBase<T, std::uint32_t, Vector>;

TEST_SUITE("MutableFwsMultimap") {
    TEST_CASE("DefaultConstruction") {
        MutableMultimapVec<int> mm;
        CHECK(mm.size() == 0);
        CHECK(mm.data_size() == 0);
        CHECK(mm.element_count() == 0);
        CHECK(mm.empty());
    }

    TEST_CASE("BucketAccess") {
        MutableMultimapVec<int> mm;

        // operator[] auto-creates buckets
        auto bucket0 = mm[0];
        CHECK(bucket0.index() == 0);
        CHECK(bucket0.empty());
        CHECK(mm.size() == 1);

        auto bucket5 = mm[5];
        CHECK(bucket5.index() == 5);
        CHECK(mm.size() == 6); // Creates buckets 0-5
    }

    TEST_CASE("BucketPushBack") {
        MutableMultimapVec<int> mm;

        auto bucket = mm[0];
        bucket.push_back(10);
        bucket.push_back(20);
        bucket.push_back(30);

        CHECK(bucket.size() == 3);
        CHECK(mm.element_count() == 3);
        CHECK(bucket[0] == 10);
        CHECK(bucket[1] == 20);
        CHECK(bucket[2] == 30);
    }

    TEST_CASE("BucketEmplaceBack") {
        struct Point {
            int x, y;
            Point() : x(0), y(0) {}
            Point(int x_, int y_) : x(x_), y(y_) {}
        };

        MutableMultimapVec<Point> mm;

        auto bucket = mm[0];
        bucket.emplace_back(1, 2);
        bucket.emplace_back(3, 4);

        CHECK(bucket.size() == 2);
        CHECK(bucket[0].x == 1);
        CHECK(bucket[0].y == 2);
        CHECK(bucket[1].x == 3);
        CHECK(bucket[1].y == 4);
    }

    TEST_CASE("MultipleBuckets") {
        MutableMultimapVec<int> mm;

        // Bucket 0
        mm[0].push_back(100);
        mm[0].push_back(200);

        // Bucket 1
        mm[1].push_back(300);
        mm[1].push_back(400);
        mm[1].push_back(500);

        // Bucket 2
        mm[2].push_back(600);

        CHECK(mm.size() == 3);
        CHECK(mm.element_count() == 6);

        CHECK(mm[0].size() == 2);
        CHECK(mm[1].size() == 3);
        CHECK(mm[2].size() == 1);

        CHECK(mm[0][0] == 100);
        CHECK(mm[1][1] == 400);
        CHECK(mm[2][0] == 600);
    }

    TEST_CASE("BucketIterators") {
        MutableMultimapVec<int> mm;

        auto bucket = mm[0];
        bucket.push_back(10);
        bucket.push_back(20);
        bucket.push_back(30);

        // Test iterators
        auto it = bucket.begin();
        CHECK(*it == 10);
        ++it;
        CHECK(*it == 20);
        ++it;
        CHECK(*it == 30);
        ++it;
        CHECK(it == bucket.end());

        // Range-based for
        int sum = 0;
        for (int val : bucket) {
            sum += val;
        }
        CHECK(sum == 60);
    }

    TEST_CASE("BucketFrontBack") {
        MutableMultimapVec<int> mm;

        auto bucket = mm[0];
        bucket.push_back(10);
        bucket.push_back(20);
        bucket.push_back(30);

        CHECK(bucket.front() == 10);
        CHECK(bucket.back() == 30);

        bucket.front() = 100;
        bucket.back() = 300;

        CHECK(bucket[0] == 100);
        CHECK(bucket[2] == 300);
    }

    TEST_CASE("BucketAt") {
        MutableMultimapVec<int> mm;

        auto bucket = mm[0];
        bucket.push_back(10);
        bucket.push_back(20);

        CHECK(bucket.at(0) == 10);
        CHECK(bucket.at(1) == 20);

        // Out of range should throw
        CHECK_THROWS_AS(bucket.at(2), std::out_of_range);
    }

    TEST_CASE("BucketReserve") {
        MutableMultimapVec<int> mm;

        auto bucket = mm[0];
        bucket.reserve(100);

        CHECK(bucket.capacity() >= 100);
        CHECK(bucket.size() == 0);

        // Can add elements without reallocation
        for (int i = 0; i < 50; ++i) {
            bucket.push_back(i);
        }
        CHECK(bucket.size() == 50);
    }

    TEST_CASE("BucketResize") {
        MutableMultimapVec<int> mm;

        auto bucket = mm[0];
        bucket.push_back(10);
        bucket.push_back(20);

        // Grow
        bucket.resize(5, 99);
        CHECK(bucket.size() == 5);
        CHECK(bucket[0] == 10);
        CHECK(bucket[1] == 20);
        CHECK(bucket[2] == 99);
        CHECK(bucket[3] == 99);
        CHECK(bucket[4] == 99);

        // Shrink
        auto bucket2 = mm[0]; // Re-fetch bucket
        bucket2.resize(2);
        CHECK(bucket2.size() == 2);
        CHECK(bucket2[0] == 10);
        CHECK(bucket2[1] == 20);
    }

    TEST_CASE("BucketPopBack") {
        MutableMultimapVec<int> mm;

        auto bucket = mm[0];
        bucket.push_back(10);
        bucket.push_back(20);
        bucket.push_back(30);

        bucket.pop_back();
        CHECK(bucket.size() == 2);
        CHECK(bucket[1] == 20);

        bucket.pop_back();
        CHECK(bucket.size() == 1);
        CHECK(bucket[0] == 10);
    }

    TEST_CASE("BucketClear") {
        MutableMultimapVec<int> mm;

        auto bucket = mm[0];
        bucket.push_back(10);
        bucket.push_back(20);
        bucket.push_back(30);

        CHECK(bucket.size() == 3);
        CHECK(mm.element_count() == 3);

        bucket.clear();
        CHECK(bucket.size() == 0);
        CHECK(bucket.empty());
        CHECK(mm.element_count() == 0);
    }

    TEST_CASE("BucketInsert") {
        MutableMultimapVec<int> mm;

        auto bucket = mm[0];
        bucket.push_back(10);
        bucket.push_back(30);

        // Insert 20 in the middle
        auto it = bucket.begin() + 1;
        bucket.insert(it, 20);

        CHECK(bucket.size() == 3);
        CHECK(bucket[0] == 10);
        CHECK(bucket[1] == 20);
        CHECK(bucket[2] == 30);
    }

    TEST_CASE("BucketEraseIterator") {
        MutableMultimapVec<int> mm;

        auto bucket = mm[0];
        bucket.push_back(10);
        bucket.push_back(20);
        bucket.push_back(30);
        bucket.push_back(40);

        // Erase 20 (index 1)
        auto it = bucket.begin() + 1;
        bucket.erase(it);

        CHECK(bucket.size() == 3);
        CHECK(bucket[0] == 10);
        CHECK(bucket[1] == 30);
        CHECK(bucket[2] == 40);
    }

    TEST_CASE("BucketEraseRange") {
        MutableMultimapVec<int> mm;

        auto bucket = mm[0];
        for (int i = 0; i < 10; ++i) {
            bucket.push_back(i * 10);
        }

        // Erase elements [2, 5) - indices 2, 3, 4
        auto first = bucket.begin() + 2;
        auto last = bucket.begin() + 5;
        bucket.erase(first, last);

        CHECK(bucket.size() == 7);
        CHECK(bucket[0] == 0);
        CHECK(bucket[1] == 10);
        CHECK(bucket[2] == 50); // Was index 5
        CHECK(bucket[3] == 60);
    }

    TEST_CASE("MultimapAt") {
        MutableMultimapVec<int> mm;

        mm[0].push_back(10);
        mm[2].push_back(20);

        // Valid access
        CHECK(mm.at(0).size() == 1);
        CHECK(mm.at(2).size() == 1);

        // Out of range
        CHECK_THROWS_AS(mm.at(5), std::out_of_range);
    }

    TEST_CASE("MultimapFrontBack") {
        MutableMultimapVec<int> mm;

        mm[0].push_back(10);
        mm[1].push_back(20);
        mm[2].push_back(30);

        auto front_bucket = mm.front();
        CHECK(front_bucket.index() == 0);
        CHECK(front_bucket[0] == 10);

        auto back_bucket = mm.back();
        CHECK(back_bucket.index() == 2);
        CHECK(back_bucket[0] == 30);
    }

    TEST_CASE("MultimapEmplaceBack") {
        MutableMultimapVec<int> mm;

        auto bucket = mm.emplace_back();
        CHECK(bucket.index() == 0);
        CHECK(mm.size() == 1);

        bucket.push_back(100);

        auto bucket2 = mm.emplace_back();
        CHECK(bucket2.index() == 1);
        CHECK(mm.size() == 2);
    }

    TEST_CASE("MultimapGetOrCreate") {
        MutableMultimapVec<int> mm;

        mm[0].push_back(10);

        // Create bucket 5
        auto bucket5 = mm.get_or_create(5);
        CHECK(bucket5.index() == 5);
        CHECK(mm.size() == 6);

        // Get existing bucket 0
        auto bucket0 = mm.get_or_create(0);
        CHECK(bucket0.index() == 0);
        CHECK(bucket0.size() == 1);
    }

    TEST_CASE("MultimapErase") {
        MutableMultimapVec<int> mm;

        mm[0].push_back(10);
        mm[0].push_back(20);
        mm[1].push_back(30);

        CHECK(mm.element_count() == 3);

        // Erase by clearing the bucket
        mm[0].clear();

        // Bucket 0 is cleared
        CHECK(mm[0].size() == 0);
        CHECK(mm.element_count() == 1);
    }

    TEST_CASE("MultimapClear") {
        MutableMultimapVec<int> mm;

        mm[0].push_back(10);
        mm[1].push_back(20);
        mm[2].push_back(30);

        CHECK(mm.size() == 3);
        CHECK(mm.element_count() == 3);

        mm.clear();

        CHECK(mm.size() == 0);
        CHECK(mm.empty());
        CHECK(mm.element_count() == 0);
    }

    TEST_CASE("MultimapReserve") {
        MutableMultimapVec<int> mm;

        mm.reserve(100, 1000);

        // Should not affect current state
        CHECK(mm.size() == 0);
        CHECK(mm.element_count() == 0);
    }

    TEST_CASE("MultimapIterators") {
        MutableMultimapVec<int> mm;

        mm[0].push_back(10);
        mm[1].push_back(20);
        mm[1].push_back(21);
        mm[2].push_back(30);

        // Iterate over buckets
        auto it = mm.begin();
        CHECK(it != mm.end());

        auto bucket0 = *it;
        CHECK(bucket0.size() == 1);
        CHECK(bucket0[0] == 10);
        ++it;

        auto bucket1 = *it;
        CHECK(bucket1.size() == 2);
        CHECK(bucket1[0] == 20);
        ++it;

        auto bucket2 = *it;
        CHECK(bucket2.size() == 1);
        CHECK(bucket2[0] == 30);
        ++it;

        CHECK(it == mm.end());
    }

    TEST_CASE("MultimapIteratorRangeFor") {
        MutableMultimapVec<int> mm;

        mm[0].push_back(10);
        mm[1].push_back(20);
        mm[1].push_back(30);

        int bucket_count = 0;
        int total = 0;
        for (auto bucket : mm) {
            bucket_count++;
            for (int val : bucket) {
                total += val;
            }
        }

        CHECK(bucket_count == 2);
        CHECK(total == 60);
    }

    TEST_CASE("BucketDataIndex") {
        MutableMultimapVec<int> mm;

        mm[0].push_back(10);
        mm[0].push_back(20);
        mm[1].push_back(30);

        auto bucket0 = mm[0];
        auto data_idx_0 = bucket0.data_index(0);
        auto data_idx_1 = bucket0.data_index(1);

        // Data indices should be sequential
        CHECK(data_idx_1 == data_idx_0 + 1);
    }

    TEST_CASE("BucketBucketIndex") {
        MutableMultimapVec<int> mm;

        auto bucket = mm[0];
        bucket.push_back(10);
        bucket.push_back(20);
        bucket.push_back(30);

        auto it = bucket.begin() + 1;
        auto idx = bucket.bucket_index(it);
        CHECK(idx == 1);

        // Out of range iterator
        auto end_it = bucket.end();
        CHECK_THROWS_AS(bucket.bucket_index(end_it), std::out_of_range);
    }

    TEST_CASE("ConstAccess") {
        MutableMultimapVec<int> mm;

        mm[0].push_back(100);
        mm[0].push_back(200);

        const auto &const_mm = mm;

        auto const_bucket = const_mm[0];
        CHECK(const_bucket.size() == 2);
        CHECK(const_bucket[0] == 100);

        // Const iterators
        auto it = const_mm.begin();
        auto bucket = *it;
        CHECK(bucket.size() == 2);

        // cbegin/cend on bucket
        auto bucket0 = mm[0];
        auto cit = bucket0.cbegin();
        CHECK(cit != bucket0.cend());
    }

    TEST_CASE("LargeScale") {
        MutableMultimapVec<int> mm;

        // Create 100 buckets with varying sizes
        for (int i = 0; i < 100; ++i) {
            int count = (i % 10) + 1;
            for (int j = 0; j < count; ++j) {
                mm[i].push_back(i * 1000 + j);
            }
        }

        CHECK(mm.size() == 100);

        // Check a few buckets
        CHECK(mm[0].size() == 1);
        CHECK(mm[0][0] == 0);

        CHECK(mm[50].size() == 1); // 50 % 10 + 1 = 1
        CHECK(mm[50][0] == 50000);

        CHECK(mm[99].size() == 10); // 99 % 10 + 1 = 10
        CHECK(mm[99][0] == 99000);
        CHECK(mm[99][9] == 99009);
    }

    TEST_CASE("SparseAllocation") {
        MutableMultimapVec<int> mm;

        // Allocate non-contiguous buckets
        mm[0].push_back(10);
        mm[10].push_back(100);
        mm[100].push_back(1000);

        CHECK(mm.size() == 101); // Creates all buckets 0-100
        CHECK(mm.element_count() == 3);

        // Empty buckets in between
        CHECK(mm[5].empty());
        CHECK(mm[50].empty());
    }

    TEST_CASE("BucketGrowth") {
        MutableMultimapVec<int> mm;

        auto bucket = mm[0];

        // Add many elements to trigger growth
        for (int i = 0; i < 100; ++i) {
            bucket.push_back(i);
        }

        CHECK(bucket.size() == 100);
        CHECK(bucket.capacity() >= 100);

        // Verify all values
        for (int i = 0; i < 100; ++i) {
            CHECK(bucket[i] == i);
        }
    }

    TEST_CASE("ModifyThroughIterator") {
        MutableMultimapVec<int> mm;

        auto bucket = mm[0];
        bucket.push_back(10);
        bucket.push_back(20);
        bucket.push_back(30);

        // Modify through iterator
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            *it *= 2;
        }

        CHECK(bucket[0] == 20);
        CHECK(bucket[1] == 40);
        CHECK(bucket[2] == 60);
    }

    TEST_CASE("MultipleBucketsIndependent") {
        MutableMultimapVec<int> mm;

        mm[0].push_back(10);
        mm[1].push_back(20);
        mm[2].push_back(30);

        // Modifying one bucket doesn't affect others
        mm[1].push_back(21);
        mm[1].push_back(22);

        CHECK(mm[0].size() == 1);
        CHECK(mm[1].size() == 3);
        CHECK(mm[2].size() == 1);

        mm[1].clear();
        CHECK(mm[0].size() == 1);
        CHECK(mm[1].size() == 0);
        CHECK(mm[2].size() == 1);
    }

    TEST_CASE("ElementCount") {
        MutableMultimapVec<int> mm;

        CHECK(mm.element_count() == 0);

        mm[0].push_back(10);
        CHECK(mm.element_count() == 1);

        mm[0].push_back(20);
        CHECK(mm.element_count() == 2);

        mm[1].push_back(30);
        CHECK(mm.element_count() == 3);

        mm[0].pop_back();
        CHECK(mm.element_count() == 2);

        mm[0].clear();
        CHECK(mm.element_count() == 1);

        mm.clear();
        CHECK(mm.element_count() == 0);
    }
}
