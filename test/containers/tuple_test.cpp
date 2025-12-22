#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "datapod/containers/tuple.hpp"
#include <string>
#include <vector>

using namespace datapod;

// ============================================================================
// Construction Tests
// ============================================================================

TEST_CASE("Tuple - Default Construction") {
    Tuple<int, double, std::string> t;
    CHECK(get<0>(t) == 0);
    CHECK(get<1>(t) == 0.0);
    CHECK(get<2>(t) == "");
}

TEST_CASE("Tuple - Value Construction") {
    Tuple<int, double, std::string> t(42, 3.14, std::string("hello"));
    CHECK(get<0>(t) == 42);
    CHECK(get<1>(t) == 3.14);
    CHECK(get<2>(t) == "hello");
}

TEST_CASE("Tuple - Copy Constructor") {
    Tuple<int, double, std::string> t1(42, 3.14, std::string("hello"));
    Tuple<int, double, std::string> t2(t1);
    CHECK(get<0>(t2) == 42);
    CHECK(get<1>(t2) == 3.14);
    CHECK(get<2>(t2) == "hello");
}

TEST_CASE("Tuple - Move Constructor") {
    Tuple<int, double, std::string> t1(42, 3.14, std::string("hello"));
    Tuple<int, double, std::string> t2(std::move(t1));
    CHECK(get<0>(t2) == 42);
    CHECK(get<1>(t2) == 3.14);
    CHECK(get<2>(t2) == "hello");
}

TEST_CASE("Tuple - Copy Assignment") {
    Tuple<int, double, std::string> t1(42, 3.14, std::string("hello"));
    Tuple<int, double, std::string> t2;
    t2 = t1;
    CHECK(get<0>(t2) == 42);
    CHECK(get<1>(t2) == 3.14);
    CHECK(get<2>(t2) == "hello");
}

TEST_CASE("Tuple - Move Assignment") {
    Tuple<int, double, std::string> t1(42, 3.14, std::string("hello"));
    Tuple<int, double, std::string> t2;
    t2 = std::move(t1);
    CHECK(get<0>(t2) == 42);
    CHECK(get<1>(t2) == 3.14);
    CHECK(get<2>(t2) == "hello");
}

TEST_CASE("Tuple - Deduction Guide") {
    auto t = Tuple(42, 3.14, std::string("hello"));
    CHECK(std::is_same_v<decltype(t), Tuple<int, double, std::string>>);
    CHECK(get<0>(t) == 42);
    CHECK(get<1>(t) == 3.14);
    CHECK(get<2>(t) == "hello");
}

// ============================================================================
// get() Tests (Free Functions)
// ============================================================================

TEST_CASE("Tuple - Free function get<I>()") {
    Tuple<int, double, std::string> t(42, 3.14, std::string("hello"));
    CHECK(get<0>(t) == 42);
    CHECK(get<1>(t) == 3.14);
    CHECK(get<2>(t) == "hello");
}

TEST_CASE("Tuple - Free function get<I>() const") {
    Tuple<int, double, std::string> const t(42, 3.14, std::string("hello"));
    CHECK(get<0>(t) == 42);
    CHECK(get<1>(t) == 3.14);
    CHECK(get<2>(t) == "hello");
}

TEST_CASE("Tuple - Free function get<I>() &&") {
    Tuple<int, double, std::string> t(42, 3.14, std::string("hello"));
    auto s = get<2>(std::move(t));
    CHECK(s == "hello");
}

TEST_CASE("Tuple - Modify via get()") {
    Tuple<int, double, std::string> t(42, 3.14, std::string("hello"));
    get<0>(t) = 100;
    get<1>(t) = 2.71;
    get<2>(t) = "world";

    CHECK(get<0>(t) == 100);
    CHECK(get<1>(t) == 2.71);
    CHECK(get<2>(t) == "world");
}

// ============================================================================
// Structured Binding Tests
// ============================================================================

TEST_CASE("Tuple - Structured Bindings") {
    Tuple<int, double, std::string> t(42, 3.14, std::string("hello"));
    auto [a, b, c] = t;
    CHECK(a == 42);
    CHECK(b == 3.14);
    CHECK(c == "hello");
}

TEST_CASE("Tuple - Structured Bindings by Reference") {
    Tuple<int, double, std::string> t(42, 3.14, std::string("hello"));
    auto &[a, b, c] = t;
    a = 100;
    b = 2.71;
    c = "world";

    CHECK(get<0>(t) == 100);
    CHECK(get<1>(t) == 2.71);
    CHECK(get<2>(t) == "world");
}

TEST_CASE("Tuple - Structured Bindings const") {
    Tuple<int, double, std::string> const t(42, 3.14, std::string("hello"));
    auto const &[a, b, c] = t;
    CHECK(a == 42);
    CHECK(b == 3.14);
    CHECK(c == "hello");
}

// ============================================================================
// Comparison Tests
// ============================================================================

TEST_CASE("Tuple - Equality Operator") {
    Tuple<int, double, std::string> t1(42, 3.14, std::string("hello"));
    Tuple<int, double, std::string> t2(42, 3.14, std::string("hello"));
    Tuple<int, double, std::string> t3(43, 3.14, std::string("hello"));

    CHECK(t1 == t2);
    CHECK_FALSE(t1 == t3);
}

TEST_CASE("Tuple - Inequality Operator") {
    Tuple<int, double, std::string> t1(42, 3.14, std::string("hello"));
    Tuple<int, double, std::string> t2(43, 3.14, std::string("hello"));

    CHECK(t1 != t2);
    CHECK_FALSE(t1 != t1);
}

TEST_CASE("Tuple - Less Than Operator") {
    Tuple<int, int> t1(1, 2);
    Tuple<int, int> t2(1, 3);
    Tuple<int, int> t3(2, 1);

    CHECK(t1 < t2);
    CHECK(t1 < t3);
    CHECK_FALSE(t2 < t1);
}

TEST_CASE("Tuple - Less Than or Equal Operator") {
    Tuple<int, int> t1(1, 2);
    Tuple<int, int> t2(1, 2);
    Tuple<int, int> t3(1, 3);

    CHECK(t1 <= t2);
    CHECK(t1 <= t3);
    CHECK_FALSE(t3 <= t1);
}

TEST_CASE("Tuple - Greater Than Operator") {
    Tuple<int, int> t1(2, 1);
    Tuple<int, int> t2(1, 2);

    CHECK(t1 > t2);
    CHECK_FALSE(t2 > t1);
}

TEST_CASE("Tuple - Greater Than or Equal Operator") {
    Tuple<int, int> t1(2, 1);
    Tuple<int, int> t2(2, 1);
    Tuple<int, int> t3(1, 2);

    CHECK(t1 >= t2);
    CHECK(t1 >= t3);
    CHECK_FALSE(t3 >= t1);
}

// ============================================================================
// Member apply() Tests
// ============================================================================

TEST_CASE("Tuple - Member apply() with Function") {
    Tuple<int, int, int> t(1, 2, 3);
    auto sum = t.apply([](int a, int b, int c) { return a + b + c; });
    CHECK(sum == 6);
}

TEST_CASE("Tuple - Member apply() const") {
    Tuple<int, int, int> const t(1, 2, 3);
    auto sum = t.apply([](int a, int b, int c) { return a + b + c; });
    CHECK(sum == 6);
}

TEST_CASE("Tuple - Member apply() &&") {
    Tuple<std::string, std::string> t("hello", "world");
    auto concat = std::move(t).apply([](std::string a, std::string b) { return a + " " + b; });
    CHECK(concat == "hello world");
}

TEST_CASE("Tuple - Member apply() with Different Return Types") {
    Tuple<int, double, std::string> t(42, 3.14, std::string("hello"));
    auto str = t.apply(
        [](int i, double d, std::string const &s) { return std::to_string(i) + " " + std::to_string(d) + " " + s; });
    CHECK(str.find("42") != std::string::npos);
    CHECK(str.find("hello") != std::string::npos);
}

// ============================================================================
// Free apply() Tests
// ============================================================================

TEST_CASE("Tuple - Free apply() with Function") {
    Tuple<int, int, int> t(1, 2, 3);
    auto sum = apply([](int a, int b, int c) { return a + b + c; }, t);
    CHECK(sum == 6);
}

TEST_CASE("Tuple - Free apply() with Two Tuples") {
    Tuple<int, int> t1(1, 2);
    Tuple<int, int> t2(3, 4);
    // apply with two tuples calls f for each pair of corresponding elements
    int count = 0;
    apply(
        [&count](int a, int b) {
            count++;
            CHECK((a + b == 4 || a + b == 6));
        },
        t1, t2);
    CHECK(count == 2); // Called twice: f(1,3) and f(2,4)
}

// ============================================================================
// Member for_each() Tests
// ============================================================================

TEST_CASE("Tuple - Member for_each() with Mutable Lambda") {
    Tuple<int, int, int> t(1, 2, 3);
    int sum = 0;
    t.for_each([&sum](auto &x) { sum += x; });
    CHECK(sum == 6);
}

TEST_CASE("Tuple - Member for_each() const") {
    Tuple<int, int, int> const t(1, 2, 3);
    int sum = 0;
    t.for_each([&sum](auto const &x) { sum += x; });
    CHECK(sum == 6);
}

TEST_CASE("Tuple - Member for_each() &&") {
    Tuple<std::string, std::string> t("hello", "world");
    std::vector<std::string> v;
    std::move(t).for_each([&v](auto &&x) { v.push_back(std::move(x)); });
    CHECK(v.size() == 2);
    CHECK(v[0] == "hello");
    CHECK(v[1] == "world");
}

TEST_CASE("Tuple - Member for_each() Modifies Elements") {
    Tuple<int, int, int> t(1, 2, 3);
    t.for_each([](auto &x) { x *= 2; });
    CHECK(get<0>(t) == 2);
    CHECK(get<1>(t) == 4);
    CHECK(get<2>(t) == 6);
}

TEST_CASE("Tuple - Member for_each() with Heterogeneous Types") {
    Tuple<int, double, std::string> t(42, 3.14, std::string("hello"));
    int count = 0;
    t.for_each([&count](auto const &) { ++count; });
    CHECK(count == 3);
}

// ============================================================================
// Type Trait Tests
// ============================================================================

TEST_CASE("Tuple - is_tuple trait") {
    CHECK(is_tuple_v<Tuple<int, double>>);
    CHECK_FALSE(is_tuple_v<int>);
    CHECK_FALSE(is_tuple_v<std::string>);
}

TEST_CASE("Tuple - tuple_size") {
    CHECK(tuple_size_v<Tuple<int>> == 1);
    CHECK(tuple_size_v<Tuple<int, double>> == 2);
    CHECK(tuple_size_v<Tuple<int, double, std::string>> == 3);
}

TEST_CASE("Tuple - std::tuple_size") {
    CHECK(std::tuple_size_v<Tuple<int>> == 1);
    CHECK(std::tuple_size_v<Tuple<int, double>> == 2);
    CHECK(std::tuple_size_v<Tuple<int, double, std::string>> == 3);
}

TEST_CASE("Tuple - tuple_element") {
    using T = Tuple<int, double, std::string>;
    CHECK(std::is_same_v<tuple_element_t<0, T>, int>);
    CHECK(std::is_same_v<tuple_element_t<1, T>, double>);
    CHECK(std::is_same_v<tuple_element_t<2, T>, std::string>);
}

TEST_CASE("Tuple - std::tuple_element") {
    using T = Tuple<int, double, std::string>;
    CHECK(std::is_same_v<std::tuple_element_t<0, T>, int>);
    CHECK(std::is_same_v<std::tuple_element_t<1, T>, double>);
    CHECK(std::is_same_v<std::tuple_element_t<2, T>, std::string>);
}

// ============================================================================
// Complex Type Tests
// ============================================================================

TEST_CASE("Tuple - Single Element") {
    Tuple<int> t(42);
    CHECK(get<0>(t) == 42);
}

TEST_CASE("Tuple - Two Elements") {
    Tuple<int, double> t(42, 3.14);
    CHECK(get<0>(t) == 42);
    CHECK(get<1>(t) == 3.14);
}

TEST_CASE("Tuple - Many Elements") {
    Tuple<int, double, std::string, char, bool> t(42, 3.14, std::string("hello"), 'A', true);
    CHECK(get<0>(t) == 42);
    CHECK(get<1>(t) == 3.14);
    CHECK(get<2>(t) == "hello");
    CHECK(get<3>(t) == 'A');
    CHECK(get<4>(t) == true);
}

TEST_CASE("Tuple - Can Store Complex Types") {
    // Test that tuples can contain complex types
    Tuple<int, std::vector<int>> t;
    get<0>(t) = 42;
    get<1>(t) = {1, 2, 3};

    CHECK(get<0>(t) == 42);
    CHECK(get<1>(t).size() == 3);
    CHECK(get<1>(t)[1] == 2);
}

TEST_CASE("Tuple - Tuple of Vectors") {
    Tuple<std::vector<int>, std::vector<std::string>> t;
    get<0>(t) = {1, 2, 3};
    get<1>(t) = {"a", "b", "c"};

    CHECK(get<0>(t).size() == 3);
    CHECK(get<1>(t).size() == 3);
    CHECK(get<0>(t)[1] == 2);
    CHECK(get<1>(t)[1] == "b");
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

TEST_CASE("Tuple - Move-only Types") {
    struct MoveOnly {
        int value;
        MoveOnly(int v) : value(v) {}
        MoveOnly(MoveOnly const &) = delete;
        MoveOnly &operator=(MoveOnly const &) = delete;
        MoveOnly(MoveOnly &&) = default;
        MoveOnly &operator=(MoveOnly &&) = default;
    };

    Tuple<MoveOnly, MoveOnly> t(MoveOnly(42), MoveOnly(100));
    CHECK(get<0>(t).value == 42);
    CHECK(get<1>(t).value == 100);

    Tuple<MoveOnly, MoveOnly> t2(std::move(t));
    CHECK(get<0>(t2).value == 42);
    CHECK(get<1>(t2).value == 100);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("Tuple - Same Types") {
    Tuple<int, int, int> t(1, 2, 3);
    CHECK(get<0>(t) == 1);
    CHECK(get<1>(t) == 2);
    CHECK(get<2>(t) == 3);
}

TEST_CASE("Tuple - Empty Types") {
    struct Empty {};
    Tuple<Empty, int, Empty> t({}, 42, {});
    CHECK(get<1>(t) == 42);
}

TEST_CASE("Tuple - Large Alignment") {
    struct alignas(64) Aligned {
        int value;
        Aligned() : value(0) {}
        Aligned(int v) : value(v) {}
    };

    Tuple<int, Aligned, double> t(42, Aligned(100), 3.14);
    CHECK(get<0>(t) == 42);
    CHECK(get<1>(t).value == 100);
    CHECK(get<2>(t) == 3.14);
}

// ============================================================================
// Real-World Use Cases
// ============================================================================

TEST_CASE("Tuple - Return Multiple Values from Function") {
    auto divide_with_remainder = [](int a, int b) -> Tuple<int, int> { return Tuple(a / b, a % b); };

    auto result = divide_with_remainder(17, 5);
    CHECK(get<0>(result) == 3);
    CHECK(get<1>(result) == 2);
}

TEST_CASE("Tuple - Function Composition with apply") {
    Tuple<int, int> coords(10, 20);

    auto distance = coords.apply([](int x, int y) { return std::sqrt(x * x + y * y); });

    CHECK(distance == doctest::Approx(22.36068));
}

TEST_CASE("Tuple - Accumulate with for_each") {
    Tuple<int, int, int, int> t(1, 2, 3, 4);
    int product = 1;
    t.for_each([&product](auto const &x) { product *= x; });
    CHECK(product == 24);
}

TEST_CASE("Tuple - Transform Elements with for_each") {
    Tuple<int, int, int> t(1, 2, 3);
    std::vector<int> doubled;
    t.for_each([&doubled](auto const &x) { doubled.push_back(x * 2); });

    REQUIRE(doubled.size() == 3);
    CHECK(doubled[0] == 2);
    CHECK(doubled[1] == 4);
    CHECK(doubled[2] == 6);
}
