#include <cassert>
#include <datapod/pods/lockfree/ring_buffer.hpp>
#include <iostream>
#include <thread>
#include <vector>

using namespace datapod;

void test_spmc_basic() {
    std::cout << "Test 1: SPMC Basic push/pop... ";

    RingBuffer<SPMC, int> ring(16);
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

void test_spmc_full() {
    std::cout << "Test 2: SPMC Full detection... ";

    RingBuffer<SPMC, int> ring(4);

    for (int i = 0; i < 4; i++) {
        assert(ring.push(i).is_ok());
    }
    assert(ring.full());
    assert(ring.size() == 4);

    auto r = ring.push(99);
    assert(!r.is_ok());

    std::cout << "PASSED\n";
}

void test_spmc_wrapping() {
    std::cout << "Test 3: SPMC Wrapping... ";

    RingBuffer<SPMC, int> ring(4);

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

void test_spmc_single_producer_multiple_consumers() {
    std::cout << "Test 4: SPMC Single producer, multiple consumers... ";

    RingBuffer<SPMC, int> ring(1024);
    const int NUM_CONSUMERS = 4;
    const int TOTAL_ITEMS = 4000;

    std::atomic<int> total_popped{0};
    std::atomic<bool> producer_done{false};

    std::thread producer([&ring, &producer_done, TOTAL_ITEMS]() {
        for (int i = 0; i < TOTAL_ITEMS; i++) {
            while (!ring.push(i).is_ok()) {
                std::this_thread::yield();
            }
        }
        producer_done.store(true, std::memory_order_release);
    });

    std::vector<std::thread> consumers;
    for (int c = 0; c < NUM_CONSUMERS; c++) {
        consumers.emplace_back([&ring, &total_popped, &producer_done]() {
            while (true) {
                auto result = ring.pop();
                if (result.is_ok()) {
                    total_popped.fetch_add(1, std::memory_order_relaxed);
                } else {
                    if (producer_done.load(std::memory_order_acquire) && ring.empty()) {
                        break;
                    }
                    std::this_thread::yield();
                }
            }
        });
    }

    producer.join();
    for (auto &t : consumers) {
        t.join();
    }

    assert(total_popped.load() == TOTAL_ITEMS);
    assert(ring.empty());

    std::cout << "PASSED\n";
}

void test_spmc_multiple_consumers_concurrent() {
    std::cout << "Test 5: SPMC Multiple consumers concurrent... ";

    const int NUM_ITEMS = 10000;
    RingBuffer<SPMC, int> ring(NUM_ITEMS);

    for (int i = 0; i < NUM_ITEMS; i++) {
        ring.push(i);
    }

    const int NUM_CONSUMERS = 8;
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

void test_spmc_emplace() {
    std::cout << "Test 6: SPMC Emplace... ";

    struct Point {
        int x, y;
        Point(int x_, int y_) : x(x_), y(y_) {}
    };

    RingBuffer<SPMC, Point> ring(8);

    auto r = ring.emplace(10, 20);
    assert(r.is_ok());

    auto p = ring.pop();
    assert(p.is_ok());
    assert(p.value().x == 10);
    assert(p.value().y == 20);

    std::cout << "PASSED\n";
}

void test_spmc_snapshot() {
    std::cout << "Test 7: SPMC Snapshot... ";

    RingBuffer<SPMC, int> ring(16);

    for (int i = 0; i < 5; i++) {
        ring.push(i);
    }

    auto snap = ring.snapshot();
    assert(snap.magic == 0x53504D43);
    assert(snap.capacity == 16);
    assert(snap.write_pos - snap.read_pos == 5);

    std::cout << "PASSED\n";
}

void test_spmc_snapshot_with_data() {
    std::cout << "Test 8: SPMC Snapshot with data... ";

    RingBuffer<SPMC, int> ring(16);

    for (int i = 0; i < 5; i++) {
        ring.push(i * 10);
    }

    auto snap = ring.snapshot_with_data();
    assert(snap.data.size() == 5);

    std::cout << "PASSED\n";
}

void test_spmc_from_snapshot() {
    std::cout << "Test 9: SPMC From snapshot... ";

    RingBuffer<SPMC, int> ring1(16);

    for (int i = 0; i < 5; i++) {
        ring1.push(i * 100);
    }

    auto snap = ring1.snapshot_with_data();
    auto result = RingBuffer<SPMC, int>::from_snapshot(snap);

    assert(result.is_ok());
    auto &ring2 = result.value();

    assert(ring2.size() == 5);

    std::cout << "PASSED\n";
}

void test_spmc_drain() {
    std::cout << "Test 10: SPMC Drain... ";

    RingBuffer<SPMC, int> ring(16);

    for (int i = 0; i < 10; i++) {
        ring.push(i);
    }

    auto drained = ring.drain();
    assert(drained.size() == 10);
    assert(ring.empty());

    std::cout << "PASSED\n";
}

void test_spmc_shared_memory() {
    std::cout << "Test 11: SPMC Shared memory... ";

    shm_unlink("/test_spmc_ring");

    auto create_result = RingBuffer<SPMC, int>::create_shm("/test_spmc_ring", 32);
    assert(create_result.is_ok());

    auto &ring_writer = create_result.value();

    for (int i = 0; i < 10; i++) {
        ring_writer.push(i + 100);
    }

    auto attach_result = RingBuffer<SPMC, int>::attach_shm("/test_spmc_ring");
    assert(attach_result.is_ok());

    auto &ring_reader = attach_result.value();
    assert(ring_reader.size() == 10);

    auto val = ring_reader.pop();
    assert(val.is_ok());
    assert(val.value() == 100);

    std::cout << "PASSED\n";
}

void test_spmc_stress() {
    std::cout << "Test 12: SPMC Stress test... ";

    RingBuffer<SPMC, int> ring(512);
    const int NUM_CONSUMERS = 8;
    const int TOTAL_ITEMS = 50000;

    std::atomic<int> total_popped{0};
    std::atomic<bool> producer_done{false};

    auto start = std::chrono::high_resolution_clock::now();

    std::thread producer([&ring, &producer_done, TOTAL_ITEMS]() {
        for (int i = 0; i < TOTAL_ITEMS; i++) {
            while (!ring.push(i).is_ok()) {
                std::this_thread::yield();
            }
        }
        producer_done.store(true, std::memory_order_release);
    });

    std::vector<std::thread> consumers;
    for (int c = 0; c < NUM_CONSUMERS; c++) {
        consumers.emplace_back([&ring, &total_popped, &producer_done]() {
            while (true) {
                auto result = ring.pop();
                if (result.is_ok()) {
                    total_popped.fetch_add(1, std::memory_order_relaxed);
                } else {
                    if (producer_done.load(std::memory_order_acquire) && ring.empty()) {
                        break;
                    }
                    std::this_thread::yield();
                }
            }
        });
    }

    producer.join();
    for (auto &t : consumers) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    assert(total_popped.load() == TOTAL_ITEMS);
    assert(ring.empty());

    std::cout << "PASSED (took " << duration.count() << "ms)\n";
}

void test_spmc_peek() {
    std::cout << "Test 13: SPMC Peek... ";

    RingBuffer<SPMC, int> ring(8);

    ring.push(42);
    ring.push(99);

    auto peek_result = ring.peek();
    assert(peek_result.is_ok());
    assert(*peek_result.value() == 42);
    assert(ring.size() == 2);

    auto pop_result = ring.pop();
    assert(pop_result.is_ok());
    assert(pop_result.value() == 42);
    assert(ring.size() == 1);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "Running SPMC RingBuffer tests...\n\n";

    test_spmc_basic();
    test_spmc_full();
    test_spmc_wrapping();
    test_spmc_single_producer_multiple_consumers();
    test_spmc_multiple_consumers_concurrent();
    test_spmc_emplace();
    test_spmc_snapshot();
    test_spmc_snapshot_with_data();
    test_spmc_from_snapshot();
    test_spmc_drain();
    test_spmc_shared_memory();
    test_spmc_stress();
    test_spmc_peek();

    std::cout << "\nAll SPMC tests PASSED!\n";

    return 0;
}
