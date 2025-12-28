#include "datapod/datapod.hpp"
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== OrderedMap Usage Examples ===" << std::endl << std::endl;

    // Example 1: Basic operations - keys are always sorted
    {
        std::cout << "1. Basic OrderedMap operations (keys always sorted):" << std::endl;

        OrderedMap<int, String> users;
        users[3] = String("Charlie");
        users[1] = String("Alice");
        users[2] = String("Bob");

        std::cout << "   Inserted in order: 3, 1, 2" << std::endl;
        std::cout << "   Iteration order (sorted by key):" << std::endl;
        for (auto it = users.begin(); it != users.end(); ++it) {
            auto [key, value] = *it;
            std::cout << "     " << key << " -> " << value.view() << std::endl;
        }
        std::cout << "   Size: " << users.size() << std::endl;
        std::cout << std::endl;
    }

    // Example 2: Range queries - unique to ordered containers
    {
        std::cout << "2. Range queries (lower_bound, upper_bound):" << std::endl;

        OrderedMap<int, String> scores;
        scores[10] = String("bronze");
        scores[50] = String("silver");
        scores[100] = String("gold");
        scores[200] = String("platinum");
        scores[500] = String("diamond");

        // Find first element >= 75
        auto it = scores.lower_bound(75);
        if (it != scores.end()) {
            auto [key, value] = *it;
            std::cout << "   First score >= 75: " << key << " (" << value.view() << ")" << std::endl;
        }

        // Find first element > 100
        it = scores.upper_bound(100);
        if (it != scores.end()) {
            auto [key, value] = *it;
            std::cout << "   First score > 100: " << key << " (" << value.view() << ")" << std::endl;
        }

        // Count elements in range [50, 200]
        size_t count = 0;
        for (auto iter = scores.lower_bound(50); iter != scores.end(); ++iter) {
            auto [key, value] = *iter;
            if (key > 200)
                break;
            ++count;
        }
        std::cout << "   Elements in range [50, 200]: " << count << std::endl;
        std::cout << std::endl;
    }

    // Example 3: Min/Max key access
    {
        std::cout << "3. Min/Max key access:" << std::endl;

        OrderedMap<String, int> inventory;
        inventory[String("apple")] = 5;
        inventory[String("banana")] = 3;
        inventory[String("cherry")] = 8;
        inventory[String("date")] = 2;

        if (!inventory.empty()) {
            std::cout << "   Min key: " << inventory.min_key().view() << std::endl;
            std::cout << "   Max key: " << inventory.max_key().view() << std::endl;
        }
        std::cout << std::endl;
    }

    // Example 4: Insert and emplace
    {
        std::cout << "4. Insert and emplace operations:" << std::endl;

        OrderedMap<int, String> map;

        // Insert with key and value
        auto [it1, inserted1] = map.insert(1, String("one"));
        std::cout << "   Inserted key 1: " << (inserted1 ? "yes" : "no") << std::endl;

        // Duplicate insert fails
        auto [it2, inserted2] = map.insert(1, String("ONE"));
        std::cout << "   Insert duplicate key 1: " << (inserted2 ? "yes" : "no") << std::endl;

        // Emplace constructs in-place
        auto [it3, inserted3] = map.emplace(2, String("two"));
        std::cout << "   Emplaced key 2: " << (inserted3 ? "yes" : "no") << std::endl;

        // operator[] for insert or access
        map[1] = String("ONE");   // Updates existing
        map[3] = String("three"); // Inserts new
        std::cout << "   After operator[]: size=" << map.size() << std::endl;
        std::cout << std::endl;
    }

    // Example 5: Erase operations
    {
        std::cout << "5. Erase operations:" << std::endl;

        OrderedMap<int, String> map;
        for (int i = 1; i <= 5; ++i) {
            map[i] = String("value");
        }
        std::cout << "   Initial size: " << map.size() << std::endl;

        // Erase by key
        size_t erased = map.erase(3);
        std::cout << "   Erased key 3: " << erased << " element(s)" << std::endl;

        // Erase by iterator
        auto it = map.find(2);
        if (it != map.end()) {
            map.erase(it);
            std::cout << "   Erased key 2 via iterator" << std::endl;
        }

        std::cout << "   Final size: " << map.size() << std::endl;
        std::cout << "   Remaining keys: ";
        for (auto iter = map.begin(); iter != map.end(); ++iter) {
            auto [key, value] = *iter;
            std::cout << key << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // Example 6: Bidirectional iteration
    {
        std::cout << "6. Bidirectional iteration (forward and reverse):" << std::endl;

        OrderedMap<int, String> map;
        map[1] = String("first");
        map[2] = String("second");
        map[3] = String("third");

        std::cout << "   Forward: ";
        for (auto it = map.begin(); it != map.end(); ++it) {
            std::cout << it.key() << " ";
        }
        std::cout << std::endl;

        std::cout << "   Reverse: ";
        for (auto it = map.rbegin(); it != map.rend(); ++it) {
            auto [key, value] = *it;
            std::cout << key << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // Example 7: Serialization
    {
        std::cout << "7. Serialization:" << std::endl;

        OrderedMap<int, String> original;
        original[100] = String("hundred");
        original[50] = String("fifty");
        original[200] = String("two hundred");

        std::cout << "   Original map size: " << original.size() << std::endl;

        // Serialize
        auto buffer = serialize(original);
        std::cout << "   Serialized to " << buffer.size() << " bytes" << std::endl;

        // Deserialize
        auto restored = deserialize<Mode::NONE, OrderedMap<int, String>>(buffer);
        std::cout << "   Restored map size: " << restored.size() << std::endl;
        std::cout << "   Restored iteration order: ";
        for (auto it = restored.begin(); it != restored.end(); ++it) {
            std::cout << it.key() << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // Example 8: Use case - Event scheduling by timestamp
    {
        std::cout << "8. Use case - Event scheduling by timestamp:" << std::endl;

        OrderedMap<uint64_t, String> events;
        events[1000] = String("Start");
        events[1500] = String("Process A");
        events[1200] = String("Initialize");
        events[2000] = String("Finish");

        std::cout << "   Events in chronological order:" << std::endl;
        for (auto it = events.begin(); it != events.end(); ++it) {
            auto [timestamp, event] = *it;
            std::cout << "     t=" << timestamp << ": " << event.view() << std::endl;
        }

        // Find next event after timestamp 1100
        auto next = events.upper_bound(1100);
        if (next != events.end()) {
            auto [timestamp, event] = *next;
            std::cout << "   Next event after t=1100: " << event.view() << " at t=" << timestamp << std::endl;
        }
        std::cout << std::endl;
    }

    std::cout << "=== OrderedMap Examples Complete ===" << std::endl;
    return 0;
}
