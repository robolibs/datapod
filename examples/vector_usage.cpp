#include "datapod/datapod.hpp"
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== Vector Usage Examples ===" << std::endl << std::endl;

    // Example 1: Basic operations
    {
        std::cout << "1. Basic Vector operations:" << std::endl;

        Vector<int> numbers;
        numbers.push_back(10);
        numbers.push_back(20);
        numbers.push_back(30);

        std::cout << "   Size: " << numbers.size() << std::endl;
        std::cout << "   First element: " << numbers[0] << std::endl;
        std::cout << "   Last element: " << numbers[numbers.size() - 1] << std::endl;
        std::cout << std::endl;
    }

    // Example 2: Initializer list and iteration
    {
        std::cout << "2. Initializer list and range-based for:" << std::endl;

        Vector<String> names{String("Alice"), String("Bob"), String("Charlie")};

        std::cout << "   Names: ";
        for (auto const &name : names) {
            std::cout << name.view() << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // Example 3: Emplace and resize
    {
        std::cout << "3. Emplace and resize:" << std::endl;

        Vector<int> data;
        data.emplace_back(100);
        data.emplace_back(200);

        std::cout << "   After emplace, size: " << data.size() << std::endl;

        data.resize(5);
        std::cout << "   After resize to 5, size: " << data.size() << std::endl;

        data[3] = 300;
        data[4] = 400;

        std::cout << "   Elements: ";
        for (auto val : data) {
            std::cout << val << " ";
        }
        std::cout << std::endl << std::endl;
    }

    // Example 4: Serialization
    {
        std::cout << "4. Serialization:" << std::endl;

        Vector<int> original{1, 2, 3, 4, 5};

        std::cout << "   Original vector size: " << original.size() << std::endl;

        // Serialize
        auto buffer = serialize(original);
        std::cout << "   Serialized to " << buffer.size() << " bytes" << std::endl;

        // Deserialize
        auto restored = deserialize<Mode::NONE, Vector<int>>(buffer);
        std::cout << "   Restored vector size: " << restored.size() << std::endl;
        std::cout << "   Restored[2]: " << restored[2] << std::endl;
        std::cout << std::endl;
    }

    std::cout << "=== Vector Examples Complete ===" << std::endl;
    return 0;
}
