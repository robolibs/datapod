#include <datapod/datapod.hpp>
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== Heap / PriorityQueue Usage Example ===\n\n";

    // Basic max-heap
    std::cout << "1. Max-Heap (default):\n";
    Heap<int> max_heap;
    max_heap.push(3);
    max_heap.push(1);
    max_heap.push(4);
    max_heap.push(1);
    max_heap.push(5);
    max_heap.push(9);

    std::cout << "   Pushed: 3, 1, 4, 1, 5, 9\n";
    std::cout << "   Pop order (largest first): ";
    while (!max_heap.empty()) {
        std::cout << max_heap.pop_top() << " ";
    }
    std::cout << "\n\n";

    // Min-heap
    std::cout << "2. Min-Heap:\n";
    MinHeap<int> min_heap{3, 1, 4, 1, 5, 9, 2, 6};
    std::cout << "   Elements: 3, 1, 4, 1, 5, 9, 2, 6\n";
    std::cout << "   Pop order (smallest first): ";
    while (!min_heap.empty()) {
        std::cout << min_heap.pop_top() << " ";
    }
    std::cout << "\n\n";

    // Priority queue alias
    std::cout << "3. PriorityQueue Alias:\n";
    PriorityQueue<int> pq;
    pq.push(10);
    pq.push(30);
    pq.push(20);
    std::cout << "   Top element: " << pq.top() << " (highest priority)\n\n";

    // Task scheduling example
    std::cout << "4. Task Scheduling Example:\n";
    struct Task {
        int priority;
        std::string name;

        // For max-heap: higher priority = higher value
        bool operator<(Task const &other) const { return priority < other.priority; }
    };

    Heap<Task> task_queue;
    task_queue.push({1, "Low priority task"});
    task_queue.push({5, "High priority task"});
    task_queue.push({3, "Medium priority task"});
    task_queue.push({5, "Another high priority"});
    task_queue.push({2, "Low-medium task"});

    std::cout << "   Processing tasks by priority:\n";
    while (!task_queue.empty()) {
        auto task = task_queue.pop_top();
        std::cout << "   [P" << task.priority << "] " << task.name << "\n";
    }
    std::cout << "\n";

    // Heap sort
    std::cout << "5. Heap Sort:\n";
    Vector<int> data{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    std::cout << "   Original: ";
    for (int n : data) {
        std::cout << n << " ";
    }
    std::cout << "\n";

    // Sort ascending using min-heap
    MinHeap<int> sort_heap(data.begin(), data.end());
    Vector<int> sorted;
    while (!sort_heap.empty()) {
        sorted.push_back(sort_heap.pop_top());
    }
    std::cout << "   Sorted:   ";
    for (int n : sorted) {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // Build heap from unsorted data (O(n))
    std::cout << "6. Build Heap from Unsorted Data:\n";
    Vector<int> unsorted{5, 3, 8, 1, 9, 2, 7};
    auto heap = Heap<int>::from_unsorted(std::move(unsorted));
    std::cout << "   Built heap, top element: " << heap.top() << "\n\n";

    // Custom comparator
    std::cout << "7. Custom Comparator (by absolute value):\n";
    auto abs_less = [](int a, int b) { return std::abs(a) < std::abs(b); };
    Heap<int, decltype(abs_less)> abs_heap(abs_less);
    abs_heap.push(3);
    abs_heap.push(-5);
    abs_heap.push(2);
    abs_heap.push(-4);
    abs_heap.push(1);

    std::cout << "   Elements: 3, -5, 2, -4, 1\n";
    std::cout << "   Pop order (largest absolute value first): ";
    while (!abs_heap.empty()) {
        std::cout << abs_heap.pop_top() << " ";
    }
    std::cout << "\n\n";

    // Serialization
    std::cout << "8. Serialization:\n";
    Heap<int> original{10, 20, 30, 40, 50};
    std::cout << "   Original top: " << original.top() << "\n";

    auto buf = serialize(original);
    std::cout << "   Serialized to " << buf.size() << " bytes\n";

    auto restored = deserialize<Mode::NONE, Heap<int>>(buf);
    std::cout << "   Restored top: " << restored.top() << "\n\n";

    // K largest elements
    std::cout << "9. Find K Largest Elements:\n";
    Vector<int> numbers{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 9, 7};
    size_t k = 5;

    // Use min-heap of size k
    MinHeap<int> k_heap;
    for (int n : numbers) {
        k_heap.push(n);
        if (k_heap.size() > k) {
            k_heap.pop(); // Remove smallest
        }
    }

    std::cout << "   Numbers: ";
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << "\n   Top " << k << " largest: ";
    while (!k_heap.empty()) {
        std::cout << k_heap.pop_top() << " ";
    }
    std::cout << "\n\n";

    // Merge K sorted lists (simplified)
    std::cout << "10. Merge Sorted Lists:\n";
    Vector<Vector<int>> lists = {{1, 4, 7}, {2, 5, 8}, {3, 6, 9}};

    struct ListItem {
        int value;
        size_t list_idx;
        size_t elem_idx;
        bool operator<(ListItem const &other) const {
            return value > other.value; // Min-heap behavior with std::less
        }
    };

    Heap<ListItem> merge_heap;
    for (size_t i = 0; i < lists.size(); ++i) {
        if (!lists[i].empty()) {
            merge_heap.push({lists[i][0], i, 0});
        }
    }

    std::cout << "   Lists: [1,4,7], [2,5,8], [3,6,9]\n";
    std::cout << "   Merged: ";
    while (!merge_heap.empty()) {
        auto item = merge_heap.pop_top();
        std::cout << item.value << " ";

        if (item.elem_idx + 1 < lists[item.list_idx].size()) {
            merge_heap.push({lists[item.list_idx][item.elem_idx + 1], item.list_idx, item.elem_idx + 1});
        }
    }
    std::cout << "\n";

    return 0;
}
