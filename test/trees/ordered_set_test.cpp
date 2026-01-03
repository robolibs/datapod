#include "datapod/datapod.hpp"
#include <doctest/doctest.h>
#include <string>
#include <vector>

using namespace datapod;

TEST_SUITE("OrderedSet") {

    TEST_CASE("Default construction") {
        OrderedSet<int> set;
        CHECK(set.empty());
        CHECK(set.size() == 0);
    }

    TEST_CASE("Initializer list construction") {
        OrderedSet<int> set{5, 3, 7, 1, 9};
        CHECK(set.size() == 5);
        CHECK(set.contains(1));
        CHECK(set.contains(3));
        CHECK(set.contains(5));
        CHECK(set.contains(7));
        CHECK(set.contains(9));
    }

    TEST_CASE("Insert single element") {
        OrderedSet<int> set;
        auto [it, inserted] = set.insert(42);
        CHECK(inserted);
        CHECK(*it == 42);
        CHECK(set.size() == 1);
        CHECK(set.contains(42));
    }

    TEST_CASE("Insert duplicate element") {
        OrderedSet<int> set;
        set.insert(42);
        auto [it, inserted] = set.insert(42);
        CHECK_FALSE(inserted);
        CHECK(*it == 42);
        CHECK(set.size() == 1);
    }

    TEST_CASE("Insert multiple elements") {
        OrderedSet<int> set;
        for (int i = 0; i < 100; ++i) {
            set.insert(i);
        }
        CHECK(set.size() == 100);
        for (int i = 0; i < 100; ++i) {
            CHECK(set.contains(i));
        }
    }

    TEST_CASE("Insert in reverse order") {
        OrderedSet<int> set;
        for (int i = 99; i >= 0; --i) {
            set.insert(i);
        }
        CHECK(set.size() == 100);

        // Verify sorted iteration
        int expected = 0;
        for (auto it = set.begin(); it != set.end(); ++it) {
            CHECK(*it == expected);
            ++expected;
        }
    }

    TEST_CASE("Find existing element") {
        OrderedSet<int> set{1, 2, 3, 4, 5};
        auto it = set.find(3);
        CHECK(it != set.end());
        CHECK(*it == 3);
    }

    TEST_CASE("Find non-existing element") {
        OrderedSet<int> set{1, 2, 3, 4, 5};
        auto it = set.find(10);
        CHECK(it == set.end());
    }

    TEST_CASE("Contains") {
        OrderedSet<int> set{1, 2, 3};
        CHECK(set.contains(1));
        CHECK(set.contains(2));
        CHECK(set.contains(3));
        CHECK_FALSE(set.contains(0));
        CHECK_FALSE(set.contains(4));
    }

    TEST_CASE("Count") {
        OrderedSet<int> set{1, 2, 3};
        CHECK(set.count(1) == 1);
        CHECK(set.count(2) == 1);
        CHECK(set.count(10) == 0);
    }

    TEST_CASE("Erase by value") {
        OrderedSet<int> set{1, 2, 3, 4, 5};
        size_t erased = set.erase(3);
        CHECK(erased == 1);
        CHECK(set.size() == 4);
        CHECK_FALSE(set.contains(3));

        erased = set.erase(10);
        CHECK(erased == 0);
        CHECK(set.size() == 4);
    }

    TEST_CASE("Erase by iterator") {
        OrderedSet<int> set{1, 2, 3, 4, 5};
        auto it = set.find(3);
        auto next = set.erase(it);
        CHECK(set.size() == 4);
        CHECK_FALSE(set.contains(3));
        CHECK(*next == 4);
    }

    TEST_CASE("Erase all elements") {
        OrderedSet<int> set{1, 2, 3, 4, 5};
        while (!set.empty()) {
            set.erase(set.begin());
        }
        CHECK(set.empty());
        CHECK(set.size() == 0);
    }

    TEST_CASE("Clear") {
        OrderedSet<int> set{1, 2, 3, 4, 5};
        set.clear();
        CHECK(set.empty());
        CHECK(set.size() == 0);

        // Can insert after clear
        set.insert(10);
        CHECK(set.size() == 1);
        CHECK(set.contains(10));
    }

    TEST_CASE("Min and max") {
        OrderedSet<int> set{5, 3, 7, 1, 9, 2, 8};
        CHECK(set.min() == 1);
        CHECK(set.max() == 9);
    }

    TEST_CASE("Min and max on empty set throws") {
        OrderedSet<int> set;
        CHECK_THROWS_AS(set.min(), std::out_of_range);
        CHECK_THROWS_AS(set.max(), std::out_of_range);
    }

    TEST_CASE("Lower bound") {
        OrderedSet<int> set{10, 20, 30, 40, 50};

        auto it = set.lower_bound(25);
        CHECK(it != set.end());
        CHECK(*it == 30);

        it = set.lower_bound(30);
        CHECK(it != set.end());
        CHECK(*it == 30);

        it = set.lower_bound(5);
        CHECK(it != set.end());
        CHECK(*it == 10);

        it = set.lower_bound(55);
        CHECK(it == set.end());
    }

    TEST_CASE("Upper bound") {
        OrderedSet<int> set{10, 20, 30, 40, 50};

        auto it = set.upper_bound(25);
        CHECK(it != set.end());
        CHECK(*it == 30);

        it = set.upper_bound(30);
        CHECK(it != set.end());
        CHECK(*it == 40);

        it = set.upper_bound(50);
        CHECK(it == set.end());
    }

    TEST_CASE("Equal range") {
        OrderedSet<int> set{10, 20, 30, 40, 50};

        auto [lower, upper] = set.equal_range(30);
        CHECK(lower != set.end());
        CHECK(*lower == 30);
        CHECK(upper != set.end());
        CHECK(*upper == 40);

        auto [lower2, upper2] = set.equal_range(25);
        CHECK(lower2 == upper2);
        CHECK(*lower2 == 30);
    }

    TEST_CASE("Forward iteration") {
        OrderedSet<int> set{5, 3, 7, 1, 9};
        std::vector<int> result;
        for (auto it = set.begin(); it != set.end(); ++it) {
            result.push_back(*it);
        }
        CHECK(result == std::vector<int>{1, 3, 5, 7, 9});
    }

    TEST_CASE("Range-based for loop") {
        OrderedSet<int> set{5, 3, 7, 1, 9};
        std::vector<int> result;
        for (int const &val : set) {
            result.push_back(val);
        }
        CHECK(result == std::vector<int>{1, 3, 5, 7, 9});
    }

    TEST_CASE("Reverse iteration") {
        OrderedSet<int> set{5, 3, 7, 1, 9};
        std::vector<int> result;
        for (auto it = set.rbegin(); it != set.rend(); ++it) {
            result.push_back(*it);
        }
        CHECK(result == std::vector<int>{9, 7, 5, 3, 1});
    }

    TEST_CASE("Bidirectional iterator") {
        OrderedSet<int> set{1, 2, 3, 4, 5};
        auto it = set.begin();
        ++it;
        ++it;
        CHECK(*it == 3);
        --it;
        CHECK(*it == 2);
    }

    TEST_CASE("Copy construction") {
        OrderedSet<int> original{1, 2, 3, 4, 5};
        OrderedSet<int> copy(original);

        CHECK(copy.size() == original.size());
        for (int i = 1; i <= 5; ++i) {
            CHECK(copy.contains(i));
        }

        // Modify original, copy should be unchanged
        original.insert(10);
        CHECK_FALSE(copy.contains(10));
    }

    TEST_CASE("Move construction") {
        OrderedSet<int> original{1, 2, 3, 4, 5};
        OrderedSet<int> moved(std::move(original));

        CHECK(moved.size() == 5);
        for (int i = 1; i <= 5; ++i) {
            CHECK(moved.contains(i));
        }
        CHECK(original.empty());
    }

    TEST_CASE("Copy assignment") {
        OrderedSet<int> original{1, 2, 3};
        OrderedSet<int> copy;
        copy = original;

        CHECK(copy.size() == 3);
        CHECK(copy.contains(1));
        CHECK(copy.contains(2));
        CHECK(copy.contains(3));
    }

    TEST_CASE("Move assignment") {
        OrderedSet<int> original{1, 2, 3};
        OrderedSet<int> moved;
        moved = std::move(original);

        CHECK(moved.size() == 3);
        CHECK(original.empty());
    }

    TEST_CASE("Equality comparison") {
        OrderedSet<int> set1{1, 2, 3};
        OrderedSet<int> set2{1, 2, 3};
        OrderedSet<int> set3{1, 2, 4};

        CHECK(set1 == set2);
        CHECK_FALSE(set1 == set3);
        CHECK(set1 != set3);
    }

    TEST_CASE("Emplace") {
        OrderedSet<std::string> set;
        auto [it, inserted] = set.emplace("hello");
        CHECK(inserted);
        CHECK(*it == "hello");
    }

    TEST_CASE("Custom comparator - greater") {
        OrderedSet<int, std::greater<int>> set{5, 3, 7, 1, 9};
        std::vector<int> result;
        for (int const &val : set) {
            result.push_back(val);
        }
        CHECK(result == std::vector<int>{9, 7, 5, 3, 1});
    }

    TEST_CASE("String elements") {
        OrderedSet<String> set;
        set.insert(String("banana"));
        set.insert(String("apple"));
        set.insert(String("cherry"));

        CHECK(set.size() == 3);
        CHECK(set.min().view() == "apple");
        CHECK(set.max().view() == "cherry");
    }

    TEST_CASE("Serialization roundtrip") {
        OrderedSet<int> original{5, 3, 7, 1, 9, 2, 8, 4, 6};

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, OrderedSet<int>>(buffer);

        CHECK(restored.size() == original.size());
        auto it1 = original.begin();
        auto it2 = restored.begin();
        while (it1 != original.end()) {
            CHECK(*it1 == *it2);
            ++it1;
            ++it2;
        }
    }

    TEST_CASE("Serialization with strings") {
        OrderedSet<String> original;
        original.insert(String("zebra"));
        original.insert(String("apple"));
        original.insert(String("mango"));

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, OrderedSet<String>>(buffer);

        CHECK(restored.size() == 3);
        CHECK(restored.min().view() == "apple");
        CHECK(restored.max().view() == "zebra");
    }

    TEST_CASE("Large set stress test") {
        OrderedSet<int> set;
        constexpr int N = 1000;

        // Insert
        for (int i = 0; i < N; ++i) {
            set.insert(i);
        }
        CHECK(set.size() == N);

        // Verify sorted order
        int expected = 0;
        for (int const &val : set) {
            CHECK(val == expected);
            ++expected;
        }

        // Erase half
        for (int i = 0; i < N; i += 2) {
            set.erase(i);
        }
        CHECK(set.size() == N / 2);

        // Verify remaining
        for (int i = 0; i < N; ++i) {
            if (i % 2 == 0) {
                CHECK_FALSE(set.contains(i));
            } else {
                CHECK(set.contains(i));
            }
        }
    }

    TEST_CASE("Iterator arrow operator") {
        struct Point {
            int x, y;
            bool operator<(Point const &other) const { return x < other.x || (x == other.x && y < other.y); }
        };

        OrderedSet<Point> set;
        set.insert(Point{1, 2});
        set.insert(Point{3, 4});

        auto it = set.begin();
        CHECK(it->x == 1);
        CHECK(it->y == 2);
    }

    TEST_CASE("Const iteration") {
        OrderedSet<int> const set{1, 2, 3, 4, 5};
        std::vector<int> result;
        for (auto it = set.begin(); it != set.end(); ++it) {
            result.push_back(*it);
        }
        CHECK(result == std::vector<int>{1, 2, 3, 4, 5});
    }

    TEST_CASE("Range query use case") {
        OrderedSet<int> scores{10, 25, 50, 75, 100, 150, 200};

        // Find all scores between 40 and 120
        std::vector<int> in_range;
        for (auto it = scores.lower_bound(40); it != scores.end() && *it <= 120; ++it) {
            in_range.push_back(*it);
        }
        CHECK(in_range == std::vector<int>{50, 75, 100});
    }
}
