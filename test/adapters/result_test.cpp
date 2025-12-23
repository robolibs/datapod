#include <doctest/doctest.h>

#include <datapod/adapters/result.hpp>

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
}
