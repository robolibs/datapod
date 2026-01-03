#include "datapod/datapod.hpp"
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== OrderedSet Usage Examples ===" << std::endl << std::endl;

    // Example 1: Basic operations - elements are always sorted
    {
        std::cout << "1. Basic OrderedSet operations (elements always sorted):" << std::endl;

        OrderedSet<int> set;
        set.insert(5);
        set.insert(3);
        set.insert(7);
        set.insert(1);
        set.insert(9);

        std::cout << "   Inserted in order: 5, 3, 7, 1, 9" << std::endl;
        std::cout << "   Iteration order (sorted): ";
        for (int const &val : set) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
        std::cout << "   Size: " << set.size() << std::endl;
        std::cout << std::endl;
    }

    // Example 2: Duplicate handling
    {
        std::cout << "2. Duplicate handling (unique elements only):" << std::endl;

        OrderedSet<int> set{1, 2, 3, 2, 1, 4, 3};
        std::cout << "   Inserted: 1, 2, 3, 2, 1, 4, 3" << std::endl;
        std::cout << "   Stored (unique): ";
        for (int const &val : set) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
        std::cout << "   Size: " << set.size() << std::endl;
        std::cout << std::endl;
    }

    // Example 3: Range queries - unique to ordered containers
    {
        std::cout << "3. Range queries (lower_bound, upper_bound):" << std::endl;

        OrderedSet<int> scores{10, 25, 50, 75, 100, 150, 200};

        // Find first element >= 40
        auto it = scores.lower_bound(40);
        if (it != scores.end()) {
            std::cout << "   First score >= 40: " << *it << std::endl;
        }

        // Find first element > 100
        it = scores.upper_bound(100);
        if (it != scores.end()) {
            std::cout << "   First score > 100: " << *it << std::endl;
        }

        // Find all scores in range [50, 150]
        std::cout << "   Scores in range [50, 150]: ";
        for (auto iter = scores.lower_bound(50); iter != scores.end() && *iter <= 150; ++iter) {
            std::cout << *iter << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // Example 4: Min/Max access
    {
        std::cout << "4. Min/Max access:" << std::endl;

        OrderedSet<String> names;
        names.insert(String("Charlie"));
        names.insert(String("Alice"));
        names.insert(String("Bob"));
        names.insert(String("Diana"));

        if (!names.empty()) {
            std::cout << "   Min (first alphabetically): " << names.min().view() << std::endl;
            std::cout << "   Max (last alphabetically): " << names.max().view() << std::endl;
        }
        std::cout << std::endl;
    }

    // Example 5: Insert and contains
    {
        std::cout << "5. Insert and contains operations:" << std::endl;

        OrderedSet<int> set;

        auto [it1, inserted1] = set.insert(42);
        std::cout << "   Insert 42: " << (inserted1 ? "success" : "already exists") << std::endl;

        auto [it2, inserted2] = set.insert(42);
        std::cout << "   Insert 42 again: " << (inserted2 ? "success" : "already exists") << std::endl;

        std::cout << "   Contains 42: " << (set.contains(42) ? "yes" : "no") << std::endl;
        std::cout << "   Contains 99: " << (set.contains(99) ? "yes" : "no") << std::endl;
        std::cout << std::endl;
    }

    // Example 6: Erase operations
    {
        std::cout << "6. Erase operations:" << std::endl;

        OrderedSet<int> set{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        std::cout << "   Initial: ";
        for (int const &val : set) {
            std::cout << val << " ";
        }
        std::cout << std::endl;

        // Erase by value
        size_t erased = set.erase(5);
        std::cout << "   Erased 5: " << erased << " element(s)" << std::endl;

        // Erase by iterator
        auto it = set.find(3);
        if (it != set.end()) {
            set.erase(it);
            std::cout << "   Erased 3 via iterator" << std::endl;
        }

        std::cout << "   After erasing: ";
        for (int const &val : set) {
            std::cout << val << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // Example 7: Bidirectional iteration
    {
        std::cout << "7. Bidirectional iteration (forward and reverse):" << std::endl;

        OrderedSet<int> set{1, 2, 3, 4, 5};

        std::cout << "   Forward: ";
        for (auto it = set.begin(); it != set.end(); ++it) {
            std::cout << *it << " ";
        }
        std::cout << std::endl;

        std::cout << "   Reverse: ";
        for (auto it = set.rbegin(); it != set.rend(); ++it) {
            std::cout << *it << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // Example 8: Custom comparator - descending order
    {
        std::cout << "8. Custom comparator (descending order):" << std::endl;

        OrderedSet<int, std::greater<int>> set{5, 3, 7, 1, 9};
        std::cout << "   Inserted: 5, 3, 7, 1, 9" << std::endl;
        std::cout << "   Iteration (descending): ";
        for (int const &val : set) {
            std::cout << val << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // Example 9: Serialization
    {
        std::cout << "9. Serialization:" << std::endl;

        OrderedSet<int> original{5, 3, 7, 1, 9, 2, 8, 4, 6};

        std::cout << "   Original set: ";
        for (int const &val : original) {
            std::cout << val << " ";
        }
        std::cout << std::endl;

        // Serialize
        auto buffer = serialize(original);
        std::cout << "   Serialized to " << buffer.size() << " bytes" << std::endl;

        // Deserialize
        auto restored = deserialize<Mode::NONE, OrderedSet<int>>(buffer);
        std::cout << "   Restored set: ";
        for (int const &val : restored) {
            std::cout << val << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // Example 10: Use case - Priority scheduling
    {
        std::cout << "10. Use case - Priority scheduling:" << std::endl;

        // Tasks with priorities (lower = higher priority)
        OrderedSet<int> task_priorities{50, 10, 30, 20, 40};

        std::cout << "   Task priorities (sorted): ";
        for (int const &priority : task_priorities) {
            std::cout << priority << " ";
        }
        std::cout << std::endl;

        // Get highest priority (lowest number)
        std::cout << "   Highest priority task: " << task_priorities.min() << std::endl;

        // Find all high-priority tasks (priority <= 25)
        std::cout << "   High priority tasks (<=25): ";
        for (auto it = task_priorities.begin(); it != task_priorities.end() && *it <= 25; ++it) {
            std::cout << *it << " ";
        }
        std::cout << std::endl << std::endl;
    }

    std::cout << "=== OrderedSet Examples Complete ===" << std::endl;
    return 0;
}
