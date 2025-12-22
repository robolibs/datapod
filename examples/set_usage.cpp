#include "datapod/datapod.hpp"
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== Set Usage Examples ===" << std::endl << std::endl;

    // Example 1: Basic operations
    {
        std::cout << "1. Basic Set operations:" << std::endl;

        Set<int> unique_ids;
        unique_ids.insert(10);
        unique_ids.insert(20);
        unique_ids.insert(30);
        unique_ids.insert(20); // Duplicate, won't be inserted

        std::cout << "   Size: " << unique_ids.size() << std::endl;
        std::cout << "   Contains 20: " << (unique_ids.contains(20) ? "yes" : "no") << std::endl;
        std::cout << "   Contains 99: " << (unique_ids.contains(99) ? "yes" : "no") << std::endl;
        std::cout << std::endl;
    }

    // Example 2: Initializer list and iteration
    {
        std::cout << "2. Initializer list and iteration:" << std::endl;

        Set<String> tags{String("cpp"), String("rust"), String("python")};

        std::cout << "   Tags: ";
        for (auto const &tag : tags) {
            std::cout << tag.view() << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // Example 3: Insert and erase
    {
        std::cout << "3. Insert and erase operations:" << std::endl;

        Set<int> numbers{1, 2, 3, 4, 5};
        std::cout << "   Initial size: " << numbers.size() << std::endl;

        numbers.erase(3);
        std::cout << "   After erasing 3, size: " << numbers.size() << std::endl;

        auto [it, inserted] = numbers.insert(10);
        std::cout << "   Inserted 10: " << (inserted ? "yes" : "no") << std::endl;

        auto [it2, inserted2] = numbers.insert(10);
        std::cout << "   Inserted 10 again: " << (inserted2 ? "yes" : "no") << std::endl;
        std::cout << std::endl;
    }

    // Example 4: Serialization
    {
        std::cout << "4. Serialization:" << std::endl;

        Set<String> original{String("apple"), String("banana"), String("cherry")};

        std::cout << "   Original set size: " << original.size() << std::endl;

        // Serialize
        auto buffer = serialize(original);
        std::cout << "   Serialized to " << buffer.size() << " bytes" << std::endl;

        // Deserialize
        auto restored = deserialize<Mode::NONE, Set<String>>(buffer);
        std::cout << "   Restored set size: " << restored.size() << std::endl;
        std::cout << "   Contains 'banana': " << (restored.contains(String("banana")) ? "yes" : "no") << std::endl;
        std::cout << std::endl;
    }

    std::cout << "=== Set Examples Complete ===" << std::endl;
    return 0;
}
