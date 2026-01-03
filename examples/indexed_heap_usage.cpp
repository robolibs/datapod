#include "datapod/datapod.hpp"
#include <iostream>
#include <limits>

using namespace datapod;

int main() {
    std::cout << "=== IndexedHeap Usage Examples ===" << std::endl << std::endl;

    // Example 1: Basic operations
    {
        std::cout << "1. Basic IndexedHeap operations (min-heap by default):" << std::endl;

        IndexedHeap<int, int> heap;
        heap.push(1, 30); // key=1, priority=30
        heap.push(2, 10); // key=2, priority=10
        heap.push(3, 20); // key=3, priority=20

        std::cout << "   Pushed: (key=1, priority=30), (key=2, priority=10), (key=3, priority=20)" << std::endl;
        std::cout << "   Top element: key=" << heap.top().key << ", priority=" << heap.top().priority << std::endl;
        std::cout << "   Size: " << heap.size() << std::endl;
        std::cout << std::endl;
    }

    // Example 2: Pop elements in priority order
    {
        std::cout << "2. Pop elements in priority order:" << std::endl;

        IndexedHeap<String, int> tasks;
        tasks.push(String("low"), 100);
        tasks.push(String("high"), 10);
        tasks.push(String("medium"), 50);

        std::cout << "   Tasks by priority:" << std::endl;
        while (!tasks.empty()) {
            auto [key, priority] = tasks.pop();
            std::cout << "     " << key.view() << " (priority=" << priority << ")" << std::endl;
        }
        std::cout << std::endl;
    }

    // Example 3: Decrease key - the key feature
    {
        std::cout << "3. Decrease key operation:" << std::endl;

        IndexedHeap<int, int> heap;
        heap.push(1, 100);
        heap.push(2, 50);
        heap.push(3, 75);

        std::cout << "   Initial top: key=" << heap.top().key << ", priority=" << heap.top().priority << std::endl;

        // Decrease priority of key 1 from 100 to 25
        heap.decrease_key(1, 25);
        std::cout << "   After decrease_key(1, 25): key=" << heap.top().key << ", priority=" << heap.top().priority
                  << std::endl;
        std::cout << std::endl;
    }

    // Example 4: Update priority (can increase or decrease)
    {
        std::cout << "4. Update priority (bidirectional):" << std::endl;

        IndexedHeap<int, int> heap;
        heap.push(1, 50);
        heap.push(2, 50);

        std::cout << "   Initial: both have priority 50" << std::endl;

        heap.update_priority(1, 10); // Decrease
        std::cout << "   After update_priority(1, 10): top key=" << heap.top().key << std::endl;

        heap.update_priority(1, 100); // Increase
        std::cout << "   After update_priority(1, 100): top key=" << heap.top().key << std::endl;
        std::cout << std::endl;
    }

    // Example 5: Contains and priority lookup
    {
        std::cout << "5. Contains and priority lookup:" << std::endl;

        IndexedHeap<String, int> heap;
        heap.push(String("alice"), 30);
        heap.push(String("bob"), 20);

        std::cout << "   Contains 'alice': " << (heap.contains(String("alice")) ? "yes" : "no") << std::endl;
        std::cout << "   Contains 'charlie': " << (heap.contains(String("charlie")) ? "yes" : "no") << std::endl;
        std::cout << "   Priority of 'alice': " << heap.priority(String("alice")) << std::endl;
        std::cout << std::endl;
    }

    // Example 6: Max heap with std::greater
    {
        std::cout << "6. Max heap (largest priority first):" << std::endl;

        MaxIndexedHeap<int, int> heap;
        heap.push(1, 10);
        heap.push(2, 30);
        heap.push(3, 20);

        std::cout << "   Pop order (max first): ";
        while (!heap.empty()) {
            auto entry = heap.pop();
            std::cout << entry.priority << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // Example 7: Dijkstra's shortest path algorithm
    {
        std::cout << "7. Dijkstra's Algorithm Example:" << std::endl;

        // Graph represented as adjacency list
        // Node 0 -> [(1, 4), (2, 1)]
        // Node 1 -> [(3, 1)]
        // Node 2 -> [(1, 2), (3, 5)]
        // Node 3 -> []

        constexpr int INF = std::numeric_limits<int>::max();
        constexpr int NUM_NODES = 4;

        // edges[from] = [(to, weight), ...]
        Vector<Vector<std::pair<int, int>>> edges;
        edges.resize(NUM_NODES);
        edges[0].push_back({1, 4});
        edges[0].push_back({2, 1});
        edges[1].push_back({3, 1});
        edges[2].push_back({1, 2});
        edges[2].push_back({3, 5});

        // Distance array
        Vector<int> dist;
        dist.resize(NUM_NODES);
        for (int i = 0; i < NUM_NODES; ++i) {
            dist[i] = INF;
        }

        // Priority queue: node -> distance
        IndexedHeap<int, int> pq;

        // Start from node 0
        int source = 0;
        dist[source] = 0;
        pq.push(source, 0);

        // Initialize other nodes with INF
        for (int i = 1; i < NUM_NODES; ++i) {
            pq.push(i, INF);
        }

        std::cout << "   Graph:" << std::endl;
        std::cout << "     0 --4--> 1 --1--> 3" << std::endl;
        std::cout << "     |        ^        ^" << std::endl;
        std::cout << "     1        2        5" << std::endl;
        std::cout << "     v        |        |" << std::endl;
        std::cout << "     2 -------+--------+" << std::endl;
        std::cout << std::endl;

        // Dijkstra's algorithm
        while (!pq.empty()) {
            auto [u, d] = pq.pop();

            if (d > dist[u]) {
                continue; // Already processed with shorter distance
            }

            // Relax edges
            for (size_t i = 0; i < edges[u].size(); ++i) {
                int v = edges[u][i].first;
                int weight = edges[u][i].second;

                if (dist[u] + weight < dist[v]) {
                    dist[v] = dist[u] + weight;
                    if (pq.contains(v)) {
                        pq.update_priority(v, dist[v]);
                    }
                }
            }
        }

        std::cout << "   Shortest distances from node 0:" << std::endl;
        for (int i = 0; i < NUM_NODES; ++i) {
            std::cout << "     Node " << i << ": " << dist[i] << std::endl;
        }
        std::cout << std::endl;
    }

    // Example 8: Serialization
    {
        std::cout << "8. Serialization:" << std::endl;

        IndexedHeap<int, int> original;
        original.push(1, 30);
        original.push(2, 10);
        original.push(3, 20);

        std::cout << "   Original top: key=" << original.top().key << ", priority=" << original.top().priority
                  << std::endl;

        auto buffer = serialize(original);
        std::cout << "   Serialized to " << buffer.size() << " bytes" << std::endl;

        auto restored = deserialize<Mode::NONE, IndexedHeap<int, int>>(buffer);
        std::cout << "   Restored top: key=" << restored.top().key << ", priority=" << restored.top().priority
                  << std::endl;
        std::cout << std::endl;
    }

    // Example 9: Task scheduling with dynamic priorities
    {
        std::cout << "9. Task scheduling with dynamic priorities:" << std::endl;

        IndexedHeap<String, int> scheduler;

        // Add tasks with initial priorities
        scheduler.push(String("compile"), 50);
        scheduler.push(String("test"), 30);
        scheduler.push(String("deploy"), 100);
        scheduler.push(String("backup"), 80);

        std::cout << "   Initial task order:" << std::endl;
        std::cout << "     Next task: " << scheduler.top().key.view() << " (priority=" << scheduler.top().priority
                  << ")" << std::endl;

        // Urgent: deploy needs to happen first!
        scheduler.decrease_key(String("deploy"), 10);
        std::cout << "   After making 'deploy' urgent (priority=10):" << std::endl;
        std::cout << "     Next task: " << scheduler.top().key.view() << " (priority=" << scheduler.top().priority
                  << ")" << std::endl;

        // Process tasks
        std::cout << "   Processing all tasks:" << std::endl;
        while (!scheduler.empty()) {
            auto [task, priority] = scheduler.pop();
            std::cout << "     Executing: " << task.view() << std::endl;
        }
        std::cout << std::endl;
    }

    std::cout << "=== IndexedHeap Examples Complete ===" << std::endl;
    return 0;
}
