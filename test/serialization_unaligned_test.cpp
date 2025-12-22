#include <doctest/doctest.h>

#include "datapod/datapod.hpp"

using namespace datapod;

// =============================================================================
// Test Structures
// =============================================================================

struct Point {
    int x;
    int y;
};

struct Data {
    int a;
    float b;
    double c;
};

// =============================================================================
// String::view() Tests
// =============================================================================

TEST_CASE("string - view() on short string") {
    String s("hello");
    auto v = s.view();

    CHECK(v.size() == 5);
    CHECK(v == "hello");
    CHECK(v.data() == s.data()); // Zero-copy
}

TEST_CASE("string - view() on heap string") {
    // Create a string longer than SSO size (23 chars)
    String s("this is a very long string that exceeds SSO limit");
    auto v = s.view();

    CHECK(v.size() == s.size());
    CHECK(v == "this is a very long string that exceeds SSO limit");
    CHECK(v.data() == s.data()); // Zero-copy
}

TEST_CASE("string - view() on empty string") {
    String s;
    auto v = s.view();

    CHECK(v.empty());
    CHECK(v.size() == 0);
}

TEST_CASE("string - view() round-trip") {
    String original("test string");
    auto v = original.view();
    String copy(v.data(), v.size());

    CHECK(copy == original);
}

// =============================================================================
// deserialize(std::string_view) Tests
// =============================================================================

TEST_CASE("serialize - deserialize from string_view") {
    int original = 42;
    auto buf = serialize(original);

    // Convert to string_view
    std::string_view view(reinterpret_cast<char const *>(buf.data()), buf.size());

    // Deserialize from string_view
    auto result = deserialize<Mode::NONE, int>(view);

    CHECK(result == 42);
}

TEST_CASE("serialize - deserialize struct from string_view") {
    Point original{10, 20};
    auto buf = serialize(original);

    std::string_view view(reinterpret_cast<char const *>(buf.data()), buf.size());
    auto result = deserialize<Mode::NONE, Point>(view);

    CHECK(result.x == 10);
    CHECK(result.y == 20);
}

TEST_CASE("serialize - deserialize String from string_view") {
    String original("hello, world!");
    auto buf = serialize(original);

    std::string_view view(reinterpret_cast<char const *>(buf.data()), buf.size());
    auto result = deserialize<Mode::NONE, String>(view);

    CHECK(result == "hello, world!");
}

// =============================================================================
// copy_from_potentially_unaligned() Tests
// =============================================================================

TEST_CASE("unaligned - deserialize aligned buffer (fast path)") {
    int original = 42;
    auto buf = serialize(original);

    // Buffer is aligned (vector allocation is aligned)
    std::string_view view(reinterpret_cast<char const *>(buf.data()), buf.size());
    auto result = copy_from_potentially_unaligned<Mode::NONE, int>(view);

    CHECK(result == 42);
}

TEST_CASE("unaligned - deserialize unaligned buffer offset +1") {
    int original = 12345;
    auto buf = serialize(original);

    // Create unaligned buffer by offsetting by 1 byte
    ByteBuf padded;
    padded.push_back(0xFF); // Padding byte
    padded.insert(padded.end(), buf.begin(), buf.end());

    // Create string_view starting at offset 1 (unaligned)
    std::string_view view(reinterpret_cast<char const *>(padded.data() + 1), buf.size());

    // This should handle unaligned access safely
    auto result = copy_from_potentially_unaligned<Mode::NONE, int>(view);

    CHECK(result == 12345);
}

TEST_CASE("unaligned - deserialize unaligned buffer offset +2") {
    double original = 3.14159;
    auto buf = serialize(original);

    // Create unaligned buffer by offsetting by 2 bytes
    ByteBuf padded;
    padded.push_back(0xAA);
    padded.push_back(0xBB);
    padded.insert(padded.end(), buf.begin(), buf.end());

    std::string_view view(reinterpret_cast<char const *>(padded.data() + 2), buf.size());

    auto result = copy_from_potentially_unaligned<Mode::NONE, double>(view);

    CHECK(result == doctest::Approx(3.14159));
}

TEST_CASE("unaligned - deserialize unaligned buffer offset +3") {
    float original = 2.718f;
    auto buf = serialize(original);

    // Create unaligned buffer by offsetting by 3 bytes
    ByteBuf padded;
    padded.push_back(0x11);
    padded.push_back(0x22);
    padded.push_back(0x33);
    padded.insert(padded.end(), buf.begin(), buf.end());

    std::string_view view(reinterpret_cast<char const *>(padded.data() + 3), buf.size());

    auto result = copy_from_potentially_unaligned<Mode::NONE, float>(view);

    CHECK(result == doctest::Approx(2.718f));
}

TEST_CASE("unaligned - deserialize struct from unaligned buffer") {
    Point original{100, 200};
    auto buf = serialize(original);

    // Create unaligned buffer
    ByteBuf padded;
    padded.push_back(0xDE);
    padded.push_back(0xAD);
    padded.push_back(0xBE);
    padded.insert(padded.end(), buf.begin(), buf.end());

    std::string_view view(reinterpret_cast<char const *>(padded.data() + 3), buf.size());

    auto result = copy_from_potentially_unaligned<Mode::NONE, Point>(view);

    CHECK(result.x == 100);
    CHECK(result.y == 200);
}

TEST_CASE("unaligned - deserialize struct with mixed types") {
    Data original{42, 3.14f, 2.718};
    auto buf = serialize(original);

    // Unaligned buffer (offset +1)
    ByteBuf padded;
    padded.push_back(0x00);
    padded.insert(padded.end(), buf.begin(), buf.end());

    std::string_view view(reinterpret_cast<char const *>(padded.data() + 1), buf.size());

    auto result = copy_from_potentially_unaligned<Mode::NONE, Data>(view);

    CHECK(result.a == 42);
    CHECK(result.b == doctest::Approx(3.14f));
    CHECK(result.c == doctest::Approx(2.718));
}

TEST_CASE("unaligned - deserialize Vector from unaligned buffer") {
    Vector<int> original;
    original.push_back(1);
    original.push_back(2);
    original.push_back(3);
    original.push_back(4);
    original.push_back(5);

    auto buf = serialize(original);

    // Unaligned buffer (offset +2)
    ByteBuf padded;
    padded.push_back(0xAA);
    padded.push_back(0xBB);
    padded.insert(padded.end(), buf.begin(), buf.end());

    std::string_view view(reinterpret_cast<char const *>(padded.data() + 2), buf.size());

    auto result = copy_from_potentially_unaligned<Mode::NONE, Vector<int>>(view);

    REQUIRE(result.size() == 5);
    CHECK(result[0] == 1);
    CHECK(result[1] == 2);
    CHECK(result[2] == 3);
    CHECK(result[3] == 4);
    CHECK(result[4] == 5);
}

TEST_CASE("unaligned - deserialize String from unaligned buffer") {
    String original("The quick brown fox jumps over the lazy dog");
    auto buf = serialize(original);

    // Unaligned buffer (offset +1)
    ByteBuf padded;
    padded.push_back(0xFF);
    padded.insert(padded.end(), buf.begin(), buf.end());

    std::string_view view(reinterpret_cast<char const *>(padded.data() + 1), buf.size());

    auto result = copy_from_potentially_unaligned<Mode::NONE, String>(view);

    CHECK(result == "The quick brown fox jumps over the lazy dog");
}

TEST_CASE("unaligned - deserialize Optional from unaligned buffer") {
    Optional<int> original(42);
    auto buf = serialize(original);

    // Unaligned buffer (offset +3)
    ByteBuf padded{0x11, 0x22, 0x33};
    padded.insert(padded.end(), buf.begin(), buf.end());

    std::string_view view(reinterpret_cast<char const *>(padded.data() + 3), buf.size());

    auto result = copy_from_potentially_unaligned<Mode::NONE, Optional<int>>(view);

    REQUIRE(result.has_value());
    CHECK(*result == 42);
}

TEST_CASE("unaligned - deserialize Pair from unaligned buffer") {
    Pair<int, float> original{42, 3.14f};
    auto buf = serialize(original);

    // Unaligned buffer (offset +1)
    ByteBuf padded{0xAA};
    padded.insert(padded.end(), buf.begin(), buf.end());

    std::string_view view(reinterpret_cast<char const *>(padded.data() + 1), buf.size());

    auto result = copy_from_potentially_unaligned<Mode::NONE, Pair<int, float>>(view);

    CHECK(result.first == 42);
    CHECK(result.second == doctest::Approx(3.14f));
}

TEST_CASE("unaligned - deserialize nested structure") {
    struct Nested {
        Point p;
        Vector<int> values;
        String name;
    };

    Nested original;
    original.p = Point{10, 20};
    original.values.push_back(1);
    original.values.push_back(2);
    original.values.push_back(3);
    original.name = String("test");

    auto buf = serialize(original);

    // Unaligned buffer (offset +2)
    ByteBuf padded{0xDE, 0xAD};
    padded.insert(padded.end(), buf.begin(), buf.end());

    std::string_view view(reinterpret_cast<char const *>(padded.data() + 2), buf.size());

    auto result = copy_from_potentially_unaligned<Mode::NONE, Nested>(view);

    CHECK(result.p.x == 10);
    CHECK(result.p.y == 20);
    REQUIRE(result.values.size() == 3);
    CHECK(result.values[0] == 1);
    CHECK(result.values[1] == 2);
    CHECK(result.values[2] == 3);
    CHECK(result.name == "test");
}

// =============================================================================
// uint8_t* and char* overload tests
// =============================================================================

TEST_CASE("unaligned - deserialize from uint8_t pointer") {
    int original = 999;
    auto buf = serialize(original);

    // Unaligned buffer
    ByteBuf padded{0xFF};
    padded.insert(padded.end(), buf.begin(), buf.end());

    auto result = copy_from_potentially_unaligned<Mode::NONE, int>(padded.data() + 1, buf.size());

    CHECK(result == 999);
}

TEST_CASE("unaligned - deserialize from char pointer") {
    float original = 1.23f;
    auto buf = serialize(original);

    // Unaligned buffer
    ByteBuf padded{0xAA, 0xBB};
    padded.insert(padded.end(), buf.begin(), buf.end());

    auto result = copy_from_potentially_unaligned<Mode::NONE, float>(reinterpret_cast<char const *>(padded.data() + 2),
                                                                     buf.size());

    CHECK(result == doctest::Approx(1.23f));
}

// =============================================================================
// Endian Mode Tests
// =============================================================================

TEST_CASE("unaligned - big endian with unaligned buffer") {
    int original = 0x12345678;
    auto buf = serialize<Mode::SERIALIZE_BIG_ENDIAN>(original);

    // Unaligned buffer
    ByteBuf padded{0x00};
    padded.insert(padded.end(), buf.begin(), buf.end());

    std::string_view view(reinterpret_cast<char const *>(padded.data() + 1), buf.size());

    auto result = copy_from_potentially_unaligned<Mode::SERIALIZE_BIG_ENDIAN, int>(view);

    CHECK(result == 0x12345678);
}
