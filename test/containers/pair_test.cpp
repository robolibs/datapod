#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "datapod/containers/pair.hpp"
#include <string>
#include <vector>

using namespace datapod;

// ============================================================================
// Construction Tests
// ============================================================================

TEST_CASE("Pair - Default Construction") {
    Pair<int, double> p;
    CHECK(p.first == 0);
    CHECK(p.second == 0.0);
}

TEST_CASE("Pair - Value Construction") {
    Pair<int, std::string> p(42, "hello");
    CHECK(p.first == 42);
    CHECK(p.second == "hello");
}

TEST_CASE("Pair - Move Construction") {
    std::string s = "moveable";
    Pair<int, std::string> p(42, std::move(s));
    CHECK(p.first == 42);
    CHECK(p.second == "moveable");
    CHECK(s.empty()); // s was moved from
}

TEST_CASE("Pair - Copy Constructor") {
    Pair<int, std::string> p1(42, "hello");
    Pair<int, std::string> p2(p1);
    CHECK(p2.first == 42);
    CHECK(p2.second == "hello");
    CHECK(p1.first == 42); // original unchanged
}

TEST_CASE("Pair - Move Constructor") {
    Pair<int, std::string> p1(42, "hello");
    Pair<int, std::string> p2(std::move(p1));
    CHECK(p2.first == 42);
    CHECK(p2.second == "hello");
}

TEST_CASE("Pair - Converting Constructor from different Pair type") {
    Pair<int, double> p1(42, 3.14);
    Pair<long, float> p2(p1);
    CHECK(p2.first == 42L);
    CHECK(p2.second == doctest::Approx(3.14f));
}

TEST_CASE("Pair - Deduction Guide") {
    auto p = Pair(42, 3.14);
    CHECK(std::is_same_v<decltype(p.first), int>);
    CHECK(std::is_same_v<decltype(p.second), double>);
    CHECK(p.first == 42);
    CHECK(p.second == 3.14);
}

TEST_CASE("Pair - make_pair Helper") {
    auto p = datapod::make_pair(42, std::string("hello"));
    CHECK(std::is_same_v<decltype(p), Pair<int, std::string>>);
    CHECK(p.first == 42);
    CHECK(p.second == "hello");
}

// ============================================================================
// Comparison Tests
// ============================================================================

TEST_CASE("Pair - Equality Operator") {
    Pair<int, std::string> p1(42, "hello");
    Pair<int, std::string> p2(42, "hello");
    Pair<int, std::string> p3(43, "hello");
    Pair<int, std::string> p4(42, "world");

    CHECK(p1 == p2);
    CHECK_FALSE(p1 == p3);
    CHECK_FALSE(p1 == p4);
}

TEST_CASE("Pair - Inequality Operator") {
    Pair<int, std::string> p1(42, "hello");
    Pair<int, std::string> p2(43, "hello");

    CHECK(p1 != p2);
    CHECK_FALSE(p1 != p1);
}

TEST_CASE("Pair - Less Than Operator") {
    Pair<int, int> p1(1, 2);
    Pair<int, int> p2(1, 3);
    Pair<int, int> p3(2, 1);

    CHECK(p1 < p2); // same first, compare second
    CHECK(p1 < p3); // different first
    CHECK_FALSE(p2 < p1);
}

TEST_CASE("Pair - Less Than or Equal Operator") {
    Pair<int, int> p1(1, 2);
    Pair<int, int> p2(1, 2);
    Pair<int, int> p3(1, 3);

    CHECK(p1 <= p2);
    CHECK(p1 <= p3);
    CHECK_FALSE(p3 <= p1);
}

TEST_CASE("Pair - Greater Than Operator") {
    Pair<int, int> p1(2, 1);
    Pair<int, int> p2(1, 2);

    CHECK(p1 > p2);
    CHECK_FALSE(p2 > p1);
}

TEST_CASE("Pair - Greater Than or Equal Operator") {
    Pair<int, int> p1(2, 1);
    Pair<int, int> p2(2, 1);
    Pair<int, int> p3(1, 2);

    CHECK(p1 >= p2);
    CHECK(p1 >= p3);
    CHECK_FALSE(p3 >= p1);
}

// ============================================================================
// Structured Binding Tests
// ============================================================================

TEST_CASE("Pair - Member get<0>()") {
    Pair<int, std::string> p(42, "hello");
    CHECK(p.get<0>() == 42);
    CHECK(p.get<1>() == "hello");
}

TEST_CASE("Pair - Member get<I>() const") {
    Pair<int, std::string> const p(42, "hello");
    CHECK(p.get<0>() == 42);
    CHECK(p.get<1>() == "hello");
}

TEST_CASE("Pair - Member get<I>() &&") {
    Pair<int, std::string> p(42, "hello");
    auto first = std::move(p).get<0>();
    auto second = std::move(p).get<1>();
    CHECK(first == 42);
    CHECK(second == "hello");
}

TEST_CASE("Pair - Free function get<I>()") {
    Pair<int, std::string> p(42, "hello");
    CHECK(datapod::get<0>(p) == 42);
    CHECK(datapod::get<1>(p) == "hello");
}

TEST_CASE("Pair - Free function get<I>() const") {
    Pair<int, std::string> const p(42, "hello");
    CHECK(datapod::get<0>(p) == 42);
    CHECK(datapod::get<1>(p) == "hello");
}

TEST_CASE("Pair - Free function get<I>() &&") {
    Pair<int, std::string> p(42, "hello");
    auto first = datapod::get<0>(std::move(p));
    // Note: p.first is now moved-from, but we can still check the value
    CHECK(first == 42);
}

TEST_CASE("Pair - Structured Bindings") {
    Pair<int, std::string> p(42, "hello");
    auto [a, b] = p;
    CHECK(a == 42);
    CHECK(b == "hello");
}

TEST_CASE("Pair - Structured Bindings by Reference") {
    Pair<int, std::string> p(42, "hello");
    auto &[a, b] = p;
    a = 100;
    b = "world";
    CHECK(p.first == 100);
    CHECK(p.second == "world");
}

TEST_CASE("Pair - Structured Bindings const") {
    Pair<int, std::string> const p(42, "hello");
    auto const &[a, b] = p;
    CHECK(a == 42);
    CHECK(b == "hello");
}

// ============================================================================
// Swap Tests
// ============================================================================

TEST_CASE("Pair - Member swap()") {
    Pair<int, std::string> p1(42, "hello");
    Pair<int, std::string> p2(100, "world");

    p1.swap(p2);

    CHECK(p1.first == 100);
    CHECK(p1.second == "world");
    CHECK(p2.first == 42);
    CHECK(p2.second == "hello");
}

TEST_CASE("Pair - Free function swap()") {
    Pair<int, std::string> p1(42, "hello");
    Pair<int, std::string> p2(100, "world");

    swap(p1, p2);

    CHECK(p1.first == 100);
    CHECK(p1.second == "world");
    CHECK(p2.first == 42);
    CHECK(p2.second == "hello");
}

TEST_CASE("Pair - swap() with ADL") {
    Pair<int, std::string> p1(42, "hello");
    Pair<int, std::string> p2(100, "world");

    datapod::swap(p1, p2); // Use datapod::swap directly

    CHECK(p1.first == 100);
    CHECK(p1.second == "world");
    CHECK(p2.first == 42);
    CHECK(p2.second == "hello");
}

// ============================================================================
// Type Trait Tests
// ============================================================================

TEST_CASE("Pair - std::tuple_size") {
    using P = Pair<int, std::string>;
    CHECK(std::tuple_size_v<P> == 2);
}

TEST_CASE("Pair - std::tuple_element") {
    using P = Pair<int, std::string>;
    CHECK(std::is_same_v<std::tuple_element_t<0, P>, int>);
    CHECK(std::is_same_v<std::tuple_element_t<1, P>, std::string>);
}

// ============================================================================
// Complex Type Tests
// ============================================================================

TEST_CASE("Pair - Nested Pairs") {
    Pair<int, Pair<double, std::string>> p(42, Pair(3.14, "hello"));
    CHECK(p.first == 42);
    CHECK(p.second.first == 3.14);
    CHECK(p.second.second == "hello");
}

TEST_CASE("Pair - Pair of Vectors") {
    Pair<std::vector<int>, std::vector<std::string>> p;
    p.first = {1, 2, 3};
    p.second = {"a", "b", "c"};

    CHECK(p.first.size() == 3);
    CHECK(p.second.size() == 3);
    CHECK(p.first[1] == 2);
    CHECK(p.second[1] == "b");
}

// ============================================================================
// Serialization Tests
// ============================================================================

TEST_CASE("Pair - members() for Serialization") {
    Pair<int, std::string> p(42, "hello");
    auto m = p.members();

    // Check that members() returns a tuple-like with correct values
    CHECK(std::get<0>(m) == 42);
    CHECK(std::get<1>(m) == "hello");
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

TEST_CASE("Pair - Move-only Types") {
    struct MoveOnly {
        int value;
        MoveOnly(int v) : value(v) {}
        MoveOnly(MoveOnly const &) = delete;
        MoveOnly &operator=(MoveOnly const &) = delete;
        MoveOnly(MoveOnly &&) = default;
        MoveOnly &operator=(MoveOnly &&) = default;
    };

    Pair<MoveOnly, MoveOnly> p(MoveOnly(42), MoveOnly(100));
    CHECK(p.first.value == 42);
    CHECK(p.second.value == 100);

    Pair<MoveOnly, MoveOnly> p2(std::move(p));
    CHECK(p2.first.value == 42);
    CHECK(p2.second.value == 100);
}

// ============================================================================
// Const Correctness Tests
// ============================================================================

TEST_CASE("Pair - Const Pair Access") {
    Pair<int, std::string> const p(42, "hello");

    // All these should compile and work with const
    CHECK(p.first == 42);
    CHECK(p.second == "hello");
    CHECK(p.get<0>() == 42);
    CHECK(p.get<1>() == "hello");
    CHECK(get<0>(p) == 42);
    CHECK(get<1>(p) == "hello");
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("Pair - Same Types") {
    Pair<int, int> p(42, 100);
    CHECK(p.first == 42);
    CHECK(p.second == 100);
    CHECK(p.get<0>() == 42);
    CHECK(p.get<1>() == 100);
}

TEST_CASE("Pair - Empty Types") {
    struct Empty {};
    Pair<Empty, int> p({}, 42);
    CHECK(p.second == 42);
}

TEST_CASE("Pair - Reference Wrapper") {
    int x = 42;
    int y = 100;
    Pair<std::reference_wrapper<int>, std::reference_wrapper<int>> p(std::ref(x), std::ref(y));

    CHECK(p.first.get() == 42);
    CHECK(p.second.get() == 100);

    p.first.get() = 200;
    CHECK(x == 200);
}
