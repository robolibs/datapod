#include <doctest/doctest.h>

#include "datapod/pods/adapters/conversions.hpp"
#include "datapod/pods/adapters/error.hpp"

using namespace datapod;

TEST_SUITE("Conversions") {

    // ========================================================================
    // Optional to Result Conversions
    // ========================================================================

    TEST_CASE("Optional::ok_or - with value") {
        Optional<int> some(42);
        auto result = some.ok_or(Error::invalid_argument("should not be used"));
        
        REQUIRE(result.is_ok());
        CHECK(result.value() == 42);
    }

    TEST_CASE("Optional::ok_or - without value") {
        Optional<int> none;
        auto result = none.ok_or(Error::not_found("value not found"));
        
        REQUIRE(result.is_err());
        CHECK(result.error().code == 3); // NOT_FOUND = 3
    }

    TEST_CASE("Optional::ok_or - move semantics") {
        Optional<int> some(42);
        auto result = std::move(some).ok_or(Error::invalid_argument("error"));
        
        REQUIRE(result.is_ok());
        CHECK(result.value() == 42);
    }

    TEST_CASE("Optional::ok_or_else - with value") {
        Optional<int> some(42);
        auto result = some.ok_or_else([]() { return Error::invalid_argument("should not be called"); });
        
        REQUIRE(result.is_ok());
        CHECK(result.value() == 42);
    }

    TEST_CASE("Optional::ok_or_else - without value") {
        Optional<int> none;
        int call_count = 0;
        auto result = none.ok_or_else([&]() {
            call_count++;
            return Error::not_found("computed error");
        });
        
        REQUIRE(result.is_err());
        CHECK(result.error().code == 3); // NOT_FOUND = 3
        CHECK(call_count == 1);
    }

    TEST_CASE("Optional::ok_or_else - lazy evaluation") {
        Optional<int> some(42);
        int call_count = 0;
        auto result = some.ok_or_else([&]() {
            call_count++;
            return Error::invalid_argument("should not be called");
        });
        
        REQUIRE(result.is_ok());
        CHECK(call_count == 0); // Function should not be called
    }

    // ========================================================================
    // Result to Optional Conversions
    // ========================================================================

    TEST_CASE("Result::ok - with Ok value") {
        Result<int, Error> result = Result<int, Error>::ok(42);
        auto opt = result.ok();
        
        REQUIRE(opt.has_value());
        CHECK(*opt == 42);
    }

    TEST_CASE("Result::ok - with Err value") {
        Result<int, Error> result = Result<int, Error>::err(Error::invalid_argument("error"));
        auto opt = result.ok();
        
        CHECK_FALSE(opt.has_value());
    }

    TEST_CASE("Result::err - with Ok value") {
        Result<int, Error> result = Result<int, Error>::ok(42);
        auto opt = result.err();
        
        CHECK_FALSE(opt.has_value());
    }

    TEST_CASE("Result::err - with Err value") {
        Result<int, Error> result = Result<int, Error>::err(Error::not_found("test"));
        auto opt = result.err();
        
        REQUIRE(opt.has_value());
        CHECK(opt->code == 3); // NOT_FOUND = 3
    }

    // ========================================================================
    // Transpose Operations
    // ========================================================================

    TEST_CASE("transpose Optional<Result> - Some(Ok)") {
        Result<int, Error> inner = Result<int, Error>::ok(42);
        Optional<Result<int, Error>> opt_result(inner);
        
        auto result_opt = transpose(opt_result);
        REQUIRE(result_opt.is_ok());
        REQUIRE(result_opt.value().has_value());
        CHECK(*result_opt.value() == 42);
    }

    TEST_CASE("transpose Optional<Result> - Some(Err)") {
        Result<int, Error> inner = Result<int, Error>::err(Error::invalid_argument("test"));
        Optional<Result<int, Error>> opt_result(inner);
        
        auto result_opt = transpose(opt_result);
        REQUIRE(result_opt.is_err());
        CHECK(result_opt.error().code == 1); // INVALID_ARGUMENT = 1
    }

    TEST_CASE("transpose Optional<Result> - None") {
        Optional<Result<int, Error>> opt_result;
        
        auto result_opt = transpose(opt_result);
        REQUIRE(result_opt.is_ok());
        CHECK_FALSE(result_opt.value().has_value());
    }

    TEST_CASE("Result::transpose - Ok(Some)") {
        Optional<int> inner(42);
        Result<Optional<int>, Error> result = Result<Optional<int>, Error>::ok(inner);
        
        auto opt_result = result.transpose<int>();
        REQUIRE(opt_result.has_value());
        REQUIRE(opt_result->is_ok());
        CHECK(opt_result->value() == 42);
    }

    TEST_CASE("Result::transpose - Ok(None)") {
        Optional<int> inner;
        Result<Optional<int>, Error> result = Result<Optional<int>, Error>::ok(inner);
        
        auto opt_result = result.transpose<int>();
        CHECK_FALSE(opt_result.has_value());
    }

    TEST_CASE("Result::transpose - Err") {
        Result<Optional<int>, Error> result = Result<Optional<int>, Error>::err(Error::not_found("test"));
        
        auto opt_result = result.transpose<int>();
        REQUIRE(opt_result.has_value());
        REQUIRE(opt_result->is_err());
        CHECK(opt_result->error().code == 3); // NOT_FOUND = 3
    }

    // ========================================================================
    // Chaining Conversions
    // ========================================================================

    TEST_CASE("Chain Optional -> Result -> Optional") {
        Optional<int> opt(42);
        auto result = opt.ok_or(Error::invalid_argument("error"));
        auto opt2 = result.ok();
        
        REQUIRE(opt2.has_value());
        CHECK(*opt2 == 42);
    }

    TEST_CASE("Chain with transformations") {
        Optional<int> opt(21);
        auto result = opt
            .ok_or(Error::invalid_argument("no value"))
            .map([](int x) { return x * 2; });
        
        REQUIRE(result.is_ok());
        CHECK(result.value() == 42);
    }

    TEST_CASE("Error propagation through conversions") {
        Optional<int> none;
        auto result = none
            .ok_or(Error::not_found("value missing"))
            .and_then([](int x) { return Result<int, Error>::ok(x * 2); });
        
        REQUIRE(result.is_err());
        CHECK(result.error().code == 3); // NOT_FOUND = 3
    }

}
