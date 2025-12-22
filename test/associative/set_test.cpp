#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "datapod/associative/set.hpp"
#include "datapod/sequential/string.hpp"

using namespace datapod;

TEST_SUITE("Set") {

    // ========================================================================
    // Construction Tests
    // ========================================================================

    TEST_CASE("Default Construction") {
        Set<int> s;
        CHECK(s.empty());
        CHECK(s.size() == 0);
    }

    TEST_CASE("Initializer List Construction") {
        Set<int> s{1, 2, 3, 4, 5};
        CHECK(s.size() == 5);
        CHECK(s.contains(1));
        CHECK(s.contains(3));
        CHECK(s.contains(5));
        CHECK_FALSE(s.contains(10));
    }

    TEST_CASE("Copy Construction") {
        Set<int> s1{10, 20, 30};
        Set<int> s2(s1);

        CHECK(s1.size() == 3);
        CHECK(s2.size() == 3);
        CHECK(s2.contains(10));
        CHECK(s2.contains(20));
        CHECK(s2.contains(30));
    }

    TEST_CASE("Move Construction") {
        Set<int> s1{1, 2, 3};
        Set<int> s2(std::move(s1));

        CHECK(s2.size() == 3);
        CHECK(s2.contains(1));
        CHECK(s2.contains(2));
        CHECK(s2.contains(3));
    }

    // ========================================================================
    // Lookup Tests
    // ========================================================================

    TEST_CASE("find() - Found") {
        Set<int> s{10, 20, 30};
        auto it = s.find(20);

        REQUIRE(it != s.end());
        CHECK(*it == 20);
    }

    TEST_CASE("find() - Not Found") {
        Set<int> s{10, 20, 30};
        auto it = s.find(99);

        CHECK(it == s.end());
    }

    TEST_CASE("contains() - Basic") {
        Set<int> s{5, 10, 15};

        CHECK(s.contains(5));
        CHECK(s.contains(10));
        CHECK(s.contains(15));
        CHECK_FALSE(s.contains(0));
        CHECK_FALSE(s.contains(20));
    }

    TEST_CASE("count() - Basic") {
        Set<int> s{1, 2, 3};

        CHECK(s.count(1) == 1);
        CHECK(s.count(2) == 1);
        CHECK(s.count(99) == 0);
    }

    // ========================================================================
    // Modifiers Tests
    // ========================================================================

    TEST_CASE("insert() - Single Value") {
        Set<int> s;
        auto [it1, inserted1] = s.insert(42);

        CHECK(inserted1);
        CHECK(*it1 == 42);
        CHECK(s.size() == 1);

        // Insert duplicate
        auto [it2, inserted2] = s.insert(42);
        CHECK_FALSE(inserted2);
        CHECK(s.size() == 1);
    }

    TEST_CASE("insert() - Multiple Values") {
        Set<int> s;
        s.insert(1);
        s.insert(2);
        s.insert(3);

        CHECK(s.size() == 3);
        CHECK(s.contains(1));
        CHECK(s.contains(2));
        CHECK(s.contains(3));
    }

    TEST_CASE("emplace() - Basic") {
        Set<String> s;
        auto [it, inserted] = s.emplace("hello");

        CHECK(inserted);
        CHECK(*it == String("hello"));
        CHECK(s.size() == 1);
    }

    TEST_CASE("erase() - By Iterator") {
        Set<int> s{10, 20, 30};

        auto it = s.find(20);
        REQUIRE(it != s.end());

        s.erase(it);

        CHECK(s.size() == 2);
        CHECK(s.contains(10));
        CHECK_FALSE(s.contains(20));
        CHECK(s.contains(30));
    }

    TEST_CASE("erase() - By Key") {
        Set<int> s{1, 2, 3, 4, 5};

        size_t removed = s.erase(3);

        CHECK(removed == 1);
        CHECK(s.size() == 4);
        CHECK_FALSE(s.contains(3));

        // Try to erase non-existent
        removed = s.erase(99);
        CHECK(removed == 0);
        CHECK(s.size() == 4);
    }

    TEST_CASE("clear()") {
        Set<int> s{1, 2, 3, 4, 5};
        CHECK(s.size() == 5);

        s.clear();

        CHECK(s.empty());
        CHECK(s.size() == 0);
    }

    TEST_CASE("swap()") {
        Set<int> s1{1, 2, 3};
        Set<int> s2{10, 20};

        s1.swap(s2);

        CHECK(s1.size() == 2);
        CHECK(s1.contains(10));
        CHECK(s1.contains(20));

        CHECK(s2.size() == 3);
        CHECK(s2.contains(1));
        CHECK(s2.contains(2));
        CHECK(s2.contains(3));
    }

    // ========================================================================
    // Capacity Tests
    // ========================================================================

    TEST_CASE("size() and empty()") {
        Set<int> s;
        CHECK(s.empty());
        CHECK(s.size() == 0);

        s.insert(1);
        CHECK_FALSE(s.empty());
        CHECK(s.size() == 1);

        s.insert(2);
        s.insert(3);
        CHECK(s.size() == 3);
    }

    TEST_CASE("max_size()") {
        Set<int> s;
        CHECK(s.max_size() > 0);
    }

    TEST_CASE("reserve()") {
        Set<int> s;
        s.reserve(100);

        // After reserve, should be able to insert without reallocation
        for (int i = 0; i < 50; ++i) {
            s.insert(i);
        }

        CHECK(s.size() == 50);
    }

    TEST_CASE("capacity()") {
        Set<int> s;
        CHECK(s.capacity() >= 0);

        s.reserve(100);
        CHECK(s.capacity() >= 100);
    }

    TEST_CASE("bucket_count()") {
        Set<int> s;
        auto initial_buckets = s.bucket_count();
        CHECK(initial_buckets >= 0);

        // Insert elements to trigger rehash
        for (int i = 0; i < 100; ++i) {
            s.insert(i);
        }

        auto final_buckets = s.bucket_count();
        CHECK(final_buckets > 0);
        CHECK(final_buckets >= initial_buckets);
    }

    TEST_CASE("load_factor() and max_load_factor()") {
        Set<int> s;
        CHECK(s.load_factor() >= 0.0f);
        CHECK(s.max_load_factor() > 0.0f);

        s.insert(1);
        s.insert(2);
        CHECK(s.load_factor() > 0.0f);
    }

    // ========================================================================
    // Iterator Tests
    // ========================================================================

    TEST_CASE("Iterators - Range-based for") {
        Set<int> s{1, 2, 3, 4, 5};

        int sum = 0;
        for (auto val : s) {
            sum += val;
        }

        CHECK(sum == 15); // 1+2+3+4+5
    }

    TEST_CASE("Iterators - begin/end") {
        Set<int> s{10, 20, 30};

        int count = 0;
        for (auto it = s.begin(); it != s.end(); ++it) {
            ++count;
        }

        CHECK(count == 3);
    }

    TEST_CASE("Iterators - Const Iterators") {
        Set<int> const s{1, 2, 3};

        int sum = 0;
        for (auto it = s.begin(); it != s.end(); ++it) {
            sum += *it;
        }

        CHECK(sum == 6);
    }

    // ========================================================================
    // Comparison Tests
    // ========================================================================

    TEST_CASE("operator== - Equal Sets") {
        Set<int> s1{1, 2, 3};
        Set<int> s2{1, 2, 3};

        CHECK(s1 == s2);
    }

    TEST_CASE("operator== - Different Sets") {
        Set<int> s1{1, 2, 3};
        Set<int> s2{4, 5, 6};

        CHECK_FALSE(s1 == s2);
    }

    TEST_CASE("operator== - Different Sizes") {
        Set<int> s1{1, 2};
        Set<int> s2{1, 2, 3};

        CHECK_FALSE(s1 == s2);
    }

    // ========================================================================
    // Large Set Test
    // ========================================================================

    TEST_CASE("Large Set - 10000 elements") {
        Set<int> s;

        // Insert 10000 elements
        for (int i = 0; i < 10000; ++i) {
            auto [it, inserted] = s.insert(i);
            CHECK(inserted);
            CHECK(*it == i);
        }

        CHECK(s.size() == 10000);

        // Verify all elements are present
        for (int i = 0; i < 10000; ++i) {
            CHECK(s.contains(i));
        }

        // Verify non-existent elements
        CHECK_FALSE(s.contains(10000));
        CHECK_FALSE(s.contains(-1));
    }

    // ========================================================================
    // String Key Test
    // ========================================================================

    TEST_CASE("Set with String Elements") {
        Set<String> s;
        s.insert(String("apple"));
        s.insert(String("banana"));
        s.insert(String("cherry"));

        CHECK(s.size() == 3);
        CHECK(s.contains(String("apple")));
        CHECK(s.contains(String("banana")));
        CHECK(s.contains(String("cherry")));
        CHECK_FALSE(s.contains(String("orange")));

        // Try to insert duplicate
        auto [it, inserted] = s.insert(String("apple"));
        CHECK_FALSE(inserted);
        CHECK(s.size() == 3);
    }

    // ========================================================================
    // Serialization Support Test
    // ========================================================================

    TEST_CASE("members() Serialization Support") {
        Set<int> s{1, 2, 3};

        // Verify that members() function exists (required for serialization)
        auto m = s.members();
        (void)m; // Suppress unused warning

        // This test just verifies the members() method compiles and can be called
        // Actual serialization is tested in serialization_hash_test.cpp
    }

    // ========================================================================
    // Duplicate Insertion Test
    // ========================================================================

    TEST_CASE("Duplicate Insertion Handling") {
        Set<int> s;

        auto [it1, inserted1] = s.insert(42);
        CHECK(inserted1);
        CHECK(s.size() == 1);

        auto [it2, inserted2] = s.insert(42);
        CHECK_FALSE(inserted2);
        CHECK(s.size() == 1);
        CHECK(it1 == it2);
    }

    // ========================================================================
    // Copy Assignment Test
    // ========================================================================

    TEST_CASE("Copy Assignment") {
        Set<int> s1{1, 2, 3};
        Set<int> s2;

        s2 = s1;

        CHECK(s1.size() == 3);
        CHECK(s2.size() == 3);
        CHECK(s2.contains(1));
        CHECK(s2.contains(2));
        CHECK(s2.contains(3));
    }

    // ========================================================================
    // Move Assignment Test
    // ========================================================================

    TEST_CASE("Move Assignment") {
        Set<int> s1{10, 20, 30};
        Set<int> s2;

        s2 = std::move(s1);

        CHECK(s2.size() == 3);
        CHECK(s2.contains(10));
        CHECK(s2.contains(20));
        CHECK(s2.contains(30));
    }

    // ========================================================================
    // Edge Cases
    // ========================================================================

    TEST_CASE("Insert and Erase All") {
        Set<int> s{1, 2, 3, 4, 5};

        s.erase(1);
        s.erase(2);
        s.erase(3);
        s.erase(4);
        s.erase(5);

        CHECK(s.empty());
        CHECK(s.size() == 0);
    }

    TEST_CASE("Multiple Inserts and Erases") {
        Set<int> s;

        for (int i = 0; i < 100; ++i) {
            s.insert(i);
        }
        CHECK(s.size() == 100);

        for (int i = 0; i < 50; ++i) {
            s.erase(i);
        }
        CHECK(s.size() == 50);

        for (int i = 0; i < 50; ++i) {
            CHECK_FALSE(s.contains(i));
        }

        for (int i = 50; i < 100; ++i) {
            CHECK(s.contains(i));
        }
    }

} // TEST_SUITE("Set")
