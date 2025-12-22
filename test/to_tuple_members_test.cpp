#include "datapod/datapod.hpp"
#include <doctest/doctest.h>

using namespace datapod;

TEST_CASE("to_tuple - uses members() when available") {
    struct MyStruct {
        int x = 10;
        int y = 20;

        auto members() noexcept { return std::tie(x, y); }
    };

    MyStruct s;
    auto tup = to_tuple(s);

    CHECK(std::get<0>(tup) == 10);
    CHECK(std::get<1>(tup) == 20);
    CHECK(std::tuple_size_v<decltype(tup)> == 2);
}

TEST_CASE("to_tuple - const members() works") {
    struct MyStruct {
        int x = 5;
        int y = 15;

        auto members() const noexcept { return std::tie(x, y); }
    };

    const MyStruct s;
    auto tup = to_tuple(s);

    CHECK(std::get<0>(tup) == 5);
    CHECK(std::get<1>(tup) == 15);
}

TEST_CASE("to_tuple - fallback to automatic reflection") {
    struct Point {
        double x = 1.0;
        double y = 2.0;
        double z = 3.0;
    };

    Point p;
    auto tup = to_tuple(p);

    CHECK(std::get<0>(tup) == 1.0);
    CHECK(std::get<1>(tup) == 2.0);
    CHECK(std::get<2>(tup) == 3.0);
    CHECK(std::tuple_size_v<decltype(tup)> == 3);
}

TEST_CASE("to_tuple - partial member selection") {
    struct MyStruct {
        int id = 42;
        String name = String{"test"};

        // Don't expose cache in reflection
        mutable bool cache_valid = false;
        mutable int cached_value = 999;

        auto members() noexcept { return std::tie(id, name); }
    };

    MyStruct s;
    auto tup = to_tuple(s);

    // Only 2 members in tuple
    CHECK(std::tuple_size_v<decltype(tup)> == 2);
    CHECK(std::get<0>(tup) == 42);
}

TEST_CASE("to_tuple - works with methods") {
    struct WithMethods {
        int x = 100;
        int y = 200;

        auto members() noexcept { return std::tie(x, y); }

        // Methods don't break reflection
        int sum() const { return x + y; }
        void reset() { x = y = 0; }
    };

    WithMethods s;
    CHECK(s.sum() == 300);

    auto tup = to_tuple(s);
    CHECK(std::get<0>(tup) == 100);
    CHECK(std::get<1>(tup) == 200);
}

TEST_CASE("to_tuple - modifying through tuple") {
    struct MyStruct {
        int x = 10;
        int y = 20;

        auto members() noexcept { return std::tie(x, y); }
    };

    MyStruct s;
    auto tup = to_tuple(s);

    // Modify through tuple references
    std::get<0>(tup) = 999;
    std::get<1>(tup) = 888;

    CHECK(s.x == 999);
    CHECK(s.y == 888);
}

TEST_CASE("to_tuple - both members() and const members()") {
    struct BothOverloads {
        int x = 50;

        auto members() noexcept { return std::tie(x); }

        auto members() const noexcept { return std::tie(x); }
    };

    BothOverloads s;
    auto tup1 = to_tuple(s);
    CHECK(std::get<0>(tup1) == 50);

    const BothOverloads cs;
    auto tup2 = to_tuple(cs);
    CHECK(std::get<0>(tup2) == 50);
}

TEST_CASE("to_tuple - private members exposed") {
    struct PrivateData {
      private:
        int secret_ = 777;

      public:
        auto members() noexcept { return std::tie(secret_); }

        int get_secret() const { return secret_; }
    };

    PrivateData pd;
    auto tup = to_tuple(pd);

    CHECK(std::get<0>(tup) == 777);
}

TEST_CASE("to_tuple - empty members()") {
    struct Empty {
        int internal_data = 42; // Not exposed

        auto members() noexcept { return std::tuple<>{}; }
    };

    Empty e;
    auto tup = to_tuple(e);

    CHECK(std::tuple_size_v<decltype(tup)> == 0);
}

TEST_CASE("to_tuple - many members") {
    struct Many {
        int a = 1, b = 2, c = 3, d = 4, e = 5;

        auto members() noexcept { return std::tie(a, b, c, d, e); }
    };

    Many m;
    auto tup = to_tuple(m);

    CHECK(std::tuple_size_v<decltype(tup)> == 5);
    CHECK(std::get<0>(tup) == 1);
    CHECK(std::get<4>(tup) == 5);
}

TEST_CASE("to_tuple - nested datapod containers") {
    struct Nested {
        Vector<int> items;
        String name = String{"nested"};

        auto members() noexcept { return std::tie(items, name); }
    };

    Nested n;
    n.items.push_back(1);
    n.items.push_back(2);

    auto tup = to_tuple(n);

    CHECK(std::tuple_size_v<decltype(tup)> == 2);
    CHECK(std::get<0>(tup).size() == 2);
}
