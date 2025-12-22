#include "datapod/datapod.hpp"
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== Map Usage Examples ===" << std::endl << std::endl;

    // Example 1: Basic operations
    {
        std::cout << "1. Basic Map operations:" << std::endl;

        Map<int, String> users;
        users[1] = String("Alice");
        users[2] = String("Bob");
        users[3] = String("Charlie");

        std::cout << "   User 1: " << users[1].view() << std::endl;
        std::cout << "   User 2: " << users[2].view() << std::endl;
        std::cout << "   Size: " << users.size() << std::endl;
        std::cout << std::endl;
    }

    // Example 2: C++17 insert_or_assign and try_emplace
    {
        std::cout << "2. C++17 methods (insert_or_assign, try_emplace):" << std::endl;

        Map<String, int> scores;

        // insert_or_assign - inserts or updates
        auto [it1, inserted1] = scores.insert_or_assign(String("player1"), 100);
        std::cout << "   Inserted player1 with score " << it1->second << std::endl;

        auto [it2, inserted2] = scores.insert_or_assign(String("player1"), 150);
        std::cout << "   Updated player1 to score " << it2->second << " (inserted=" << inserted2 << ")" << std::endl;

        // try_emplace - only inserts if key doesn't exist
        auto [it3, inserted3] = scores.try_emplace(String("player2"), 200);
        std::cout << "   Emplaced player2 with score " << it3->second << std::endl;

        auto [it4, inserted4] = scores.try_emplace(String("player2"), 999);
        std::cout << "   Try emplace player2 failed (inserted=" << inserted4 << ")" << std::endl;
        std::cout << std::endl;
    }

    // Example 3: Find and contains
    {
        std::cout << "3. Lookup operations (find, contains):" << std::endl;

        Map<String, int> inventory{{String("apple"), 5}, {String("banana"), 3}, {String("orange"), 7}};

        if (inventory.contains(String("apple"))) {
            std::cout << "   Found apple: " << inventory[String("apple")] << " units" << std::endl;
        }

        auto it = inventory.find(String("grape"));
        if (it == inventory.end()) {
            std::cout << "   Grape not found" << std::endl;
        }
        std::cout << std::endl;
    }

    // Example 4: Serialization
    {
        std::cout << "4. Serialization:" << std::endl;

        Map<int, String> original{{1, String("first")}, {2, String("second")}, {3, String("third")}};

        std::cout << "   Original map size: " << original.size() << std::endl;

        // Serialize
        auto buffer = serialize(original);
        std::cout << "   Serialized to " << buffer.size() << " bytes" << std::endl;

        // Deserialize
        auto restored = deserialize<Mode::NONE, Map<int, String>>(buffer);
        std::cout << "   Restored map size: " << restored.size() << std::endl;
        std::cout << "   Restored[2]: " << restored[2].view() << std::endl;
        std::cout << std::endl;
    }

    std::cout << "=== Map Examples Complete ===" << std::endl;
    return 0;
}
