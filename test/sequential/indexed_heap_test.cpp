#include "datapod/datapod.hpp"
#include <doctest/doctest.h>
#include <string>
#include <vector>

using namespace datapod;

TEST_SUITE("IndexedHeap") {

    TEST_CASE("Default construction") {
        IndexedHeap<int, int> heap;
        CHECK(heap.empty());
        CHECK(heap.size() == 0);
    }

    TEST_CASE("Push single element") {
        IndexedHeap<int, int> heap;
        heap.push(1, 10);
        CHECK(heap.size() == 1);
        CHECK_FALSE(heap.empty());
        CHECK(heap.top().key == 1);
        CHECK(heap.top().priority == 10);
    }

    TEST_CASE("Push multiple elements - min heap") {
        IndexedHeap<int, int> heap;
        heap.push(1, 30);
        heap.push(2, 10);
        heap.push(3, 20);

        CHECK(heap.size() == 3);
        CHECK(heap.top().key == 2); // Smallest priority
        CHECK(heap.top().priority == 10);
    }

    TEST_CASE("Pop elements in order") {
        IndexedHeap<int, int> heap;
        heap.push(1, 30);
        heap.push(2, 10);
        heap.push(3, 20);

        auto e1 = heap.pop();
        CHECK(e1.key == 2);
        CHECK(e1.priority == 10);

        auto e2 = heap.pop();
        CHECK(e2.key == 3);
        CHECK(e2.priority == 20);

        auto e3 = heap.pop();
        CHECK(e3.key == 1);
        CHECK(e3.priority == 30);

        CHECK(heap.empty());
    }

    TEST_CASE("Contains") {
        IndexedHeap<int, int> heap;
        heap.push(1, 10);
        heap.push(2, 20);

        CHECK(heap.contains(1));
        CHECK(heap.contains(2));
        CHECK_FALSE(heap.contains(3));
    }

    TEST_CASE("Priority lookup") {
        IndexedHeap<int, int> heap;
        heap.push(1, 10);
        heap.push(2, 20);

        CHECK(heap.priority(1) == 10);
        CHECK(heap.priority(2) == 20);
    }

    TEST_CASE("Decrease key") {
        IndexedHeap<int, int> heap;
        heap.push(1, 30);
        heap.push(2, 20);
        heap.push(3, 10);

        // Key 1 has priority 30, decrease to 5
        heap.decrease_key(1, 5);

        CHECK(heap.top().key == 1);
        CHECK(heap.top().priority == 5);
    }

    TEST_CASE("Decrease key throws on invalid decrease") {
        IndexedHeap<int, int> heap;
        heap.push(1, 10);

        CHECK_THROWS_AS(heap.decrease_key(1, 20), std::invalid_argument);
    }

    TEST_CASE("Update priority - decrease") {
        IndexedHeap<int, int> heap;
        heap.push(1, 30);
        heap.push(2, 20);

        heap.update_priority(1, 5);
        CHECK(heap.top().key == 1);
        CHECK(heap.top().priority == 5);
    }

    TEST_CASE("Update priority - increase") {
        IndexedHeap<int, int> heap;
        heap.push(1, 10);
        heap.push(2, 20);

        heap.update_priority(1, 50);
        CHECK(heap.top().key == 2);
        CHECK(heap.top().priority == 20);
    }

    TEST_CASE("Push updates existing key") {
        IndexedHeap<int, int> heap;
        heap.push(1, 30);
        heap.push(2, 20);

        // Push same key with different priority
        heap.push(1, 5);

        CHECK(heap.size() == 2); // Still 2 elements
        CHECK(heap.top().key == 1);
        CHECK(heap.top().priority == 5);
    }

    TEST_CASE("Erase element") {
        IndexedHeap<int, int> heap;
        heap.push(1, 30);
        heap.push(2, 10);
        heap.push(3, 20);

        bool erased = heap.erase(2);
        CHECK(erased);
        CHECK(heap.size() == 2);
        CHECK_FALSE(heap.contains(2));

        // Heap property maintained
        CHECK(heap.top().key == 3);
        CHECK(heap.top().priority == 20);
    }

    TEST_CASE("Erase non-existent element") {
        IndexedHeap<int, int> heap;
        heap.push(1, 10);

        bool erased = heap.erase(99);
        CHECK_FALSE(erased);
        CHECK(heap.size() == 1);
    }

    TEST_CASE("Clear") {
        IndexedHeap<int, int> heap;
        heap.push(1, 10);
        heap.push(2, 20);

        heap.clear();
        CHECK(heap.empty());
        CHECK(heap.size() == 0);
    }

    TEST_CASE("Top on empty heap throws") {
        IndexedHeap<int, int> heap;
        CHECK_THROWS_AS(heap.top(), std::out_of_range);
    }

    TEST_CASE("Pop on empty heap throws") {
        IndexedHeap<int, int> heap;
        CHECK_THROWS_AS(heap.pop(), std::out_of_range);
    }

    TEST_CASE("Max heap with std::greater") {
        MaxIndexedHeap<int, int> heap;
        heap.push(1, 10);
        heap.push(2, 30);
        heap.push(3, 20);

        CHECK(heap.top().key == 2); // Largest priority
        CHECK(heap.top().priority == 30);
    }

    TEST_CASE("String keys") {
        IndexedHeap<String, int> heap;
        heap.push(String("alice"), 30);
        heap.push(String("bob"), 10);
        heap.push(String("charlie"), 20);

        CHECK(heap.top().key.view() == "bob");
        CHECK(heap.top().priority == 10);
    }

    TEST_CASE("Dijkstra simulation") {
        // Simulate Dijkstra's algorithm
        IndexedHeap<int, int> pq; // node -> distance

        // Initial distances
        pq.push(0, 0);   // Source
        pq.push(1, 100); // INF
        pq.push(2, 100);
        pq.push(3, 100);

        // Process source (node 0)
        auto [node, dist] = pq.pop();
        CHECK(node == 0);
        CHECK(dist == 0);

        // Relax edges from node 0
        // Edge 0->1 with weight 5
        if (dist + 5 < pq.priority(1)) {
            pq.decrease_key(1, dist + 5);
        }
        // Edge 0->2 with weight 10
        if (dist + 10 < pq.priority(2)) {
            pq.decrease_key(2, dist + 10);
        }

        // Next node should be 1 (distance 5)
        auto [node2, dist2] = pq.pop();
        CHECK(node2 == 1);
        CHECK(dist2 == 5);

        // Relax edge 1->3 with weight 3
        if (dist2 + 3 < pq.priority(3)) {
            pq.decrease_key(3, dist2 + 3);
        }

        // Next should be 3 (distance 8)
        auto [node3, dist3] = pq.pop();
        CHECK(node3 == 3);
        CHECK(dist3 == 8);

        // Finally node 2 (distance 10)
        auto [node4, dist4] = pq.pop();
        CHECK(node4 == 2);
        CHECK(dist4 == 10);
    }

    TEST_CASE("Large heap stress test") {
        IndexedHeap<int, int> heap;
        constexpr int N = 1000;

        // Insert in reverse order
        for (int i = N - 1; i >= 0; --i) {
            heap.push(i, i);
        }

        CHECK(heap.size() == N);
        CHECK(heap.top().priority == 0);

        // Pop all and verify order
        for (int i = 0; i < N; ++i) {
            auto entry = heap.pop();
            CHECK(entry.key == i);
            CHECK(entry.priority == i);
        }

        CHECK(heap.empty());
    }

    TEST_CASE("Decrease key stress test") {
        IndexedHeap<int, int> heap;
        constexpr int N = 100;

        // Insert with high priorities
        for (int i = 0; i < N; ++i) {
            heap.push(i, 1000 + i);
        }

        // Decrease all keys to their index
        for (int i = 0; i < N; ++i) {
            heap.decrease_key(i, i);
        }

        // Verify order
        for (int i = 0; i < N; ++i) {
            auto entry = heap.pop();
            CHECK(entry.key == i);
            CHECK(entry.priority == i);
        }
    }

    TEST_CASE("Serialization roundtrip") {
        IndexedHeap<int, int> original;
        original.push(1, 30);
        original.push(2, 10);
        original.push(3, 20);

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, IndexedHeap<int, int>>(buffer);

        CHECK(restored.size() == original.size());
        CHECK(restored.top().key == original.top().key);
        CHECK(restored.top().priority == original.top().priority);
    }

    TEST_CASE("Serialization with strings") {
        IndexedHeap<String, int> original;
        original.push(String("alice"), 30);
        original.push(String("bob"), 10);
        original.push(String("charlie"), 20);

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, IndexedHeap<String, int>>(buffer);

        CHECK(restored.size() == 3);
        CHECK(restored.top().key.view() == "bob");
        CHECK(restored.top().priority == 10);
    }

    TEST_CASE("Copy construction") {
        IndexedHeap<int, int> original;
        original.push(1, 10);
        original.push(2, 20);

        IndexedHeap<int, int> copy(original);
        CHECK(copy.size() == 2);
        CHECK(copy.top().key == 1);

        // Modify original
        original.pop();
        CHECK(copy.size() == 2); // Copy unchanged
    }

    TEST_CASE("Move construction") {
        IndexedHeap<int, int> original;
        original.push(1, 10);
        original.push(2, 20);

        IndexedHeap<int, int> moved(std::move(original));
        CHECK(moved.size() == 2);
        CHECK(moved.top().key == 1);
    }

    TEST_CASE("Reserve") {
        IndexedHeap<int, int> heap;
        heap.reserve(100);

        for (int i = 0; i < 100; ++i) {
            heap.push(i, i);
        }
        CHECK(heap.size() == 100);
    }

    TEST_CASE("Iteration") {
        IndexedHeap<int, int> heap;
        heap.push(1, 30);
        heap.push(2, 10);
        heap.push(3, 20);

        // Iteration is not in heap order, just for inspection
        int count = 0;
        for (auto const &entry : heap) {
            CHECK(entry.key >= 1);
            CHECK(entry.key <= 3);
            ++count;
        }
        CHECK(count == 3);
    }
}
