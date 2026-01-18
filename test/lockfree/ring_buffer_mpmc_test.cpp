#include <atomic>
#include <cassert>
#include <chrono>
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
    assert(ring.size() == 1);

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
    assert(ring.size() == 4);

    auto r = ring.push(99);
    assert(!r.is_ok());

    std::cout << "PASSED\n";
}

void test_mpmc_wrapping() {
    std::cout << "Test 3: MPMC Wrapping... ";

    RingBuffer<MPMC, int> ring(4);

    for (int round = 0; round < 10; round++) {
        for (int i = 0; i < 4; i++) {
            ring.push(i);
        }
        for (int i = 0; i < 4; i++) {
            auto val = ring.pop();
            assert(val.value() == i);
        }
    }

    std::cout << "PASSED\n";
}

void test_mpmc_multiple_producers() {
    std::cout << "Test 4: MPMC Multiple producers... ";

    RingBuffer<MPMC, int> ring(4096);
    const int NUM_PRODUCERS = 4;
    const int ITEMS_PER_PRODUCER = 1000;
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
    std::cout << "Test 5: MPMC Multiple consumers... ";

    const int NUM_ITEMS = 4000;
    RingBuffer<MPMC, int> ring(NUM_ITEMS);

    for (int i = 0; i < NUM_ITEMS; i++) {
        ring.push(i);
    }

    const int NUM_CONSUMERS = 4;
    std::atomic<int> total_popped{0};
    std::vector<int> all_values[NUM_CONSUMERS];

    std::vector<std::thread> consumers;
    for (int c = 0; c < NUM_CONSUMERS; c++) {
        consumers.emplace_back([&ring, &total_popped, &all_values, c]() {
            while (true) {
                auto result = ring.pop();
                if (!result.is_ok()) {
                    if (ring.empty()) {
                        break;
                    }
                    std::this_thread::yield();
                    continue;
                }
                all_values[c].push_back(result.value());
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
    std::cout << "Test 6: MPMC Multiple producers and consumers... ";

    RingBuffer<MPMC, int> ring(256);
    const int NUM_PRODUCERS = 3;
    const int NUM_CONSUMERS = 3;
    const int ITEMS_PER_PRODUCER = 1000;

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

void test_mpmc_emplace() {
    std::cout << "Test 7: MPMC Emplace... ";

    struct Point {
        int x, y;
        Point(int x_, int y_) : x(x_), y(y_) {}
    };

    RingBuffer<MPMC, Point> ring(8);

    auto r = ring.emplace(10, 20);
    assert(r.is_ok());

    auto p = ring.pop();
    assert(p.is_ok());
    assert(p.value().x == 10);
    assert(p.value().y == 20);

    std::cout << "PASSED\n";
}

void test_mpmc_snapshot() {
    std::cout << "Test 8: MPMC Snapshot... ";

    RingBuffer<MPMC, int> ring(16);

    for (int i = 0; i < 5; i++) {
        ring.push(i);
    }

    auto snap = ring.snapshot();
    assert(snap.magic == 0x4D504D43);
    assert(snap.capacity == 16);
    assert(snap.write_pos - snap.read_pos == 5);

    std::cout << "PASSED\n";
}

void test_mpmc_snapshot_with_data() {
    std::cout << "Test 9: MPMC Snapshot with data... ";

    RingBuffer<MPMC, int> ring(16);

    for (int i = 0; i < 5; i++) {
        ring.push(i * 10);
    }

    auto snap = ring.snapshot_with_data();
    assert(snap.data.size() == 5);

    std::cout << "PASSED\n";
}

void test_mpmc_from_snapshot() {
    std::cout << "Test 10: MPMC From snapshot... ";

    RingBuffer<MPMC, int> ring1(16);

    for (int i = 0; i < 5; i++) {
        ring1.push(i * 100);
    }

    auto snap = ring1.snapshot_with_data();
    auto result = RingBuffer<MPMC, int>::from_snapshot(snap);

    assert(result.is_ok());
    auto &ring2 = result.value();

    assert(ring2.size() == 5);

    std::cout << "PASSED\n";
}

void test_mpmc_drain() {
    std::cout << "Test 11: MPMC Drain... ";

    RingBuffer<MPMC, int> ring(16);

    for (int i = 0; i < 10; i++) {
        ring.push(i);
    }

    auto drained = ring.drain();
    assert(drained.size() == 10);
    assert(ring.empty());

    std::cout << "PASSED\n";
}

void test_mpmc_shared_memory() {
    std::cout << "Test 12: MPMC Shared memory... ";

    shm_unlink("/test_mpmc_ring");

    auto create_result = RingBuffer<MPMC, int>::create_shm("/test_mpmc_ring", 32);
    assert(create_result.is_ok());

    auto &ring_writer = create_result.value();

    for (int i = 0; i < 10; i++) {
        ring_writer.push(i + 100);
    }

    auto attach_result = RingBuffer<MPMC, int>::attach_shm("/test_mpmc_ring");
    assert(attach_result.is_ok());

    auto &ring_reader = attach_result.value();
    assert(ring_reader.size() == 10);

    auto val = ring_reader.pop();
    assert(val.is_ok());
    assert(val.value() == 100);

    std::cout << "PASSED\n";
}

void test_mpmc_stress() {
    std::cout << "Test 13: MPMC Stress test... ";

    RingBuffer<MPMC, int> ring(512);
    const int NUM_PRODUCERS = 8;
    const int NUM_CONSUMERS = 8;
    const int ITEMS_PER_PRODUCER = 5000;

    std::atomic<int> total_pushed{0};
    std::atomic<int> total_popped{0};
    std::atomic<bool> producers_done{false};

    auto start = std::chrono::high_resolution_clock::now();

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

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    assert(total_pushed.load() == NUM_PRODUCERS * ITEMS_PER_PRODUCER);
    assert(total_popped.load() == NUM_PRODUCERS * ITEMS_PER_PRODUCER);
    assert(ring.empty());

    std::cout << "PASSED (took " << duration.count() << "ms)\n";
}

int main() {
    std::cout << "Running MPMC RingBuffer tests...\n\n";

    test_mpmc_basic();
    test_mpmc_full();
    test_mpmc_wrapping();
    test_mpmc_multiple_producers();
    test_mpmc_multiple_consumers();
    test_mpmc_producers_and_consumers();
    test_mpmc_emplace();
    test_mpmc_snapshot();
    test_mpmc_snapshot_with_data();
    test_mpmc_from_snapshot();
    test_mpmc_drain();
    test_mpmc_shared_memory();
    test_mpmc_stress();

    std::cout << "\nAll MPMC tests PASSED!\n";

    return 0;
}
