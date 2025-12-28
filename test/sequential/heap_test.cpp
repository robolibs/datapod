#include <doctest/doctest.h>

#include "datapod/datapod.hpp"

using namespace datapod;

TEST_CASE("Heap: default construction") {
    Heap<int> heap;
    CHECK(heap.empty());
    CHECK(heap.size() == 0);
}

TEST_CASE("Heap: initializer list construction (max-heap)") {
    Heap<int> heap{3, 1, 4, 1, 5, 9, 2, 6};
    CHECK(heap.size() == 8);
    CHECK(heap.top() == 9); // Max element
}

TEST_CASE("Heap: push and top (max-heap)") {
    Heap<int> heap;

    heap.push(5);
    CHECK(heap.top() == 5);

    heap.push(3);
    CHECK(heap.top() == 5);

    heap.push(7);
    CHECK(heap.top() == 7);

    heap.push(1);
    CHECK(heap.top() == 7);

    heap.push(9);
    CHECK(heap.top() == 9);
}

TEST_CASE("Heap: pop (max-heap)") {
    Heap<int> heap{3, 1, 4, 1, 5, 9, 2, 6};

    CHECK(heap.pop_top() == 9);
    CHECK(heap.pop_top() == 6);
    CHECK(heap.pop_top() == 5);
    CHECK(heap.pop_top() == 4);
    CHECK(heap.pop_top() == 3);
    CHECK(heap.pop_top() == 2);
    CHECK(heap.pop_top() == 1);
    CHECK(heap.pop_top() == 1);
    CHECK(heap.empty());
}

TEST_CASE("Heap: min-heap") {
    MinHeap<int> heap{3, 1, 4, 1, 5, 9, 2, 6};

    CHECK(heap.top() == 1); // Min element

    CHECK(heap.pop_top() == 1);
    CHECK(heap.pop_top() == 1);
    CHECK(heap.pop_top() == 2);
    CHECK(heap.pop_top() == 3);
    CHECK(heap.pop_top() == 4);
    CHECK(heap.pop_top() == 5);
    CHECK(heap.pop_top() == 6);
    CHECK(heap.pop_top() == 9);
    CHECK(heap.empty());
}

TEST_CASE("Heap: top throws on empty") {
    Heap<int> heap;
    CHECK_THROWS_AS(heap.top(), std::out_of_range);
}

TEST_CASE("Heap: pop throws on empty") {
    Heap<int> heap;
    CHECK_THROWS_AS(heap.pop(), std::out_of_range);
}

TEST_CASE("Heap: pop_top throws on empty") {
    Heap<int> heap;
    CHECK_THROWS_AS(heap.pop_top(), std::out_of_range);
}

TEST_CASE("Heap: emplace") {
    Heap<std::pair<int, int>> heap;

    heap.emplace(1, 2);
    CHECK(heap.size() == 1);
    CHECK(heap.top().first == 1);

    heap.emplace(3, 4);
    CHECK(heap.top().first == 3);

    heap.emplace(2, 5);
    CHECK(heap.top().first == 3);
}

TEST_CASE("Heap: clear") {
    Heap<int> heap{1, 2, 3, 4, 5};
    CHECK(heap.size() == 5);

    heap.clear();
    CHECK(heap.empty());
}

TEST_CASE("Heap: reserve") {
    Heap<int> heap;
    heap.reserve(100);
    CHECK(heap.empty());

    for (int i = 0; i < 50; ++i) {
        heap.push(i);
    }
    CHECK(heap.size() == 50);
}

TEST_CASE("Heap: copy construction") {
    Heap<int> heap1{3, 1, 4, 1, 5};
    Heap<int> heap2(heap1);

    CHECK(heap2.size() == 5);
    CHECK(heap2.top() == 5);

    // Modify original
    heap1.pop();
    CHECK(heap1.top() == 4);
    CHECK(heap2.top() == 5); // Copy unchanged
}

TEST_CASE("Heap: move construction") {
    Heap<int> heap1{3, 1, 4, 1, 5};
    Heap<int> heap2(std::move(heap1));

    CHECK(heap2.size() == 5);
    CHECK(heap2.top() == 5);
    CHECK(heap1.empty());
}

TEST_CASE("Heap: copy assignment") {
    Heap<int> heap1{3, 1, 4, 1, 5};
    Heap<int> heap2;

    heap2 = heap1;

    CHECK(heap2.size() == 5);
    CHECK(heap2.top() == 5);
}

TEST_CASE("Heap: move assignment") {
    Heap<int> heap1{3, 1, 4, 1, 5};
    Heap<int> heap2;

    heap2 = std::move(heap1);

    CHECK(heap2.size() == 5);
    CHECK(heap2.top() == 5);
    CHECK(heap1.empty());
}

TEST_CASE("Heap: from_unsorted") {
    Vector<int> data{3, 1, 4, 1, 5, 9, 2, 6};
    auto heap = Heap<int>::from_unsorted(std::move(data));

    CHECK(heap.size() == 8);
    CHECK(heap.top() == 9);
}

TEST_CASE("Heap: heapify after modification") {
    Heap<int> heap{5, 3, 7, 1, 9};

    // Simulate deserialization that might break heap property
    // (In practice, members() preserves heap order, but heapify is available)
    heap.heapify();

    CHECK(heap.top() == 9);
}

TEST_CASE("Heap: range constructor") {
    Vector<int> data{3, 1, 4, 1, 5, 9};
    Heap<int> heap(data.begin(), data.end());

    CHECK(heap.size() == 6);
    CHECK(heap.top() == 9);
}

TEST_CASE("Heap: custom comparator") {
    // Max-heap by absolute value
    auto abs_less = [](int a, int b) { return std::abs(a) < std::abs(b); };
    Heap<int, decltype(abs_less)> heap(abs_less);

    heap.push(3);
    heap.push(-5);
    heap.push(2);
    heap.push(-1);

    CHECK(heap.top() == -5); // Largest absolute value
    heap.pop();
    CHECK(heap.top() == 3);
}

TEST_CASE("Heap: members() for serialization") {
    Heap<int> heap{1, 2, 3};

    auto m = heap.members();
    static_assert(std::tuple_size_v<decltype(m)> == 1, "members() should return 1 element");

    Heap<int> const &cheap = heap;
    auto cm = cheap.members();
    static_assert(std::tuple_size_v<decltype(cm)> == 1, "const members() should return 1 element");
}

TEST_CASE("Heap: serialization round-trip") {
    Heap<int> original{10, 20, 30, 40, 50};

    auto buf = serialize(original);
    auto restored = deserialize<Mode::NONE, Heap<int>>(buf);

    CHECK(restored.size() == original.size());

    // Pop all elements and compare
    while (!original.empty()) {
        CHECK(original.top() == restored.top());
        original.pop();
        restored.pop();
    }
}

TEST_CASE("Heap: serialization with min-heap") {
    MinHeap<int> original{10, 20, 30, 40, 50};

    auto buf = serialize(original);
    auto restored = deserialize<Mode::NONE, MinHeap<int>>(buf);

    CHECK(restored.size() == original.size());
    CHECK(restored.top() == 10); // Min element
}

TEST_CASE("Heap: equality comparison") {
    Heap<int> heap1{3, 1, 4, 1, 5};
    Heap<int> heap2{3, 1, 4, 1, 5};
    Heap<int> heap3{3, 1, 4, 1, 6};

    CHECK(heap1 == heap2);
    CHECK(heap1 != heap3);
}

TEST_CASE("Heap: heap sort") {
    Vector<int> data{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    Heap<int> heap(data.begin(), data.end());

    Vector<int> sorted;
    while (!heap.empty()) {
        sorted.push_back(heap.pop_top());
    }

    // Should be sorted in descending order (max-heap)
    CHECK(sorted.size() == 11);
    for (size_t i = 1; i < sorted.size(); ++i) {
        CHECK(sorted[i - 1] >= sorted[i]);
    }
}

TEST_CASE("Heap: min-heap sort") {
    Vector<int> data{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    MinHeap<int> heap(data.begin(), data.end());

    Vector<int> sorted;
    while (!heap.empty()) {
        sorted.push_back(heap.pop_top());
    }

    // Should be sorted in ascending order (min-heap)
    CHECK(sorted.size() == 11);
    for (size_t i = 1; i < sorted.size(); ++i) {
        CHECK(sorted[i - 1] <= sorted[i]);
    }
}

TEST_CASE("Heap: with strings") {
    Heap<std::string> heap;
    heap.push("banana");
    heap.push("apple");
    heap.push("cherry");

    CHECK(heap.top() == "cherry"); // Lexicographically largest
    heap.pop();
    CHECK(heap.top() == "banana");
    heap.pop();
    CHECK(heap.top() == "apple");
}

TEST_CASE("Heap: PriorityQueue alias") {
    PriorityQueue<int> pq;
    pq.push(3);
    pq.push(1);
    pq.push(4);

    CHECK(pq.top() == 4);
}

TEST_CASE("Heap: single element") {
    Heap<int> heap;
    heap.push(42);

    CHECK(heap.size() == 1);
    CHECK(heap.top() == 42);

    heap.pop();
    CHECK(heap.empty());
}

TEST_CASE("Heap: duplicate elements") {
    Heap<int> heap{5, 5, 5, 5, 5};

    CHECK(heap.size() == 5);
    for (int i = 0; i < 5; ++i) {
        CHECK(heap.top() == 5);
        heap.pop();
    }
    CHECK(heap.empty());
}

TEST_CASE("Heap: stress test") {
    Heap<int> heap;

    // Push 1000 elements
    for (int i = 0; i < 1000; ++i) {
        heap.push(i);
    }

    CHECK(heap.size() == 1000);
    CHECK(heap.top() == 999);

    // Pop all and verify order
    int prev = heap.pop_top();
    while (!heap.empty()) {
        int curr = heap.pop_top();
        CHECK(prev >= curr);
        prev = curr;
    }
}
