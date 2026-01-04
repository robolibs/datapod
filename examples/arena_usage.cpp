#include "datapod/pods/memory/arena.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace datapod;

// Example struct for demonstration
struct Point {
    float x, y, z;

    Point(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    void print() const { std::cout << "Point(" << x << ", " << y << ", " << z << ")\n"; }
};

int main() {
    std::cout << "=== Arena Allocator Demo ===\n\n";

    // 1. Basic allocation with default block size (64KB)
    {
        std::cout << "1. Basic Allocation:\n";
        Arena<int> arena;

        int *nums = arena.allocate(5);
        for (int i = 0; i < 5; ++i) {
            arena.construct(&nums[i], i * 10);
        }

        std::cout << "   Allocated numbers: ";
        for (int i = 0; i < 5; ++i) {
            std::cout << nums[i] << " ";
        }
        std::cout << "\n";
        std::cout << "   Bytes used: " << arena.bytes_used() << "\n";
        std::cout << "   Capacity: " << arena.bytes_capacity() << "\n\n";

        // Clean up
        for (int i = 0; i < 5; ++i) {
            arena.destroy(&nums[i]);
        }
    }

    // 2. Custom block size
    {
        std::cout << "2. Custom Block Size (1KB):\n";
        Arena<char> arena(1024);

        std::cout << "   Block size: " << arena.block_size() << " bytes\n";

        char *buffer = arena.allocate(512);
        std::cout << "   Allocated 512 bytes\n";
        std::cout << "   Bytes used: " << arena.bytes_used() << "\n";
        std::cout << "   Capacity: " << arena.bytes_capacity() << "\n\n";
    }

    // 3. Complex types
    {
        std::cout << "3. Complex Types (std::string):\n";
        Arena<std::string> arena;

        std::string *words = arena.allocate(3);
        arena.construct(&words[0], "Arena");
        arena.construct(&words[1], "allocator");
        arena.construct(&words[2], "example");

        std::cout << "   Strings: ";
        for (int i = 0; i < 3; ++i) {
            std::cout << words[i] << " ";
        }
        std::cout << "\n\n";

        // Clean up
        for (int i = 0; i < 3; ++i) {
            arena.destroy(&words[i]);
        }
    }

    // 4. Struct allocation
    {
        std::cout << "4. Struct Allocation:\n";
        Arena<Point> arena;

        Point *points = arena.allocate(3);
        arena.construct(&points[0], 1.0f, 2.0f, 3.0f);
        arena.construct(&points[1], 4.0f, 5.0f, 6.0f);
        arena.construct(&points[2], 7.0f, 8.0f, 9.0f);

        std::cout << "   Points:\n";
        for (int i = 0; i < 3; ++i) {
            std::cout << "   ";
            points[i].print();
        }
        std::cout << "\n";

        // Clean up
        for (int i = 0; i < 3; ++i) {
            arena.destroy(&points[i]);
        }
    }

    // 5. Reset and reuse
    {
        std::cout << "5. Reset and Reuse:\n";
        Arena<int> arena;

        int *p1 = arena.allocate(100);
        std::cout << "   First allocation: " << arena.bytes_used() << " bytes used\n";

        arena.reset();
        std::cout << "   After reset: " << arena.bytes_used() << " bytes used\n";
        std::cout << "   Capacity retained: " << arena.bytes_capacity() << " bytes\n";

        int *p2 = arena.allocate(100);
        std::cout << "   Second allocation: " << arena.bytes_used() << " bytes used\n";
        std::cout << "   Same pointer? " << (p1 == p2 ? "Yes" : "No") << "\n\n";
    }

    // 6. Growth demonstration
    {
        std::cout << "6. Arena Growth:\n";
        Arena<char> arena(256); // Small block for demo

        std::cout << "   Initial capacity: " << arena.bytes_capacity() << "\n";

        arena.allocate(100);
        std::cout << "   After 100 bytes: capacity = " << arena.bytes_capacity() << "\n";

        arena.allocate(200);
        std::cout << "   After 200 more: capacity = " << arena.bytes_capacity() << "\n";

        arena.allocate(500);
        std::cout << "   After 500 more: capacity = " << arena.bytes_capacity() << "\n\n";
    }

    // 7. Bulk allocation pattern (typical use case)
    {
        std::cout << "7. Bulk Allocation Pattern:\n";
        Arena<int> arena;

        // Simulate a frame-based system that allocates many objects per frame
        for (int frame = 0; frame < 3; ++frame) {
            std::cout << "   Frame " << frame << ":\n";

            // Allocate many objects for this frame
            std::vector<int *> frame_objects;
            for (int i = 0; i < 10; ++i) {
                int *obj = arena.allocate(1);
                arena.construct(obj, frame * 100 + i);
                frame_objects.push_back(obj);
            }

            std::cout << "     Allocated 10 objects, bytes used: " << arena.bytes_used() << "\n";

            // Process objects...
            int sum = 0;
            for (auto *obj : frame_objects) {
                sum += *obj;
            }
            std::cout << "     Sum of objects: " << sum << "\n";

            // Destroy objects
            for (auto *obj : frame_objects) {
                arena.destroy(obj);
            }

            // At end of frame, reset arena for next frame
            arena.reset();
        }
        std::cout << "\n";
    }

    // 8. Clear vs Reset
    {
        std::cout << "8. Clear vs Reset:\n";
        Arena<int> arena;

        arena.allocate(1000);
        std::cout << "   After allocation:\n";
        std::cout << "     Bytes used: " << arena.bytes_used() << "\n";
        std::cout << "     Capacity: " << arena.bytes_capacity() << "\n";

        arena.reset();
        std::cout << "   After reset:\n";
        std::cout << "     Bytes used: " << arena.bytes_used() << "\n";
        std::cout << "     Capacity: " << arena.bytes_capacity() << " (retained)\n";

        arena.allocate(1000);
        arena.clear();
        std::cout << "   After clear:\n";
        std::cout << "     Bytes used: " << arena.bytes_used() << "\n";
        std::cout << "     Capacity: " << arena.bytes_capacity() << " (freed)\n\n";
    }

    std::cout << "=== Key Advantages of Arena Allocators ===\n";
    std::cout << "• Extremely fast allocation (just pointer bump)\n";
    std::cout << "• No individual deallocation overhead\n";
    std::cout << "• Perfect for frame-based or phase-based allocation\n";
    std::cout << "• Great for temporary objects with same lifetime\n";
    std::cout << "• Minimal fragmentation\n";

    return 0;
}
