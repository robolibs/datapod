#include <doctest/doctest.h>

#include "datapod/datapod.hpp"

using namespace datapod;

TEST_CASE("List: default construction") {
    List<int> list;
    CHECK(list.empty());
    CHECK(list.size() == 0);
}

TEST_CASE("List: initializer list construction") {
    List<int> list{1, 2, 3, 4, 5};
    CHECK(list.size() == 5);
    CHECK(list.front() == 1);
    CHECK(list.back() == 5);

    // Check order
    auto it = list.begin();
    CHECK(*it++ == 1);
    CHECK(*it++ == 2);
    CHECK(*it++ == 3);
    CHECK(*it++ == 4);
    CHECK(*it++ == 5);
    CHECK(it == list.end());
}

TEST_CASE("List: push_front") {
    List<int> list;

    list.push_front(3);
    CHECK(list.size() == 1);
    CHECK(list.front() == 3);
    CHECK(list.back() == 3);

    list.push_front(2);
    CHECK(list.size() == 2);
    CHECK(list.front() == 2);
    CHECK(list.back() == 3);

    list.push_front(1);
    CHECK(list.size() == 3);
    CHECK(list.front() == 1);
    CHECK(list.back() == 3);
}

TEST_CASE("List: push_back") {
    List<int> list;

    list.push_back(1);
    CHECK(list.size() == 1);
    CHECK(list.front() == 1);
    CHECK(list.back() == 1);

    list.push_back(2);
    CHECK(list.size() == 2);
    CHECK(list.front() == 1);
    CHECK(list.back() == 2);

    list.push_back(3);
    CHECK(list.size() == 3);
    CHECK(list.front() == 1);
    CHECK(list.back() == 3);
}

TEST_CASE("List: pop_front") {
    List<int> list{1, 2, 3};

    list.pop_front();
    CHECK(list.size() == 2);
    CHECK(list.front() == 2);

    list.pop_front();
    CHECK(list.size() == 1);
    CHECK(list.front() == 3);

    list.pop_front();
    CHECK(list.empty());
}

TEST_CASE("List: pop_back") {
    List<int> list{1, 2, 3};

    list.pop_back();
    CHECK(list.size() == 2);
    CHECK(list.back() == 2);

    list.pop_back();
    CHECK(list.size() == 1);
    CHECK(list.back() == 1);

    list.pop_back();
    CHECK(list.empty());
}

TEST_CASE("List: pop_front throws on empty") {
    List<int> list;
    CHECK_THROWS_AS(list.pop_front(), std::out_of_range);
}

TEST_CASE("List: pop_back throws on empty") {
    List<int> list;
    CHECK_THROWS_AS(list.pop_back(), std::out_of_range);
}

TEST_CASE("List: front/back throw on empty") {
    List<int> list;
    CHECK_THROWS_AS(list.front(), std::out_of_range);
    CHECK_THROWS_AS(list.back(), std::out_of_range);

    List<int> const &clist = list;
    CHECK_THROWS_AS(clist.front(), std::out_of_range);
    CHECK_THROWS_AS(clist.back(), std::out_of_range);
}

TEST_CASE("List: emplace_front and emplace_back") {
    List<std::pair<int, int>> list;

    list.emplace_front(1, 2);
    CHECK(list.size() == 1);
    CHECK(list.front().first == 1);
    CHECK(list.front().second == 2);

    list.emplace_back(3, 4);
    CHECK(list.size() == 2);
    CHECK(list.back().first == 3);
    CHECK(list.back().second == 4);
}

TEST_CASE("List: insert") {
    List<int> list{1, 3};

    auto it = list.begin();
    ++it; // Point to 3
    list.insert(it, 2);

    CHECK(list.size() == 3);

    // Check order: 1, 2, 3
    it = list.begin();
    CHECK(*it++ == 1);
    CHECK(*it++ == 2);
    CHECK(*it++ == 3);
}

TEST_CASE("List: insert at beginning") {
    List<int> list{2, 3};

    list.insert(list.begin(), 1);

    CHECK(list.size() == 3);
    CHECK(list.front() == 1);
}

TEST_CASE("List: insert at end") {
    List<int> list{1, 2};

    list.insert(list.end(), 3);

    CHECK(list.size() == 3);
    CHECK(list.back() == 3);
}

TEST_CASE("List: erase") {
    List<int> list{1, 2, 3};

    auto it = list.begin();
    ++it; // Point to 2
    it = list.erase(it);

    CHECK(list.size() == 2);
    CHECK(*it == 3);

    // Check order: 1, 3
    it = list.begin();
    CHECK(*it++ == 1);
    CHECK(*it++ == 3);
}

TEST_CASE("List: erase first element") {
    List<int> list{1, 2, 3};

    list.erase(list.begin());

    CHECK(list.size() == 2);
    CHECK(list.front() == 2);
}

TEST_CASE("List: erase last element") {
    List<int> list{1, 2, 3};

    auto it = list.begin();
    ++it;
    ++it; // Point to 3
    list.erase(it);

    CHECK(list.size() == 2);
    CHECK(list.back() == 2);
}

TEST_CASE("List: clear") {
    List<int> list{1, 2, 3, 4, 5};
    CHECK(list.size() == 5);

    list.clear();
    CHECK(list.empty());
    CHECK(list.size() == 0);
}

TEST_CASE("List: reverse") {
    List<int> list{1, 2, 3, 4, 5};

    list.reverse();

    auto it = list.begin();
    CHECK(*it++ == 5);
    CHECK(*it++ == 4);
    CHECK(*it++ == 3);
    CHECK(*it++ == 2);
    CHECK(*it++ == 1);

    CHECK(list.front() == 5);
    CHECK(list.back() == 1);
}

TEST_CASE("List: move_to_front") {
    List<int> list{1, 2, 3, 4, 5};

    auto it = list.begin();
    ++it;
    ++it; // Point to 3
    list.move_to_front(it);

    CHECK(list.front() == 3);
    CHECK(list.size() == 5);

    // Check order: 3, 1, 2, 4, 5
    it = list.begin();
    CHECK(*it++ == 3);
    CHECK(*it++ == 1);
    CHECK(*it++ == 2);
    CHECK(*it++ == 4);
    CHECK(*it++ == 5);
}

TEST_CASE("List: move_to_front last element") {
    List<int> list{1, 2, 3};

    auto it = list.begin();
    ++it;
    ++it; // Point to 3 (last)
    list.move_to_front(it);

    CHECK(list.front() == 3);
    CHECK(list.back() == 2);
}

TEST_CASE("List: copy construction") {
    List<int> list1{1, 2, 3};
    List<int> list2(list1);

    CHECK(list2.size() == 3);
    CHECK(list2.front() == 1);
    CHECK(list2.back() == 3);

    // Modify original, copy should be unchanged
    list1.push_front(0);
    CHECK(list1.size() == 4);
    CHECK(list2.size() == 3);
}

TEST_CASE("List: move construction") {
    List<int> list1{1, 2, 3};
    List<int> list2(std::move(list1));

    CHECK(list2.size() == 3);
    CHECK(list2.front() == 1);
    CHECK(list1.empty());
}

TEST_CASE("List: copy assignment") {
    List<int> list1{1, 2, 3};
    List<int> list2;

    list2 = list1;

    CHECK(list2.size() == 3);
    CHECK(list2.front() == 1);
}

TEST_CASE("List: move assignment") {
    List<int> list1{1, 2, 3};
    List<int> list2;

    list2 = std::move(list1);

    CHECK(list2.size() == 3);
    CHECK(list2.front() == 1);
    CHECK(list1.empty());
}

TEST_CASE("List: equality comparison") {
    List<int> list1{1, 2, 3};
    List<int> list2{1, 2, 3};
    List<int> list3{1, 2, 4};
    List<int> list4{1, 2};

    CHECK(list1 == list2);
    CHECK(list1 != list3);
    CHECK(list1 != list4);
}

TEST_CASE("List: reverse iteration") {
    List<int> list{1, 2, 3, 4, 5};

    Vector<int> reversed;
    for (auto it = list.rbegin(); it != list.rend(); ++it) {
        reversed.push_back(*it);
    }

    CHECK(reversed.size() == 5);
    CHECK(reversed[0] == 5);
    CHECK(reversed[1] == 4);
    CHECK(reversed[2] == 3);
    CHECK(reversed[3] == 2);
    CHECK(reversed[4] == 1);
}

TEST_CASE("List: bidirectional iteration") {
    List<int> list{1, 2, 3};

    auto it = list.begin();
    CHECK(*it == 1);
    ++it;
    CHECK(*it == 2);
    ++it;
    CHECK(*it == 3);
    --it;
    CHECK(*it == 2);
    --it;
    CHECK(*it == 1);
}

TEST_CASE("List: node reuse via free list") {
    List<int> list;

    // Add and remove nodes
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    list.pop_front();
    list.pop_front();

    // Now add more - should reuse freed nodes
    list.push_back(4);
    list.push_back(5);

    CHECK(list.size() == 3);

    auto it = list.begin();
    CHECK(*it++ == 3);
    CHECK(*it++ == 4);
    CHECK(*it++ == 5);
}

TEST_CASE("List: range-based for") {
    List<int> list{1, 2, 3, 4, 5};

    int sum = 0;
    for (int const &val : list) {
        sum += val;
    }

    CHECK(sum == 15);
}

TEST_CASE("List: members() for serialization") {
    List<int> list{1, 2, 3};

    // Check that members() returns a tuple (compile-time check)
    auto m = list.members();
    static_assert(std::tuple_size_v<decltype(m)> == 5, "members() should return 5 elements");

    // Const version
    List<int> const &clist = list;
    auto cm = clist.members();
    static_assert(std::tuple_size_v<decltype(cm)> == 5, "const members() should return 5 elements");
}

TEST_CASE("List: serialization round-trip") {
    List<int> original{10, 20, 30, 40, 50};

    // Serialize
    auto buf = serialize(original);

    // Deserialize
    auto restored = deserialize<Mode::NONE, List<int>>(buf);

    CHECK(restored.size() == original.size());
    CHECK(restored == original);
}

TEST_CASE("List: serialization with complex type") {
    struct Point {
        int x, y;
        auto members() noexcept { return std::tie(x, y); }
        auto members() const noexcept { return std::tie(x, y); }
        bool operator==(Point const &other) const { return x == other.x && y == other.y; }
    };

    List<Point> original;
    original.push_back({1, 2});
    original.push_back({2, 3});
    original.push_back({3, 4});

    // Serialize
    auto buf = serialize(original);

    // Deserialize
    auto restored = deserialize<Mode::NONE, List<Point>>(buf);

    CHECK(restored.size() == 3);
    CHECK(restored == original);
}

TEST_CASE("List: with strings") {
    List<std::string> list;
    list.push_back("hello");
    list.push_back("world");

    CHECK(list.size() == 2);
    CHECK(list.front() == "hello");
    CHECK(list.back() == "world");
}

TEST_CASE("List: LRU cache simulation") {
    // Simulate an LRU cache with capacity 3
    List<int> cache;
    size_t capacity = 3;

    auto access = [&](int value) {
        // Check if value exists
        for (auto it = cache.begin(); it != cache.end(); ++it) {
            if (*it == value) {
                cache.move_to_front(it);
                return;
            }
        }
        // Not found, add to front
        if (cache.size() >= capacity) {
            cache.pop_back(); // Remove least recently used
        }
        cache.push_front(value);
    };

    access(1); // [1]
    access(2); // [2, 1]
    access(3); // [3, 2, 1]
    access(2); // [2, 3, 1] - 2 moved to front
    access(4); // [4, 2, 3] - 1 evicted

    CHECK(cache.size() == 3);
    CHECK(cache.front() == 4);
    CHECK(cache.back() == 3);

    auto it = cache.begin();
    CHECK(*it++ == 4);
    CHECK(*it++ == 2);
    CHECK(*it++ == 3);
}
