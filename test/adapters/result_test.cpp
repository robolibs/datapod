#include <doctest/doctest.h>

#include <datapod/pods/adapters/result.hpp>

using namespace datapod;

TEST_SUITE("Result") {
    TEST_CASE("ok() construction") {
        auto result = Result<int, Error>::ok(42);
        CHECK(result.is_ok());
        CHECK_FALSE(result.is_err());
        CHECK(result.value() == 42);
    }

    TEST_CASE("err() construction") {
        auto result = Result<int, Error>::err(Error{10, "Failed"});
        CHECK(result.is_err());
        CHECK_FALSE(result.is_ok());
        CHECK(result.error().code == 10);
        CHECK(result.error().message == "Failed");
    }

    TEST_CASE("operator bool - ok") {
        auto result = Result<int, Error>::ok(42);
        CHECK(static_cast<bool>(result));
    }

    TEST_CASE("operator bool - err") {
        auto result = Result<int, Error>::err(Error{1, "error"});
        CHECK_FALSE(static_cast<bool>(result));
    }

    TEST_CASE("value_or - ok case") {
        auto result = Result<int, Error>::ok(42);
        CHECK(result.value_or(0) == 42);
    }

    TEST_CASE("value_or - err case") {
        auto result = Result<int, Error>::err(Error{1, "error"});
        CHECK(result.value_or(99) == 99);
    }

    TEST_CASE("map - ok case") {
        auto result = Result<int, Error>::ok(5);
        auto mapped = result.map([](int x) { return x * 2; });
        CHECK(mapped.is_ok());
        CHECK(mapped.value() == 10);
    }

    TEST_CASE("map - err case") {
        auto result = Result<int, Error>::err(Error{1, "error"});
        auto mapped = result.map([](int x) { return x * 2; });
        CHECK(mapped.is_err());
        CHECK(mapped.error().code == 1);
    }

    TEST_CASE("map_err - ok case") {
        auto result = Result<int, Error>::ok(42);
        auto mapped = result.map_err([](const Error &e) { return Error{e.code + 100, "Modified"}; });
        CHECK(mapped.is_ok());
        CHECK(mapped.value() == 42);
    }

    TEST_CASE("map_err - err case") {
        auto result = Result<int, Error>::err(Error{5, "original"});
        auto mapped = result.map_err([](const Error &e) { return Error{e.code + 100, "Modified"}; });
        CHECK(mapped.is_err());
        CHECK(mapped.error().code == 105);
        CHECK(mapped.error().message == "Modified");
    }

    TEST_CASE("and_then - ok case") {
        auto divide = [](int x) -> Result<int, Error> {
            if (x == 0)
                return Result<int, Error>::err(Error::invalid_argument("div by zero"));
            return Result<int, Error>::ok(100 / x);
        };

        auto result = Result<int, Error>::ok(10);
        auto chained = result.and_then(divide);
        CHECK(chained.is_ok());
        CHECK(chained.value() == 10);
    }

    TEST_CASE("and_then - err case from first") {
        auto divide = [](int x) -> Result<int, Error> { return Result<int, Error>::ok(100 / x); };

        auto result = Result<int, Error>::err(Error{1, "first error"});
        auto chained = result.and_then(divide);
        CHECK(chained.is_err());
        CHECK(chained.error().message == "first error");
    }

    TEST_CASE("and_then - err case from second") {
        auto divide = [](int x) -> Result<int, Error> {
            if (x == 0)
                return Result<int, Error>::err(Error::invalid_argument("div by zero"));
            return Result<int, Error>::ok(100 / x);
        };

        auto result = Result<int, Error>::ok(0);
        auto chained = result.and_then(divide);
        CHECK(chained.is_err());
        CHECK(chained.error().code == Error::INVALID_ARGUMENT);
    }

    TEST_CASE("or_else - ok case") {
        auto recover = [](const Error &e) -> Result<int, Error> { return Result<int, Error>::ok(-1); };

        auto result = Result<int, Error>::ok(42);
        auto recovered = result.or_else(recover);
        CHECK(recovered.is_ok());
        CHECK(recovered.value() == 42);
    }

    TEST_CASE("or_else - err case") {
        auto recover = [](const Error &e) -> Result<int, Error> { return Result<int, Error>::ok(-1); };

        auto result = Result<int, Error>::err(Error{1, "error"});
        auto recovered = result.or_else(recover);
        CHECK(recovered.is_ok());
        CHECK(recovered.value() == -1);
    }

    TEST_CASE("Chaining multiple operations") {
        auto result = Result<int, Error>::ok(10).map([](int x) { return x * 2; }).map([](int x) { return x + 5; });

        CHECK(result.is_ok());
        CHECK(result.value() == 25);
    }

    TEST_CASE("operator== - both ok") {
        auto r1 = Result<int, Error>::ok(42);
        auto r2 = Result<int, Error>::ok(42);
        CHECK(r1 == r2);
    }

    TEST_CASE("operator== - both err") {
        auto r1 = Result<int, Error>::err(Error{1, "msg"});
        auto r2 = Result<int, Error>::err(Error{1, "msg"});
        CHECK(r1 == r2);
    }

    TEST_CASE("operator!= - ok vs err") {
        auto r1 = Result<int, Error>::ok(42);
        auto r2 = Result<int, Error>::err(Error{1, "msg"});
        CHECK(r1 != r2);
    }

    TEST_CASE("operator!= - different values") {
        auto r1 = Result<int, Error>::ok(42);
        auto r2 = Result<int, Error>::ok(43);
        CHECK(r1 != r2);
    }

    TEST_CASE("members() reflection") {
        auto result = Result<int, Error>::ok(42);
        auto m = result.members();
        CHECK(&std::get<0>(m) == &result.data);
    }

    TEST_CASE("Res<T> alias") {
        Res<int> result = Res<int>::ok(42);
        CHECK(result.is_ok());
        CHECK(result.value() == 42);
    }

    TEST_CASE("Custom error type") {
        struct MyError {
            int id;
            auto members() { return std::tie(id); }
            auto members() const { return std::tie(id); }
            bool operator==(const MyError &o) const { return id == o.id; }
        };

        auto result = Result<int, MyError>::ok(100);
        CHECK(result.is_ok());
        CHECK(result.value() == 100);

        auto err_result = Result<int, MyError>::err(MyError{999});
        CHECK(err_result.is_err());
        CHECK(err_result.error().id == 999);
    }

    TEST_CASE("Real-world example - division") {
        auto safe_divide = [](int a, int b) -> Result<int, Error> {
            if (b == 0) {
                return Result<int, Error>::err(Error::invalid_argument("Division by zero"));
            }
            return Result<int, Error>::ok(a / b);
        };

        auto result1 = safe_divide(10, 2);
        CHECK(result1.is_ok());
        CHECK(result1.value() == 5);

        auto result2 = safe_divide(10, 0);
        CHECK(result2.is_err());
        CHECK(result2.error().code == Error::INVALID_ARGUMENT);
    }

    TEST_CASE("Real-world example - file reading simulation") {
        auto read_file = [](const String &path) -> Result<String, Error> {
            if (path.empty()) {
                return Result<String, Error>::err(Error::invalid_argument("Empty path"));
            }
            if (path == "nonexistent") {
                return Result<String, Error>::err(Error::not_found("File not found"));
            }
            return Result<String, Error>::ok("file contents");
        };

        auto result1 = read_file("test.txt");
        CHECK(result1.is_ok());
        CHECK(result1.value() == "file contents");

        auto result2 = read_file("");
        CHECK(result2.is_err());
        CHECK(result2.error().code == Error::INVALID_ARGUMENT);

        auto result3 = read_file("nonexistent");
        CHECK(result3.is_err());
        CHECK(result3.error().code == Error::NOT_FOUND);
    }

    // ========================================================================
    // New Monadic Operations Tests
    // ========================================================================

    TEST_CASE("is_ok_and") {
        Result<int, Error> ok_result = Result<int, Error>::ok(42);
        Result<int, Error> err_result = Result<int, Error>::err(Error::invalid_argument("error"));

        CHECK(ok_result.is_ok_and([](int x) { return x > 40; }));
        CHECK_FALSE(ok_result.is_ok_and([](int x) { return x < 40; }));
        CHECK_FALSE(err_result.is_ok_and([](int) { return true; }));
    }

    TEST_CASE("is_err_and") {
        Result<int, Error> ok_result = Result<int, Error>::ok(42);
        Result<int, Error> err_result = Result<int, Error>::err(Error::invalid_argument("test"));

        CHECK(err_result.is_err_and([](const Error &e) { return e.code == 1; }));       // INVALID_ARGUMENT = 1
        CHECK_FALSE(err_result.is_err_and([](const Error &e) { return e.code == 3; })); // NOT_FOUND = 3
        CHECK_FALSE(ok_result.is_err_and([](const Error &) { return true; }));
    }

    TEST_CASE("inspect") {
        Result<int, Error> ok_result = Result<int, Error>::ok(42);
        Result<int, Error> err_result = Result<int, Error>::err(Error::invalid_argument("error"));

        int inspected = 0;
        auto result1 = ok_result.inspect([&](int x) { inspected = x; });
        CHECK(inspected == 42);
        CHECK(result1.is_ok());
        CHECK(result1.value() == 42);

        inspected = 0;
        auto result2 = err_result.inspect([&](int x) { inspected = x; });
        CHECK(inspected == 0);
        CHECK(result2.is_err());
    }

    TEST_CASE("inspect_err") {
        Result<int, Error> ok_result = Result<int, Error>::ok(42);
        Result<int, Error> err_result = Result<int, Error>::err(Error::invalid_argument("test error"));

        uint32_t inspected_code = 0;
        auto result1 = err_result.inspect_err([&](const Error &e) { inspected_code = e.code; });
        CHECK(inspected_code == 1); // INVALID_ARGUMENT = 1
        CHECK(result1.is_err());

        inspected_code = 99;
        auto result2 = ok_result.inspect_err([&](const Error &e) { inspected_code = e.code; });
        CHECK(inspected_code == 99); // Should not be called
        CHECK(result2.is_ok());
    }

    TEST_CASE("expect") {
        Result<int, Error> ok_result = Result<int, Error>::ok(42);
        Result<int, Error> err_result = Result<int, Error>::err(Error::invalid_argument("error"));

        CHECK(ok_result.expect("should have value") == 42);
        CHECK_THROWS_AS(err_result.expect("should fail"), std::runtime_error);
    }

    TEST_CASE("expect_err") {
        Result<int, Error> ok_result = Result<int, Error>::ok(42);
        Result<int, Error> err_result = Result<int, Error>::err(Error::invalid_argument("test"));

        CHECK(err_result.expect_err("should have error").code == 1); // INVALID_ARGUMENT = 1
        CHECK_THROWS_AS(ok_result.expect_err("should fail"), std::runtime_error);
    }

    TEST_CASE("unwrap_or") {
        Result<int, Error> ok_result = Result<int, Error>::ok(42);
        Result<int, Error> err_result = Result<int, Error>::err(Error::invalid_argument("error"));

        CHECK(ok_result.unwrap_or(100) == 42);
        CHECK(err_result.unwrap_or(100) == 100);
    }

    TEST_CASE("unwrap_or_else") {
        Result<int, Error> ok_result = Result<int, Error>::ok(42);
        Result<int, Error> err_result = Result<int, Error>::err(Error::invalid_argument("error"));

        CHECK(ok_result.unwrap_or_else([](const Error &) { return 100; }) == 42);
        CHECK(err_result.unwrap_or_else([](const Error &) { return 100; }) == 100);
    }

    TEST_CASE("unwrap_or_default") {
        Result<int, Error> ok_result = Result<int, Error>::ok(42);
        Result<int, Error> err_result = Result<int, Error>::err(Error::invalid_argument("error"));

        CHECK(ok_result.unwrap_or_default() == 42);
        CHECK(err_result.unwrap_or_default() == 0);
    }

    TEST_CASE("flatten") {
        Result<int, Error> inner_ok = Result<int, Error>::ok(42);
        Result<Result<int, Error>, Error> nested_ok = Result<Result<int, Error>, Error>::ok(inner_ok);
        auto flattened1 = nested_ok.flatten<int, Error>();
        CHECK(flattened1.is_ok());
        CHECK(flattened1.value() == 42);

        Result<int, Error> inner_err = Result<int, Error>::err(Error::invalid_argument("inner"));
        Result<Result<int, Error>, Error> nested_inner_err = Result<Result<int, Error>, Error>::ok(inner_err);
        auto flattened2 = nested_inner_err.flatten<int, Error>();
        CHECK(flattened2.is_err());
        CHECK(flattened2.error().code == 1); // INVALID_ARGUMENT = 1

        Result<Result<int, Error>, Error> nested_outer_err =
            Result<Result<int, Error>, Error>::err(Error::not_found("outer"));
        auto flattened3 = nested_outer_err.flatten<int, Error>();
        CHECK(flattened3.is_err());
        CHECK(flattened3.error().code == 3); // NOT_FOUND = 3
    }

    TEST_CASE("copied helper") {
        int value = 42;
        Result<int *, Error> ok_ptr = Result<int *, Error>::ok(&value);
        auto ok_val = copied(ok_ptr);
        CHECK(ok_val.is_ok());
        CHECK(ok_val.value() == 42);

        Result<int *, Error> err_ptr = Result<int *, Error>::err(Error::invalid_argument("error"));
        auto err_val = copied(err_ptr);
        CHECK(err_val.is_err());
        CHECK(err_val.error().code == 1); // INVALID_ARGUMENT = 1
    }

    TEST_CASE("cloned helper") {
        int value = 42;
        Result<int *, Error> ok_ptr = Result<int *, Error>::ok(&value);
        auto ok_val = cloned(ok_ptr);
        CHECK(ok_val.is_ok());
        CHECK(ok_val.value() == 42);

        Result<int *, Error> err_ptr = Result<int *, Error>::err(Error::invalid_argument("error"));
        auto err_val = cloned(err_ptr);
        CHECK(err_val.is_err());
        CHECK(err_val.error().code == 1); // INVALID_ARGUMENT = 1
    }

    TEST_CASE("copied helper") {
        int value = 42;
        Result<int *, Error> ok_ptr = Result<int *, Error>::ok(&value);
        auto ok_val = copied(ok_ptr);
        CHECK(ok_val.is_ok());
        CHECK(ok_val.value() == 42);

        Result<int *, Error> err_ptr = Result<int *, Error>::err(Error::invalid_argument("error"));
        auto err_val = copied(err_ptr);
        CHECK(err_val.is_err());
        CHECK(err_val.error().code == Error::INVALID_ARGUMENT);
    }

    TEST_CASE("cloned helper") {
        int value = 42;
        Result<int *, Error> ok_ptr = Result<int *, Error>::ok(&value);
        auto ok_val = cloned(ok_ptr);
        CHECK(ok_val.is_ok());
        CHECK(ok_val.value() == 42);

        Result<int *, Error> err_ptr = Result<int *, Error>::err(Error::invalid_argument("error"));
        auto err_val = cloned(err_ptr);
        CHECK(err_val.is_err());
        CHECK(err_val.error().code == Error::INVALID_ARGUMENT);
    }
}
