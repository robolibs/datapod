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
    auto const h_map = type_hash<Map<int, double>>();
    auto const h_set = type_hash<Set<int>>();

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

// ============================================================================
// Primitive Type ID Tests
// ============================================================================

TEST_CASE("primitive_type_id - all primitives have IDs") {
    // Signed integers
    CHECK(has_primitive_type_id_v<i8>);
    CHECK(has_primitive_type_id_v<i16>);
    CHECK(has_primitive_type_id_v<i32>);
    CHECK(has_primitive_type_id_v<i64>);

    // Unsigned integers
    CHECK(has_primitive_type_id_v<u8>);
    CHECK(has_primitive_type_id_v<u16>);
    CHECK(has_primitive_type_id_v<u32>);
    CHECK(has_primitive_type_id_v<u64>);

    // Size types
    CHECK(has_primitive_type_id_v<usize>);
    CHECK(has_primitive_type_id_v<isize>);

    // Floating point
    CHECK(has_primitive_type_id_v<f32>);
    CHECK(has_primitive_type_id_v<f64>);

    // Characters
    CHECK(has_primitive_type_id_v<char8>);
    CHECK(has_primitive_type_id_v<char16>);
    CHECK(has_primitive_type_id_v<char32>);

    // Boolean
    CHECK(has_primitive_type_id_v<boolean>);
}

TEST_CASE("primitive_type_id - signed vs unsigned have different IDs") {
    CHECK(primitive_type_id<i8>::id != primitive_type_id<u8>::id);
    CHECK(primitive_type_id<i16>::id != primitive_type_id<u16>::id);
    CHECK(primitive_type_id<i32>::id != primitive_type_id<u32>::id);
    CHECK(primitive_type_id<i64>::id != primitive_type_id<u64>::id);
}

TEST_CASE("type_hash - signed vs unsigned have different hashes") {
    // This was a bug before: signed and unsigned of same size had same hash
    CHECK(type_hash<i8>() != type_hash<u8>());
    CHECK(type_hash<i16>() != type_hash<u16>());
    CHECK(type_hash<i32>() != type_hash<u32>());
    CHECK(type_hash<i64>() != type_hash<u64>());
}

TEST_CASE("type_hash - all datapod primitives have unique hashes") {
    Set<hash_t> hashes;

    // Collect all primitive hashes
    hashes.insert(type_hash<i8>());
    hashes.insert(type_hash<i16>());
    hashes.insert(type_hash<i32>());
    hashes.insert(type_hash<i64>());
    hashes.insert(type_hash<u8>());
    hashes.insert(type_hash<u16>());
    hashes.insert(type_hash<u32>());
    hashes.insert(type_hash<u64>());
    hashes.insert(type_hash<f32>());
    hashes.insert(type_hash<f64>());
    hashes.insert(type_hash<char8>());
    hashes.insert(type_hash<char16>());
    hashes.insert(type_hash<char32>());
    hashes.insert(type_hash<boolean>());

    // All 14 primitives should have unique hashes
    CHECK(hashes.size() == 14);
}

TEST_CASE("primitive_type_id - names are correct") {
    CHECK(std::string_view(primitive_type_id<i8>::name) == "i8");
    CHECK(std::string_view(primitive_type_id<i16>::name) == "i16");
    CHECK(std::string_view(primitive_type_id<i32>::name) == "i32");
    CHECK(std::string_view(primitive_type_id<i64>::name) == "i64");
    CHECK(std::string_view(primitive_type_id<u8>::name) == "u8");
    CHECK(std::string_view(primitive_type_id<u16>::name) == "u16");
    CHECK(std::string_view(primitive_type_id<u32>::name) == "u32");
    CHECK(std::string_view(primitive_type_id<u64>::name) == "u64");
    CHECK(std::string_view(primitive_type_id<f32>::name) == "f32");
    CHECK(std::string_view(primitive_type_id<f64>::name) == "f64");
    CHECK(std::string_view(primitive_type_id<boolean>::name) == "boolean");
}
