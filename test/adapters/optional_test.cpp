#include <doctest/doctest.h>

#include "datapod/adapters/optional.hpp"
#include "datapod/sequential/string.hpp"

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

    // ========================================================================
    // New Monadic Operations Tests
    // ========================================================================

    TEST_CASE("is_some_and") {
        Optional<int> some(42);
        Optional<int> none;

        CHECK(some.is_some_and([](int x) { return x > 40; }));
        CHECK_FALSE(some.is_some_and([](int x) { return x < 40; }));
        CHECK_FALSE(none.is_some_and([](int) { return true; }));
    }

    TEST_CASE("is_none_or") {
        Optional<int> some(42);
        Optional<int> none;

        CHECK(none.is_none_or([](int) { return false; }));
        CHECK(some.is_none_or([](int x) { return x == 42; }));
        CHECK_FALSE(some.is_none_or([](int x) { return x != 42; }));
    }

    TEST_CASE("filter") {
        Optional<int> some(42);
        Optional<int> none;

        auto filtered1 = some.filter([](int x) { return x > 40; });
        REQUIRE(filtered1.has_value());
        CHECK(*filtered1 == 42);

        auto filtered2 = some.filter([](int x) { return x < 40; });
        CHECK_FALSE(filtered2.has_value());

        auto filtered3 = none.filter([](int) { return true; });
        CHECK_FALSE(filtered3.has_value());
    }

    TEST_CASE("inspect") {
        Optional<int> some(42);
        Optional<int> none;

        int inspected = 0;
        auto result1 = some.inspect([&](int x) { inspected = x; });
        CHECK(inspected == 42);
        REQUIRE(result1.has_value());
        CHECK(*result1 == 42);

        inspected = 0;
        auto result2 = none.inspect([&](int x) { inspected = x; });
        CHECK(inspected == 0);
        CHECK_FALSE(result2.has_value());
    }

    TEST_CASE("expect") {
        Optional<int> some(42);
        Optional<int> none;

        CHECK(some.expect("should have value") == 42);
        CHECK_THROWS_AS(none.expect("should fail"), std::runtime_error);
    }

    TEST_CASE("take") {
        Optional<int> opt(42);
        REQUIRE(opt.has_value());

        auto taken = opt.take();
        CHECK_FALSE(opt.has_value());
        REQUIRE(taken.has_value());
        CHECK(*taken == 42);

        auto taken2 = opt.take();
        CHECK_FALSE(taken2.has_value());
    }

    TEST_CASE("take_if") {
        Optional<int> opt1(42);
        auto taken1 = opt1.take_if([](int x) { return x > 40; });
        CHECK_FALSE(opt1.has_value());
        REQUIRE(taken1.has_value());
        CHECK(*taken1 == 42);

        Optional<int> opt2(30);
        auto taken2 = opt2.take_if([](int x) { return x > 40; });
        REQUIRE(opt2.has_value());
        CHECK(*opt2 == 30);
        CHECK_FALSE(taken2.has_value());
    }

    TEST_CASE("replace") {
        Optional<int> opt(42);
        auto old = opt.replace(100);
        REQUIRE(opt.has_value());
        CHECK(*opt == 100);
        REQUIRE(old.has_value());
        CHECK(*old == 42);

        Optional<int> none;
        auto old2 = none.replace(50);
        REQUIRE(none.has_value());
        CHECK(*none == 50);
        CHECK_FALSE(old2.has_value());
    }

    TEST_CASE("flatten") {
        Optional<int> inner_some(42);
        Optional<Optional<int>> nested_some(inner_some);
        auto flattened1 = nested_some.template flatten<int>();
        REQUIRE(flattened1.has_value());
        CHECK(*flattened1 == 42);

        Optional<int> inner_none;
        Optional<Optional<int>> nested_none(inner_none);
        auto flattened2 = nested_none.template flatten<int>();
        CHECK_FALSE(flattened2.has_value());

        Optional<Optional<int>> none;
        auto flattened3 = none.template flatten<int>();
        CHECK_FALSE(flattened3.has_value());
    }

    TEST_CASE("zip") {
        Optional<int> some1(42);
        Optional<String> some2(String("hello"));
        Optional<int> none;

        auto zipped1 = some1.zip(some2);
        REQUIRE(zipped1.has_value());
        CHECK(zipped1->first == 42);
        CHECK(zipped1->second.view() == "hello");

        auto zipped2 = some1.zip(Optional<String>());
        CHECK_FALSE(zipped2.has_value());

        auto zipped3 = none.zip(some2);
        CHECK_FALSE(zipped3.has_value());
    }

    TEST_CASE("zip_with") {
        Optional<int> some1(42);
        Optional<int> some2(8);

        auto result = some1.zip_with(some2, [](int a, int b) { return a + b; });
        REQUIRE(result.has_value());
        CHECK(*result == 50);

        auto result2 = some1.zip_with(Optional<int>(), [](int a, int b) { return a + b; });
        CHECK_FALSE(result2.has_value());
    }

    TEST_CASE("unwrap_or_default") {
        Optional<int> some(42);
        CHECK(some.unwrap_or_default() == 42);

        Optional<int> none;
        CHECK(none.unwrap_or_default() == 0);
    }

    TEST_CASE("get_or_insert") {
        Optional<int> opt;
        CHECK_FALSE(opt.has_value());

        int &ref = opt.get_or_insert(42);
        REQUIRE(opt.has_value());
        CHECK(*opt == 42);
        CHECK(ref == 42);

        int &ref2 = opt.get_or_insert(100);
        CHECK(*opt == 42); // Should not change
        CHECK(ref2 == 42);
    }

    TEST_CASE("get_or_insert_with") {
        Optional<int> opt;
        int call_count = 0;

        int &ref = opt.get_or_insert_with([&]() {
            call_count++;
            return 42;
        });

        REQUIRE(opt.has_value());
        CHECK(*opt == 42);
        CHECK(ref == 42);
        CHECK(call_count == 1);

        int &ref2 = opt.get_or_insert_with([&]() {
            call_count++;
            return 100;
        });

        CHECK(*opt == 42); // Should not change
        CHECK(ref2 == 42);
        CHECK(call_count == 1); // Should not be called again
    }

    TEST_CASE("copied helper") {
        int value = 42;
        Optional<int *> opt_ptr(&value);

        auto opt_val = copied(opt_ptr);
        REQUIRE(opt_val.has_value());
        CHECK(*opt_val == 42);

        Optional<int *> none_ptr;
        auto none_val = copied(none_ptr);
        CHECK_FALSE(none_val.has_value());
    }

    TEST_CASE("cloned helper") {
        int value = 42;
        Optional<int *> opt_ptr(&value);

        auto opt_val = cloned(opt_ptr);
        REQUIRE(opt_val.has_value());
        CHECK(*opt_val == 42);

        Optional<int *> none_ptr;
        auto none_val = cloned(none_ptr);
        CHECK_FALSE(none_val.has_value());
    }

} // TEST_SUITE("Optional")
