#include <cstdint>
#include <datapod/pods/lockfree/ring_buffer.hpp>
#include <iomanip>
#include <iostream>
#include <limits>

using namespace datapod;

void print_size_info(const char *type_name, size_t type_size, size_t capacity) {
    size_t header_size = 128 + 64; // Approximate header size with padding
    size_t buffer_size = capacity * type_size;
    size_t total_size = header_size + buffer_size;

    std::cout << std::left << std::setw(20) << type_name << " | Capacity: " << std::setw(15) << capacity
              << " | Buffer: " << std::setw(12) << (buffer_size / (1024.0 * 1024.0)) << " MB"
              << " | Total: " << std::setw(12) << (total_size / (1024.0 * 1024.0)) << " MB"
              << "\n";
}

void test_theoretical_limits() {
    std::cout << "=== Theoretical Limits ===\n\n";

    std::cout << "Data type sizes:\n";
    std::cout << "  uint64_t capacity field: " << sizeof(uint64_t) << " bytes\n";
    std::cout << "  Maximum uint64_t value: " << std::numeric_limits<uint64_t>::max() << "\n";
    std::cout << "  Maximum size_t value: " << std::numeric_limits<size_t>::max() << "\n\n";

    std::cout << "Position counters:\n";
    std::cout << "  write_pos/read_pos type: uint64_t (64-bit)\n";
    std::cout << "  These are monotonically increasing counters\n";
    std::cout << "  Actual index = position % capacity\n\n";
}

void test_memory_limits() {
    std::cout << "=== Memory Allocation Limits ===\n\n";

    std::cout << std::left << std::setw(20) << "Element Type"
              << " | " << std::setw(15) << "Capacity"
              << " | " << std::setw(12) << "Buffer Size"
              << " | " << std::setw(12) << "Total Size"
              << "\n";
    std::cout << std::string(80, '-') << "\n";

    // Small types
    print_size_info("uint8_t", sizeof(uint8_t), 1024);
    print_size_info("uint8_t", sizeof(uint8_t), 1024 * 1024);
    print_size_info("uint8_t", sizeof(uint8_t), 1024 * 1024 * 1024);

    std::cout << "\n";

    // Medium types
    print_size_info("int", sizeof(int), 1024);
    print_size_info("int", sizeof(int), 1024 * 1024);
    print_size_info("int", sizeof(int), 256 * 1024 * 1024);

    std::cout << "\n";

    // Large types
    struct LargeStruct {
        char data[1024];
    };
    print_size_info("1KB struct", sizeof(LargeStruct), 1024);
    print_size_info("1KB struct", sizeof(LargeStruct), 1024 * 1024);

    std::cout << "\n";
}

void test_practical_allocations() {
    std::cout << "=== Practical Allocation Tests ===\n\n";

    // Test small allocation
    {
        std::cout << "Test 1: Small ring (1KB elements, 1K capacity)... ";
        try {
            RingBuffer<SPSC, uint8_t> ring(1024);
            std::cout << "SUCCESS - " << ring.capacity() << " elements\n";
        } catch (...) {
            std::cout << "FAILED\n";
        }
    }

    // Test medium allocation
    {
        std::cout << "Test 2: Medium ring (int, 1M capacity)... ";
        try {
            RingBuffer<SPSC, int> ring(1024 * 1024);
            std::cout << "SUCCESS - " << ring.capacity() << " elements (~4 MB)\n";
        } catch (...) {
            std::cout << "FAILED\n";
        }
    }

    // Test large allocation
    {
        std::cout << "Test 3: Large ring (int, 256M capacity)... ";
        try {
            RingBuffer<SPSC, int> ring(256 * 1024 * 1024);
            std::cout << "SUCCESS - " << ring.capacity() << " elements (~1 GB)\n";
        } catch (...) {
            std::cout << "FAILED\n";
        }
    }

    // Test very large allocation (might fail on some systems)
    {
        std::cout << "Test 4: Very large ring (uint8_t, 1GB capacity)... ";
        try {
            RingBuffer<SPSC, uint8_t> ring(1024ULL * 1024 * 1024);
            std::cout << "SUCCESS - " << ring.capacity() << " elements (~1 GB)\n";
        } catch (...) {
            std::cout << "FAILED (expected on systems with limited memory)\n";
        }
    }

    std::cout << "\n";
}

void test_shared_memory_limits() {
    std::cout << "=== Shared Memory Limits ===\n\n";

    std::cout << "Shared memory is limited by:\n";
    std::cout << "  1. System SHMMAX (max shared memory segment size)\n";
    std::cout << "  2. System SHMALL (total shared memory pages)\n";
    std::cout << "  3. Available RAM\n\n";

    std::cout << "Check your system limits with:\n";
    std::cout << "  cat /proc/sys/kernel/shmmax  # Max segment size\n";
    std::cout << "  cat /proc/sys/kernel/shmall  # Total pages\n\n";

    // Test small shared memory
    {
        shm_unlink("/test_size_small");
        std::cout << "Test: Small shared memory (1MB)... ";
        auto result = RingBuffer<SPSC, int>::create_shm("/test_size_small", 256 * 1024);
        if (result.is_ok()) {
            std::cout << "SUCCESS - " << result.value().capacity() << " elements\n";
        } else {
            std::cout << "FAILED\n";
        }
    }

    // Test medium shared memory
    {
        shm_unlink("/test_size_medium");
        std::cout << "Test: Medium shared memory (100MB)... ";
        auto result = RingBuffer<SPSC, int>::create_shm("/test_size_medium", 25 * 1024 * 1024);
        if (result.is_ok()) {
            std::cout << "SUCCESS - " << result.value().capacity() << " elements\n";
        } else {
            std::cout << "FAILED\n";
        }
    }

    std::cout << "\n";
}

void test_wraparound() {
    std::cout << "=== Position Counter Wraparound ===\n\n";

    std::cout << "The write_pos and read_pos are uint64_t counters.\n";
    std::cout << "They increment monotonically and wrap around at 2^64.\n\n";

    uint64_t max_uint64 = std::numeric_limits<uint64_t>::max();
    std::cout << "Maximum uint64_t: " << max_uint64 << "\n";
    std::cout << "At 1 billion ops/sec, time to overflow: " << (max_uint64 / 1000000000.0 / 60.0 / 60.0 / 24.0 / 365.0)
              << " years\n\n";

    std::cout << "Practical implication: Counter overflow is not a concern.\n\n";
}

void test_capacity_edge_cases() {
    std::cout << "=== Capacity Edge Cases ===\n\n";

    // Zero capacity
    {
        std::cout << "Test: Zero capacity... ";
        RingBuffer<SPSC, int> ring(0);
        std::cout << "Actual capacity: " << ring.capacity() << " (auto-adjusted to 1)\n";
    }

    // Power of 2 capacities (optimal for modulo)
    {
        std::cout << "Test: Power-of-2 capacities (optimal):\n";
        for (size_t pow = 10; pow <= 20; pow++) {
            size_t cap = 1ULL << pow;
            RingBuffer<SPSC, int> ring(cap);
            std::cout << "  2^" << pow << " = " << cap << " elements (" << (cap * sizeof(int) / 1024.0 / 1024.0)
                      << " MB)\n";
        }
    }

    std::cout << "\n";
}

int main() {
    std::cout << "Ring Buffer Size Limits Analysis\n";
    std::cout << "=================================\n\n";

    test_theoretical_limits();
    test_memory_limits();
    test_practical_allocations();
    test_shared_memory_limits();
    test_wraparound();
    test_capacity_edge_cases();

    std::cout << "\n=== Summary ===\n\n";
    std::cout << "Maximum theoretical capacity: 2^64 - 1 elements\n";
    std::cout << "Practical limits:\n";
    std::cout << "  - In-memory: Limited by available RAM and std::aligned_alloc\n";
    std::cout << "  - Shared memory: Limited by system SHMMAX setting\n";
    std::cout << "  - Typical safe range: 1K to 1B elements depending on element size\n";
    std::cout << "  - Position counters will not overflow in practice\n\n";

    std::cout << "Recommendations:\n";
    std::cout << "  - Use power-of-2 capacities for optimal modulo performance\n";
    std::cout << "  - For large buffers, consider shared memory for IPC\n";
    std::cout << "  - Monitor memory usage for very large allocations\n";

    return 0;
}
