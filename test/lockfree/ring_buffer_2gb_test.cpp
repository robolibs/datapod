#include <datapod/pods/lockfree/ring_buffer.hpp>
#include <iomanip>
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "Testing Ring Buffer with 2GB allocation\n";
    std::cout << "========================================\n\n";

    // 2GB of uint8_t = 2 * 1024 * 1024 * 1024 = 2,147,483,648 elements
    size_t capacity_2gb = 2ULL * 1024 * 1024 * 1024;

    std::cout << "Attempting to allocate 2GB ring buffer...\n";
    std::cout << "Capacity: " << capacity_2gb << " uint8_t elements\n";
    std::cout << "Size: " << (capacity_2gb / 1024.0 / 1024.0 / 1024.0) << " GB\n\n";

    try {
        std::cout << "Creating ring buffer... ";
        std::cout.flush();

        RingBuffer<SPSC, uint8_t> ring(capacity_2gb);

        std::cout << "SUCCESS!\n\n";

        std::cout << "Ring buffer info:\n";
        std::cout << "  Capacity: " << ring.capacity() << " elements\n";
        std::cout << "  Empty: " << (ring.empty() ? "yes" : "no") << "\n";
        std::cout << "  Full: " << (ring.full() ? "yes" : "no") << "\n\n";

        // Test basic operations
        std::cout << "Testing basic operations...\n";

        std::cout << "  Pushing 100 elements... ";
        for (int i = 0; i < 100; i++) {
            auto result = ring.push(static_cast<uint8_t>(i));
            if (!result.is_ok()) {
                std::cout << "FAILED at element " << i << "\n";
                return 1;
            }
        }
        std::cout << "OK\n";

        std::cout << "  Size after push: " << ring.size() << "\n";

        std::cout << "  Popping 100 elements... ";
        for (int i = 0; i < 100; i++) {
            auto result = ring.pop();
            if (!result.is_ok() || result.value() != static_cast<uint8_t>(i)) {
                std::cout << "FAILED at element " << i << "\n";
                return 1;
            }
        }
        std::cout << "OK\n";

        std::cout << "  Size after pop: " << ring.size() << "\n";
        std::cout << "  Empty: " << (ring.empty() ? "yes" : "no") << "\n\n";

        // Test filling a large portion
        std::cout << "Testing large fill (10 million elements)...\n";
        size_t large_fill = 10 * 1000 * 1000;

        std::cout << "  Pushing " << large_fill << " elements... ";
        std::cout.flush();
        for (size_t i = 0; i < large_fill; i++) {
            ring.push(static_cast<uint8_t>(i & 0xFF));
        }
        std::cout << "OK\n";

        std::cout << "  Size: " << ring.size() << " elements\n";

        std::cout << "  Popping " << large_fill << " elements... ";
        std::cout.flush();
        for (size_t i = 0; i < large_fill; i++) {
            auto result = ring.pop();
            if (!result.is_ok()) {
                std::cout << "FAILED\n";
                return 1;
            }
        }
        std::cout << "OK\n\n";

        std::cout << "=== 2GB RING BUFFER TEST PASSED ===\n";

    } catch (const std::exception &e) {
        std::cout << "FAILED\n";
        std::cout << "Exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cout << "FAILED\n";
        std::cout << "Unknown exception or allocation failure\n";
        return 1;
    }

    return 0;
}
