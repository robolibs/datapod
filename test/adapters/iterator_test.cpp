#include <doctest/doctest.h>

#include "datapod/pods/adapters/conversions.hpp"
#include "datapod/pods/adapters/error.hpp"
#include <algorithm>
#include <numeric>

using namespace datapod;

TEST_SUITE("Iterator Support") {

    // ========================================================================
    // Optional Iterator Tests
    // ========================================================================

    TEST_CASE("Optional - range-based for loop with value") {
        Optional<int> opt(42);
        int count = 0;
        int sum = 0;

        for (int val : opt) {
            count++;
            sum += val;
        }

        CHECK(count == 1);
        CHECK(sum == 42);
    }

    TEST_CASE("Optional - range-based for loop without value") {
        Optional<int> opt;
        int count = 0;

        for (int val : opt) {
            count++;
            (void)val;
        }

        CHECK(count == 0);
    }

    TEST_CASE("Optional - const range-based for loop") {
        const Optional<int> opt(42);
        int sum = 0;

        for (int val : opt) {
            sum += val;
        }

        CHECK(sum == 42);
    }

    TEST_CASE("Optional - begin/end iterators with value") {
        Optional<int> opt(42);

        auto it = opt.begin();
        REQUIRE(it != opt.end());
        CHECK(*it == 42);

        ++it;
        CHECK(it == opt.end());
    }

    TEST_CASE("Optional - begin/end iterators without value") {
        Optional<int> opt;

        CHECK(opt.begin() == opt.end());
        CHECK(opt.begin() == nullptr);
    }

    TEST_CASE("Optional - const iterators") {
        Optional<int> opt(42);

        auto it = opt.cbegin();
        REQUIRE(it != opt.cend());
        CHECK(*it == 42);
    }

    TEST_CASE("Optional - data() with value") {
        Optional<int> opt(42);

        int *ptr = opt.data();
        REQUIRE(ptr != nullptr);
        CHECK(*ptr == 42);

        *ptr = 100;
        CHECK(*opt == 100);
    }

    TEST_CASE("Optional - data() without value") {
        Optional<int> opt;

        CHECK(opt.data() == nullptr);
    }

    TEST_CASE("Optional - const data()") {
        const Optional<int> opt(42);

        const int *ptr = opt.data();
        REQUIRE(ptr != nullptr);
        CHECK(*ptr == 42);
    }

    TEST_CASE("Optional - std::find") {
        Optional<int> opt(42);

        auto it = std::find(opt.begin(), opt.end(), 42);
        REQUIRE(it != opt.end());
        CHECK(*it == 42);

        auto it2 = std::find(opt.begin(), opt.end(), 100);
        CHECK(it2 == opt.end());
    }

    TEST_CASE("Optional - std::accumulate") {
        Optional<int> opt(42);

        int sum = std::accumulate(opt.begin(), opt.end(), 0);
        CHECK(sum == 42);

        Optional<int> none;
        int sum2 = std::accumulate(none.begin(), none.end(), 10);
        CHECK(sum2 == 10); // Initial value unchanged
    }

    TEST_CASE("Optional - std::for_each") {
        Optional<int> opt(5);

        std::for_each(opt.begin(), opt.end(), [](int &x) { x *= 2; });
        CHECK(*opt == 10);
    }

    // ========================================================================
    // Result Iterator Tests
    // ========================================================================

    TEST_CASE("Result - range-based for loop with Ok") {
        Result<int, Error> result = Result<int, Error>::ok(42);
        int count = 0;
        int sum = 0;

        for (int val : result) {
            count++;
            sum += val;
        }

        CHECK(count == 1);
        CHECK(sum == 42);
    }

    TEST_CASE("Result - range-based for loop with Err") {
        Result<int, Error> result = Result<int, Error>::err(Error::invalid_argument("error"));
        int count = 0;

        for (int val : result) {
            count++;
            (void)val;
        }

        CHECK(count == 0);
    }

    TEST_CASE("Result - const range-based for loop") {
        const Result<int, Error> result = Result<int, Error>::ok(42);
        int sum = 0;

        for (int val : result) {
            sum += val;
        }

        CHECK(sum == 42);
    }

    TEST_CASE("Result - begin/end iterators with Ok") {
        Result<int, Error> result = Result<int, Error>::ok(42);

        auto it = result.begin();
        REQUIRE(it != result.end());
        CHECK(*it == 42);

        ++it;
        CHECK(it == result.end());
    }

    TEST_CASE("Result - begin/end iterators with Err") {
        Result<int, Error> result = Result<int, Error>::err(Error::invalid_argument("error"));

        CHECK(result.begin() == result.end());
        CHECK(result.begin() == nullptr);
    }

    TEST_CASE("Result - const iterators") {
        Result<int, Error> result = Result<int, Error>::ok(42);

        auto it = result.cbegin();
        REQUIRE(it != result.cend());
        CHECK(*it == 42);
    }

    TEST_CASE("Result - std::find") {
        Result<int, Error> result = Result<int, Error>::ok(42);

        auto it = std::find(result.begin(), result.end(), 42);
        REQUIRE(it != result.end());
        CHECK(*it == 42);

        auto it2 = std::find(result.begin(), result.end(), 100);
        CHECK(it2 == result.end());
    }

    TEST_CASE("Result - std::accumulate") {
        Result<int, Error> ok_result = Result<int, Error>::ok(42);
        int sum = std::accumulate(ok_result.begin(), ok_result.end(), 0);
        CHECK(sum == 42);

        Result<int, Error> err_result = Result<int, Error>::err(Error::invalid_argument("error"));
        int sum2 = std::accumulate(err_result.begin(), err_result.end(), 10);
        CHECK(sum2 == 10); // Initial value unchanged
    }

    TEST_CASE("Result - std::for_each") {
        Result<int, Error> result = Result<int, Error>::ok(5);

        std::for_each(result.begin(), result.end(), [](int &x) { x *= 2; });
        CHECK(result.value() == 10);
    }

    TEST_CASE("Result - mutation through iterator") {
        Result<int, Error> result = Result<int, Error>::ok(42);

        for (int &val : result) {
            val = 100;
        }

        CHECK(result.value() == 100);
    }

    // ========================================================================
    // Combined Tests
    // ========================================================================

    TEST_CASE("Chaining Optional and Result with iterators") {
        Optional<int> opt(21);
        auto result = opt.ok_or(Error::invalid_argument("no value"));

        int doubled = 0;
        for (int val : result) {
            doubled = val * 2;
        }

        CHECK(doubled == 42);
    }

    TEST_CASE("Empty iteration chain") {
        Optional<int> none;
        auto result = none.ok_or(Error::not_found("missing"));

        int count = 0;
        for (int val : result) {
            count++;
            (void)val;
        }

        CHECK(count == 0);
    }
}
