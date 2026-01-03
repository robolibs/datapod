#include <datapod/datapod.hpp>
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== List (Doubly Linked List) Usage Example ===\n\n";

    // Basic usage
    std::cout << "1. Basic Operations:\n";
    List<int> numbers;
    numbers.push_back(2);
    numbers.push_back(3);
    numbers.push_front(1);
    numbers.push_back(4);

    std::cout << "   After push_back(2,3,4) and push_front(1):\n   ";
    for (int n : numbers) {
        std::cout << n << " ";
    }
    std::cout << "\n   Front: " << numbers.front() << ", Back: " << numbers.back() << "\n\n";

    // Initializer list construction
    std::cout << "2. Initializer List Construction:\n";
    List<std::string> words{"hello", "world", "from", "datapod"};
    std::cout << "   ";
    for (auto const &w : words) {
        std::cout << w << " ";
    }
    std::cout << "\n\n";

    // Insert in the middle
    std::cout << "3. Insert in Middle:\n";
    List<int> list{1, 3, 4};
    auto it = list.begin();
    ++it; // Point to 3
    list.insert(it, 2);
    std::cout << "   After inserting 2 before 3: ";
    for (int n : list) {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // Erase from middle
    std::cout << "4. Erase from Middle:\n";
    List<int> list2{1, 2, 3, 4, 5};
    auto it2 = list2.begin();
    ++it2;
    ++it2; // Point to 3
    list2.erase(it2);
    std::cout << "   After erasing 3: ";
    for (int n : list2) {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // Reverse iteration
    std::cout << "5. Reverse Iteration:\n";
    List<int> list3{1, 2, 3, 4, 5};
    std::cout << "   Forward:  ";
    for (int n : list3) {
        std::cout << n << " ";
    }
    std::cout << "\n   Backward: ";
    for (auto rit = list3.rbegin(); rit != list3.rend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << "\n\n";

    // Reverse the list
    std::cout << "6. Reverse List:\n";
    List<int> list4{1, 2, 3, 4, 5};
    std::cout << "   Before: ";
    for (int n : list4) {
        std::cout << n << " ";
    }
    list4.reverse();
    std::cout << "\n   After:  ";
    for (int n : list4) {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // Move to front (LRU-style)
    std::cout << "7. Move to Front (LRU-style):\n";
    List<int> list5{1, 2, 3, 4, 5};
    std::cout << "   Before: ";
    for (int n : list5) {
        std::cout << n << " ";
    }
    auto it5 = list5.begin();
    ++it5;
    ++it5; // Point to 3
    list5.move_to_front(it5);
    std::cout << "\n   After moving 3 to front: ";
    for (int n : list5) {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // LRU Cache simulation
    std::cout << "8. LRU Cache Simulation (capacity=3):\n";
    List<int> cache;
    size_t capacity = 3;

    auto access = [&](int value) {
        // Check if value exists
        for (auto it = cache.begin(); it != cache.end(); ++it) {
            if (*it == value) {
                cache.move_to_front(it);
                std::cout << "   Access " << value << " (hit):  ";
                for (int n : cache) {
                    std::cout << n << " ";
                }
                std::cout << "\n";
                return;
            }
        }
        // Not found, add to front
        if (cache.size() >= capacity) {
            std::cout << "   Access " << value << " (miss, evict " << cache.back() << "): ";
            cache.pop_back();
        } else {
            std::cout << "   Access " << value << " (miss): ";
        }
        cache.push_front(value);
        for (int n : cache) {
            std::cout << n << " ";
        }
        std::cout << "\n";
    };

    access(1);
    access(2);
    access(3);
    access(2); // Hit
    access(4); // Evict 1
    access(3); // Hit
    access(5); // Evict 2
    std::cout << "\n";

    // Serialization
    std::cout << "9. Serialization:\n";
    List<int> original{10, 20, 30, 40, 50};

    auto buf = serialize(original);
    std::cout << "   Serialized " << original.size() << " elements to " << buf.size() << " bytes\n";

    auto restored = deserialize<Mode::NONE, List<int>>(buf);
    std::cout << "   Restored: ";
    for (int n : restored) {
        std::cout << n << " ";
    }
    std::cout << "\n\n";

    // Bidirectional navigation
    std::cout << "10. Bidirectional Navigation:\n";
    List<int> list6{1, 2, 3, 4, 5};
    auto nav = list6.begin();
    std::cout << "   Start at: " << *nav << "\n";
    ++nav;
    std::cout << "   Forward:  " << *nav << "\n";
    ++nav;
    std::cout << "   Forward:  " << *nav << "\n";
    --nav;
    std::cout << "   Backward: " << *nav << "\n";
    --nav;
    std::cout << "   Backward: " << *nav << "\n";

    return 0;
}
