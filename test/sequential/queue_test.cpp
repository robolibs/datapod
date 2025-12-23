#include <doctest/doctest.h>

#include "datapod/datapod.hpp"

using namespace datapod;

TEST_SUITE("Queue") {
    TEST_CASE("Construction - Default") {
        Queue<int> q;
        CHECK(q.empty());
        CHECK(q.size() == 0);
    }

    TEST_CASE("Push/Pop - FIFO order") {
        Queue<int> q;
        q.push(1);
        q.push(2);
        q.push(3);

        CHECK(q.front() == 1);
        CHECK(q.back() == 3);

        q.pop();
        CHECK(q.front() == 2);
        q.pop();
        CHECK(q.front() == 3);
        q.pop();
        CHECK(q.empty());
    }

    TEST_CASE("Interleaving - preserves order") {
        Queue<int> q;
        q.push(1);
        q.push(2);
        q.pop(); // consume 1
        q.push(3);
        q.push(4);

        CHECK(q.front() == 2);
        CHECK(q.back() == 4);
        q.pop();
        CHECK(q.front() == 3);
        q.pop();
        CHECK(q.front() == 4);
        q.pop();
        CHECK(q.empty());
    }

    TEST_CASE("Empty throws") {
        Queue<int> q;
        CHECK_THROWS_AS(q.front(), std::out_of_range);
        CHECK_THROWS_AS(q.back(), std::out_of_range);
        CHECK_THROWS_AS(q.pop(), std::out_of_range);
    }

    TEST_CASE("members() Serialization") {
        Queue<int> original;
        original.push(1);
        original.push(2);
        original.push(3);
        original.push(4);
        original.pop(); // leaves internal state in 'out_'
        original.push(5); // mixes 'in_' and 'out_'

        auto buf = serialize(original);
        auto loaded = deserialize<Mode::NONE, Queue<int>>(buf);

        CHECK(loaded.size() == 4);
        CHECK(loaded.front() == 2);
        CHECK(loaded.back() == 5);
        loaded.pop();
        CHECK(loaded.front() == 3);
        loaded.pop();
        CHECK(loaded.front() == 4);
        loaded.pop();
        CHECK(loaded.front() == 5);
        loaded.pop();
        CHECK(loaded.empty());
    }
}

TEST_SUITE("FixedQueue") {
    TEST_CASE("No overwrite - full handling") {
        FixedQueue<int, 3, false> q;
        CHECK(q.try_push(1));
        CHECK(q.try_push(2));
        CHECK(q.try_push(3));
        CHECK(q.full());
        CHECK(q.size() == 3);
        CHECK(q.front() == 1);
        CHECK(q.back() == 3);

        CHECK(q.try_push(4) == false);
        CHECK_THROWS_AS(q.push(4), std::out_of_range);

        q.pop();
        CHECK(q.front() == 2);
        q.push(5);
        CHECK(q.back() == 5);
    }

    TEST_CASE("Overwrite - replaces oldest") {
        FixedQueue<int, 3, true> q;
        q.push(1);
        q.push(2);
        q.push(3);
        CHECK(q.front() == 1);

        q.push(4); // overwrites 1
        CHECK(q.size() == 3);
        CHECK(q.front() == 2);
        CHECK(q.back() == 4);

        q.pop();
        CHECK(q.front() == 3);
    }

    TEST_CASE("Empty throws") {
        FixedQueue<int, 2, false> q;
        CHECK_THROWS_AS(q.front(), std::out_of_range);
        CHECK_THROWS_AS(q.back(), std::out_of_range);
        CHECK_THROWS_AS(q.pop(), std::out_of_range);
    }

    TEST_CASE("members() Serialization") {
        FixedQueue<int, 3, true> original;
        original.push(1);
        original.push(2);
        original.push(3);
        original.push(4); // overwrites 1

        auto buf = serialize(original);
        auto loaded = deserialize<Mode::NONE, FixedQueue<int, 3, true>>(buf);

        CHECK(loaded.size() == 3);
        CHECK(loaded.front() == 2);
        CHECK(loaded.back() == 4);
        loaded.pop();
        CHECK(loaded.front() == 3);
    }
}
