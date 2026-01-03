#include <datapod/datapod.hpp>
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== ForwardList Usage Example ===\n\n";

    // Basic usage - singly linked list with O(1) prepend
    std::cout << "1. Basic Operations:\n";
    ForwardList<int> numbers;
    numbers.push_front(3);
    numbers.push_front(2);
    numbers.push_front(1);

    std::cout << "   After push_front(3), push_front(2), push_front(1):\n   ";
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // Initializer list construction
    std::cout << "2. Initializer List Construction:\n";
    ForwardList<std::string> words{"hello", "world", "from", "datapod"};
    std::cout << "   ";
    for (auto const &w : words) {
        std::cout << w << " ";
    }
    std::cout << "\n\n";

    // Insert after
    std::cout << "3. Insert After:\n";
    ForwardList<int> list{1, 3, 4};
    auto it = list.begin();
    list.insert_after(it, 2); // Insert 2 after 1
    std::cout << "   After inserting 2 after 1: ";
    for (int n : list) {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // Erase after
    std::cout << "4. Erase After:\n";
    ForwardList<int> list2{1, 2, 3, 4, 5};
    auto it2 = list2.begin();
    ++it2;                  // Point to 2
    list2.erase_after(it2); // Erase 3
    std::cout << "   After erasing element after 2: ";
    for (int n : list2) {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // Reverse
    std::cout << "5. Reverse:\n";
    ForwardList<int> list3{1, 2, 3, 4, 5};
    std::cout << "   Before: ";
    for (int n : list3) {
        std::cout << n << " ";
    }
    list3.reverse();
    std::cout << "\n   After:  ";
    for (int n : list3) {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // Use case: Adjacency list for a simple graph
    std::cout << "6. Use Case - Graph Adjacency List:\n";
    struct Edge {
        int to;
        double weight;
        auto members() noexcept { return std::tie(to, weight); }
        auto members() const noexcept { return std::tie(to, weight); }
    };

    // Graph with 4 vertices (0, 1, 2, 3)
    Vector<ForwardList<Edge>> adjacency(4);

    // Add edges: 0->1, 0->2, 1->2, 2->3
    adjacency[0].push_front({1, 1.0});
    adjacency[0].push_front({2, 2.0});
    adjacency[1].push_front({2, 1.5});
    adjacency[2].push_front({3, 1.0});

    std::cout << "   Graph edges:\n";
    for (size_t v = 0; v < adjacency.size(); ++v) {
        std::cout << "   Vertex " << v << " -> ";
        for (auto const &edge : adjacency[v]) {
            std::cout << edge.to << "(w=" << edge.weight << ") ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";

    // Serialization
    std::cout << "7. Serialization:\n";
    ForwardList<int> original{10, 20, 30, 40, 50};

    auto buf = serialize(original);
    std::cout << "   Serialized " << original.size() << " elements to " << buf.size() << " bytes\n";

    auto restored = deserialize<Mode::NONE, ForwardList<int>>(buf);
    std::cout << "   Restored: ";
    for (int n : restored) {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // Node reuse demonstration
    std::cout << "8. Node Reuse (Free List):\n";
    ForwardList<int> reuse_demo;
    reuse_demo.push_front(1);
    reuse_demo.push_front(2);
    reuse_demo.push_front(3);
    std::cout << "   Added 3 elements\n";

    reuse_demo.pop_front();
    reuse_demo.pop_front();
    std::cout << "   Removed 2 elements (nodes go to free list)\n";

    reuse_demo.push_front(4);
    reuse_demo.push_front(5);
    std::cout << "   Added 2 more (reuses freed nodes)\n";
    std::cout << "   Result: ";
    for (int n : reuse_demo) {
        std::cout << n << " ";
    }
    std::cout << "\n";

    return 0;
}
