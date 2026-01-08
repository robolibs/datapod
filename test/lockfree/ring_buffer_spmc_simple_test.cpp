#include <atomic>
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

    auto r = ring.push(99);
    assert(!r.is_ok());

    std::cout << "PASSED\n";
}

void test_spmc_single_producer_multiple_consumers() {
    std::cout << "Test 3: SPMC Single producer, multiple consumers... ";

    RingBuffer<SPMC, int> ring(256);
    const int NUM_CONSUMERS = 2;
    const int TOTAL_ITEMS = 200;

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

void test_spmc_snapshot() {
    std::cout << "Test 4: SPMC Snapshot... ";

    RingBuffer<SPMC, int> ring(16);

    for (int i = 0; i < 5; i++) {
        ring.push(i);
    }

    auto snap = ring.snapshot();
    assert(snap.magic == 0x53504D43);
    assert(snap.capacity == 16);

    std::cout << "PASSED\n";
}

void test_spmc_drain() {
    std::cout << "Test 5: SPMC Drain... ";

    RingBuffer<SPMC, int> ring(16);

    for (int i = 0; i < 10; i++) {
        ring.push(i);
    }

    auto drained = ring.drain();
    assert(drained.size() == 10);
    assert(ring.empty());

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "Running SPMC RingBuffer tests (simple)...\n\n";

    test_spmc_basic();
    test_spmc_full();
    test_spmc_single_producer_multiple_consumers();
    test_spmc_snapshot();
    test_spmc_drain();

    std::cout << "\nAll SPMC tests PASSED!\n";

    return 0;
}
