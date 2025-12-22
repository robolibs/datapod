#include <doctest/doctest.h>

#include "bitcon/bitcon.hpp"

using namespace bitcon;

// Test structs for version tracking
struct Point {
    int x;
    int y;
};

struct Person {
    int age;
    String name;
};

struct Data {
    Vector<int> values;
    String label;
};

// Different struct with same name (for version mismatch testing)
struct Modified {
    int x;
    int y;
    int z; // Extra field - different schema
};

// Test basic version tracking with scalars
TEST_CASE("version - scalar int") {
    int val = 42;
    auto buf = serialize<Mode::WITH_VERSION>(val);

    // Should deserialize successfully with matching type
    auto result = deserialize<Mode::WITH_VERSION, int>(buf);
    CHECK(result == 42);
}

TEST_CASE("version - scalar float") {
    float val = 3.14f;
    auto buf = serialize<Mode::WITH_VERSION>(val);

    auto result = deserialize<Mode::WITH_VERSION, float>(buf);
    CHECK(result == doctest::Approx(3.14f));
}

// Test version tracking with struct
TEST_CASE("version - struct point") {
    Point p{10, 20};
    auto buf = serialize<Mode::WITH_VERSION>(p);

    auto result = deserialize<Mode::WITH_VERSION, Point>(buf);
    CHECK(result.x == 10);
    CHECK(result.y == 20);
}

TEST_CASE("version - struct person") {
    Person person{25, String("Alice")};
    auto buf = serialize<Mode::WITH_VERSION>(person);

    auto result = deserialize<Mode::WITH_VERSION, Person>(buf);
    CHECK(result.age == 25);
    CHECK(result.name == "Alice");
}

// Test version tracking with containers
TEST_CASE("version - vector") {
    Vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    auto buf = serialize<Mode::WITH_VERSION>(vec);
    auto result = deserialize<Mode::WITH_VERSION, Vector<int>>(buf);

    REQUIRE(result.size() == 3);
    CHECK(result[0] == 1);
    CHECK(result[1] == 2);
    CHECK(result[2] == 3);
}

TEST_CASE("version - string") {
    String str = "Hello, Version!";
    auto buf = serialize<Mode::WITH_VERSION>(str);
    auto result = deserialize<Mode::WITH_VERSION, String>(buf);

    CHECK(result == "Hello, Version!");
}

// Test version tracking with nested structures
TEST_CASE("version - nested struct") {
    Data data;
    data.values.push_back(10);
    data.values.push_back(20);
    data.label = String("test");

    auto buf = serialize<Mode::WITH_VERSION>(data);
    auto result = deserialize<Mode::WITH_VERSION, Data>(buf);

    REQUIRE(result.values.size() == 2);
    CHECK(result.values[0] == 10);
    CHECK(result.values[1] == 20);
    CHECK(result.label == "test");
}

// Test version mismatch detection - int vs float
TEST_CASE("version - mismatch int vs float") {
    int val = 42;
    auto buf = serialize<Mode::WITH_VERSION>(val);

    // Try to deserialize as float (different type hash)
    bool caught_error = false;
    try {
        auto result = deserialize<Mode::WITH_VERSION, float>(buf);
        (void)result; // Suppress unused warning
    } catch (...) {
        caught_error = true;
    }

    CHECK(caught_error == true);
}

// Test version mismatch detection - Point vs Modified
TEST_CASE("version - mismatch struct schemas") {
    Point p{10, 20};
    auto buf = serialize<Mode::WITH_VERSION>(p);

    // Try to deserialize as Modified (different struct)
    bool caught_error = false;
    try {
        auto result = deserialize<Mode::WITH_VERSION, Modified>(buf);
        (void)result; // Suppress unused warning
    } catch (...) {
        caught_error = true;
    }

    CHECK(caught_error == true);
}

// Test version tracking with Optional
TEST_CASE("version - optional with value") {
    Optional<int> opt = 42;
    auto buf = serialize<Mode::WITH_VERSION>(opt);
    auto result = deserialize<Mode::WITH_VERSION, Optional<int>>(buf);

    REQUIRE(result.has_value());
    CHECK(*result == 42);
}

TEST_CASE("version - optional empty") {
    Optional<int> opt;
    auto buf = serialize<Mode::WITH_VERSION>(opt);
    auto result = deserialize<Mode::WITH_VERSION, Optional<int>>(buf);

    CHECK(!result.has_value());
}

// Test version tracking with Pair
TEST_CASE("version - pair") {
    Pair<int, String> pair{42, String("answer")};
    auto buf = serialize<Mode::WITH_VERSION>(pair);
    auto result = deserialize<Mode::WITH_VERSION, Pair<int, String>>(buf);

    CHECK(result.first == 42);
    CHECK(result.second == "answer");
}

// Test version tracking with Tuple
TEST_CASE("version - tuple") {
    Tuple<int, float, String> tuple{42, 3.14f, String("test")};
    auto buf = serialize<Mode::WITH_VERSION>(tuple);
    auto result = deserialize<Mode::WITH_VERSION, Tuple<int, float, String>>(buf);

    CHECK(get<0>(result) == 42);
    CHECK(get<1>(result) == doctest::Approx(3.14f));
    CHECK(get<2>(result) == "test");
}

// Test version tracking with Variant
TEST_CASE("version - variant") {
    Variant<int, String> var = String("hello");
    auto buf = serialize<Mode::WITH_VERSION>(var);
    auto result = deserialize<Mode::WITH_VERSION, Variant<int, String>>(buf);

    REQUIRE(result.index() == 1);
    CHECK(get<String>(result) == "hello");
}

// Test version tracking with HashMap
TEST_CASE("version - hashmap") {
    HashMap<int, String> map;
    map.insert({1, String("one")});
    map.insert({2, String("two")});

    auto buf = serialize<Mode::WITH_VERSION>(map);
    auto result = deserialize<Mode::WITH_VERSION, HashMap<int, String>>(buf);

    REQUIRE(result.size() == 2);
    CHECK(result[1] == "one");
    CHECK(result[2] == "two");
}

// Test version tracking with HashSet
TEST_CASE("version - hashset") {
    HashSet<int> set;
    set.insert(10);
    set.insert(20);
    set.insert(30);

    auto buf = serialize<Mode::WITH_VERSION>(set);
    auto result = deserialize<Mode::WITH_VERSION, HashSet<int>>(buf);

    REQUIRE(result.size() == 3);
    CHECK(result.find(10) != result.end());
    CHECK(result.find(20) != result.end());
    CHECK(result.find(30) != result.end());
}

// Test combined modes: WITH_VERSION + SERIALIZE_BIG_ENDIAN
TEST_CASE("version - with big endian") {
    int val = 0x12345678;
    auto buf = serialize<Mode::WITH_VERSION | Mode::SERIALIZE_BIG_ENDIAN>(val);
    auto result = deserialize<Mode::WITH_VERSION | Mode::SERIALIZE_BIG_ENDIAN, int>(buf);

    CHECK(result == 0x12345678);
}

// Test that deserialization without version works on versioned data (should fail)
TEST_CASE("version - deserialize without version on versioned data") {
    int val = 42;
    auto buf = serialize<Mode::WITH_VERSION>(val);

    // Try to deserialize without version check - will read version hash as data
    auto result = deserialize<Mode::NONE, int>(buf);

    // Result will be the type hash, not the original value
    CHECK(result != 42);
}

// Test that versioned deserialization fails on non-versioned data
TEST_CASE("version - deserialize with version on non-versioned data") {
    int val = 42;
    auto buf = serialize<Mode::NONE>(val);

    // Try to deserialize with version check on non-versioned data
    bool caught_error = false;
    try {
        auto result = deserialize<Mode::WITH_VERSION, int>(buf);
        (void)result;
    } catch (...) {
        caught_error = true;
    }

    // Should fail because it reads the actual data as version hash
    CHECK(caught_error == true);
}
