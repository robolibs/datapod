#include <cstdint>
#include <datapod/associative/mutable_fws_multimap.hpp>
#include <iostream>
#include <string>

using namespace datapod;

// Type alias for easier usage
template <typename T> using MutableMultimap = DynamicFwsMultimapBase<T, std::uint32_t, Vector>;

void example_basic_usage() {
    std::cout << "=== Basic Usage ===" << std::endl;

    MutableMultimap<int> mm;

    // Access creates buckets automatically
    auto bucket0 = mm[0];
    bucket0.push_back(100);
    bucket0.push_back(101);

    auto bucket1 = mm[1];
    bucket1.push_back(200);
    bucket1.push_back(201);
    bucket1.push_back(202);

    auto bucket2 = mm[2];
    bucket2.push_back(300);

    std::cout << "Total buckets: " << mm.size() << std::endl;
    std::cout << "Total elements: " << mm.element_count() << std::endl;

    std::cout << "\nBucket 0 (" << mm[0].size() << " items):" << std::endl;
    for (const auto &item : mm[0]) {
        std::cout << "  - " << item << std::endl;
    }

    std::cout << std::endl;
}

void example_bucket_operations() {
    std::cout << "=== Bucket Operations ===" << std::endl;

    MutableMultimap<int> mm;
    auto bucket = mm[0];

    // Push back
    bucket.push_back(10);
    bucket.push_back(20);
    bucket.push_back(30);
    std::cout << "After push_back: size=" << bucket.size() << std::endl;

    // Access elements
    std::cout << "bucket[0] = " << bucket[0] << std::endl;
    std::cout << "bucket.front() = " << bucket.front() << std::endl;
    std::cout << "bucket.back() = " << bucket.back() << std::endl;

    // Pop back
    bucket.pop_back();
    std::cout << "After pop_back: size=" << bucket.size() << std::endl;

    // Clear
    bucket.clear();
    std::cout << "After clear: size=" << bucket.size() << ", empty=" << (bucket.empty() ? "yes" : "no") << std::endl;

    std::cout << std::endl;
}

void example_bucket_capacity() {
    std::cout << "=== Bucket Capacity ===" << std::endl;

    MutableMultimap<int> mm;
    auto bucket = mm[0];

    std::cout << "Initial capacity: " << bucket.capacity() << std::endl;

    // Reserve space
    bucket.reserve(100);
    std::cout << "After reserve(100): capacity=" << bucket.capacity() << ", size=" << bucket.size() << std::endl;

    // Add elements
    for (int i = 0; i < 50; ++i) {
        bucket.push_back(i);
    }
    std::cout << "After adding 50 elements: size=" << bucket.size() << std::endl;

    // Resize
    bucket.resize(10);
    std::cout << "After resize(10): size=" << bucket.size() << std::endl;

    std::cout << std::endl;
}

void example_bucket_insert() {
    std::cout << "=== Bucket Insert ===" << std::endl;

    MutableMultimap<int> mm;
    auto bucket = mm[0];

    bucket.push_back(10);
    bucket.push_back(30);
    bucket.push_back(40);

    std::cout << "Before insert: [";
    for (size_t i = 0; i < bucket.size(); ++i) {
        if (i > 0)
            std::cout << ", ";
        std::cout << bucket[i];
    }
    std::cout << "]" << std::endl;

    // Insert 20 at position 1
    auto it = bucket.begin() + 1;
    bucket.insert(it, 20);

    std::cout << "After insert(20 at pos 1): [";
    for (size_t i = 0; i < bucket.size(); ++i) {
        if (i > 0)
            std::cout << ", ";
        std::cout << bucket[i];
    }
    std::cout << "]" << std::endl;

    std::cout << std::endl;
}

void example_bucket_erase() {
    std::cout << "=== Bucket Erase ===" << std::endl;

    MutableMultimap<int> mm;
    auto bucket = mm[0];

    for (int i = 0; i < 10; ++i) {
        bucket.push_back(i * 10);
    }

    std::cout << "Original size: " << bucket.size() << std::endl;

    // Erase single element
    auto it = bucket.begin() + 2; // Erase element at index 2
    bucket.erase(it);
    std::cout << "After erase(pos 2): size=" << bucket.size() << std::endl;

    // Erase range
    auto first = bucket.begin() + 1;
    auto last = bucket.begin() + 4;
    bucket.erase(first, last);
    std::cout << "After erase(range [1,4)): size=" << bucket.size() << std::endl;

    std::cout << "Remaining elements: [";
    for (size_t i = 0; i < bucket.size(); ++i) {
        if (i > 0)
            std::cout << ", ";
        std::cout << bucket[i];
    }
    std::cout << "]" << std::endl;

    std::cout << std::endl;
}

void example_bucket_emplace() {
    std::cout << "=== Bucket Emplace ===" << std::endl;

    struct Point {
        int x, y;
        Point() : x(0), y(0) {}
        Point(int x_, int y_) : x(x_), y(y_) {}
    };

    MutableMultimap<Point> mm;
    auto bucket = mm[0];

    // Emplace back - construct in place
    bucket.emplace_back(10, 20);
    bucket.emplace_back(30, 40);

    std::cout << "Emplaced points:" << std::endl;
    for (const auto &p : bucket) {
        std::cout << "  Point(" << p.x << ", " << p.y << ")" << std::endl;
    }

    std::cout << std::endl;
}

void example_bucket_iterators() {
    std::cout << "=== Bucket Iterators ===" << std::endl;

    MutableMultimap<int> mm;
    auto bucket = mm[0];

    for (int i = 1; i <= 5; ++i) {
        bucket.push_back(i * 10);
    }

    // Forward iteration
    std::cout << "Forward: ";
    for (auto it = bucket.begin(); it != bucket.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // Range-based for
    std::cout << "Range-for: ";
    for (int val : bucket) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    // Modify through iterator
    for (auto it = bucket.begin(); it != bucket.end(); ++it) {
        *it *= 2;
    }
    std::cout << "After doubling: ";
    for (int val : bucket) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    std::cout << std::endl;
}

void example_multiple_buckets() {
    std::cout << "=== Multiple Buckets ===" << std::endl;

    MutableMultimap<int> mm;

    // Populate multiple buckets
    for (int bucket_id = 0; bucket_id < 5; ++bucket_id) {
        auto bucket = mm[bucket_id];
        for (int i = 0; i < bucket_id + 1; ++i) {
            bucket.push_back(bucket_id * 100 + i);
        }
    }

    std::cout << "Total buckets: " << mm.size() << std::endl;
    std::cout << "Total elements: " << mm.element_count() << std::endl;

    // Access specific buckets
    std::cout << "\nBucket 0: " << mm[0].size() << " elements" << std::endl;
    std::cout << "Bucket 3: " << mm[3].size() << " elements" << std::endl;

    std::cout << "\nBucket 3 contents: [";
    for (size_t i = 0; i < mm[3].size(); ++i) {
        if (i > 0)
            std::cout << ", ";
        std::cout << mm[3][i];
    }
    std::cout << "]" << std::endl;

    std::cout << std::endl;
}

void example_multimap_iterators() {
    std::cout << "=== Multimap Iterators ===" << std::endl;

    MutableMultimap<int> mm;

    mm[0].push_back(10);
    mm[1].push_back(20);
    mm[1].push_back(21);
    mm[2].push_back(30);
    mm[2].push_back(31);
    mm[2].push_back(32);

    std::cout << "Iterating over all buckets:" << std::endl;
    int bucket_num = 0;
    for (auto bucket : mm) {
        std::cout << "  Bucket " << bucket_num++ << " [" << bucket.size() << " items]: ";
        for (int val : bucket) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;
}

void example_get_or_create() {
    std::cout << "=== Get or Create ===" << std::endl;

    MutableMultimap<int> mm;

    mm[0].push_back(100);

    std::cout << "Initial size: " << mm.size() << std::endl;

    // Get or create bucket 10 (creates buckets 0-10)
    auto bucket10 = mm.get_or_create(10);
    std::cout << "After get_or_create(10): size=" << mm.size() << std::endl;

    bucket10.push_back(1000);
    std::cout << "Bucket 10 size: " << bucket10.size() << std::endl;

    // Get existing bucket 0
    auto bucket0 = mm.get_or_create(0);
    std::cout << "Bucket 0 size (existing): " << bucket0.size() << std::endl;

    std::cout << std::endl;
}

void example_emplace_back_bucket() {
    std::cout << "=== Emplace Back Bucket ===" << std::endl;

    MutableMultimap<int> mm;

    // Emplace back creates a new bucket at the end
    auto bucket0 = mm.emplace_back();
    std::cout << "First emplace_back, index: " << bucket0.index() << std::endl;
    bucket0.push_back(100);

    auto bucket1 = mm.emplace_back();
    std::cout << "Second emplace_back, index: " << bucket1.index() << std::endl;
    bucket1.push_back(200);

    std::cout << "Total buckets: " << mm.size() << std::endl;

    std::cout << std::endl;
}

void example_front_back() {
    std::cout << "=== Front and Back ===" << std::endl;

    MutableMultimap<int> mm;

    mm[0].push_back(10);
    mm[1].push_back(20);
    mm[2].push_back(30);

    auto front_bucket = mm.front();
    std::cout << "Front bucket index: " << front_bucket.index() << std::endl;
    std::cout << "Front bucket first element: " << front_bucket[0] << std::endl;

    auto back_bucket = mm.back();
    std::cout << "Back bucket index: " << back_bucket.index() << std::endl;
    std::cout << "Back bucket first element: " << back_bucket[0] << std::endl;

    std::cout << std::endl;
}

void example_at_method() {
    std::cout << "=== At Method (with bounds checking) ===" << std::endl;

    MutableMultimap<int> mm;

    mm[0].push_back(10);
    mm[2].push_back(20);

    try {
        auto bucket0 = mm.at(0);
        std::cout << "bucket.at(0): valid, size=" << bucket0.size() << std::endl;

        auto bucket5 = mm.at(5); // Out of range
        std::cout << "bucket.at(5): should not reach here" << std::endl;
    } catch (const std::out_of_range &e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }

    // Element at() on bucket
    auto bucket0 = mm[0];
    bucket0.push_back(100);
    bucket0.push_back(200);

    try {
        std::cout << "bucket[0].at(0) = " << bucket0.at(0) << std::endl;
        std::cout << "bucket[0].at(1) = " << bucket0.at(1) << std::endl;
        std::cout << "bucket[0].at(10) = " << bucket0.at(10) << std::endl; // Out of range
    } catch (const std::out_of_range &e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }

    std::cout << std::endl;
}

void example_clear_operations() {
    std::cout << "=== Clear Operations ===" << std::endl;

    MutableMultimap<int> mm;

    mm[0].push_back(10);
    mm[0].push_back(20);
    mm[1].push_back(30);
    mm[2].push_back(40);

    std::cout << "Before clear: " << mm.size() << " buckets, " << mm.element_count() << " elements" << std::endl;

    // Clear one bucket
    mm[0].clear();
    std::cout << "After clearing bucket 0: " << mm.size() << " buckets, " << mm.element_count() << " elements"
              << std::endl;

    // Clear entire multimap
    mm.clear();
    std::cout << "After clearing multimap: " << mm.size() << " buckets, " << mm.element_count() << " elements"
              << std::endl;

    std::cout << std::endl;
}

void example_reserve_multimap() {
    std::cout << "=== Reserve Multimap ===" << std::endl;

    MutableMultimap<int> mm;

    // Reserve space for buckets and data
    mm.reserve(100, 1000);
    std::cout << "Reserved space for 100 buckets and 1000 elements" << std::endl;

    // Add some data
    for (int i = 0; i < 10; ++i) {
        mm[i].push_back(i * 10);
    }

    std::cout << "After adding data: " << mm.size() << " buckets, " << mm.element_count() << " elements" << std::endl;

    std::cout << std::endl;
}

void example_sparse_allocation() {
    std::cout << "=== Sparse Allocation ===" << std::endl;

    MutableMultimap<int> mm;

    // Non-contiguous bucket allocation
    mm[0].push_back(100);
    mm[10].push_back(200);
    mm[100].push_back(300);

    std::cout << "Allocated buckets: 0, 10, 100" << std::endl;
    std::cout << "Total buckets created: " << mm.size() << std::endl;
    std::cout << "Total elements: " << mm.element_count() << std::endl;

    // Empty buckets in between
    std::cout << "Bucket 5 empty: " << (mm[5].empty() ? "yes" : "no") << std::endl;
    std::cout << "Bucket 50 empty: " << (mm[50].empty() ? "yes" : "no") << std::endl;

    std::cout << std::endl;
}

void example_use_case_adjacency_list() {
    std::cout << "=== Use Case: Mutable Graph Adjacency List ===" << std::endl;

    MutableMultimap<uint32_t> graph;

    // Build graph edges
    graph[0].push_back(1);
    graph[0].push_back(2);

    graph[1].push_back(3);
    graph[1].push_back(4);

    graph[2].push_back(1);

    // Add more edges dynamically
    graph[0].push_back(5);
    graph[3].push_back(2);

    std::cout << "Graph adjacency list:" << std::endl;
    for (uint32_t node = 0; node < graph.size(); ++node) {
        auto neighbors = graph[node];
        if (!neighbors.empty()) {
            std::cout << "  Node " << node << " -> [";
            for (size_t i = 0; i < neighbors.size(); ++i) {
                if (i > 0)
                    std::cout << ", ";
                std::cout << neighbors[i];
            }
            std::cout << "]" << std::endl;
        }
    }

    // Remove an edge
    graph[1].pop_back(); // Remove last edge from node 1
    std::cout << "\nAfter removing edge from node 1:" << std::endl;
    std::cout << "  Node 1 -> [";
    for (size_t i = 0; i < graph[1].size(); ++i) {
        if (i > 0)
            std::cout << ", ";
        std::cout << graph[1][i];
    }
    std::cout << "]" << std::endl;

    std::cout << std::endl;
}

void example_const_access() {
    std::cout << "=== Const Access ===" << std::endl;

    MutableMultimap<int> mm;

    mm[0].push_back(100);
    mm[0].push_back(200);
    mm[1].push_back(300);

    const auto &const_mm = mm;

    std::cout << "Const multimap size: " << const_mm.size() << std::endl;

    auto const_bucket = const_mm[0];
    std::cout << "Const bucket 0 size: " << const_bucket.size() << std::endl;
    std::cout << "Const bucket 0 first element: " << const_bucket[0] << std::endl;

    // Iterate const
    for (const auto &bucket : const_mm) {
        for (int val : bucket) {
            std::cout << val << " ";
        }
    }
    std::cout << std::endl;

    std::cout << std::endl;
}

int main() {
    std::cout << "DataPod MutableFwsMultimap Usage Examples" << std::endl;
    std::cout << "==========================================" << std::endl << std::endl;

    example_basic_usage();
    example_bucket_operations();
    example_bucket_capacity();
    example_bucket_insert();
    example_bucket_erase();
    example_bucket_emplace();
    example_bucket_iterators();
    example_multiple_buckets();
    example_multimap_iterators();
    example_get_or_create();
    example_emplace_back_bucket();
    example_front_back();
    example_at_method();
    example_clear_operations();
    example_reserve_multimap();
    example_sparse_allocation();
    example_use_case_adjacency_list();
    example_const_access();

    std::cout << "All examples completed successfully!" << std::endl;

    return 0;
}
