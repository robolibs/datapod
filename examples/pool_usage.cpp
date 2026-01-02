#include "datapod/memory/pool.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

using namespace datapod;

// Example game entity
struct Entity {
    int id;
    float x, y, z;
    bool active;

    Entity(int id_, float x_, float y_, float z_) : id(id_), x(x_), y(y_), z(z_), active(true) {}

    void print() const {
        std::cout << "Entity[" << id << "] at (" << x << ", " << y << ", " << z << ") "
                  << (active ? "active" : "inactive") << "\n";
    }
};

// Simple benchmark helper
template <typename Func> double measure_ms(Func &&func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    std::cout << "=== Pool Allocator Demo ===\n\n";

    // 1. Basic allocation and deallocation
    {
        std::cout << "1. Basic Allocation:\n";
        Pool<int> pool;

        int *nums[5];
        for (int i = 0; i < 5; ++i) {
            nums[i] = pool.allocate(1);
            pool.construct(nums[i], i * 10);
        }

        std::cout << "   Allocated numbers: ";
        for (int i = 0; i < 5; ++i) {
            std::cout << *nums[i] << " ";
        }
        std::cout << "\n";
        std::cout << "   Allocated count: " << pool.allocated_count() << "\n";
        std::cout << "   Chunk count: " << pool.chunk_count() << "\n";
        std::cout << "   Total capacity: " << pool.capacity() << "\n\n";

        // Clean up
        for (int i = 0; i < 5; ++i) {
            pool.destroy(nums[i]);
            pool.deallocate(nums[i], 1);
        }
    }

    // 2. Free list reuse
    {
        std::cout << "2. Free List Reuse:\n";
        Pool<int> pool;

        int *p1 = pool.allocate(1);
        int *p2 = pool.allocate(1);
        int *p3 = pool.allocate(1);

        std::cout << "   Allocated 3 blocks: p1=" << p1 << ", p2=" << p2 << ", p3=" << p3 << "\n";

        pool.deallocate(p2, 1);
        pool.deallocate(p3, 1);

        std::cout << "   Deallocated p2 and p3\n";
        std::cout << "   Free count: " << pool.free_count() << "\n";

        int *p4 = pool.allocate(1);
        int *p5 = pool.allocate(1);

        std::cout << "   Allocated 2 new blocks: p4=" << p4 << ", p5=" << p5 << "\n";
        std::cout << "   p4 reused p3? " << (p4 == p3 ? "Yes" : "No") << "\n";
        std::cout << "   p5 reused p2? " << (p5 == p2 ? "Yes" : "No") << "\n\n";

        pool.deallocate(p1, 1);
        pool.deallocate(p4, 1);
        pool.deallocate(p5, 1);
    }

    // 3. Custom chunk size
    {
        std::cout << "3. Custom Chunk Size:\n";
        Pool<int> pool(16); // 16 blocks per chunk

        std::cout << "   Chunk size: " << pool.chunk_size() << " blocks\n";

        std::vector<int *> ptrs;
        for (int i = 0; i < 20; ++i) {
            ptrs.push_back(pool.allocate(1));
        }

        std::cout << "   Allocated 20 blocks\n";
        std::cout << "   Chunks allocated: " << pool.chunk_count() << "\n";
        std::cout << "   Total capacity: " << pool.capacity() << "\n\n";

        for (auto *p : ptrs) {
            pool.deallocate(p, 1);
        }
    }

    // 4. Game entity pool example
    {
        std::cout << "4. Game Entity Pool:\n";
        Pool<Entity> entity_pool(32);

        std::vector<Entity *> entities;

        // Spawn entities
        for (int i = 0; i < 5; ++i) {
            Entity *e = entity_pool.allocate(1);
            entity_pool.construct(e, i, i * 1.0f, i * 2.0f, i * 3.0f);
            entities.push_back(e);
        }

        std::cout << "   Spawned entities:\n";
        for (auto *e : entities) {
            std::cout << "   ";
            e->print();
        }

        std::cout << "   Pool stats:\n";
        std::cout << "     Allocated: " << entity_pool.allocated_count() << "\n";
        std::cout << "     Free: " << entity_pool.free_count() << "\n";
        std::cout << "     Capacity: " << entity_pool.capacity() << "\n\n";

        // Despawn middle entity
        Entity *to_remove = entities[2];
        std::cout << "   Despawning entity " << to_remove->id << "\n";
        entity_pool.destroy(to_remove);
        entity_pool.deallocate(to_remove, 1);
        entities.erase(entities.begin() + 2);

        std::cout << "   After despawn:\n";
        std::cout << "     Allocated: " << entity_pool.allocated_count() << "\n";
        std::cout << "     Free: " << entity_pool.free_count() << "\n\n";

        // Clean up
        for (auto *e : entities) {
            entity_pool.destroy(e);
            entity_pool.deallocate(e, 1);
        }
    }

    // 5. String pool
    {
        std::cout << "5. String Pool:\n";
        Pool<std::string> string_pool;

        std::vector<std::string *> strings;
        const char *words[] = {"Pool", "allocator", "is", "fast", "and", "efficient"};

        for (auto *word : words) {
            std::string *s = string_pool.allocate(1);
            string_pool.construct(s, word);
            strings.push_back(s);
        }

        std::cout << "   Strings: ";
        for (auto *s : strings) {
            std::cout << *s << " ";
        }
        std::cout << "\n\n";

        // Clean up
        for (auto *s : strings) {
            string_pool.destroy(s);
            string_pool.deallocate(s, 1);
        }
    }

    // 6. Memory statistics
    {
        std::cout << "6. Memory Statistics:\n";
        Pool<int> pool(64);

        std::cout << "   Initial state:\n";
        std::cout << "     Chunks: " << pool.chunk_count() << "\n";
        std::cout << "     Capacity: " << pool.capacity() << "\n";
        std::cout << "     Free: " << pool.free_count() << "\n";

        std::vector<int *> ptrs;
        for (int i = 0; i < 100; ++i) {
            ptrs.push_back(pool.allocate(1));
        }

        std::cout << "   After 100 allocations:\n";
        std::cout << "     Chunks: " << pool.chunk_count() << "\n";
        std::cout << "     Capacity: " << pool.capacity() << "\n";
        std::cout << "     Allocated: " << pool.allocated_count() << "\n";
        std::cout << "     Free: " << pool.free_count() << "\n";

        for (int i = 0; i < 50; ++i) {
            pool.deallocate(ptrs[i], 1);
        }

        std::cout << "   After 50 deallocations:\n";
        std::cout << "     Allocated: " << pool.allocated_count() << "\n";
        std::cout << "     Free: " << pool.free_count() << "\n\n";

        for (size_t i = 50; i < ptrs.size(); ++i) {
            pool.deallocate(ptrs[i], 1);
        }
    }

    // 7. Performance comparison
    {
        std::cout << "7. Performance Comparison (10000 allocations):\n";
        constexpr int N = 10000;

        // Pool allocator
        double pool_time = measure_ms([&]() {
            Pool<int> pool;
            std::vector<int *> ptrs;
            for (int i = 0; i < N; ++i) {
                ptrs.push_back(pool.allocate(1));
            }
            for (auto *p : ptrs) {
                pool.deallocate(p, 1);
            }
        });

        // Standard malloc/free
        double malloc_time = measure_ms([&]() {
            std::vector<int *> ptrs;
            for (int i = 0; i < N; ++i) {
                ptrs.push_back(static_cast<int *>(malloc(sizeof(int))));
            }
            for (auto *p : ptrs) {
                free(p);
            }
        });

        std::cout << "   Pool allocator: " << pool_time << " ms\n";
        std::cout << "   malloc/free:    " << malloc_time << " ms\n";
        std::cout << "   Speedup:        " << (malloc_time / pool_time) << "x\n\n";
    }

    // 8. Clear demonstration
    {
        std::cout << "8. Clear Operation:\n";
        Pool<int> pool;

        std::vector<int *> ptrs;
        for (int i = 0; i < 50; ++i) {
            ptrs.push_back(pool.allocate(1));
        }

        std::cout << "   Before clear:\n";
        std::cout << "     Allocated: " << pool.allocated_count() << "\n";
        std::cout << "     Chunks: " << pool.chunk_count() << "\n";
        std::cout << "     Capacity: " << pool.capacity() << "\n";

        pool.clear();

        std::cout << "   After clear:\n";
        std::cout << "     Allocated: " << pool.allocated_count() << "\n";
        std::cout << "     Chunks: " << pool.chunk_count() << "\n";
        std::cout << "     Capacity: " << pool.capacity() << "\n\n";
    }

    std::cout << "=== Key Advantages of Pool Allocators ===\n";
    std::cout << "• O(1) allocation and deallocation (just pointer manipulation)\n";
    std::cout << "• Excellent cache locality (objects allocated close together)\n";
    std::cout << "• Minimal memory fragmentation\n";
    std::cout << "• Perfect for fixed-size object pools (entities, particles, nodes)\n";
    std::cout << "• Predictable memory usage\n";
    std::cout << "• Fast free-list reuse of deallocated blocks\n";

    return 0;
}
