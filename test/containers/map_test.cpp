#include <doctest/doctest.h>

#include "datapod/datapod.hpp"

using namespace datapod;

TEST_SUITE("Map") {
    TEST_CASE("Construction - Default") {
        Map<int, String> m;
        CHECK(m.size() == 0);
        CHECK(m.empty() == true);
    }

    TEST_CASE("Construction - Initializer list") {
        Map<int, String> m{{1, "one"}, {2, "two"}, {3, "three"}};
        CHECK(m.size() == 3);
        CHECK(m[1] == String("one"));
        CHECK(m[2] == String("two"));
        CHECK(m[3] == String("three"));
    }

    TEST_CASE("Construction - Copy") {
        Map<int, String> m1{{1, "one"}, {2, "two"}};
        Map<int, String> m2(m1);

        CHECK(m2.size() == 2);
        CHECK(m2[1] == String("one"));
        CHECK(m2[2] == String("two"));

        // Modify m2, m1 should remain unchanged
        m2[3] = "three";
        CHECK(m1.size() == 2);
        CHECK(m2.size() == 3);
    }

    TEST_CASE("Construction - Move") {
        Map<int, String> m1{{1, "one"}, {2, "two"}};
        Map<int, String> m2(std::move(m1));

        CHECK(m2.size() == 2);
        CHECK(m2[1] == String("one"));
        CHECK(m2[2] == String("two"));
        CHECK(m1.size() == 0);
    }

    TEST_CASE("Element Access - operator[]") {
        Map<int, String> m;
        m[1] = "one";
        m[2] = "two";

        CHECK(m[1] == String("one"));
        CHECK(m[2] == String("two"));

        // Modify via operator[]
        m[1] = "ONE";
        CHECK(m[1] == String("ONE"));
    }

    TEST_CASE("Element Access - operator[] creates default") {
        Map<int, int> m;
        int value = m[42]; // Should create entry with default value

        CHECK(m.size() == 1);
        CHECK(m.contains(42));
        CHECK(value == 0); // Default int value
    }

    TEST_CASE("Element Access - at()") {
        Map<int, String> m{{1, "one"}, {2, "two"}};

        CHECK(m.at(1) == String("one"));
        CHECK(m.at(2) == String("two"));

        // Modify via at()
        m.at(1) = "ONE";
        CHECK(m.at(1) == String("ONE"));
    }

    TEST_CASE("Element Access - at() throws on missing key") {
        Map<int, String> m{{1, "one"}};

        CHECK_THROWS_AS(m.at(99), std::out_of_range);
    }

    TEST_CASE("Element Access - get()") {
        Map<int, String> m{{1, "one"}, {2, "two"}};

        auto opt1 = m.get(1);
        CHECK(opt1.has_value());
        CHECK(*opt1 == String("one"));

        auto opt2 = m.get(99);
        CHECK_FALSE(opt2.has_value());
    }

    TEST_CASE("Lookup - find()") {
        Map<int, String> m{{1, "one"}, {2, "two"}, {3, "three"}};

        auto it1 = m.find(2);
        CHECK(it1 != m.end());
        CHECK(it1->first == 2);
        CHECK(it1->second == String("two"));

        auto it2 = m.find(99);
        CHECK(it2 == m.end());
    }

    TEST_CASE("Lookup - contains()") {
        Map<int, String> m{{1, "one"}, {2, "two"}, {3, "three"}};

        CHECK(m.contains(1));
        CHECK(m.contains(2));
        CHECK(m.contains(3));
        CHECK_FALSE(m.contains(99));
    }

    TEST_CASE("Lookup - count()") {
        Map<int, String> m{{1, "one"}, {2, "two"}, {3, "three"}};

        CHECK(m.count(1) == 1);
        CHECK(m.count(2) == 1);
        CHECK(m.count(99) == 0);
    }

    TEST_CASE("Modifiers - insert() with pair") {
        Map<int, String> m;
        auto result1 = m.insert({1, "one"});

        CHECK(result1.second == true); // Inserted
        CHECK(result1.first->first == 1);
        CHECK(result1.first->second == String("one"));

        auto result2 = m.insert({1, "ONE"});           // Try to insert duplicate
        CHECK(result2.second == false);                // Not inserted
        CHECK(result2.first->second == String("one")); // Original value unchanged
    }

    TEST_CASE("Modifiers - emplace()") {
        Map<int, String> m;
        auto result1 = m.emplace(1, "one");

        CHECK(result1.second == true);
        CHECK(result1.first->first == 1);
        CHECK(result1.first->second == String("one"));

        auto result2 = m.emplace(1, "ONE"); // Try duplicate
        CHECK(result2.second == false);
    }

    TEST_CASE("Modifiers - insert_or_assign() (C++17)") {
        Map<int, String> m;

        // Insert new key
        auto result1 = m.insert_or_assign(1, "one");
        CHECK(result1.second == true); // Inserted
        CHECK(m[1] == String("one"));

        // Assign to existing key
        auto result2 = m.insert_or_assign(1, "ONE");
        CHECK(result2.second == false); // Not inserted, assigned
        CHECK(m[1] == String("ONE"));   // Value updated
    }

    TEST_CASE("Modifiers - try_emplace() (C++17)") {
        Map<int, String> m;

        // Emplace new key
        auto result1 = m.try_emplace(1, "one");
        CHECK(result1.second == true);
        CHECK(m[1] == String("one"));

        // Try to emplace existing key - should NOT change value
        auto result2 = m.try_emplace(1, "ONE");
        CHECK(result2.second == false);
        CHECK(m[1] == String("one")); // Original value preserved
    }

    TEST_CASE("Modifiers - erase() by key") {
        Map<int, String> m{{1, "one"}, {2, "two"}, {3, "three"}};

        auto count1 = m.erase(2);
        CHECK(count1 == 1);
        CHECK(m.size() == 2);
        CHECK_FALSE(m.contains(2));

        auto count2 = m.erase(99); // Non-existent key
        CHECK(count2 == 0);
        CHECK(m.size() == 2);
    }

    TEST_CASE("Modifiers - erase() by iterator") {
        Map<int, String> m{{1, "one"}, {2, "two"}, {3, "three"}};

        auto it = m.find(2);
        REQUIRE(it != m.end());

        m.erase(it);
        CHECK(m.size() == 2);
        CHECK_FALSE(m.contains(2));
    }

    TEST_CASE("Modifiers - clear()") {
        Map<int, String> m{{1, "one"}, {2, "two"}, {3, "three"}};

        m.clear();
        CHECK(m.size() == 0);
        CHECK(m.empty());
    }

    TEST_CASE("Modifiers - swap()") {
        Map<int, String> m1{{1, "one"}, {2, "two"}};
        Map<int, String> m2{{3, "three"}, {4, "four"}};

        m1.swap(m2);

        CHECK(m1.size() == 2);
        CHECK(m1.contains(3));
        CHECK(m1.contains(4));

        CHECK(m2.size() == 2);
        CHECK(m2.contains(1));
        CHECK(m2.contains(2));
    }

    TEST_CASE("Capacity - size() and empty()") {
        Map<int, String> m;
        CHECK(m.size() == 0);
        CHECK(m.empty());

        m[1] = "one";
        CHECK(m.size() == 1);
        CHECK_FALSE(m.empty());

        m[2] = "two";
        CHECK(m.size() == 2);
    }

    TEST_CASE("Capacity - max_size()") {
        Map<int, String> m;
        CHECK(m.max_size() > 0);
    }

    TEST_CASE("Capacity - capacity() and bucket_count()") {
        Map<int, int> m;

        // Initially should have some capacity
        auto initial_capacity = m.capacity();
        auto initial_buckets = m.bucket_count();

        CHECK(initial_buckets == initial_capacity);

        // Add elements
        for (int i = 0; i < 100; ++i) {
            m[i] = i * 10;
        }

        // Capacity should have grown
        CHECK(m.capacity() > initial_capacity);
        CHECK(m.bucket_count() == m.capacity());
    }

    TEST_CASE("Capacity - reserve()") {
        Map<int, int> m;
        m.reserve(100);

        auto capacity = m.capacity();
        CHECK(capacity >= 100);

        // Add elements - should not trigger resize
        for (int i = 0; i < 50; ++i) {
            m[i] = i * 10;
        }

        CHECK(m.capacity() == capacity); // Capacity unchanged
    }

    TEST_CASE("Capacity - load_factor()") {
        Map<int, String> m;

        // Empty map
        CHECK(m.load_factor() == 0.0f);

        // Add elements
        m[1] = "one";
        m[2] = "two";
        m[3] = "three";

        float lf = m.load_factor();
        CHECK(lf > 0.0f);
        // Load factor can temporarily exceed max during growth
        CHECK(lf >= 0.0f); // Just check it's positive
    }

    TEST_CASE("Capacity - max_load_factor()") {
        Map<int, String> m;

        // Swiss tables target ~87.5% load factor
        CHECK(m.max_load_factor() == doctest::Approx(0.875f));
    }

    TEST_CASE("Iterators - begin() and end()") {
        Map<int, String> m{{1, "one"}, {2, "two"}, {3, "three"}};

        int count = 0;
        for (auto it = m.begin(); it != m.end(); ++it) {
            ++count;
            CHECK(it->first >= 1);
            CHECK(it->first <= 3);
        }

        CHECK(count == 3);
    }

    TEST_CASE("Iterators - range-based for loop") {
        Map<int, String> m{{1, "one"}, {2, "two"}, {3, "three"}};

        int count = 0;
        for (auto const &pair : m) {
            ++count;
            CHECK(pair.first >= 1);
            CHECK(pair.first <= 3);
        }

        CHECK(count == 3);
    }

    TEST_CASE("Iterators - const iterators") {
        Map<int, String> const m{{1, "one"}, {2, "two"}, {3, "three"}};

        int count = 0;
        for (auto it = m.cbegin(); it != m.cend(); ++it) {
            ++count;
        }

        CHECK(count == 3);
    }

    TEST_CASE("Comparison - operator==") {
        Map<int, String> m1{{1, "one"}, {2, "two"}, {3, "three"}};
        Map<int, String> m2{{1, "one"}, {2, "two"}, {3, "three"}};
        Map<int, String> m3{{1, "one"}, {2, "TWO"}, {3, "three"}};

        CHECK(m1 == m2);
        CHECK_FALSE(m1 == m3);
    }

    TEST_CASE("Medium Map - 100 elements") {
        Map<int, int> m;

        // Insert 100 elements
        for (int i = 0; i < 100; ++i) {
            m[i] = i * 2;
        }

        CHECK(m.size() == 100);

        // Verify all elements exist
        for (int i = 0; i < 100; ++i) {
            CHECK(m.contains(i));
            CHECK(m[i] == i * 2);
        }

        // Delete every other element
        for (int i = 0; i < 100; i += 2) {
            auto count = m.erase(i);
            CHECK(count == 1);
        }

        CHECK(m.size() == 50);

        // Verify remaining elements
        for (int i = 1; i < 100; i += 2) {
            CHECK(m.contains(i));
            CHECK(m[i] == i * 2);
        }
    }

    TEST_CASE("Large Map - 10000 elements") {
        Map<int, int> m;

        // Insert 10000 elements
        for (int i = 0; i < 10000; ++i) {
            m[i] = i * 2;
        }

        CHECK(m.size() == 10000);

        // Verify all elements exist
        for (int i = 0; i < 10000; ++i) {
            CHECK(m.contains(i));
            CHECK(m[i] == i * 2);
        }

        // Delete every other element
        for (int i = 0; i < 10000; i += 2) {
            auto count = m.erase(i);
            CHECK(count == 1);
        }

        CHECK(m.size() == 5000);

        // Verify remaining elements
        for (int i = 1; i < 10000; i += 2) {
            CHECK(m.contains(i));
            CHECK(m[i] == i * 2);
        }
    }

    TEST_CASE("String Keys") {
        Map<String, int> m;
        m["one"] = 1;
        m["two"] = 2;
        m["three"] = 3;

        CHECK(m.size() == 3);
        CHECK(m["one"] == 1);
        CHECK(m["two"] == 2);
        CHECK(m["three"] == 3);

        CHECK(m.contains("two"));
        CHECK_FALSE(m.contains("four"));
    }

    TEST_CASE("Backward Compatibility - HashMap still works") {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

        HashMap<int, String> hm{{1, "one"}, {2, "two"}};
        CHECK(hm.size() == 2);
        CHECK(hm[1] == String("one"));

#pragma GCC diagnostic pop
    }

    TEST_CASE("members() Serialization Support") {
        Map<int, String> m{{1, "one"}, {2, "two"}};

        // Ensure members() exists and returns a tuple
        auto tuple = m.members();
        (void)tuple; // Suppress unused variable warning

        // The fact that this compiles means members() works
        CHECK(true);
    }
}
