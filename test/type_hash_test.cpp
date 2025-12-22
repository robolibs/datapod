#include <doctest/doctest.h>

#include "datapod/datapod.hpp"

using namespace datapod;

// Test structs
struct SimpleStruct {
    int x;
    double y;
};

struct NestedStruct {
    SimpleStruct inner;
    int z;
};

struct WithPointer {
    int *ptr;
};

struct WithString {
    String str;
};

struct WithVector {
    Vector<int> vec;
};

struct WithOptional {
    Optional<int> opt;
};

// Test type_name extraction
TEST_CASE("type_name - primitives") {
    CHECK(type_str<int>() == "int");
    CHECK(type_str<double>() == "double");
    CHECK(type_str<char>() == "char");
    CHECK(type_str<bool>() == "bool");
}

TEST_CASE("type_name - pointers") {
    auto const name = type_str<int *>();
    // Should contain "int" and "*"
    CHECK(name.find("int") != std::string_view::npos);
}

TEST_CASE("type_name - containers") {
    // Just check they compile and return non-empty
    CHECK(!type_str<Vector<int>>().empty());
    CHECK(!type_str<String>().empty());
    CHECK(!type_str<Optional<int>>().empty());
}

TEST_CASE("canonical_type_str - removes anonymous namespace") {
    // Test that canonical_type_str removes anonymous namespace markers
    auto const name = canonical_type_str<SimpleStruct>();
    CHECK(name.find("{anonymous}") == std::string::npos);
    CHECK(name.find("(anonymous namespace)") == std::string::npos);
    CHECK(name.find("`anonymous-namespace'") == std::string::npos);
}

// Test type_hash uniqueness
TEST_CASE("type_hash - different types have different hashes") {
    auto const h1 = type_hash<int>();
    auto const h2 = type_hash<double>();
    auto const h3 = type_hash<char>();

    CHECK(h1 != h2);
    CHECK(h2 != h3);
    CHECK(h1 != h3);
}

TEST_CASE("type_hash - same type has same hash") {
    auto const h1 = type_hash<int>();
    auto const h2 = type_hash<int>();

    CHECK(h1 == h2);
}

TEST_CASE("type_hash - pointers") {
    auto const h1 = type_hash<int *>();
    auto const h2 = type_hash<double *>();
    auto const h3 = type_hash<void *>();

    CHECK(h1 != h2);
    CHECK(h2 != h3);
    CHECK(h1 != h3);
}

TEST_CASE("type_hash - structs") {
    auto const h1 = type_hash<SimpleStruct>();
    auto const h2 = type_hash<NestedStruct>();
    auto const h3 = type_hash<WithPointer>();

    CHECK(h1 != h2);
    CHECK(h2 != h3);
    CHECK(h1 != h3);
}

TEST_CASE("type_hash - containers") {
    auto const h_vec_int = type_hash<Vector<int>>();
    auto const h_vec_double = type_hash<Vector<double>>();
    auto const h_str = type_hash<String>();

    CHECK(h_vec_int != h_vec_double);
    CHECK(h_vec_int != h_str);
    CHECK(h_vec_double != h_str);
}

TEST_CASE("type_hash - array") {
    auto const h1 = type_hash<Array<int, 10>>();
    auto const h2 = type_hash<Array<int, 20>>();
    auto const h3 = type_hash<Array<double, 10>>();

    // Different sizes should have different hashes
    CHECK(h1 != h2);
    // Different element types should have different hashes
    CHECK(h1 != h3);
}

TEST_CASE("type_hash - pair") {
    auto const h1 = type_hash<Pair<int, double>>();
    auto const h2 = type_hash<Pair<double, int>>();
    auto const h3 = type_hash<Pair<int, int>>();

    // Different types should have different hashes
    CHECK(h1 != h2);
    CHECK(h1 != h3);
    CHECK(h2 != h3);
}

TEST_CASE("type_hash - optional") {
    auto const h1 = type_hash<Optional<int>>();
    auto const h2 = type_hash<Optional<double>>();
    auto const h3 = type_hash<int>();

    CHECK(h1 != h2);
    CHECK(h1 != h3);
}

TEST_CASE("type_hash - variant") {
    auto const h1 = type_hash<Variant<int, double>>();
    auto const h2 = type_hash<Variant<double, int>>();
    auto const h3 = type_hash<Variant<int, double, char>>();

    // Order matters in variant
    CHECK(h1 != h2);
    // Different number of types
    CHECK(h1 != h3);
}

TEST_CASE("type_hash - tuple") {
    auto const h1 = type_hash<Tuple<int, double>>();
    auto const h2 = type_hash<Tuple<double, int>>();
    auto const h3 = type_hash<Tuple<int, double, char>>();

    // Order matters in tuple
    CHECK(h1 != h2);
    // Different number of types
    CHECK(h1 != h3);
}

TEST_CASE("type_hash - unique_ptr") {
    auto const h1 = type_hash<UniquePtr<int>>();
    auto const h2 = type_hash<UniquePtr<double>>();
    auto const h3 = type_hash<int>();

    CHECK(h1 != h2);
    CHECK(h1 != h3);
}

TEST_CASE("type_hash - strong typedef") {
    struct Tag1 {};
    struct Tag2 {};

    auto const h1 = type_hash<Strong<int, Tag1>>();
    auto const h2 = type_hash<Strong<int, Tag2>>();
    auto const h3 = type_hash<int>();

    // Same base type but different tags
    CHECK(h1 != h2);
    // Strong type vs base type
    CHECK(h1 != h3);
}

TEST_CASE("type_hash - hash_map and hash_set") {
    auto const h_map = type_hash<HashMap<int, double>>();
    auto const h_set = type_hash<HashSet<int>>();

    CHECK(h_map != h_set);
}

TEST_CASE("type_hash - stability across calls") {
    // Call type_hash multiple times and ensure it's stable
    for (int i = 0; i < 10; ++i) {
        CHECK(type_hash<int>() == type_hash<int>());
        CHECK(type_hash<SimpleStruct>() == type_hash<SimpleStruct>());
        CHECK(type_hash<Vector<int>>() == type_hash<Vector<int>>());
    }
}

TEST_CASE("type_hash - nested containers") {
    auto const h1 = type_hash<Vector<Vector<int>>>();
    auto const h2 = type_hash<Vector<int>>();
    auto const h3 = type_hash<Vector<Vector<double>>>();

    CHECK(h1 != h2);
    CHECK(h1 != h3);
}

TEST_CASE("type_hash - complex nested struct") {
    auto const h1 = type_hash<NestedStruct>();
    auto const h2 = type_hash<SimpleStruct>();

    // Nested struct should have different hash than inner struct
    CHECK(h1 != h2);
}

TEST_CASE("type_hash - integrals of different sizes") {
    auto const h1 = type_hash<int8_t>();
    auto const h2 = type_hash<int16_t>();
    auto const h3 = type_hash<int32_t>();
    auto const h4 = type_hash<int64_t>();

    // Different sizes should yield different hashes
    CHECK(h1 != h2);
    CHECK(h2 != h3);
    CHECK(h3 != h4);
}
