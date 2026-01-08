#include <atomic>
#include <cassert>
#include <datapod/pods/lockfree/ring_buffer.hpp>
#include <iostream>
#include <thread>
#include <vector>

using namespace datapod;

void test_mpmc_basic() {
    std::cout << "Test 1: MPMC Basic push/pop... ";

    RingBuffer<MPMC, int> ring(16);
    assert(ring.empty());
    assert(ring.capacity() == 16);

    auto r1 = ring.push(42);
    assert(r1.is_ok());
    assert(!ring.empty());

    auto r2 = ring.pop();
    assert(r2.is_ok());
    assert(r2.value() == 42);
    assert(ring.empty());

    std::cout << "PASSED\n";
}

void test_mpmc_full() {
    std::cout << "Test 2: MPMC Full detection... ";

    RingBuffer<MPMC, int> ring(4);

    for (int i = 0; i < 4; i++) {
        assert(ring.push(i).is_ok());
    }
    assert(ring.full());

    auto r = ring.push(99);
    assert(!r.is_ok());

    std::cout << "PASSED\n";
}

void test_mpmc_multiple_producers() {
    std::cout << "Test 3: MPMC Multiple producers... ";

    RingBuffer<MPMC, int> ring(256);
    const int NUM_PRODUCERS = 2;
    const int ITEMS_PER_PRODUCER = 100;
    std::atomic<int> total_pushed{0};

    std::vector<std::thread> producers;
    for (int p = 0; p < NUM_PRODUCERS; p++) {
        producers.emplace_back([&ring, &total_pushed, p, ITEMS_PER_PRODUCER]() {
            for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
                int value = p * ITEMS_PER_PRODUCER + i;
                while (!ring.push(value).is_ok()) {
                    std::this_thread::yield();
                }
                total_pushed.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto &t : producers) {
        t.join();
    }

    assert(total_pushed.load() == NUM_PRODUCERS * ITEMS_PER_PRODUCER);
    assert(ring.size() == NUM_PRODUCERS * ITEMS_PER_PRODUCER);

    std::cout << "PASSED\n";
}

void test_mpmc_multiple_consumers() {
    std::cout << "Test 4: MPMC Multiple consumers... ";

    RingBuffer<MPMC, int> ring(256);
    const int NUM_ITEMS = 200;

    for (int i = 0; i < NUM_ITEMS; i++) {
        ring.push(i);
    }

    const int NUM_CONSUMERS = 2;
    std::atomic<int> total_popped{0};

    std::vector<std::thread> consumers;
    for (int c = 0; c < NUM_CONSUMERS; c++) {
        consumers.emplace_back([&ring, &total_popped]() {
            while (true) {
                auto result = ring.pop();
                if (!result.is_ok()) {
                    if (ring.empty()) {
                        break;
                    }
                    std::this_thread::yield();
                    continue;
                }
                total_popped.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto &t : consumers) {
        t.join();
    }

    assert(total_popped.load() == NUM_ITEMS);
    assert(ring.empty());

    std::cout << "PASSED\n";
}

void test_mpmc_producers_and_consumers() {
    std::cout << "Test 5: MPMC Producers and consumers... ";

    RingBuffer<MPMC, int> ring(128);
    const int NUM_PRODUCERS = 2;
    const int NUM_CONSUMERS = 2;
    const int ITEMS_PER_PRODUCER = 100;

    std::atomic<int> total_pushed{0};
    std::atomic<int> total_popped{0};
    std::atomic<bool> producers_done{false};

    std::vector<std::thread> producers;
    for (int p = 0; p < NUM_PRODUCERS; p++) {
        producers.emplace_back([&ring, &total_pushed, p, ITEMS_PER_PRODUCER]() {
            for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
                int value = p * ITEMS_PER_PRODUCER + i;
                while (!ring.push(value).is_ok()) {
                    std::this_thread::yield();
                }
                total_pushed.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    std::vector<std::thread> consumers;
    for (int c = 0; c < NUM_CONSUMERS; c++) {
        consumers.emplace_back([&ring, &total_popped, &producers_done]() {
            while (true) {
                auto result = ring.pop();
                if (result.is_ok()) {
                    total_popped.fetch_add(1, std::memory_order_relaxed);
                } else {
                    if (producers_done.load(std::memory_order_acquire) && ring.empty()) {
                        break;
                    }
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto &t : producers) {
        t.join();
    }
    producers_done.store(true, std::memory_order_release);

    for (auto &t : consumers) {
        t.join();
    }

    assert(total_pushed.load() == NUM_PRODUCERS * ITEMS_PER_PRODUCER);
    assert(total_popped.load() == NUM_PRODUCERS * ITEMS_PER_PRODUCER);
    assert(ring.empty());

    std::cout << "PASSED\n";
}

void test_mpmc_snapshot() {
    std::cout << "Test 6: MPMC Snapshot... ";

    RingBuffer<MPMC, int> ring(16);

    for (int i = 0; i < 5; i++) {
        ring.push(i);
    }

    auto snap = ring.snapshot();
    assert(snap.magic == 0x4D504D43);
    assert(snap.capacity == 16);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "Running MPMC RingBuffer tests (simple)...\n\n";

    test_mpmc_basic();
    test_mpmc_full();
    test_mpmc_multiple_producers();
    test_mpmc_multiple_consumers();
    test_mpmc_producers_and_consumers();
    test_mpmc_snapshot();

    std::cout << "\nAll MPMC tests PASSED!\n";

    return 0;
}
