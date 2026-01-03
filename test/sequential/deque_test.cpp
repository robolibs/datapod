#include <doctest/doctest.h>

#include "datapod/datapod.hpp"

using namespace datapod;

TEST_CASE("Deque: default construction") {
    Deque<int> deque;
    CHECK(deque.empty());
    CHECK(deque.size() == 0);
}

TEST_CASE("Deque: count construction") {
    Deque<int> deque(5);
    CHECK(deque.size() == 5);
    for (size_t i = 0; i < 5; ++i) {
        CHECK(deque[i] == 0);
    }
}

TEST_CASE("Deque: count with value construction") {
    Deque<int> deque(5, 42);
    CHECK(deque.size() == 5);
    for (size_t i = 0; i < 5; ++i) {
        CHECK(deque[i] == 42);
    }
}

TEST_CASE("Deque: initializer list construction") {
    Deque<int> deque{1, 2, 3, 4, 5};
    CHECK(deque.size() == 5);
    CHECK(deque[0] == 1);
    CHECK(deque[1] == 2);
    CHECK(deque[2] == 3);
    CHECK(deque[3] == 4);
    CHECK(deque[4] == 5);
}

TEST_CASE("Deque: push_back") {
    Deque<int> deque;

    deque.push_back(1);
    CHECK(deque.size() == 1);
    CHECK(deque.front() == 1);
    CHECK(deque.back() == 1);

    deque.push_back(2);
    CHECK(deque.size() == 2);
    CHECK(deque.front() == 1);
    CHECK(deque.back() == 2);

    deque.push_back(3);
    CHECK(deque.size() == 3);
    CHECK(deque.front() == 1);
    CHECK(deque.back() == 3);
}

TEST_CASE("Deque: push_front") {
    Deque<int> deque;

    deque.push_front(3);
    CHECK(deque.size() == 1);
    CHECK(deque.front() == 3);
    CHECK(deque.back() == 3);

    deque.push_front(2);
    CHECK(deque.size() == 2);
    CHECK(deque.front() == 2);
    CHECK(deque.back() == 3);

    deque.push_front(1);
    CHECK(deque.size() == 3);
    CHECK(deque.front() == 1);
    CHECK(deque.back() == 3);
}

TEST_CASE("Deque: mixed push_front and push_back") {
    Deque<int> deque;

    deque.push_back(3);
    deque.push_front(2);
    deque.push_back(4);
    deque.push_front(1);
    deque.push_back(5);

    CHECK(deque.size() == 5);
    CHECK(deque[0] == 1);
    CHECK(deque[1] == 2);
    CHECK(deque[2] == 3);
    CHECK(deque[3] == 4);
    CHECK(deque[4] == 5);
}

TEST_CASE("Deque: pop_back") {
    Deque<int> deque{1, 2, 3};

    deque.pop_back();
    CHECK(deque.size() == 2);
    CHECK(deque.back() == 2);

    deque.pop_back();
    CHECK(deque.size() == 1);
    CHECK(deque.back() == 1);

    deque.pop_back();
    CHECK(deque.empty());
}

TEST_CASE("Deque: pop_front") {
    Deque<int> deque{1, 2, 3};

    deque.pop_front();
    CHECK(deque.size() == 2);
    CHECK(deque.front() == 2);

    deque.pop_front();
    CHECK(deque.size() == 1);
    CHECK(deque.front() == 3);

    deque.pop_front();
    CHECK(deque.empty());
}

TEST_CASE("Deque: pop_front with rebalancing") {
    Deque<int> deque;
    // Only push to back, then pop from front
    for (int i = 1; i <= 10; ++i) {
        deque.push_back(i);
    }

    for (int i = 1; i <= 10; ++i) {
        CHECK(deque.front() == i);
        deque.pop_front();
    }
    CHECK(deque.empty());
}

TEST_CASE("Deque: pop_back with rebalancing") {
    Deque<int> deque;
    // Only push to front, then pop from back
    for (int i = 10; i >= 1; --i) {
        deque.push_front(i);
    }

    for (int i = 10; i >= 1; --i) {
        CHECK(deque.back() == i);
        deque.pop_back();
    }
    CHECK(deque.empty());
}

TEST_CASE("Deque: pop_front throws on empty") {
    Deque<int> deque;
    CHECK_THROWS_AS(deque.pop_front(), std::out_of_range);
}

TEST_CASE("Deque: pop_back throws on empty") {
    Deque<int> deque;
    CHECK_THROWS_AS(deque.pop_back(), std::out_of_range);
}

TEST_CASE("Deque: front/back throw on empty") {
    Deque<int> deque;
    CHECK_THROWS_AS(deque.front(), std::out_of_range);
    CHECK_THROWS_AS(deque.back(), std::out_of_range);

    Deque<int> const &cdeque = deque;
    CHECK_THROWS_AS(cdeque.front(), std::out_of_range);
    CHECK_THROWS_AS(cdeque.back(), std::out_of_range);
}

TEST_CASE("Deque: at with bounds checking") {
    Deque<int> deque{1, 2, 3};

    CHECK(deque.at(0) == 1);
    CHECK(deque.at(1) == 2);
    CHECK(deque.at(2) == 3);
    CHECK_THROWS_AS(deque.at(3), std::out_of_range);
}

TEST_CASE("Deque: random access operator[]") {
    Deque<int> deque;
    deque.push_front(2);
    deque.push_front(1);
    deque.push_back(3);
    deque.push_back(4);

    CHECK(deque[0] == 1);
    CHECK(deque[1] == 2);
    CHECK(deque[2] == 3);
    CHECK(deque[3] == 4);

    // Modify via operator[]
    deque[1] = 20;
    CHECK(deque[1] == 20);
}

TEST_CASE("Deque: emplace_front and emplace_back") {
    Deque<std::pair<int, int>> deque;

    deque.emplace_front(1, 2);
    CHECK(deque.front().first == 1);
    CHECK(deque.front().second == 2);

    deque.emplace_back(3, 4);
    CHECK(deque.back().first == 3);
    CHECK(deque.back().second == 4);
}

TEST_CASE("Deque: clear") {
    Deque<int> deque{1, 2, 3, 4, 5};
    CHECK(deque.size() == 5);

    deque.clear();
    CHECK(deque.empty());
}

TEST_CASE("Deque: resize") {
    Deque<int> deque{1, 2, 3};

    deque.resize(5);
    CHECK(deque.size() == 5);
    CHECK(deque[0] == 1);
    CHECK(deque[1] == 2);
    CHECK(deque[2] == 3);
    CHECK(deque[3] == 0);
    CHECK(deque[4] == 0);

    deque.resize(2);
    CHECK(deque.size() == 2);
    CHECK(deque[0] == 1);
    CHECK(deque[1] == 2);
}

TEST_CASE("Deque: resize with value") {
    Deque<int> deque{1, 2};

    deque.resize(5, 42);
    CHECK(deque.size() == 5);
    CHECK(deque[2] == 42);
    CHECK(deque[3] == 42);
    CHECK(deque[4] == 42);
}

TEST_CASE("Deque: copy construction") {
    Deque<int> deque1{1, 2, 3};
    Deque<int> deque2(deque1);

    CHECK(deque2.size() == 3);
    CHECK(deque2[0] == 1);
    CHECK(deque2[1] == 2);
    CHECK(deque2[2] == 3);

    // Modify original
    deque1.push_back(4);
    CHECK(deque1.size() == 4);
    CHECK(deque2.size() == 3);
}

TEST_CASE("Deque: move construction") {
    Deque<int> deque1{1, 2, 3};
    Deque<int> deque2(std::move(deque1));

    CHECK(deque2.size() == 3);
    CHECK(deque2[0] == 1);
    CHECK(deque1.empty());
}

TEST_CASE("Deque: copy assignment") {
    Deque<int> deque1{1, 2, 3};
    Deque<int> deque2;

    deque2 = deque1;

    CHECK(deque2.size() == 3);
    CHECK(deque2[0] == 1);
}

TEST_CASE("Deque: move assignment") {
    Deque<int> deque1{1, 2, 3};
    Deque<int> deque2;

    deque2 = std::move(deque1);

    CHECK(deque2.size() == 3);
    CHECK(deque2[0] == 1);
    CHECK(deque1.empty());
}

TEST_CASE("Deque: equality comparison") {
    Deque<int> deque1{1, 2, 3};
    Deque<int> deque2{1, 2, 3};
    Deque<int> deque3{1, 2, 4};
    Deque<int> deque4{1, 2};

    CHECK(deque1 == deque2);
    CHECK(deque1 != deque3);
    CHECK(deque1 != deque4);
}

TEST_CASE("Deque: less-than comparison") {
    Deque<int> deque1{1, 2, 3};
    Deque<int> deque2{1, 2, 4};
    Deque<int> deque3{1, 2};

    CHECK(deque1 < deque2);
    CHECK(deque3 < deque1);
    CHECK(!(deque2 < deque1));
}

TEST_CASE("Deque: forward iteration") {
    Deque<int> deque{1, 2, 3, 4, 5};

    int sum = 0;
    for (auto it = deque.begin(); it != deque.end(); ++it) {
        sum += *it;
    }
    CHECK(sum == 15);
}

TEST_CASE("Deque: range-based for") {
    Deque<int> deque{1, 2, 3, 4, 5};

    int sum = 0;
    for (int val : deque) {
        sum += val;
    }
    CHECK(sum == 15);
}

TEST_CASE("Deque: reverse iteration") {
    Deque<int> deque{1, 2, 3, 4, 5};

    Vector<int> reversed;
    for (auto it = deque.rbegin(); it != deque.rend(); ++it) {
        reversed.push_back(*it);
    }

    CHECK(reversed.size() == 5);
    CHECK(reversed[0] == 5);
    CHECK(reversed[1] == 4);
    CHECK(reversed[2] == 3);
    CHECK(reversed[3] == 2);
    CHECK(reversed[4] == 1);
}

TEST_CASE("Deque: iterator arithmetic") {
    Deque<int> deque{1, 2, 3, 4, 5};

    auto it = deque.begin();
    CHECK(*it == 1);
    CHECK(*(it + 2) == 3);
    CHECK(it[3] == 4);

    it += 4;
    CHECK(*it == 5);

    it -= 2;
    CHECK(*it == 3);

    auto it2 = deque.end();
    CHECK(it2 - deque.begin() == 5);
}

TEST_CASE("Deque: members() for serialization") {
    Deque<int> deque{1, 2, 3};

    auto m = deque.members();
    static_assert(std::tuple_size_v<decltype(m)> == 2, "members() should return 2 elements");

    Deque<int> const &cdeque = deque;
    auto cm = cdeque.members();
    static_assert(std::tuple_size_v<decltype(cm)> == 2, "const members() should return 2 elements");
}

TEST_CASE("Deque: serialization round-trip") {
    Deque<int> original;
    original.push_front(2);
    original.push_front(1);
    original.push_back(3);
    original.push_back(4);
    original.push_back(5);

    auto buf = serialize(original);
    auto restored = deserialize<Mode::NONE, Deque<int>>(buf);

    CHECK(restored.size() == original.size());
    CHECK(restored == original);
}

TEST_CASE("Deque: serialization with complex type") {
    struct Point {
        int x, y;
        auto members() noexcept { return std::tie(x, y); }
        auto members() const noexcept { return std::tie(x, y); }
        bool operator==(Point const &other) const { return x == other.x && y == other.y; }
    };

    Deque<Point> original;
    original.push_back({1, 2});
    original.push_back({3, 4});
    original.push_front({0, 1});

    auto buf = serialize(original);
    auto restored = deserialize<Mode::NONE, Deque<Point>>(buf);

    CHECK(restored.size() == 3);
    CHECK(restored == original);
}

TEST_CASE("Deque: with strings") {
    Deque<std::string> deque;
    deque.push_back("world");
    deque.push_front("hello");
    deque.push_back("!");

    CHECK(deque.size() == 3);
    CHECK(deque[0] == "hello");
    CHECK(deque[1] == "world");
    CHECK(deque[2] == "!");
}

TEST_CASE("Deque: BFS simulation") {
    // Simple BFS using deque
    Deque<int> queue;
    Vector<int> visited;

    queue.push_back(1);
    while (!queue.empty()) {
        int node = queue.front();
        queue.pop_front();
        visited.push_back(node);

        // Simulate adding neighbors
        if (node < 4) {
            queue.push_back(node * 2);
            queue.push_back(node * 2 + 1);
        }
    }

    // BFS order: 1, 2, 3, 4, 5, 6, 7
    CHECK(visited.size() == 7);
    CHECK(visited[0] == 1);
    CHECK(visited[1] == 2);
    CHECK(visited[2] == 3);
}

TEST_CASE("Deque: sliding window") {
    Vector<int> data{1, 3, -1, -3, 5, 3, 6, 7};
    size_t k = 3;

    // Find max in each sliding window of size k
    Deque<size_t> window; // Store indices
    Vector<int> maxes;

    for (size_t i = 0; i < data.size(); ++i) {
        // Remove elements outside window
        while (!window.empty() && window.front() + k <= i) {
            window.pop_front();
        }

        // Remove smaller elements (they can't be max)
        while (!window.empty() && data[window.back()] < data[i]) {
            window.pop_back();
        }

        window.push_back(i);

        if (i >= k - 1) {
            maxes.push_back(data[window.front()]);
        }
    }

    // Expected maxes: [3, 3, 5, 5, 6, 7]
    CHECK(maxes.size() == 6);
    CHECK(maxes[0] == 3);
    CHECK(maxes[1] == 3);
    CHECK(maxes[2] == 5);
    CHECK(maxes[3] == 5);
    CHECK(maxes[4] == 6);
    CHECK(maxes[5] == 7);
}

TEST_CASE("Deque: stress test") {
    Deque<int> deque;

    // Alternate push_front and push_back
    for (int i = 0; i < 1000; ++i) {
        if (i % 2 == 0) {
            deque.push_back(i);
        } else {
            deque.push_front(i);
        }
    }

    CHECK(deque.size() == 1000);

    // Pop from both ends
    for (int i = 0; i < 500; ++i) {
        deque.pop_front();
        deque.pop_back();
    }

    CHECK(deque.empty());
}
