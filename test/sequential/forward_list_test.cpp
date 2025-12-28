#include <doctest/doctest.h>

#include "datapod/datapod.hpp"

using namespace datapod;

TEST_CASE("ForwardList: default construction") {
    ForwardList<int> list;
    CHECK(list.empty());
    CHECK(list.size() == 0);
}

TEST_CASE("ForwardList: initializer list construction") {
    ForwardList<int> list{1, 2, 3, 4, 5};
    CHECK(list.size() == 5);
    CHECK(list.front() == 1);

    // Check order
    auto it = list.begin();
    CHECK(*it++ == 1);
    CHECK(*it++ == 2);
    CHECK(*it++ == 3);
    CHECK(*it++ == 4);
    CHECK(*it++ == 5);
    CHECK(it == list.end());
}

TEST_CASE("ForwardList: push_front") {
    ForwardList<int> list;

    list.push_front(3);
    CHECK(list.size() == 1);
    CHECK(list.front() == 3);

    list.push_front(2);
    CHECK(list.size() == 2);
    CHECK(list.front() == 2);

    list.push_front(1);
    CHECK(list.size() == 3);
    CHECK(list.front() == 1);

    // Check order: 1, 2, 3
    auto it = list.begin();
    CHECK(*it++ == 1);
    CHECK(*it++ == 2);
    CHECK(*it++ == 3);
}

TEST_CASE("ForwardList: pop_front") {
    ForwardList<int> list{1, 2, 3};

    list.pop_front();
    CHECK(list.size() == 2);
    CHECK(list.front() == 2);

    list.pop_front();
    CHECK(list.size() == 1);
    CHECK(list.front() == 3);

    list.pop_front();
    CHECK(list.empty());
}

TEST_CASE("ForwardList: pop_front throws on empty") {
    ForwardList<int> list;
    CHECK_THROWS_AS(list.pop_front(), std::out_of_range);
}

TEST_CASE("ForwardList: front throws on empty") {
    ForwardList<int> list;
    CHECK_THROWS_AS(list.front(), std::out_of_range);

    ForwardList<int> const &clist = list;
    CHECK_THROWS_AS(clist.front(), std::out_of_range);
}

TEST_CASE("ForwardList: emplace_front") {
    ForwardList<std::pair<int, int>> list;

    list.emplace_front(1, 2);
    CHECK(list.size() == 1);
    CHECK(list.front().first == 1);
    CHECK(list.front().second == 2);

    list.emplace_front(3, 4);
    CHECK(list.size() == 2);
    CHECK(list.front().first == 3);
    CHECK(list.front().second == 4);
}

TEST_CASE("ForwardList: insert_after") {
    ForwardList<int> list{1, 3};

    auto it = list.begin();
    list.insert_after(it, 2);

    CHECK(list.size() == 3);

    // Check order: 1, 2, 3
    it = list.begin();
    CHECK(*it++ == 1);
    CHECK(*it++ == 2);
    CHECK(*it++ == 3);
}

TEST_CASE("ForwardList: erase_after") {
    ForwardList<int> list{1, 2, 3};

    auto it = list.begin();
    list.erase_after(it);

    CHECK(list.size() == 2);

    // Check order: 1, 3
    it = list.begin();
    CHECK(*it++ == 1);
    CHECK(*it++ == 3);
}

TEST_CASE("ForwardList: clear") {
    ForwardList<int> list{1, 2, 3, 4, 5};
    CHECK(list.size() == 5);

    list.clear();
    CHECK(list.empty());
    CHECK(list.size() == 0);
}

TEST_CASE("ForwardList: reverse") {
    ForwardList<int> list{1, 2, 3, 4, 5};

    list.reverse();

    auto it = list.begin();
    CHECK(*it++ == 5);
    CHECK(*it++ == 4);
    CHECK(*it++ == 3);
    CHECK(*it++ == 2);
    CHECK(*it++ == 1);
}

TEST_CASE("ForwardList: copy construction") {
    ForwardList<int> list1{1, 2, 3};
    ForwardList<int> list2(list1);

    CHECK(list2.size() == 3);
    CHECK(list2.front() == 1);

    // Modify original, copy should be unchanged
    list1.push_front(0);
    CHECK(list1.size() == 4);
    CHECK(list2.size() == 3);
}

TEST_CASE("ForwardList: move construction") {
    ForwardList<int> list1{1, 2, 3};
    ForwardList<int> list2(std::move(list1));

    CHECK(list2.size() == 3);
    CHECK(list2.front() == 1);
    CHECK(list1.empty());
}

TEST_CASE("ForwardList: copy assignment") {
    ForwardList<int> list1{1, 2, 3};
    ForwardList<int> list2;

    list2 = list1;

    CHECK(list2.size() == 3);
    CHECK(list2.front() == 1);
}

TEST_CASE("ForwardList: move assignment") {
    ForwardList<int> list1{1, 2, 3};
    ForwardList<int> list2;

    list2 = std::move(list1);

    CHECK(list2.size() == 3);
    CHECK(list2.front() == 1);
    CHECK(list1.empty());
}

TEST_CASE("ForwardList: equality comparison") {
    ForwardList<int> list1{1, 2, 3};
    ForwardList<int> list2{1, 2, 3};
    ForwardList<int> list3{1, 2, 4};
    ForwardList<int> list4{1, 2};

    CHECK(list1 == list2);
    CHECK(list1 != list3);
    CHECK(list1 != list4);
}

TEST_CASE("ForwardList: node reuse via free list") {
    ForwardList<int> list;

    // Add and remove nodes
    list.push_front(1);
    list.push_front(2);
    list.push_front(3);
    list.pop_front();
    list.pop_front();

    // Now add more - should reuse freed nodes
    list.push_front(4);
    list.push_front(5);

    CHECK(list.size() == 3);

    auto it = list.begin();
    CHECK(*it++ == 5);
    CHECK(*it++ == 4);
    CHECK(*it++ == 1);
}

TEST_CASE("ForwardList: iteration with range-based for") {
    ForwardList<int> list{1, 2, 3, 4, 5};

    int sum = 0;
    for (int const &val : list) {
        sum += val;
    }

    CHECK(sum == 15);
}

TEST_CASE("ForwardList: const iteration") {
    ForwardList<int> const list{1, 2, 3};

    int sum = 0;
    for (auto it = list.cbegin(); it != list.cend(); ++it) {
        sum += *it;
    }

    CHECK(sum == 6);
}

TEST_CASE("ForwardList: members() for serialization") {
    ForwardList<int> list{1, 2, 3};

    // Check that members() returns a tuple (compile-time check)
    auto m = list.members();
    static_assert(std::tuple_size_v<decltype(m)> == 4, "members() should return 4 elements");

    // Const version
    ForwardList<int> const &clist = list;
    auto cm = clist.members();
    static_assert(std::tuple_size_v<decltype(cm)> == 4, "const members() should return 4 elements");
}

TEST_CASE("ForwardList: serialization round-trip") {
    ForwardList<int> original{10, 20, 30, 40, 50};

    // Serialize
    auto buf = serialize(original);

    // Deserialize
    auto restored = deserialize<Mode::NONE, ForwardList<int>>(buf);

    CHECK(restored.size() == original.size());
    CHECK(restored == original);
}

TEST_CASE("ForwardList: serialization with complex type") {
    struct Point {
        int x, y;
        auto members() noexcept { return std::tie(x, y); }
        auto members() const noexcept { return std::tie(x, y); }
        bool operator==(Point const &other) const { return x == other.x && y == other.y; }
    };

    ForwardList<Point> original;
    original.push_front({3, 4});
    original.push_front({2, 3});
    original.push_front({1, 2});

    // Serialize
    auto buf = serialize(original);

    // Deserialize
    auto restored = deserialize<Mode::NONE, ForwardList<Point>>(buf);

    CHECK(restored.size() == 3);
    CHECK(restored == original);
}

TEST_CASE("ForwardList: with strings") {
    ForwardList<std::string> list;
    list.push_front("world");
    list.push_front("hello");

    CHECK(list.size() == 2);
    CHECK(list.front() == "hello");

    auto it = list.begin();
    CHECK(*it++ == "hello");
    CHECK(*it++ == "world");
}
