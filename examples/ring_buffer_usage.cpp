#include <datapod/pods/lockfree/ring_buffer.hpp>
#include <iostream>
#include <thread>
#include <chrono>

using namespace datapod;

// Example 1: Basic in-memory usage
void example_basic() {
    std::cout << "=== Example 1: Basic Usage ===\n";
    
    RingBuffer<SPSC, int> ring(16);
    
    // Producer pushes data
    for (int i = 0; i < 10; i++) {
        auto result = ring.push(i * 10);
        if (result.is_ok()) {
            std::cout << "Pushed: " << i * 10 << "\n";
        }
    }
    
    std::cout << "Ring size: " << ring.size() << "/" << ring.capacity() << "\n";
    
    // Consumer pops data
    while (!ring.empty()) {
        auto result = ring.pop();
        if (result.is_ok()) {
            std::cout << "Popped: " << result.value() << "\n";
        }
    }
    
    std::cout << "\n";
}

// Example 2: Shared memory IPC
void example_shared_memory() {
    std::cout << "=== Example 2: Shared Memory IPC ===\n";
    
    // Clean up any existing shared memory
    shm_unlink("/example_ring");
    
    // Process A: Create and write
    auto create_result = RingBuffer<SPSC, int>::create_shm("/example_ring", 32);
    if (!create_result.is_ok()) {
        std::cerr << "Failed to create shared memory ring\n";
        return;
    }
    
    auto& ring_writer = create_result.value();
    std::cout << "Created shared memory ring\n";
    
    for (int i = 0; i < 5; i++) {
        ring_writer.push(i + 100);
        std::cout << "Writer pushed: " << i + 100 << "\n";
    }
    
    // Process B: Attach and read
    auto attach_result = RingBuffer<SPSC, int>::attach_shm("/example_ring");
    if (!attach_result.is_ok()) {
        std::cerr << "Failed to attach to shared memory ring\n";
        return;
    }
    
    auto& ring_reader = attach_result.value();
    std::cout << "Attached to shared memory ring\n";
    
    while (!ring_reader.empty()) {
        auto result = ring_reader.pop();
        if (result.is_ok()) {
            std::cout << "Reader popped: " << result.value() << "\n";
        }
    }
    
    std::cout << "\n";
}

// Example 3: Multi-threaded producer-consumer
void example_multithreaded() {
    std::cout << "=== Example 3: Multi-threaded ===\n";
    
    shm_unlink("/mt_ring");
    
    auto result = RingBuffer<SPSC, int>::create_shm("/mt_ring", 128);
    if (!result.is_ok()) {
        std::cerr << "Failed to create ring\n";
        return;
    }
    
    const int ITEMS = 1000;
    
    // Producer thread
    std::thread producer([&]() {
        auto ring_result = RingBuffer<SPSC, int>::attach_shm("/mt_ring");
        auto& ring = ring_result.value();
        
        for (int i = 0; i < ITEMS; i++) {
            while (!ring.push(i).is_ok()) {
                std::this_thread::yield();
            }
            if (i % 100 == 0) {
                std::cout << "Produced: " << i << "\n";
            }
        }
        std::cout << "Producer done\n";
    });
    
    // Consumer thread
    std::thread consumer([&]() {
        auto ring_result = RingBuffer<SPSC, int>::attach_shm("/mt_ring");
        auto& ring = ring_result.value();
        
        int received = 0;
        for (int i = 0; i < ITEMS; i++) {
            Result<int, Error> val;
            while (!(val = ring.pop()).is_ok()) {
                std::this_thread::yield();
            }
            received++;
            if (i % 100 == 0) {
                std::cout << "Consumed: " << val.value() << "\n";
            }
        }
        std::cout << "Consumer done, received: " << received << " items\n";
    });
    
    producer.join();
    consumer.join();
    
    std::cout << "\n";
}

// Example 4: Snapshot and restore
void example_snapshot() {
    std::cout << "=== Example 4: Snapshot and Restore ===\n";
    
    RingBuffer<SPSC, int> ring1(16);
    
    // Fill with data
    for (int i = 0; i < 8; i++) {
        ring1.push(i * 5);
    }
    
    std::cout << "Original ring size: " << ring1.size() << "\n";
    
    // Take snapshot with data
    auto snap = ring1.snapshot_with_data();
    std::cout << "Snapshot captured " << snap.data.size() << " elements\n";
    
    // Restore from snapshot
    auto restore_result = RingBuffer<SPSC, int>::from_snapshot(snap);
    if (restore_result.is_ok()) {
        auto& ring2 = restore_result.value();
        std::cout << "Restored ring size: " << ring2.size() << "\n";
        
        std::cout << "Restored data: ";
        while (!ring2.empty()) {
            auto val = ring2.pop();
            if (val.is_ok()) {
                std::cout << val.value() << " ";
            }
        }
        std::cout << "\n";
    }
    
    std::cout << "\n";
}

// Example 5: Drain operation
void example_drain() {
    std::cout << "=== Example 5: Drain ===\n";
    
    RingBuffer<SPSC, int> ring(32);
    
    // Fill with data
    for (int i = 0; i < 15; i++) {
        ring.push(i);
    }
    
    std::cout << "Ring size before drain: " << ring.size() << "\n";
    
    // Drain all elements
    auto drained = ring.drain();
    
    std::cout << "Drained " << drained.size() << " elements\n";
    std::cout << "Ring size after drain: " << ring.size() << "\n";
    
    std::cout << "Drained data: ";
    for (const auto& val : drained) {
        std::cout << val << " ";
    }
    std::cout << "\n\n";
}

// Example 6: Peek without removing
void example_peek() {
    std::cout << "=== Example 6: Peek ===\n";
    
    RingBuffer<SPSC, int> ring(8);
    
    ring.push(42);
    ring.push(99);
    
    // Peek at front element
    auto peek_result = ring.peek();
    if (peek_result.is_ok()) {
        std::cout << "Peeked value: " << *peek_result.value() << "\n";
        std::cout << "Ring size (unchanged): " << ring.size() << "\n";
    }
    
    // Now actually remove it
    auto pop_result = ring.pop();
    if (pop_result.is_ok()) {
        std::cout << "Popped value: " << pop_result.value() << "\n";
        std::cout << "Ring size (after pop): " << ring.size() << "\n";
    }
    
    std::cout << "\n";
}

// Example 7: Emplace construction
void example_emplace() {
    std::cout << "=== Example 7: Emplace ===\n";
    
    struct Point {
        int x, y;
        Point(int x_, int y_) : x(x_), y(y_) {
            std::cout << "Point(" << x << ", " << y << ") constructed\n";
        }
    };
    
    RingBuffer<SPSC, Point> ring(8);
    
    // Emplace constructs in-place
    ring.emplace(10, 20);
    ring.emplace(30, 40);
    
    std::cout << "Ring size: " << ring.size() << "\n";
    
    auto p1 = ring.pop();
    if (p1.is_ok()) {
        std::cout << "Popped Point(" << p1.value().x << ", " << p1.value().y << ")\n";
    }
    
    std::cout << "\n";
}

int main() {
    std::cout << "RingBuffer Usage Examples\n";
    std::cout << "==========================\n\n";
    
    example_basic();
    example_shared_memory();
    example_multithreaded();
    example_snapshot();
    example_drain();
    example_peek();
    example_emplace();
    
    std::cout << "All examples completed!\n";
    
    return 0;
}
