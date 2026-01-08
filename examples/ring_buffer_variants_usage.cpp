#include <atomic>
#include <datapod/pods/lockfree/ring_buffer.hpp>
#include <iostream>
#include <thread>
#include <vector>

using namespace datapod;

void example_spsc() {
    std::cout << "=== SPSC (Single Producer Single Consumer) ===\n";
    std::cout << "Best for: One thread writes, one thread reads\n";
    std::cout << "Performance: Fastest (no CAS operations)\n\n";

    RingBuffer<SPSC, int> ring(32);

    std::thread producer([&ring]() {
        for (int i = 0; i < 10; i++) {
            ring.push(i * 10);
            std::cout << "Producer: pushed " << i * 10 << "\n";
        }
    });

    std::thread consumer([&ring]() {
        int count = 0;
        while (count < 10) {
            auto result = ring.pop();
            if (result.is_ok()) {
                std::cout << "Consumer: popped " << result.value() << "\n";
                count++;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    std::cout << "\n";
}

void example_spmc() {
    std::cout << "=== SPMC (Single Producer Multiple Consumer) ===\n";
    std::cout << "Best for: One thread writes, multiple threads read\n";
    std::cout << "Performance: Fast writes, CAS on reads\n\n";

    RingBuffer<SPMC, int> ring(64);
    const int TOTAL_ITEMS = 20;
    std::atomic<bool> producer_done{false};
    std::atomic<int> total_consumed{0};

    std::thread producer([&ring, &producer_done]() {
        for (int i = 0; i < TOTAL_ITEMS; i++) {
            ring.push(i);
            std::cout << "Producer: pushed " << i << "\n";
        }
        producer_done.store(true, std::memory_order_release);
    });

    const int NUM_CONSUMERS = 3;
    std::vector<std::thread> consumers;

    for (int c = 0; c < NUM_CONSUMERS; c++) {
        consumers.emplace_back([&ring, &producer_done, &total_consumed, c]() {
            int my_count = 0;
            while (true) {
                auto result = ring.pop();
                if (result.is_ok()) {
                    std::cout << "Consumer " << c << ": popped " << result.value() << "\n";
                    my_count++;
                    total_consumed.fetch_add(1, std::memory_order_relaxed);
                } else {
                    if (producer_done.load(std::memory_order_acquire) && ring.empty()) {
                        break;
                    }
                    std::this_thread::yield();
                }
            }
            std::cout << "Consumer " << c << " finished with " << my_count << " items\n";
        });
    }

    producer.join();
    for (auto &t : consumers) {
        t.join();
    }

    std::cout << "Total consumed: " << total_consumed.load() << "/" << TOTAL_ITEMS << "\n\n";
}

void example_mpmc() {
    std::cout << "=== MPMC (Multiple Producer Multiple Consumer) ===\n";
    std::cout << "Best for: Multiple threads write, multiple threads read\n";
    std::cout << "Performance: CAS on both reads and writes\n\n";

    RingBuffer<MPMC, int> ring(64);
    const int NUM_PRODUCERS = 2;
    const int NUM_CONSUMERS = 2;
    const int ITEMS_PER_PRODUCER = 10;

    std::atomic<int> total_produced{0};
    std::atomic<int> total_consumed{0};
    std::atomic<bool> producers_done{false};

    std::vector<std::thread> producers;
    for (int p = 0; p < NUM_PRODUCERS; p++) {
        producers.emplace_back([&ring, &total_produced, p, ITEMS_PER_PRODUCER]() {
            for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {
                int value = p * 100 + i;
                while (!ring.push(value).is_ok()) {
                    std::this_thread::yield();
                }
                std::cout << "Producer " << p << ": pushed " << value << "\n";
                total_produced.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    std::vector<std::thread> consumers;
    for (int c = 0; c < NUM_CONSUMERS; c++) {
        consumers.emplace_back([&ring, &total_consumed, &producers_done, c]() {
            int my_count = 0;
            while (true) {
                auto result = ring.pop();
                if (result.is_ok()) {
                    std::cout << "Consumer " << c << ": popped " << result.value() << "\n";
                    my_count++;
                    total_consumed.fetch_add(1, std::memory_order_relaxed);
                } else {
                    if (producers_done.load(std::memory_order_acquire) && ring.empty()) {
                        break;
                    }
                    std::this_thread::yield();
                }
            }
            std::cout << "Consumer " << c << " finished with " << my_count << " items\n";
        });
    }

    for (auto &t : producers) {
        t.join();
    }
    producers_done.store(true, std::memory_order_release);

    for (auto &t : consumers) {
        t.join();
    }

    std::cout << "Total produced: " << total_produced.load() << "\n";
    std::cout << "Total consumed: " << total_consumed.load() << "\n\n";
}

void example_comparison() {
    std::cout << "=== Performance Characteristics ===\n\n";

    std::cout << "SPSC:\n";
    std::cout << "  - Fastest option\n";
    std::cout << "  - No atomic CAS operations\n";
    std::cout << "  - Use when: Single producer, single consumer\n\n";

    std::cout << "SPMC:\n";
    std::cout << "  - Fast writes (no CAS)\n";
    std::cout << "  - CAS on reads (consumer contention)\n";
    std::cout << "  - Use when: One data source, multiple workers\n";
    std::cout << "  - Example: Event dispatcher, work distribution\n\n";

    std::cout << "MPMC:\n";
    std::cout << "  - CAS on both reads and writes\n";
    std::cout << "  - Most flexible, slightly slower\n";
    std::cout << "  - Use when: Multiple producers and consumers\n";
    std::cout << "  - Example: Thread pool, general message passing\n\n";
}

void example_shared_memory() {
    std::cout << "=== Shared Memory Support ===\n";
    std::cout << "All variants support shared memory for IPC\n\n";

    shm_unlink("/demo_mpmc");

    auto result = RingBuffer<MPMC, int>::create_shm("/demo_mpmc", 32);
    if (result.is_ok()) {
        auto &ring = result.value();
        ring.push(42);
        ring.push(99);

        std::cout << "Created shared memory ring buffer\n";
        std::cout << "Size: " << ring.size() << "/" << ring.capacity() << "\n";

        auto attach_result = RingBuffer<MPMC, int>::attach_shm("/demo_mpmc");
        if (attach_result.is_ok()) {
            auto &attached = attach_result.value();
            std::cout << "Attached to shared memory\n";
            std::cout << "Size from attached: " << attached.size() << "\n";

            auto val = attached.pop();
            if (val.is_ok()) {
                std::cout << "Read from attached: " << val.value() << "\n";
            }
        }
    }

    std::cout << "\n";
}

int main() {
    std::cout << "Ring Buffer Variants Usage Examples\n";
    std::cout << "====================================\n\n";

    example_spsc();
    example_spmc();
    example_mpmc();
    example_comparison();
    example_shared_memory();

    std::cout << "All examples completed!\n";

    return 0;
}
