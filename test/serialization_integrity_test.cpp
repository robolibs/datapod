#include <doctest/doctest.h>

#include "bitcon/bitcon.hpp"

using namespace bitcon;

// Test structs
struct Point {
    int x;
    int y;
};

struct Data {
    Vector<int> values;
    String label;
};

// Test basic integrity checking with scalars
TEST_CASE("integrity - scalar int") {
    int val = 42;
    auto buf = serialize<Mode::WITH_INTEGRITY>(val);

    // Should deserialize successfully with valid checksum
    auto result = deserialize<Mode::WITH_INTEGRITY, int>(buf);
    CHECK(result == 42);
}

TEST_CASE("integrity - scalar double") {
    double val = 3.14159;
    auto buf = serialize<Mode::WITH_INTEGRITY>(val);

    auto result = deserialize<Mode::WITH_INTEGRITY, double>(buf);
    CHECK(result == doctest::Approx(3.14159));
}

// Test integrity checking with struct
TEST_CASE("integrity - struct") {
    Point p{10, 20};
    auto buf = serialize<Mode::WITH_INTEGRITY>(p);

    auto result = deserialize<Mode::WITH_INTEGRITY, Point>(buf);
    CHECK(result.x == 10);
    CHECK(result.y == 20);
}

// Test integrity checking with containers
TEST_CASE("integrity - vector") {
    Vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    auto buf = serialize<Mode::WITH_INTEGRITY>(vec);
    auto result = deserialize<Mode::WITH_INTEGRITY, Vector<int>>(buf);

    REQUIRE(result.size() == 3);
    CHECK(result[0] == 1);
    CHECK(result[1] == 2);
    CHECK(result[2] == 3);
}

TEST_CASE("integrity - string") {
    String str = "Hello, Integrity!";
    auto buf = serialize<Mode::WITH_INTEGRITY>(str);
    auto result = deserialize<Mode::WITH_INTEGRITY, String>(buf);

    CHECK(result == "Hello, Integrity!");
}

// Test integrity checking with nested structures
TEST_CASE("integrity - nested struct") {
    Data data;
    data.values.push_back(10);
    data.values.push_back(20);
    data.label = String("test");

    auto buf = serialize<Mode::WITH_INTEGRITY>(data);
    auto result = deserialize<Mode::WITH_INTEGRITY, Data>(buf);

    REQUIRE(result.values.size() == 2);
    CHECK(result.values[0] == 10);
    CHECK(result.values[1] == 20);
    CHECK(result.label == "test");
}

// Test data corruption detection
TEST_CASE("integrity - detect single byte corruption") {
    Vector<int> vec;
    vec.push_back(100);
    vec.push_back(200);
    vec.push_back(300);

    auto buf = serialize<Mode::WITH_INTEGRITY>(vec);

    // Corrupt one byte in the data (skip checksum at beginning)
    buf[buf.size() / 2] ^= 0xFF;

    // Should fail integrity check
    bool caught_error = false;
    try {
        auto result = deserialize<Mode::WITH_INTEGRITY, Vector<int>>(buf);
        (void)result;
    } catch (...) {
        caught_error = true;
    }

    CHECK(caught_error == true);
}

TEST_CASE("integrity - detect multiple byte corruption") {
    String str = "This is a test string for corruption detection";
    auto buf = serialize<Mode::WITH_INTEGRITY>(str);

    // Corrupt multiple bytes
    for (std::size_t i = 20; i < 30 && i < buf.size(); ++i) {
        buf[i] ^= 0xAA;
    }

    // Should fail integrity check
    bool caught_error = false;
    try {
        auto result = deserialize<Mode::WITH_INTEGRITY, String>(buf);
        (void)result;
    } catch (...) {
        caught_error = true;
    }

    CHECK(caught_error == true);
}

TEST_CASE("integrity - detect checksum tampering") {
    int val = 42;
    auto buf = serialize<Mode::WITH_INTEGRITY>(val);

    // Tamper with the checksum itself (first 8 bytes after alignment)
    buf[0] ^= 0xFF;

    // Should fail integrity check
    bool caught_error = false;
    try {
        auto result = deserialize<Mode::WITH_INTEGRITY, int>(buf);
        (void)result;
    } catch (...) {
        caught_error = true;
    }

    CHECK(caught_error == true);
}

// Test combined modes: WITH_INTEGRITY + WITH_VERSION
TEST_CASE("integrity - with version tracking") {
    Point p{100, 200};
    auto buf = serialize<Mode::WITH_INTEGRITY | Mode::WITH_VERSION>(p);

    auto result = deserialize<Mode::WITH_INTEGRITY | Mode::WITH_VERSION, Point>(buf);
    CHECK(result.x == 100);
    CHECK(result.y == 200);
}

TEST_CASE("integrity - with version and corruption") {
    Vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    auto buf = serialize<Mode::WITH_INTEGRITY | Mode::WITH_VERSION>(vec);

    // Corrupt data
    buf[buf.size() / 2] ^= 0xFF;

    // Should fail on integrity check (before version check)
    bool caught_error = false;
    try {
        auto result = deserialize<Mode::WITH_INTEGRITY | Mode::WITH_VERSION, Vector<int>>(buf);
        (void)result;
    } catch (...) {
        caught_error = true;
    }

    CHECK(caught_error == true);
}

// Test combined modes: WITH_INTEGRITY + SERIALIZE_BIG_ENDIAN
TEST_CASE("integrity - with big endian") {
    int val = 0x12345678;
    auto buf = serialize<Mode::WITH_INTEGRITY | Mode::SERIALIZE_BIG_ENDIAN>(val);
    auto result = deserialize<Mode::WITH_INTEGRITY | Mode::SERIALIZE_BIG_ENDIAN, int>(buf);

    CHECK(result == 0x12345678);
}

// Test all modes combined
TEST_CASE("integrity - all modes combined") {
    Data data;
    data.values.push_back(10);
    data.values.push_back(20);
    data.label = String("test");

    auto buf = serialize<Mode::WITH_INTEGRITY | Mode::WITH_VERSION | Mode::SERIALIZE_BIG_ENDIAN>(data);
    auto result = deserialize<Mode::WITH_INTEGRITY | Mode::WITH_VERSION | Mode::SERIALIZE_BIG_ENDIAN, Data>(buf);

    REQUIRE(result.values.size() == 2);
    CHECK(result.values[0] == 10);
    CHECK(result.values[1] == 20);
    CHECK(result.label == "test");
}

// Test that deserialize without integrity works on integrity-protected data (should fail)
TEST_CASE("integrity - deserialize without integrity on protected data") {
    int val = 42;
    auto buf = serialize<Mode::WITH_INTEGRITY>(val);

    // Deserialize without integrity check - will read checksum as data
    auto result = deserialize<Mode::NONE, int>(buf);

    // Result will be the checksum value, not the original data
    CHECK(result != 42);
}

// Test with HashMap
TEST_CASE("integrity - hashmap") {
    HashMap<int, String> map;
    map.insert({1, String("one")});
    map.insert({2, String("two")});

    auto buf = serialize<Mode::WITH_INTEGRITY>(map);
    auto result = deserialize<Mode::WITH_INTEGRITY, HashMap<int, String>>(buf);

    REQUIRE(result.size() == 2);
    CHECK(result[1] == "one");
    CHECK(result[2] == "two");
}

// Test with Tuple
TEST_CASE("integrity - tuple") {
    Tuple<int, String, double> tuple{42, String("test"), 3.14};
    auto buf = serialize<Mode::WITH_INTEGRITY>(tuple);
    auto result = deserialize<Mode::WITH_INTEGRITY, Tuple<int, String, double>>(buf);

    CHECK(get<0>(result) == 42);
    CHECK(get<1>(result) == "test");
    CHECK(get<2>(result) == doctest::Approx(3.14));
}

// Test with Variant
TEST_CASE("integrity - variant") {
    Variant<int, String> var = String("hello");
    auto buf = serialize<Mode::WITH_INTEGRITY>(var);
    auto result = deserialize<Mode::WITH_INTEGRITY, Variant<int, String>>(buf);

    REQUIRE(result.index() == 1);
    CHECK(get<String>(result) == "hello");
}

// Test with Optional
TEST_CASE("integrity - optional with value") {
    Optional<int> opt = 42;
    auto buf = serialize<Mode::WITH_INTEGRITY>(opt);
    auto result = deserialize<Mode::WITH_INTEGRITY, Optional<int>>(buf);

    REQUIRE(result.has_value());
    CHECK(*result == 42);
}

TEST_CASE("integrity - optional empty") {
    Optional<int> opt;
    auto buf = serialize<Mode::WITH_INTEGRITY>(opt);
    auto result = deserialize<Mode::WITH_INTEGRITY, Optional<int>>(buf);

    CHECK(!result.has_value());
}

// Test large data structures
TEST_CASE("integrity - large vector") {
    Vector<int> vec;
    for (int i = 0; i < 1000; ++i) {
        vec.push_back(i);
    }

    auto buf = serialize<Mode::WITH_INTEGRITY>(vec);
    auto result = deserialize<Mode::WITH_INTEGRITY, Vector<int>>(buf);

    REQUIRE(result.size() == 1000);
    CHECK(result[0] == 0);
    CHECK(result[500] == 500);
    CHECK(result[999] == 999);
}

// Test corruption in large data
TEST_CASE("integrity - detect corruption in large data") {
    Vector<int> vec;
    for (int i = 0; i < 1000; ++i) {
        vec.push_back(i);
    }

    auto buf = serialize<Mode::WITH_INTEGRITY>(vec);

    // Corrupt a byte near the end
    buf[buf.size() - 10] ^= 0x01;

    // Should fail integrity check
    bool caught_error = false;
    try {
        auto result = deserialize<Mode::WITH_INTEGRITY, Vector<int>>(buf);
        (void)result;
    } catch (...) {
        caught_error = true;
    }

    CHECK(caught_error == true);
}
