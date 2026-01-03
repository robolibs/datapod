#include <doctest/doctest.h>

#include "datapod/datapod.hpp"

using namespace datapod;

TEST_CASE("OrderedMap: default construction") {
    OrderedMap<int, std::string> map;
    CHECK(map.empty());
    CHECK(map.size() == 0);
}

TEST_CASE("OrderedMap: initializer list construction") {
    OrderedMap<int, std::string> map{{1, "one"}, {3, "three"}, {2, "two"}};
    CHECK(map.size() == 3);
    CHECK(map.at(1) == "one");
    CHECK(map.at(2) == "two");
    CHECK(map.at(3) == "three");
}

TEST_CASE("OrderedMap: insert") {
    OrderedMap<int, std::string> map;

    auto [it1, inserted1] = map.insert(2, "two");
    CHECK(inserted1);
    CHECK(it1.key() == 2);
    CHECK(it1.value() == "two");
    CHECK(map.size() == 1);

    auto [it2, inserted2] = map.insert(1, "one");
    CHECK(inserted2);
    CHECK(map.size() == 2);

    auto [it3, inserted3] = map.insert(3, "three");
    CHECK(inserted3);
    CHECK(map.size() == 3);

    // Duplicate key
    auto [it4, inserted4] = map.insert(2, "TWO");
    CHECK(!inserted4);
    CHECK(map.size() == 3);
    CHECK(map.at(2) == "two"); // Original value unchanged
}

TEST_CASE("OrderedMap: operator[]") {
    OrderedMap<int, std::string> map;

    map[1] = "one";
    map[2] = "two";
    map[3] = "three";

    CHECK(map.size() == 3);
    CHECK(map[1] == "one");
    CHECK(map[2] == "two");
    CHECK(map[3] == "three");

    // Modify existing
    map[2] = "TWO";
    CHECK(map[2] == "TWO");
}

TEST_CASE("OrderedMap: at") {
    OrderedMap<int, std::string> map{{1, "one"}, {2, "two"}};

    CHECK(map.at(1) == "one");
    CHECK(map.at(2) == "two");
    CHECK_THROWS_AS(map.at(3), std::out_of_range);

    // Const version
    OrderedMap<int, std::string> const &cmap = map;
    CHECK(cmap.at(1) == "one");
    CHECK_THROWS_AS(cmap.at(3), std::out_of_range);
}

TEST_CASE("OrderedMap: find") {
    OrderedMap<int, std::string> map{{1, "one"}, {2, "two"}, {3, "three"}};

    auto it = map.find(2);
    CHECK(it != map.end());
    CHECK(it.key() == 2);
    CHECK(it.value() == "two");

    auto it_not_found = map.find(4);
    CHECK(it_not_found == map.end());
}

TEST_CASE("OrderedMap: contains and count") {
    OrderedMap<int, std::string> map{{1, "one"}, {2, "two"}};

    CHECK(map.contains(1));
    CHECK(map.contains(2));
    CHECK(!map.contains(3));

    CHECK(map.count(1) == 1);
    CHECK(map.count(3) == 0);
}

TEST_CASE("OrderedMap: erase by key") {
    OrderedMap<int, std::string> map{{1, "one"}, {2, "two"}, {3, "three"}};

    CHECK(map.erase(2) == 1);
    CHECK(map.size() == 2);
    CHECK(!map.contains(2));

    CHECK(map.erase(4) == 0); // Key not found
    CHECK(map.size() == 2);
}

TEST_CASE("OrderedMap: erase by iterator") {
    OrderedMap<int, std::string> map{{1, "one"}, {2, "two"}, {3, "three"}};

    auto it = map.find(2);
    auto next_it = map.erase(it);

    CHECK(map.size() == 2);
    CHECK(!map.contains(2));
    CHECK(next_it.key() == 3);
}

TEST_CASE("OrderedMap: clear") {
    OrderedMap<int, std::string> map{{1, "one"}, {2, "two"}, {3, "three"}};
    CHECK(map.size() == 3);

    map.clear();
    CHECK(map.empty());
}

TEST_CASE("OrderedMap: sorted iteration") {
    OrderedMap<int, std::string> map;
    map.insert(5, "five");
    map.insert(2, "two");
    map.insert(8, "eight");
    map.insert(1, "one");
    map.insert(9, "nine");
    map.insert(3, "three");

    Vector<int> keys;
    for (auto it = map.begin(); it != map.end(); ++it) {
        keys.push_back(it.key());
    }

    CHECK(keys.size() == 6);
    CHECK(keys[0] == 1);
    CHECK(keys[1] == 2);
    CHECK(keys[2] == 3);
    CHECK(keys[3] == 5);
    CHECK(keys[4] == 8);
    CHECK(keys[5] == 9);
}

TEST_CASE("OrderedMap: reverse iteration") {
    OrderedMap<int, std::string> map{{1, "one"}, {2, "two"}, {3, "three"}};

    Vector<int> keys;
    for (auto it = map.rbegin(); it != map.rend(); ++it) {
        auto [k, v] = *it;
        keys.push_back(k);
    }

    CHECK(keys.size() == 3);
    CHECK(keys[0] == 3);
    CHECK(keys[1] == 2);
    CHECK(keys[2] == 1);
}

TEST_CASE("OrderedMap: lower_bound") {
    OrderedMap<int, std::string> map{{1, "one"}, {3, "three"}, {5, "five"}, {7, "seven"}};

    auto it1 = map.lower_bound(3);
    CHECK(it1.key() == 3);

    auto it2 = map.lower_bound(4);
    CHECK(it2.key() == 5);

    auto it3 = map.lower_bound(0);
    CHECK(it3.key() == 1);

    auto it4 = map.lower_bound(8);
    CHECK(it4 == map.end());
}

TEST_CASE("OrderedMap: upper_bound") {
    OrderedMap<int, std::string> map{{1, "one"}, {3, "three"}, {5, "five"}, {7, "seven"}};

    auto it1 = map.upper_bound(3);
    CHECK(it1.key() == 5);

    auto it2 = map.upper_bound(4);
    CHECK(it2.key() == 5);

    auto it3 = map.upper_bound(0);
    CHECK(it3.key() == 1);

    auto it4 = map.upper_bound(7);
    CHECK(it4 == map.end());
}

TEST_CASE("OrderedMap: min_key and max_key") {
    OrderedMap<int, std::string> map{{5, "five"}, {2, "two"}, {8, "eight"}, {1, "one"}};

    CHECK(map.min_key() == 1);
    CHECK(map.max_key() == 8);
}

TEST_CASE("OrderedMap: min_key and max_key throw on empty") {
    OrderedMap<int, std::string> map;
    CHECK_THROWS_AS(map.min_key(), std::out_of_range);
    CHECK_THROWS_AS(map.max_key(), std::out_of_range);
}

TEST_CASE("OrderedMap: copy construction") {
    OrderedMap<int, std::string> map1{{1, "one"}, {2, "two"}, {3, "three"}};
    OrderedMap<int, std::string> map2(map1);

    CHECK(map2.size() == 3);
    CHECK(map2.at(1) == "one");
    CHECK(map2.at(2) == "two");
    CHECK(map2.at(3) == "three");

    // Modify original
    map1[4] = "four";
    CHECK(map1.size() == 4);
    CHECK(map2.size() == 3);
}

TEST_CASE("OrderedMap: move construction") {
    OrderedMap<int, std::string> map1{{1, "one"}, {2, "two"}};
    OrderedMap<int, std::string> map2(std::move(map1));

    CHECK(map2.size() == 2);
    CHECK(map2.at(1) == "one");
    CHECK(map1.empty());
}

TEST_CASE("OrderedMap: copy assignment") {
    OrderedMap<int, std::string> map1{{1, "one"}, {2, "two"}};
    OrderedMap<int, std::string> map2;

    map2 = map1;

    CHECK(map2.size() == 2);
    CHECK(map2.at(1) == "one");
}

TEST_CASE("OrderedMap: move assignment") {
    OrderedMap<int, std::string> map1{{1, "one"}, {2, "two"}};
    OrderedMap<int, std::string> map2;

    map2 = std::move(map1);

    CHECK(map2.size() == 2);
    CHECK(map2.at(1) == "one");
    CHECK(map1.empty());
}

TEST_CASE("OrderedMap: equality comparison") {
    OrderedMap<int, std::string> map1{{1, "one"}, {2, "two"}};
    OrderedMap<int, std::string> map2{{1, "one"}, {2, "two"}};
    OrderedMap<int, std::string> map3{{1, "one"}, {2, "TWO"}};
    OrderedMap<int, std::string> map4{{1, "one"}};

    CHECK(map1 == map2);
    CHECK(map1 != map3);
    CHECK(map1 != map4);
}

TEST_CASE("OrderedMap: members() for serialization") {
    OrderedMap<int, std::string> map{{1, "one"}, {2, "two"}};

    auto m = map.members();
    static_assert(std::tuple_size_v<decltype(m)> == 4, "members() should return 4 elements");

    OrderedMap<int, std::string> const &cmap = map;
    auto cm = cmap.members();
    static_assert(std::tuple_size_v<decltype(cm)> == 4, "const members() should return 4 elements");
}

TEST_CASE("OrderedMap: serialization round-trip") {
    OrderedMap<int, int> original;
    original.insert(5, 50);
    original.insert(2, 20);
    original.insert(8, 80);
    original.insert(1, 10);
    original.insert(9, 90);

    auto buf = serialize(original);
    auto restored = deserialize<Mode::NONE, OrderedMap<int, int>>(buf);

    CHECK(restored.size() == original.size());
    CHECK(restored == original);
}

TEST_CASE("OrderedMap: with string keys") {
    OrderedMap<std::string, int> map;
    map["banana"] = 2;
    map["apple"] = 1;
    map["cherry"] = 3;

    CHECK(map.size() == 3);

    // Should iterate in sorted order
    Vector<std::string> keys;
    for (auto it = map.begin(); it != map.end(); ++it) {
        keys.push_back(std::string(it.key()));
    }

    CHECK(keys[0] == "apple");
    CHECK(keys[1] == "banana");
    CHECK(keys[2] == "cherry");
}

TEST_CASE("OrderedMap: custom comparator (reverse order)") {
    OrderedMap<int, std::string, std::greater<int>> map;
    map.insert(1, "one");
    map.insert(3, "three");
    map.insert(2, "two");

    Vector<int> keys;
    for (auto it = map.begin(); it != map.end(); ++it) {
        keys.push_back(it.key());
    }

    // Should be in reverse order
    CHECK(keys[0] == 3);
    CHECK(keys[1] == 2);
    CHECK(keys[2] == 1);
}

TEST_CASE("OrderedMap: range query") {
    OrderedMap<int, std::string> map;
    for (int i = 1; i <= 10; ++i) {
        map.insert(i, "val" + std::to_string(i));
    }

    // Get all keys in range [3, 7)
    Vector<int> range_keys;
    for (auto it = map.lower_bound(3); it != map.upper_bound(6); ++it) {
        range_keys.push_back(it.key());
    }

    CHECK(range_keys.size() == 4);
    CHECK(range_keys[0] == 3);
    CHECK(range_keys[1] == 4);
    CHECK(range_keys[2] == 5);
    CHECK(range_keys[3] == 6);
}

TEST_CASE("OrderedMap: stress test - insert and erase") {
    OrderedMap<int, int> map;

    // Insert 100 elements
    for (int i = 0; i < 100; ++i) {
        map.insert(i, i * 10);
    }
    CHECK(map.size() == 100);

    // Verify sorted order
    int prev = -1;
    for (auto it = map.begin(); it != map.end(); ++it) {
        CHECK(it.key() > prev);
        prev = it.key();
    }

    // Erase every other element
    for (int i = 0; i < 100; i += 2) {
        map.erase(i);
    }
    CHECK(map.size() == 50);

    // Verify remaining elements
    for (int i = 1; i < 100; i += 2) {
        CHECK(map.contains(i));
        CHECK(map.at(i) == i * 10);
    }
}

TEST_CASE("OrderedMap: node reuse via free list") {
    OrderedMap<int, int> map;

    // Insert and erase
    for (int i = 0; i < 10; ++i) {
        map.insert(i, i);
    }
    for (int i = 0; i < 5; ++i) {
        map.erase(i);
    }

    // Insert more - should reuse freed nodes
    for (int i = 100; i < 105; ++i) {
        map.insert(i, i);
    }

    CHECK(map.size() == 10);
}

TEST_CASE("OrderedMap: bidirectional iterator") {
    OrderedMap<int, std::string> map{{1, "one"}, {2, "two"}, {3, "three"}};

    auto it = map.begin();
    CHECK(it.key() == 1);
    ++it;
    CHECK(it.key() == 2);
    ++it;
    CHECK(it.key() == 3);
    --it;
    CHECK(it.key() == 2);
    --it;
    CHECK(it.key() == 1);
}

TEST_CASE("OrderedMap: single element") {
    OrderedMap<int, std::string> map;
    map.insert(42, "answer");

    CHECK(map.size() == 1);
    CHECK(map.min_key() == 42);
    CHECK(map.max_key() == 42);
    CHECK(map.at(42) == "answer");

    map.erase(42);
    CHECK(map.empty());
}
