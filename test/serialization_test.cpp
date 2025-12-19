#include <doctest/doctest.h>

#include "datagram/datagram.hpp"

using namespace datagram;

// Test structs
struct Point {
    int x;
    int y;
};

struct Person {
    int age;
    float height;
};

struct Container {
    Vector<int> values;
    String name;
};

// Test scalar serialization
TEST_CASE("serialize - scalars") {
    int val = 42;
    auto buf = serialize(val);
    CHECK(buf.size() == sizeof(int));

    auto result = deserialize<Mode::NONE, int>(buf);
    CHECK(result == 42);
}

TEST_CASE("serialize - multiple scalars") {
    float val = 3.14f;
    auto buf = serialize(val);

    auto result = deserialize<Mode::NONE, float>(buf);
    CHECK(result == doctest::Approx(3.14f));
}

// Test struct serialization with reflection
TEST_CASE("serialize - simple struct") {
    Point p{10, 20};
    auto buf = serialize(p);

    auto result = deserialize<Mode::NONE, Point>(buf);
    CHECK(result.x == 10);
    CHECK(result.y == 20);
}

TEST_CASE("serialize - struct with float") {
    Person person{25, 1.75f};
    auto buf = serialize(person);

    auto result = deserialize<Mode::NONE, Person>(buf);
    CHECK(result.age == 25);
    CHECK(result.height == doctest::Approx(1.75f));
}

// Test Vector serialization
TEST_CASE("serialize - vector of ints") {
    Vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);
    vec.push_back(5);
    auto buf = serialize(vec);

    auto result = deserialize<Mode::NONE, Vector<int>>(buf);
    REQUIRE(result.size() == 5);
    CHECK(result[0] == 1);
    CHECK(result[1] == 2);
    CHECK(result[2] == 3);
    CHECK(result[3] == 4);
    CHECK(result[4] == 5);
}

TEST_CASE("serialize - empty vector") {
    Vector<int> vec;
    auto buf = serialize(vec);

    auto result = deserialize<Mode::NONE, Vector<int>>(buf);
    CHECK(result.size() == 0);
}

// Test String serialization
TEST_CASE("serialize - string") {
    String str = "Hello, World!";
    auto buf = serialize(str);

    auto result = deserialize<Mode::NONE, String>(buf);
    CHECK(result == "Hello, World!");
}

TEST_CASE("serialize - empty string") {
    String str = "";
    auto buf = serialize(str);

    auto result = deserialize<Mode::NONE, String>(buf);
    CHECK(result == "");
}

// Test Optional serialization
TEST_CASE("serialize - optional with value") {
    Optional<int> opt = 42;
    auto buf = serialize(opt);

    auto result = deserialize<Mode::NONE, Optional<int>>(buf);
    REQUIRE(result.has_value());
    CHECK(*result == 42);
}

TEST_CASE("serialize - optional without value") {
    Optional<int> opt;
    auto buf = serialize(opt);

    auto result = deserialize<Mode::NONE, Optional<int>>(buf);
    CHECK_FALSE(result.has_value());
}

// Test Pair serialization
TEST_CASE("serialize - pair") {
    Pair<int, float> p{42, 3.14f};
    auto buf = serialize(p);

    auto result = deserialize<Mode::NONE, Pair<int, float>>(buf);
    CHECK(result.first == 42);
    CHECK(result.second == doctest::Approx(3.14f));
}

// Test complex nested structures
TEST_CASE("serialize - struct with containers") {
    Container c;
    c.values.push_back(1);
    c.values.push_back(2);
    c.values.push_back(3);
    c.name = "test";

    auto buf = serialize(c);

    auto result = deserialize<Mode::NONE, Container>(buf);
    REQUIRE(result.values.size() == 3);
    CHECK(result.values[0] == 1);
    CHECK(result.values[1] == 2);
    CHECK(result.values[2] == 3);
    CHECK(result.name == "test");
}

// Test endian modes
TEST_CASE("serialize - big endian mode") {
    int val = 0x12345678;
    auto buf = serialize<Mode::SERIALIZE_BIG_ENDIAN>(val);

    auto result = deserialize<Mode::SERIALIZE_BIG_ENDIAN, int>(buf);
    CHECK(result == 0x12345678);
}

// Test round-trip with different types
TEST_CASE("serialize - vector of structs") {
    Vector<Point> points;
    points.push_back({1, 2});
    points.push_back({3, 4});
    points.push_back({5, 6});

    auto buf = serialize(points);

    auto result = deserialize<Mode::NONE, Vector<Point>>(buf);
    REQUIRE(result.size() == 3);
    CHECK(result[0].x == 1);
    CHECK(result[0].y == 2);
    CHECK(result[1].x == 3);
    CHECK(result[1].y == 4);
    CHECK(result[2].x == 5);
    CHECK(result[2].y == 6);
}
