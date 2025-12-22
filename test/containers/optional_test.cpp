#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "datapod/containers/optional.hpp"
#include "datapod/containers/string.hpp"

using namespace datapod;

TEST_SUITE("Optional") {

    // ========================================================================
    // Construction Tests
    // ========================================================================

    TEST_CASE("Default Construction") {
        Optional<int> opt;
        CHECK_FALSE(opt.has_value());
        CHECK_FALSE(opt);
    }

    TEST_CASE("nullopt Construction") {
        Optional<int> opt(nullopt);
        CHECK_FALSE(opt.has_value());
    }

    TEST_CASE("Value Construction") {
        Optional<int> opt(42);
        REQUIRE(opt.has_value());
        CHECK(*opt == 42);
    }

    TEST_CASE("Copy Construction - With Value") {
        Optional<int> opt1(10);
        Optional<int> opt2(opt1);

        REQUIRE(opt1.has_value());
        REQUIRE(opt2.has_value());
        CHECK(*opt1 == 10);
        CHECK(*opt2 == 10);
    }

    TEST_CASE("Copy Construction - Empty") {
        Optional<int> opt1;
        Optional<int> opt2(opt1);

        CHECK_FALSE(opt1.has_value());
        CHECK_FALSE(opt2.has_value());
    }

    TEST_CASE("Move Construction - With Value") {
        Optional<String> opt1(String("hello"));
        Optional<String> opt2(std::move(opt1));

        REQUIRE(opt2.has_value());
        CHECK(opt2->view() == "hello");
    }

    TEST_CASE("Move Construction - Empty") {
        Optional<String> opt1;
        Optional<String> opt2(std::move(opt1));

        CHECK_FALSE(opt2.has_value());
    }

    // ========================================================================
    // Assignment Tests
    // ========================================================================

    TEST_CASE("Assignment - nullopt") {
        Optional<int> opt(42);
        opt = nullopt;

        CHECK_FALSE(opt.has_value());
    }

    TEST_CASE("Assignment - Value") {
        Optional<int> opt;
        opt = 99;

        REQUIRE(opt.has_value());
        CHECK(*opt == 99);
    }

    TEST_CASE("Assignment - Copy") {
        Optional<int> opt1(10);
        Optional<int> opt2;
        opt2 = opt1;

        REQUIRE(opt2.has_value());
        CHECK(*opt2 == 10);
    }

    TEST_CASE("Assignment - Move") {
        Optional<String> opt1(String("world"));
        Optional<String> opt2;
        opt2 = std::move(opt1);

        REQUIRE(opt2.has_value());
        CHECK(opt2->view() == "world");
    }

    // ========================================================================
    // Observers Tests
    // ========================================================================

    TEST_CASE("operator* - Lvalue") {
        Optional<int> opt(42);
        CHECK(*opt == 42);

        *opt = 100;
        CHECK(*opt == 100);
    }

    TEST_CASE("operator* - Const") {
        Optional<int> const opt(42);
        CHECK(*opt == 42);
    }

    TEST_CASE("operator* - Rvalue") {
        Optional<int> opt(42);
        int val = *std::move(opt);
        CHECK(val == 42);
    }

    TEST_CASE("operator-> - Access Members") {
        struct Point {
            int x, y;
        };
        Optional<Point> opt(Point{10, 20});

        CHECK(opt->x == 10);
        CHECK(opt->y == 20);
    }

    TEST_CASE("has_value()") {
        Optional<int> opt1;
        Optional<int> opt2(42);

        CHECK_FALSE(opt1.has_value());
        CHECK(opt2.has_value());
    }

    TEST_CASE("operator bool") {
        Optional<int> opt1;
        Optional<int> opt2(42);

        CHECK_FALSE(static_cast<bool>(opt1));
        CHECK(static_cast<bool>(opt2));
    }

    TEST_CASE("value() - Success") {
        Optional<int> opt(42);
        CHECK(opt.value() == 42);
    }

    TEST_CASE("value() - Throws on Empty") {
        Optional<int> opt;
        CHECK_THROWS_AS(opt.value(), std::runtime_error);
    }

    TEST_CASE("value_or() - Has Value") {
        Optional<int> opt(42);
        CHECK(opt.value_or(100) == 42);
    }

    TEST_CASE("value_or() - Empty") {
        Optional<int> opt;
        CHECK(opt.value_or(100) == 100);
    }

    TEST_CASE("value_or() - Rvalue") {
        Optional<String> opt(String("hello"));
        String result = std::move(opt).value_or(String("default"));
        CHECK(result.view() == "hello");
    }

    // ========================================================================
    // Modifiers Tests
    // ========================================================================

    TEST_CASE("reset() - With Value") {
        Optional<int> opt(42);
        opt.reset();

        CHECK_FALSE(opt.has_value());
    }

    TEST_CASE("reset() - Already Empty") {
        Optional<int> opt;
        opt.reset(); // Should not crash

        CHECK_FALSE(opt.has_value());
    }

    TEST_CASE("emplace() - Create Value") {
        Optional<String> opt;
        opt.emplace("test");

        REQUIRE(opt.has_value());
        CHECK(opt->view() == "test");
    }

    TEST_CASE("emplace() - Replace Value") {
        Optional<String> opt(String("old"));
        opt.emplace("new");

        REQUIRE(opt.has_value());
        CHECK(opt->view() == "new");
    }

    TEST_CASE("swap() - Both Have Values") {
        Optional<int> opt1(10);
        Optional<int> opt2(20);

        opt1.swap(opt2);

        REQUIRE(opt1.has_value());
        REQUIRE(opt2.has_value());
        CHECK(*opt1 == 20);
        CHECK(*opt2 == 10);
    }

    TEST_CASE("swap() - One Empty") {
        Optional<int> opt1(10);
        Optional<int> opt2;

        opt1.swap(opt2);

        CHECK_FALSE(opt1.has_value());
        REQUIRE(opt2.has_value());
        CHECK(*opt2 == 10);
    }

    TEST_CASE("swap() - Both Empty") {
        Optional<int> opt1;
        Optional<int> opt2;

        opt1.swap(opt2);

        CHECK_FALSE(opt1.has_value());
        CHECK_FALSE(opt2.has_value());
    }

    // ========================================================================
    // Monadic Operations Tests (C++23)
    // ========================================================================

    TEST_CASE("and_then() - Has Value") {
        Optional<int> opt(10);
        auto result = opt.and_then([](int x) { return Optional<int>(x * 2); });

        REQUIRE(result.has_value());
        CHECK(*result == 20);
    }

    TEST_CASE("and_then() - Empty") {
        Optional<int> opt;
        auto result = opt.and_then([](int x) { return Optional<int>(x * 2); });

        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("and_then() - Chain Transformations") {
        Optional<int> opt(5);
        auto result = opt.and_then([](int x) { return Optional<int>(x + 1); }).and_then([](int x) {
            return Optional<int>(x * 2);
        });

        REQUIRE(result.has_value());
        CHECK(*result == 12); // (5 + 1) * 2
    }

    TEST_CASE("and_then() - Short Circuit") {
        Optional<int> opt(10);
        auto result = opt.and_then([](int) { return Optional<int>{}; })            // Returns empty
                          .and_then([](int x) { return Optional<int>(x * 100); }); // Should not execute

        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("transform() - Has Value") {
        Optional<int> opt(10);
        auto result = opt.transform([](int x) { return x * 2; });

        REQUIRE(result.has_value());
        CHECK(*result == 20);
    }

    TEST_CASE("transform() - Empty") {
        Optional<int> opt;
        auto result = opt.transform([](int x) { return x * 2; });

        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("transform() - Change Type") {
        Optional<int> opt(42);
        auto result = opt.transform([](int x) { return String::format("{}", x); });

        REQUIRE(result.has_value());
        CHECK(result->view() == "42");
    }

    TEST_CASE("transform() - Chain Transformations") {
        Optional<int> opt(5);
        auto result =
            opt.transform([](int x) { return x + 1; }).transform([](int x) { return x * 2; }).transform([](int x) {
                return x - 1;
            });

        REQUIRE(result.has_value());
        CHECK(*result == 11); // ((5 + 1) * 2) - 1
    }

    TEST_CASE("or_else() - Has Value") {
        Optional<int> opt(42);
        auto result = opt.or_else([]() { return Optional<int>(100); });

        REQUIRE(result.has_value());
        CHECK(*result == 42); // Original value, not fallback
    }

    TEST_CASE("or_else() - Empty") {
        Optional<int> opt;
        auto result = opt.or_else([]() { return Optional<int>(100); });

        REQUIRE(result.has_value());
        CHECK(*result == 100);
    }

    TEST_CASE("or_else() - Rvalue") {
        Optional<String> opt;
        auto result = std::move(opt).or_else([]() { return Optional<String>(String("fallback")); });

        REQUIRE(result.has_value());
        CHECK(result->view() == "fallback");
    }

    TEST_CASE("Monadic - Complex Pipeline") {
        // Simulate a pipeline: parse int -> double it -> convert to string -> uppercase
        Optional<int> opt(5);

        auto result = opt.transform([](int x) { return x * 2; })                                      // 10
                          .and_then([](int x) { return x > 5 ? Optional<int>(x) : Optional<int>{}; }) // Still 10
                          .transform([](int x) { return String::format("Value: {}", x); })            // "Value: 10"
                          .or_else([]() { return Optional<String>(String("No value")); });

        REQUIRE(result.has_value());
        CHECK(result->view() == "Value: 10");
    }

    // ========================================================================
    // Comparison Operators Tests
    // ========================================================================

    TEST_CASE("operator== - Both Have Same Value") {
        Optional<int> opt1(42);
        Optional<int> opt2(42);

        CHECK(opt1 == opt2);
    }

    TEST_CASE("operator== - Different Values") {
        Optional<int> opt1(42);
        Optional<int> opt2(100);

        CHECK_FALSE(opt1 == opt2);
    }

    TEST_CASE("operator== - Both Empty") {
        Optional<int> opt1;
        Optional<int> opt2;

        CHECK(opt1 == opt2);
    }

    TEST_CASE("operator== - One Empty") {
        Optional<int> opt1(42);
        Optional<int> opt2;

        CHECK_FALSE(opt1 == opt2);
    }

    TEST_CASE("operator!= - Different Values") {
        Optional<int> opt1(42);
        Optional<int> opt2(100);

        CHECK(opt1 != opt2);
    }

    TEST_CASE("operator== with nullopt") {
        Optional<int> opt1;
        Optional<int> opt2(42);

        CHECK(opt1 == nullopt);
        CHECK(nullopt == opt1);
        CHECK_FALSE(opt2 == nullopt);
        CHECK_FALSE(nullopt == opt2);
    }

    TEST_CASE("operator!= with nullopt") {
        Optional<int> opt1;
        Optional<int> opt2(42);

        CHECK_FALSE(opt1 != nullopt);
        CHECK_FALSE(nullopt != opt1);
        CHECK(opt2 != nullopt);
        CHECK(nullopt != opt2);
    }

    // ========================================================================
    // make_optional Helper Tests
    // ========================================================================

    TEST_CASE("make_optional()") {
        auto opt = make_optional(42);

        REQUIRE(opt.has_value());
        CHECK(*opt == 42);
    }

    TEST_CASE("make_optional() - String") {
        auto opt = make_optional(String("hello"));

        REQUIRE(opt.has_value());
        CHECK(opt->view() == "hello");
    }

    // ========================================================================
    // Serialization Support Tests
    // ========================================================================

    TEST_CASE("members() - Serialization Support") {
        Optional<int> opt(42);

        // Verify that members() function exists (required for serialization)
        auto m = opt.members();
        (void)m; // Suppress unused warning

        // This test just verifies the members() method compiles and can be called
        // Actual serialization is tested in serialization tests
    }

    // ========================================================================
    // Edge Cases Tests
    // ========================================================================

    TEST_CASE("Multiple Reset Calls") {
        Optional<int> opt(42);
        opt.reset();
        opt.reset();
        opt.reset();

        CHECK_FALSE(opt.has_value());
    }

    TEST_CASE("Assign After Reset") {
        Optional<int> opt(42);
        opt.reset();
        opt = 100;

        REQUIRE(opt.has_value());
        CHECK(*opt == 100);
    }

    TEST_CASE("Self Assignment") {
        Optional<int> opt(42);
        opt = opt; // Self assignment

        REQUIRE(opt.has_value());
        CHECK(*opt == 42);
    }

    TEST_CASE("Complex Type - String") {
        Optional<String> opt;
        CHECK_FALSE(opt.has_value());

        opt = String("test");
        REQUIRE(opt.has_value());
        CHECK(opt->view() == "test");

        opt.reset();
        CHECK_FALSE(opt.has_value());
    }

} // TEST_SUITE("Optional")
