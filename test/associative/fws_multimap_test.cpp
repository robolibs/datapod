#include <doctest/doctest.h>

#include <datapod/associative/fws_multimap.hpp>
#include <datapod/sequential/vector.hpp>

using namespace datapod;

TEST_SUITE("FwsMultimap") {
    TEST_CASE("DefaultConstruction") {
        FwsMultimapVec<uint32_t, int> mm;
        CHECK(mm.data_size() == 0);
        CHECK(mm.index_size() == 0);
        CHECK(!mm.finished());
    }

    TEST_CASE("SingleKeyMultipleValues") {
        FwsMultimapVec<uint32_t, int> mm;

        // Add values for key 0
        mm.push_back(10);
        mm.push_back(20);
        mm.push_back(30);
        mm.finish_key();
        mm.finish_map();

        CHECK(mm.finished());
        CHECK(mm.data_size() == 3);
        CHECK(mm.index_size() == 2); // index[0] = start, index[1] = end

        auto entry = mm[0];
        CHECK(entry.size() == 3);
        CHECK(!entry.empty());
        CHECK(entry[0] == 10);
        CHECK(entry[1] == 20);
        CHECK(entry[2] == 30);
    }

    TEST_CASE("MultipleKeysMultipleValues") {
        FwsMultimapVec<uint32_t, int> mm;

        // Key 0: [100, 200]
        mm.push_back(100);
        mm.push_back(200);
        mm.finish_key();

        // Key 1: [300, 400, 500]
        mm.push_back(300);
        mm.push_back(400);
        mm.push_back(500);
        mm.finish_key();

        // Key 2: [600]
        mm.push_back(600);
        mm.finish_key();

        mm.finish_map();

        CHECK(mm.finished());
        CHECK(mm.data_size() == 6);
        CHECK(mm.index_size() == 4); // 3 keys + 1 end marker

        // Check key 0
        auto entry0 = mm[0];
        CHECK(entry0.size() == 2);
        CHECK(entry0[0] == 100);
        CHECK(entry0[1] == 200);

        // Check key 1
        auto entry1 = mm[1];
        CHECK(entry1.size() == 3);
        CHECK(entry1[0] == 300);
        CHECK(entry1[1] == 400);
        CHECK(entry1[2] == 500);

        // Check key 2
        auto entry2 = mm[2];
        CHECK(entry2.size() == 1);
        CHECK(entry2[0] == 600);
    }

    TEST_CASE("EmptyKeys") {
        FwsMultimapVec<uint32_t, int> mm;

        // Key 0: empty
        mm.finish_key();

        // Key 1: [10, 20]
        mm.push_back(10);
        mm.push_back(20);
        mm.finish_key();

        // Key 2: empty
        mm.finish_key();

        mm.finish_map();

        auto entry0 = mm[0];
        CHECK(entry0.empty());
        CHECK(entry0.size() == 0);

        auto entry1 = mm[1];
        CHECK(entry1.size() == 2);
        CHECK(entry1[0] == 10);

        auto entry2 = mm[2];
        CHECK(entry2.empty());
    }

    TEST_CASE("EmplaceBack") {
        struct Point {
            int x, y;
            Point(int x_, int y_) : x(x_), y(y_) {}
        };

        FwsMultimapVec<uint32_t, Point> mm;

        mm.emplace_back(1, 2);
        mm.emplace_back(3, 4);
        mm.finish_key();
        mm.finish_map();

        auto entry = mm[0];
        CHECK(entry.size() == 2);
        CHECK(entry[0].x == 1);
        CHECK(entry[0].y == 2);
        CHECK(entry[1].x == 3);
        CHECK(entry[1].y == 4);
    }

    TEST_CASE("CurrentKey") {
        FwsMultimapVec<uint32_t, int> mm;

        CHECK(mm.current_key() == 0);
        mm.push_back(1);
        mm.finish_key();

        CHECK(mm.current_key() == 1);
        mm.push_back(2);
        mm.finish_key();

        CHECK(mm.current_key() == 2);
    }

    TEST_CASE("ReserveIndex") {
        FwsMultimapVec<uint32_t, int> mm;
        mm.reserve_index(100);

        // Should not affect current state
        CHECK(mm.data_size() == 0);
        CHECK(mm.index_size() == 0);
    }

    TEST_CASE("EntryIterators") {
        FwsMultimapVec<uint32_t, int> mm;

        mm.push_back(10);
        mm.push_back(20);
        mm.push_back(30);
        mm.finish_key();
        mm.finish_map();

        auto entry = mm[0];

        // Test entry iterators
        auto it = entry.begin();
        CHECK(*it == 10);
        ++it;
        CHECK(*it == 20);
        ++it;
        CHECK(*it == 30);
        ++it;
        CHECK(it == entry.end());

        // Test const iterators
        auto cit = entry.cbegin();
        CHECK(*cit == 10);
        CHECK(entry.cbegin() != entry.cend());

        // Range-based for
        int sum = 0;
        for (int val : entry) {
            sum += val;
        }
        CHECK(sum == 60);
    }

    TEST_CASE("MultimapIterators") {
        FwsMultimapVec<uint32_t, int> mm;

        // Key 0
        mm.push_back(1);
        mm.finish_key();

        // Key 1
        mm.push_back(2);
        mm.push_back(3);
        mm.finish_key();

        // Key 2
        mm.push_back(4);
        mm.push_back(5);
        mm.push_back(6);
        mm.finish_key();

        mm.finish_map();

        // Iterate over all entries
        auto it = mm.begin();
        CHECK(it != mm.end());

        // Entry 0
        auto entry0 = *it;
        CHECK(entry0.size() == 1);
        CHECK(entry0[0] == 1);
        ++it;

        // Entry 1
        auto entry1 = *it;
        CHECK(entry1.size() == 2);
        CHECK(entry1[0] == 2);
        CHECK(entry1[1] == 3);
        ++it;

        // Entry 2
        auto entry2 = *it;
        CHECK(entry2.size() == 3);
        CHECK(entry2[0] == 4);
        ++it;

        CHECK(it == mm.end());
    }

    TEST_CASE("MultimapIteratorRangeFor") {
        FwsMultimapVec<uint32_t, int> mm;

        mm.push_back(10);
        mm.finish_key();

        mm.push_back(20);
        mm.push_back(30);
        mm.finish_key();

        mm.finish_map();

        int total = 0;
        int key_count = 0;
        for (auto entry : mm) {
            key_count++;
            for (int val : entry) {
                total += val;
            }
        }

        CHECK(key_count == 2);
        CHECK(total == 60);
    }

    TEST_CASE("IteratorArithmetic") {
        FwsMultimapVec<uint32_t, int> mm;

        for (int i = 0; i < 5; ++i) {
            mm.push_back(i * 10);
            mm.finish_key();
        }
        mm.finish_map();

        auto it = mm.begin();
        auto it2 = it + 2;

        auto entry = *it2;
        CHECK(entry[0] == 20);

        auto it3 = it2 - 1;
        auto entry3 = *it3;
        CHECK(entry3[0] == 10);

        CHECK(it2 - it == 2);

        auto it4 = mm.begin();
        it4 += 3;
        auto entry4 = *it4;
        CHECK(entry4[0] == 30);

        it4 -= 2;
        auto entry5 = *it4;
        CHECK(entry5[0] == 10);
    }

    TEST_CASE("IteratorComparisons") {
        FwsMultimapVec<uint32_t, int> mm;

        mm.push_back(1);
        mm.finish_key();
        mm.push_back(2);
        mm.finish_key();
        mm.push_back(3);
        mm.finish_key();
        mm.finish_map();

        auto it1 = mm.begin();
        auto it2 = it1 + 1;
        auto it3 = it1 + 2;

        CHECK(it1 == it1);
        CHECK(it1 != it2);
        CHECK(it1 < it2);
        CHECK(it2 > it1);
        CHECK(it1 <= it1);
        CHECK(it1 <= it2);
        CHECK(it2 >= it2);
        CHECK(it3 >= it1);
    }

    TEST_CASE("IteratorSubscript") {
        FwsMultimapVec<uint32_t, int> mm;

        for (int i = 0; i < 5; ++i) {
            mm.push_back(i * 100);
            mm.finish_key();
        }
        mm.finish_map();

        auto it = mm.begin();
        auto entry0 = it[0];
        CHECK(entry0[0] == 0);

        auto entry2 = it[2];
        CHECK(entry2[0] == 200);

        auto entry4 = it[4];
        CHECK(entry4[0] == 400);
    }

    TEST_CASE("LargeMultimap") {
        FwsMultimapVec<uint32_t, int> mm;
        mm.reserve_index(1000);

        // Create 100 keys with varying numbers of values
        for (int key = 0; key < 100; ++key) {
            int num_values = (key % 10) + 1;
            for (int val = 0; val < num_values; ++val) {
                mm.push_back(key * 1000 + val);
            }
            mm.finish_key();
        }
        mm.finish_map();

        CHECK(mm.index_size() == 101);

        // Check a few entries
        auto entry0 = mm[0];
        CHECK(entry0.size() == 1);
        CHECK(entry0[0] == 0);

        auto entry50 = mm[50];
        CHECK(entry50.size() == 1); // 50 % 10 + 1 = 1
        CHECK(entry50[0] == 50000);

        auto entry99 = mm[99];
        CHECK(entry99.size() == 10); // 99 % 10 + 1 = 10
        CHECK(entry99[0] == 99000);
        CHECK(entry99[9] == 99009);
    }

    TEST_CASE("EntryDataIndex") {
        FwsMultimapVec<uint32_t, int> mm;

        // Key 0: skip
        mm.finish_key();

        // Key 1: [10, 20, 30]
        mm.push_back(10);
        mm.push_back(20);
        mm.push_back(30);
        mm.finish_key();

        mm.finish_map();

        auto entry = mm[1];
        CHECK(entry.data_index(0) == 0); // First value in data_
        CHECK(entry.data_index(1) == 1);
        CHECK(entry.data_index(2) == 2);
    }

    TEST_CASE("Serialization") {
        FwsMultimapVec<uint32_t, int> mm;

        mm.push_back(1);
        mm.push_back(2);
        mm.finish_key();
        mm.push_back(3);
        mm.finish_key();
        mm.finish_map();

        // Test members() function exists
        auto [data, index, current_start, complete] = mm.members();
        CHECK(complete == true);
        (void)data;
        (void)index;
        (void)current_start;
    }

    TEST_CASE("ConstAccess") {
        FwsMultimapVec<uint32_t, int> mm;

        mm.push_back(100);
        mm.push_back(200);
        mm.finish_key();
        mm.finish_map();

        const auto &const_mm = mm;
        auto entry = const_mm[0];
        CHECK(entry.size() == 2);
        CHECK(entry[0] == 100);

        // Const iterators
        auto it = const_mm.begin();
        auto entry_it = *it;
        CHECK(entry_it.size() == 2);

        // cbegin/cend
        auto cit = const_mm.cbegin();
        CHECK(cit != const_mm.cend());
    }

    TEST_CASE("AllEmpty") {
        FwsMultimapVec<uint32_t, int> mm;

        // Create 3 empty keys
        mm.finish_key();
        mm.finish_key();
        mm.finish_key();
        mm.finish_map();

        CHECK(mm.data_size() == 0);
        CHECK(mm.index_size() == 4);

        for (int i = 0; i < 3; ++i) {
            auto entry = mm[i];
            CHECK(entry.empty());
            CHECK(entry.size() == 0);
        }
    }

    TEST_CASE("SingleValue") {
        FwsMultimapVec<uint32_t, int> mm;

        mm.push_back(42);
        mm.finish_key();
        mm.finish_map();

        auto entry = mm[0];
        CHECK(entry.size() == 1);
        CHECK(entry[0] == 42);
    }

    TEST_CASE("DifferentTypes") {
        SUBCASE("StringValues") {
            FwsMultimapVec<uint32_t, std::string> mm;

            mm.push_back("hello");
            mm.push_back("world");
            mm.finish_key();
            mm.finish_map();

            auto entry = mm[0];
            CHECK(entry.size() == 2);
            CHECK(entry[0] == "hello");
            CHECK(entry[1] == "world");
        }

        SUBCASE("DoubleValues") {
            FwsMultimapVec<uint32_t, double> mm;

            mm.push_back(3.14);
            mm.push_back(2.71);
            mm.finish_key();
            mm.finish_map();

            auto entry = mm[0];
            CHECK(entry.size() == 2);
            CHECK(entry[0] == doctest::Approx(3.14));
            CHECK(entry[1] == doctest::Approx(2.71));
        }
    }
}
