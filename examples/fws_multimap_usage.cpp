#include <cstdint>
#include <datapod/pods/associative/fws_multimap.hpp>
#include <iostream>
#include <string>

using namespace datapod;

void example_basic_usage() {
    std::cout << "=== Basic Usage ===" << std::endl;

    // Create a multimap: Key -> [Values]
    FwsMultimapVec<uint32_t, std::string> mm;

    // Key 0: languages
    mm.push_back("C++");
    mm.push_back("Python");
    mm.push_back("Rust");
    mm.finish_key();

    // Key 1: frameworks
    mm.push_back("Qt");
    mm.push_back("React");
    mm.finish_key();

    // Key 2: databases
    mm.push_back("PostgreSQL");
    mm.finish_key();

    // Must call finish_map() before reading
    mm.finish_map();

    std::cout << "Data size: " << mm.data_size() << std::endl;
    std::cout << "Index size: " << mm.index_size() << std::endl;

    // Access by key
    auto languages = mm[0];
    std::cout << "\nKey 0 (languages) has " << languages.size() << " values:" << std::endl;
    for (const auto &lang : languages) {
        std::cout << "  - " << lang << std::endl;
    }

    auto frameworks = mm[1];
    std::cout << "\nKey 1 (frameworks) has " << frameworks.size() << " values:" << std::endl;
    for (const auto &fw : frameworks) {
        std::cout << "  - " << fw << std::endl;
    }

    std::cout << std::endl;
}

void example_build_pattern() {
    std::cout << "=== Build Pattern ===" << std::endl;

    FwsMultimapVec<uint32_t, int> mm;

    std::cout << "Current key: " << mm.current_key() << std::endl;

    // Add values for key 0
    mm.push_back(10);
    mm.push_back(20);
    mm.finish_key();

    std::cout << "After first key, current key: " << mm.current_key() << std::endl;

    // Add values for key 1
    mm.push_back(30);
    mm.finish_key();

    std::cout << "After second key, current key: " << mm.current_key() << std::endl;

    mm.finish_map();
    std::cout << "Map finished: " << (mm.finished() ? "yes" : "no") << std::endl;

    std::cout << std::endl;
}

void example_empty_keys() {
    std::cout << "=== Empty Keys ===" << std::endl;

    FwsMultimapVec<uint32_t, int> mm;

    // Key 0: has values
    mm.push_back(100);
    mm.push_back(200);
    mm.finish_key();

    // Key 1: empty (no values)
    mm.finish_key();

    // Key 2: has values
    mm.push_back(300);
    mm.finish_key();

    // Key 3: empty
    mm.finish_key();

    mm.finish_map();

    std::cout << "Key 0: " << mm[0].size() << " values" << std::endl;
    std::cout << "Key 1: " << mm[1].size() << " values (empty: " << (mm[1].empty() ? "yes" : "no") << ")" << std::endl;
    std::cout << "Key 2: " << mm[2].size() << " values" << std::endl;
    std::cout << "Key 3: " << mm[3].size() << " values (empty: " << (mm[3].empty() ? "yes" : "no") << ")" << std::endl;

    std::cout << std::endl;
}

void example_iterating_entries() {
    std::cout << "=== Iterating Entries ===" << std::endl;

    FwsMultimapVec<uint32_t, int> mm;

    // Add multiple keys
    for (int key = 0; key < 3; ++key) {
        for (int val = 0; val < key + 1; ++val) {
            mm.push_back(key * 10 + val);
        }
        mm.finish_key();
    }
    mm.finish_map();

    std::cout << "Iterating over all entries:" << std::endl;
    int key_num = 0;
    for (auto entry : mm) {
        std::cout << "  Key " << key_num++ << ": [";
        bool first = true;
        for (int val : entry) {
            if (!first)
                std::cout << ", ";
            std::cout << val;
            first = false;
        }
        std::cout << "]" << std::endl;
    }

    std::cout << std::endl;
}

void example_entry_operations() {
    std::cout << "=== Entry Operations ===" << std::endl;

    FwsMultimapVec<uint32_t, int> mm;

    mm.push_back(100);
    mm.push_back(200);
    mm.push_back(300);
    mm.push_back(400);
    mm.finish_key();
    mm.finish_map();

    auto entry = mm[0];

    // Size and empty
    std::cout << "Entry size: " << entry.size() << std::endl;
    std::cout << "Entry empty: " << (entry.empty() ? "yes" : "no") << std::endl;

    // Index access
    std::cout << "Entry[0]: " << entry[0] << std::endl;
    std::cout << "Entry[2]: " << entry[2] << std::endl;

    // Iterator access
    std::cout << "Using iterators: ";
    for (auto it = entry.begin(); it != entry.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // Data index
    std::cout << "Data index of entry[1]: " << entry.data_index(1) << std::endl;

    std::cout << std::endl;
}

void example_emplace_back() {
    std::cout << "=== Emplace Back ===" << std::endl;

    struct Point {
        int x, y;
        Point(int x_, int y_) : x(x_), y(y_) {
            std::cout << "    Point(" << x_ << ", " << y_ << ") constructed" << std::endl;
        }
    };

    FwsMultimapVec<uint32_t, Point> mm;

    std::cout << "Emplacing points:" << std::endl;
    mm.emplace_back(10, 20);
    mm.emplace_back(30, 40);
    mm.finish_key();
    mm.finish_map();

    auto entry = mm[0];
    std::cout << "\nStored points:" << std::endl;
    for (const auto &p : entry) {
        std::cout << "  Point(" << p.x << ", " << p.y << ")" << std::endl;
    }

    std::cout << std::endl;
}

void example_reserve_index() {
    std::cout << "=== Reserve Index ===" << std::endl;

    FwsMultimapVec<uint32_t, int> mm;

    // Pre-allocate space for 1000 keys
    mm.reserve_index(1000);
    std::cout << "Reserved index for 1000 keys" << std::endl;

    // Add 5 keys
    for (int i = 0; i < 5; ++i) {
        mm.push_back(i * 100);
        mm.finish_key();
    }
    mm.finish_map();

    std::cout << "Added 5 keys, index_size: " << mm.index_size() << std::endl;

    std::cout << std::endl;
}

void example_iterator_arithmetic() {
    std::cout << "=== Iterator Arithmetic ===" << std::endl;

    FwsMultimapVec<uint32_t, int> mm;

    for (int i = 0; i < 5; ++i) {
        mm.push_back(i * 10);
        mm.finish_key();
    }
    mm.finish_map();

    auto it = mm.begin();
    std::cout << "it[0]: " << (*it)[0] << std::endl;

    auto it2 = it + 2;
    std::cout << "it+2: " << (*it2)[0] << std::endl;

    auto it3 = it2 - 1;
    std::cout << "it+2-1: " << (*it3)[0] << std::endl;

    std::cout << "Distance (it2 - it): " << (it2 - it) << std::endl;

    // Subscript operator
    std::cout << "it[3]: " << it[3][0] << std::endl;

    // Comparisons
    std::cout << "it < it2: " << (it < it2 ? "true" : "false") << std::endl;
    std::cout << "it == it: " << (it == it ? "true" : "false") << std::endl;

    std::cout << std::endl;
}

void example_sparse_data() {
    std::cout << "=== Sparse Data Example ===" << std::endl;

    // Simulating user-to-friends mapping
    FwsMultimapVec<uint32_t, uint32_t> user_friends;

    // User 0: has 3 friends
    user_friends.push_back(10);
    user_friends.push_back(20);
    user_friends.push_back(30);
    user_friends.finish_key();

    // User 1: no friends
    user_friends.finish_key();

    // User 2: has 1 friend
    user_friends.push_back(15);
    user_friends.finish_key();

    // User 3: has 5 friends
    for (uint32_t i = 100; i < 105; ++i) {
        user_friends.push_back(i);
    }
    user_friends.finish_key();

    user_friends.finish_map();

    std::cout << "User-Friends mapping:" << std::endl;
    for (uint32_t user = 0; user < 4; ++user) {
        auto friends = user_friends[user];
        std::cout << "  User " << user << " has " << friends.size() << " friends: [";
        bool first = true;
        for (auto friend_id : friends) {
            if (!first)
                std::cout << ", ";
            std::cout << friend_id;
            first = false;
        }
        std::cout << "]" << std::endl;
    }

    std::cout << std::endl;
}

void example_large_scale() {
    std::cout << "=== Large Scale Example ===" << std::endl;

    FwsMultimapVec<uint32_t, int> mm;
    mm.reserve_index(1000);

    // Create 100 keys with varying numbers of values
    for (int key = 0; key < 100; ++key) {
        int num_values = (key % 5) + 1; // 1 to 5 values per key
        for (int val = 0; val < num_values; ++val) {
            mm.push_back(key * 1000 + val);
        }
        mm.finish_key();
    }
    mm.finish_map();

    std::cout << "Created multimap with 100 keys" << std::endl;
    std::cout << "Total data size: " << mm.data_size() << std::endl;
    std::cout << "Index size: " << mm.index_size() << std::endl;

    // Sample a few entries
    std::cout << "\nSample entries:" << std::endl;
    for (int key : {0, 25, 50, 75, 99}) {
        auto entry = mm[key];
        std::cout << "  Key " << key << ": " << entry.size() << " values, first = " << entry[0] << std::endl;
    }

    std::cout << std::endl;
}

void example_const_access() {
    std::cout << "=== Const Access ===" << std::endl;

    FwsMultimapVec<uint32_t, int> mm;
    mm.push_back(100);
    mm.push_back(200);
    mm.finish_key();
    mm.finish_map();

    // Const reference
    const auto &const_mm = mm;

    auto entry = const_mm[0];
    std::cout << "Const access - entry size: " << entry.size() << std::endl;
    std::cout << "First value: " << entry[0] << std::endl;

    // Const iterators
    std::cout << "Using cbegin/cend: ";
    for (auto it = const_mm.cbegin(); it != const_mm.cend(); ++it) {
        auto e = *it;
        for (int val : e) {
            std::cout << val << " ";
        }
    }
    std::cout << std::endl;

    std::cout << std::endl;
}

void example_use_case_graph() {
    std::cout << "=== Use Case: Adjacency List (Graph) ===" << std::endl;

    // Directed graph represented as adjacency list
    FwsMultimapVec<uint32_t, uint32_t> graph;

    // Node 0 -> [1, 2, 3]
    graph.push_back(1);
    graph.push_back(2);
    graph.push_back(3);
    graph.finish_key();

    // Node 1 -> [3]
    graph.push_back(3);
    graph.finish_key();

    // Node 2 -> [0, 1]
    graph.push_back(0);
    graph.push_back(1);
    graph.finish_key();

    // Node 3 -> [] (no outgoing edges)
    graph.finish_key();

    graph.finish_map();

    std::cout << "Graph adjacency list:" << std::endl;
    for (uint32_t node = 0; node < 4; ++node) {
        auto neighbors = graph[node];
        std::cout << "  Node " << node << " -> [";
        bool first = true;
        for (auto neighbor : neighbors) {
            if (!first)
                std::cout << ", ";
            std::cout << neighbor;
            first = false;
        }
        std::cout << "]" << std::endl;
    }

    std::cout << std::endl;
}

int main() {
    std::cout << "DataPod FwsMultimap Usage Examples" << std::endl;
    std::cout << "==================================" << std::endl << std::endl;

    example_basic_usage();
    example_build_pattern();
    example_empty_keys();
    example_iterating_entries();
    example_entry_operations();
    example_emplace_back();
    example_reserve_index();
    example_iterator_arithmetic();
    example_sparse_data();
    example_large_scale();
    example_const_access();
    example_use_case_graph();

    std::cout << "All examples completed successfully!" << std::endl;

    return 0;
}
