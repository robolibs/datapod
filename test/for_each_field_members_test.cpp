#include "datapod/datapod.hpp"
#include <doctest/doctest.h>

using namespace datapod;

TEST_CASE("for_each_field - works with members()") {
    struct MyStruct {
        int a = 1;
        int b = 2;
        int c = 3;

        auto members() noexcept { return std::tie(a, b, c); }
    };

    MyStruct s;
    int sum = 0;

    for_each_field(s, [&sum](auto &field) { sum += field; });

    CHECK(sum == 6);
}

TEST_CASE("for_each_field - partial members") {
    struct MyStruct {
        int x = 10;
        int y = 20;
        int internal_cache = 999;

        auto members() noexcept {
            return std::tie(x, y); // Don't expose cache
        }
    };

    MyStruct s;
    int sum = 0;

    for_each_field(s, [&sum](auto &field) { sum += field; });

    CHECK(sum == 30); // Only x + y, not cache
}

TEST_CASE("for_each_field - modify fields") {
    struct MyStruct {
        int a = 1;
        int b = 2;

        auto members() noexcept { return std::tie(a, b); }
    };

    MyStruct s;

    for_each_field(s, [](auto &field) { field *= 10; });

    CHECK(s.a == 10);
    CHECK(s.b == 20);
}

TEST_CASE("for_each_field_indexed - with members()") {
    struct MyStruct {
        int a = 100;
        int b = 200;
        int c = 300;

        auto members() noexcept { return std::tie(a, b, c); }
    };

    MyStruct s;

    for_each_field_indexed(s, [](auto &field, auto idx) {
        if constexpr (idx == 0) {
            CHECK(field == 100);
        } else if constexpr (idx == 1) {
            CHECK(field == 200);
        } else if constexpr (idx == 2) {
            CHECK(field == 300);
        }
    });
}

TEST_CASE("for_each_field - with String and Vector") {
    struct MyStruct {
        String name = String{"test"};
        Vector<int> items;

        auto members() noexcept { return std::tie(name, items); }
    };

    MyStruct s;
    s.items.push_back(1);
    s.items.push_back(2);

    int field_count = 0;
    for_each_field(s, [&field_count](auto &field) { field_count++; });

    CHECK(field_count == 2);
}

TEST_CASE("for_each_field - const struct") {
    struct MyStruct {
        int x = 42;
        int y = 84;

        auto members() const noexcept { return std::tie(x, y); }
    };

    const MyStruct s;
    int sum = 0;

    for_each_field(s, [&sum](auto const &field) { sum += field; });

    CHECK(sum == 126);
}

TEST_CASE("for_each_field - nested structs with members()") {
    struct Inner {
        int value = 10;
        auto members() noexcept { return std::tie(value); }
    };

    struct Outer {
        Inner inner1;
        Inner inner2;

        auto members() noexcept { return std::tie(inner1, inner2); }
    };

    Outer o;
    o.inner2.value = 20;

    int sum = 0;
    for_each_field(o, [&sum](auto &inner) { for_each_field(inner, [&sum](auto &val) { sum += val; }); });

    CHECK(sum == 30);
}

TEST_CASE("for_each_field - fallback to automatic reflection") {
    struct SimpleStruct {
        int x = 5;
        int y = 7;
    };

    SimpleStruct s;
    int product = 1;

    for_each_field(s, [&product](auto &field) { product *= field; });

    CHECK(product == 35);
}

TEST_CASE("for_each_field - empty members()") {
    struct EmptyMembers {
        int hidden = 99;

        auto members() noexcept { return std::tuple<>{}; }
    };

    EmptyMembers e;
    int count = 0;

    for_each_field(e, [&count](auto &field) { count++; });

    CHECK(count == 0);
}

TEST_CASE("for_each_field - private members exposed") {
    struct PrivateData {
      private:
        int secret1_ = 111;
        int secret2_ = 222;

      public:
        auto members() noexcept { return std::tie(secret1_, secret2_); }
    };

    PrivateData pd;
    int sum = 0;

    for_each_field(pd, [&sum](auto &field) { sum += field; });

    CHECK(sum == 333);
}
