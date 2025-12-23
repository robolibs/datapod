#include <doctest/doctest.h>

#include "datapod/datapod.hpp"

using namespace datapod;

TEST_SUITE("Stack") {
    TEST_CASE("Construction - Default") {
        Stack<int> s;
        CHECK(s.empty());
        CHECK(s.size() == 0);
    }

    TEST_CASE("Push/Pop - LIFO order") {
        Stack<int> s;
        s.push(1);
        s.push(2);
        s.push(3);

        CHECK(s.top() == 3);
        s.pop();
        CHECK(s.top() == 2);
        s.pop();
        CHECK(s.top() == 1);
        s.pop();
        CHECK(s.empty());
    }

    TEST_CASE("Emplace") {
        struct Point {
            int x{};
            int y{};
            bool operator==(Point const &o) const { return x == o.x && y == o.y; }
        };

        Stack<Point> s;
        s.emplace(1, 2);
        CHECK(s.top() == Point{1, 2});
    }

    TEST_CASE("Empty throws") {
        Stack<int> s;
        CHECK_THROWS_AS(s.top(), std::out_of_range);
        CHECK_THROWS_AS(s.pop(), std::out_of_range);
    }

    TEST_CASE("members() Serialization") {
        Stack<int> original;
        original.push(10);
        original.push(20);
        original.push(30);

        auto buf = serialize(original);
        auto loaded = deserialize<Mode::NONE, Stack<int>>(buf);

        CHECK(loaded.size() == 3);
        CHECK(loaded.top() == 30);
        loaded.pop();
        CHECK(loaded.top() == 20);
        loaded.pop();
        CHECK(loaded.top() == 10);
        loaded.pop();
        CHECK(loaded.empty());
    }
}
