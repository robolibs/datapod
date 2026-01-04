#include <doctest/doctest.h>

#include <datapod/reflection/to_tuple.hpp>
#include <datapod/reflection/for_each_field.hpp>
#include <datapod/reflection/arity.hpp>

using namespace datapod;

// Test struct with 15 fields (more than old 10 limit)
struct LargeStruct15 {
    int a, b, c, d, e, f, g, h, i, j, k, l, m, n, o;
};

// Test struct with 25 fields
struct LargeStruct25 {
    int a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, u, v, w, x, y, z;
};

// Test struct with 32 fields
struct LargeStruct32 {
    int f1, f2, f3, f4, f5, f6, f7, f8, f9, f10;
    int f11, f12, f13, f14, f15, f16, f17, f18, f19, f20;
    int f21, f22, f23, f24, f25, f26, f27, f28, f29, f30;
    int f31, f32;
};

// Test struct with 50 fields
struct LargeStruct50 {
    int f1, f2, f3, f4, f5, f6, f7, f8, f9, f10;
    int f11, f12, f13, f14, f15, f16, f17, f18, f19, f20;
    int f21, f22, f23, f24, f25, f26, f27, f28, f29, f30;
    int f31, f32, f33, f34, f35, f36, f37, f38, f39, f40;
    int f41, f42, f43, f44, f45, f46, f47, f48, f49, f50;
};

TEST_CASE("Extended Reflection - Arity detection") {
    CHECK(arity_v<LargeStruct15> == 15);
    CHECK(arity_v<LargeStruct25> == 25);
    CHECK(arity_v<LargeStruct32> == 32);
    CHECK(arity_v<LargeStruct50> == 50);
}

TEST_CASE("Extended Reflection - 15 fields to_tuple") {
    LargeStruct15 s{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    
    auto tuple = to_tuple(s);
    
    CHECK(std::get<0>(tuple) == 1);
    CHECK(std::get<5>(tuple) == 6);
    CHECK(std::get<10>(tuple) == 11);
    CHECK(std::get<14>(tuple) == 15);
}

TEST_CASE("Extended Reflection - 25 fields to_tuple") {
    LargeStruct25 s{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25};
    
    auto tuple = to_tuple(s);
    
    CHECK(std::get<0>(tuple) == 1);
    CHECK(std::get<12>(tuple) == 13);
    CHECK(std::get<24>(tuple) == 25);
}

TEST_CASE("Extended Reflection - 32 fields to_tuple") {
    LargeStruct32 s{};
    for (int i = 0; i < 32; ++i) {
        // Initialize via tuple
        auto tuple = to_tuple(s);
        // We can't easily set individual fields, so just verify we can access them
    }
    
    auto tuple = to_tuple(s);
    CHECK(std::tuple_size_v<decltype(tuple)> == 32);
}

TEST_CASE("Extended Reflection - 50 fields to_tuple") {
    LargeStruct50 s{};
    
    auto tuple = to_tuple(s);
    CHECK(std::tuple_size_v<decltype(tuple)> == 50);
}

TEST_CASE("Extended Reflection - for_each_field with 15 fields") {
    LargeStruct15 s{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    
    int sum = 0;
    for_each_field(s, [&sum](auto& field) {
        sum += field;
    });
    
    CHECK(sum == 120); // 1+2+...+15 = 120
}

TEST_CASE("Extended Reflection - for_each_field with 25 fields") {
    LargeStruct25 s{};
    // Initialize all to 1
    for_each_field(s, [](auto& field) {
        field = 1;
    });
    
    int count = 0;
    for_each_field(s, [&count](auto& field) {
        count += field;
    });
    
    CHECK(count == 25);
}

TEST_CASE("Extended Reflection - Modify fields via to_tuple") {
    LargeStruct15 s{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    
    auto tuple = to_tuple(s);
    std::get<0>(tuple) = 100;
    std::get<14>(tuple) = 200;
    
    CHECK(s.a == 100);
    CHECK(s.o == 200);
}

TEST_CASE("Extended Reflection - Const to_tuple") {
    const LargeStruct15 s{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    
    auto tuple = to_tuple(s);
    
    CHECK(std::get<0>(tuple) == 1);
    CHECK(std::get<14>(tuple) == 15);
}

TEST_CASE("Extended Reflection - for_each_field_indexed") {
    LargeStruct15 s{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    
    for_each_field_indexed(s, [](auto& field, auto index) {
        CHECK(field == index.value + 1);
    });
}

TEST_CASE("Extended Reflection - Backwards compatibility with members()") {
    // Struct with manual members() should still work
    struct WithMembers {
        int x, y, z;
        auto members() { return std::tie(x, y, z); }
        auto members() const { return std::tie(x, y, z); }
    };
    
    WithMembers s{10, 20, 30};
    auto tuple = to_tuple(s);
    
    CHECK(std::get<0>(tuple) == 10);
    CHECK(std::get<1>(tuple) == 20);
    CHECK(std::get<2>(tuple) == 30);
}

TEST_CASE("Extended Reflection - Mixed types") {
    struct MixedLarge {
        int a; double b; float c; char d; long e;
        int f; double g; float h; char i; long j;
        int k; double l; float m; char n; long o;
    };
    
    MixedLarge s{1, 2.0, 3.0f, 'a', 5L, 6, 7.0, 8.0f, 'b', 10L, 11, 12.0, 13.0f, 'c', 15L};
    
    auto tuple = to_tuple(s);
    
    CHECK(std::get<0>(tuple) == 1);
    CHECK(std::get<1>(tuple) == 2.0);
    CHECK(std::get<3>(tuple) == 'a');
    CHECK(std::get<14>(tuple) == 15L);
}
