#include <iostream>
#include "bitcon/bitcon.hpp"

using namespace bitcon;

#define TEST(name)                                                                                                     \
    void name();                                                                                                       \
    struct name##_runner {                                                                                             \
        name##_runner() {                                                                                              \
            std::cout << "Running " #name "..." << std::endl;                                                          \
            name();                                                                                                    \
            std::cout << "  ✓ " #name " passed" << std::endl;                                                          \
        }                                                                                                              \
    } name##_instance;                                                                                                 \
    void name()

#define ASSERT(cond)                                                                                                   \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::cerr << "Assertion failed: " #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl;            \
            std::exit(1);                                                                                              \
        }                                                                                                              \
    } while (0)

#define ASSERT_EQ(a, b)                                                                                                \
    do {                                                                                                               \
        if ((a) != (b)) {                                                                                              \
            std::cerr << "Assertion failed: " #a " == " #b << " at " << __FILE__ << ":" << __LINE__ << std::endl;     \
            std::exit(1);                                                                                              \
        }                                                                                                              \
    } while (0)

// ==================== HashMap Tests ====================

TEST(test_hashmap_empty) {
    HashMap<int, int> map;
    ASSERT_EQ(map.empty(), true);
    ASSERT_EQ(map.size(), 0);
}

TEST(test_hashmap_insert) {
    HashMap<int, int> map;
    auto res1 = map.insert({1, 100});
    ASSERT_EQ(res1.second, true); // inserted
    ASSERT_EQ(res1.first->first, 1);
    ASSERT_EQ(res1.first->second, 100);
    
    auto res2 = map.insert({1, 200});
    ASSERT_EQ(res2.second, false); // already exists
    ASSERT_EQ(map.size(), 1);
}

TEST(test_hashmap_operator_bracket) {
    HashMap<int, int> map;
    map[1] = 100;
    map[2] = 200;
    map[3] = 300;
    
    ASSERT_EQ(map.size(), 3);
    ASSERT_EQ(map[1], 100);
    ASSERT_EQ(map[2], 200);
    ASSERT_EQ(map[3], 300);
}

TEST(test_hashmap_find) {
    HashMap<int, int> map;
    map[1] = 100;
    map[2] = 200;
    
    auto it1 = map.find(1);
    ASSERT(it1 != map.end());
    ASSERT_EQ(it1->second, 100);
    
    auto it2 = map.find(99);
    ASSERT(it2 == map.end());
}

TEST(test_hashmap_erase) {
    HashMap<int, int> map;
    map[1] = 100;
    map[2] = 200;
    map[3] = 300;
    
    auto count = map.erase(2);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(map.size(), 2);
    ASSERT(map.find(2) == map.end());
}

TEST(test_hashmap_iteration) {
    HashMap<int, int> map;
    map[1] = 100;
    map[2] = 200;
    map[3] = 300;
    
    int sum = 0;
    for (auto const& kv : map) {
        sum += kv.second;
    }
    ASSERT_EQ(sum, 600);
}

TEST(test_hashmap_string_keys) {
    HashMap<BasicString<>, int> map;
    map[BasicString<>("hello")] = 1;
    map[BasicString<>("world")] = 2;
    
    ASSERT_EQ(map.size(), 2);
    ASSERT_EQ(map[BasicString<>("hello")], 1);
    ASSERT_EQ(map[BasicString<>("world")], 2);
}

// ==================== HashSet Tests ====================

TEST(test_hashset_empty) {
    HashSet<int> set;
    ASSERT_EQ(set.empty(), true);
    ASSERT_EQ(set.size(), 0);
}

TEST(test_hashset_insert) {
    HashSet<int> set;
    auto res1 = set.insert(1);
    ASSERT_EQ(res1.second, true); // inserted
    ASSERT_EQ(*res1.first, 1);
    
    auto res2 = set.insert(1);
    ASSERT_EQ(res2.second, false); // already exists
    ASSERT_EQ(set.size(), 1);
}

TEST(test_hashset_find) {
    HashSet<int> set;
    set.insert(1);
    set.insert(2);
    set.insert(3);
    
    auto it1 = set.find(2);
    ASSERT(it1 != set.end());
    ASSERT_EQ(*it1, 2);
    
    auto it2 = set.find(99);
    ASSERT(it2 == set.end());
}

TEST(test_hashset_erase) {
    HashSet<int> set;
    set.insert(1);
    set.insert(2);
    set.insert(3);
    
    auto count = set.erase(2);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(set.size(), 2);
    ASSERT(set.find(2) == set.end());
}

TEST(test_hashset_iteration) {
    HashSet<int> set;
    set.insert(1);
    set.insert(2);
    set.insert(3);
    
    int sum = 0;
    for (auto val : set) {
        sum += val;
    }
    ASSERT_EQ(sum, 6);
}

TEST(test_hashset_string) {
    HashSet<BasicString<>> set;
    set.insert("hello");
    set.insert("world");
    set.insert("hello"); // duplicate
    
    ASSERT_EQ(set.size(), 2);
    ASSERT(set.find("hello") != set.end());
    ASSERT(set.find("world") != set.end());
}

int main() {
    std::cout << "\n=== Phase 6 Hash Container Tests ===" << std::endl;
    std::cout << "\nAll tests passed! ✓\n" << std::endl;
    return 0;
}
