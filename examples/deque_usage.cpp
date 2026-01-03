#include <datapod/datapod.hpp>
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== Deque (Double-Ended Queue) Usage Example ===\n\n";

    // Basic usage
    std::cout << "1. Basic Operations:\n";
    Deque<int> deque;
    deque.push_back(3);
    deque.push_front(2);
    deque.push_back(4);
    deque.push_front(1);
    deque.push_back(5);

    std::cout << "   After push_back(3,4,5) and push_front(2,1):\n   ";
    for (int n : deque) {
        std::cout << n << " ";
    }
    std::cout << "\n   Front: " << deque.front() << ", Back: " << deque.back() << "\n\n";

    // Random access
    std::cout << "2. Random Access:\n";
    Deque<int> deque2{10, 20, 30, 40, 50};
    std::cout << "   deque[0] = " << deque2[0] << "\n";
    std::cout << "   deque[2] = " << deque2[2] << "\n";
    std::cout << "   deque[4] = " << deque2[4] << "\n\n";

    // Pop from both ends
    std::cout << "3. Pop from Both Ends:\n";
    Deque<int> deque3{1, 2, 3, 4, 5};
    std::cout << "   Original: ";
    for (int n : deque3) {
        std::cout << n << " ";
    }
    std::cout << "\n";

    deque3.pop_front();
    deque3.pop_back();
    std::cout << "   After pop_front() and pop_back(): ";
    for (int n : deque3) {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // BFS (Breadth-First Search) example
    std::cout << "4. BFS Traversal:\n";
    std::cout << "   Tree structure:\n";
    std::cout << "        1\n";
    std::cout << "       / \\\n";
    std::cout << "      2   3\n";
    std::cout << "     / \\\n";
    std::cout << "    4   5\n\n";

    // Adjacency list representation
    Vector<Vector<int>> adj(6);
    adj[1] = {2, 3};
    adj[2] = {4, 5};

    Deque<int> bfs_queue;
    Vector<bool> visited(6, false);
    Vector<int> bfs_order;

    bfs_queue.push_back(1);
    visited[1] = true;

    while (!bfs_queue.empty()) {
        int node = bfs_queue.front();
        bfs_queue.pop_front();
        bfs_order.push_back(node);

        for (int neighbor : adj[node]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                bfs_queue.push_back(neighbor);
            }
        }
    }

    std::cout << "   BFS order: ";
    for (int n : bfs_order) {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // Sliding window maximum
    std::cout << "5. Sliding Window Maximum (k=3):\n";
    Vector<int> data{1, 3, -1, -3, 5, 3, 6, 7};
    size_t k = 3;

    std::cout << "   Data: ";
    for (int n : data) {
        std::cout << n << " ";
    }
    std::cout << "\n";

    Deque<size_t> window;
    Vector<int> maxes;

    for (size_t i = 0; i < data.size(); ++i) {
        // Remove elements outside window
        while (!window.empty() && window.front() + k <= i) {
            window.pop_front();
        }

        // Remove smaller elements
        while (!window.empty() && data[window.back()] < data[i]) {
            window.pop_back();
        }

        window.push_back(i);

        if (i >= k - 1) {
            maxes.push_back(data[window.front()]);
        }
    }

    std::cout << "   Window maxes: ";
    for (int m : maxes) {
        std::cout << m << " ";
    }
    std::cout << "\n\n";

    // Palindrome check using deque
    std::cout << "6. Palindrome Check:\n";
    auto is_palindrome = [](std::string const &s) {
        Deque<char> deque;
        for (char c : s) {
            if (std::isalpha(c)) {
                deque.push_back(std::tolower(c));
            }
        }

        while (deque.size() > 1) {
            if (deque.front() != deque.back()) {
                return false;
            }
            deque.pop_front();
            deque.pop_back();
        }
        return true;
    };

    std::cout << "   'racecar': " << (is_palindrome("racecar") ? "yes" : "no") << "\n";
    std::cout << "   'A man a plan a canal Panama': " << (is_palindrome("A man a plan a canal Panama") ? "yes" : "no")
              << "\n";
    std::cout << "   'hello': " << (is_palindrome("hello") ? "yes" : "no") << "\n\n";

    // Reverse iteration
    std::cout << "7. Reverse Iteration:\n";
    Deque<int> deque4{1, 2, 3, 4, 5};
    std::cout << "   Forward:  ";
    for (int n : deque4) {
        std::cout << n << " ";
    }
    std::cout << "\n   Backward: ";
    for (auto it = deque4.rbegin(); it != deque4.rend(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << "\n\n";

    // Serialization
    std::cout << "8. Serialization:\n";
    Deque<int> original;
    original.push_front(2);
    original.push_front(1);
    original.push_back(3);
    original.push_back(4);

    std::cout << "   Original: ";
    for (int n : original) {
        std::cout << n << " ";
    }
    std::cout << "\n";

    auto buf = serialize(original);
    std::cout << "   Serialized to " << buf.size() << " bytes\n";

    auto restored = deserialize<Mode::NONE, Deque<int>>(buf);
    std::cout << "   Restored: ";
    for (int n : restored) {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // Work-stealing queue simulation
    std::cout << "9. Work-Stealing Queue Simulation:\n";
    Deque<std::string> work_queue;

    // Owner pushes work to back
    work_queue.push_back("task1");
    work_queue.push_back("task2");
    work_queue.push_back("task3");
    work_queue.push_back("task4");

    std::cout << "   Owner added: task1, task2, task3, task4\n";

    // Owner pops from back (LIFO for locality)
    std::cout << "   Owner takes: " << work_queue.back() << " (from back)\n";
    work_queue.pop_back();

    // Thief steals from front (FIFO)
    std::cout << "   Thief steals: " << work_queue.front() << " (from front)\n";
    work_queue.pop_front();

    std::cout << "   Remaining: ";
    for (auto const &task : work_queue) {
        std::cout << task << " ";
    }
    std::cout << "\n";

    return 0;
}
