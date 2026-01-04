#include <doctest/doctest.h>

#include "datapod/pods/adapters/either.hpp"
#include <string>

using namespace datapod;

TEST_SUITE("Either") {
    TEST_CASE("Either - left construction") {
        auto e = Either<int, std::string>::left(42);
        CHECK(e.is_left());
        CHECK(!e.is_right());
        CHECK(e.left_value() == 42);
    }

    TEST_CASE("Either - right construction") {
        auto e = Either<int, std::string>::right("hello");
        CHECK(!e.is_left());
        CHECK(e.is_right());
        CHECK(e.right_value() == "hello");
    }

    TEST_CASE("Either - Left helper") {
        auto e = Left<int, std::string>(42);
        CHECK(e.is_left());
        CHECK(e.left_value() == 42);
    }

    TEST_CASE("Either - Right helper") {
        auto e = Right<int, std::string>("hello");
        CHECK(e.is_right());
        CHECK(e.right_value() == "hello");
    }

    TEST_CASE("Either - map_right on Right") {
        auto e = Either<int, int>::right(5);
        auto result = e.map_right([](int x) { return x * 2; });

        CHECK(result.is_right());
        CHECK(result.right_value() == 10);
    }

    TEST_CASE("Either - map_right on Left") {
        auto e = Either<int, int>::left(5);
        auto result = e.map_right([](int x) { return x * 2; });

        CHECK(result.is_left());
        CHECK(result.left_value() == 5);
    }

    TEST_CASE("Either - map_left on Left") {
        auto e = Either<int, int>::left(5);
        auto result = e.map_left([](int x) { return x * 2; });

        CHECK(result.is_left());
        CHECK(result.left_value() == 10);
    }

    TEST_CASE("Either - map_left on Right") {
        auto e = Either<int, int>::right(5);
        auto result = e.map_left([](int x) { return x * 2; });

        CHECK(result.is_right());
        CHECK(result.right_value() == 5);
    }

    TEST_CASE("Either - bimap on Left") {
        auto e = Either<int, int>::left(5);
        auto result = e.bimap([](int x) { return x * 2; }, [](int x) { return x + 10; });

        CHECK(result.is_left());
        CHECK(result.left_value() == 10);
    }

    TEST_CASE("Either - bimap on Right") {
        auto e = Either<int, int>::right(5);
        auto result = e.bimap([](int x) { return x * 2; }, [](int x) { return x + 10; });

        CHECK(result.is_right());
        CHECK(result.right_value() == 15);
    }

    TEST_CASE("Either - fold on Left") {
        auto e = Either<int, std::string>::left(42);
        auto result = e.fold([](int x) { return std::to_string(x); }, [](std::string const &s) { return s; });

        CHECK(result == "42");
    }

    TEST_CASE("Either - fold on Right") {
        auto e = Either<int, std::string>::right("hello");
        auto result = e.fold([](int x) { return std::to_string(x); }, [](std::string const &s) { return s; });

        CHECK(result == "hello");
    }

    TEST_CASE("Either - swap Left to Right") {
        auto e = Either<int, std::string>::left(42);
        auto swapped = e.swap();

        CHECK(swapped.is_right());
        CHECK(swapped.right_value() == 42);
    }

    TEST_CASE("Either - swap Right to Left") {
        auto e = Either<int, std::string>::right("hello");
        auto swapped = e.swap();

        CHECK(swapped.is_left());
        CHECK(swapped.left_value() == "hello");
    }

    TEST_CASE("Either - right_or with Right") {
        auto e = Either<int, std::string>::right("hello");
        auto result = e.right_or("default");

        CHECK(result == "hello");
    }

    TEST_CASE("Either - right_or with Left") {
        auto e = Either<int, std::string>::left(42);
        auto result = e.right_or("default");

        CHECK(result == "default");
    }

    TEST_CASE("Either - left_or with Left") {
        auto e = Either<int, std::string>::left(42);
        auto result = e.left_or(0);

        CHECK(result == 42);
    }

    TEST_CASE("Either - left_or with Right") {
        auto e = Either<int, std::string>::right("hello");
        auto result = e.left_or(0);

        CHECK(result == 0);
    }

    TEST_CASE("Either - inspect_right") {
        auto e = Either<int, int>::right(42);
        int inspected = 0;

        e.inspect_right([&inspected](int x) { inspected = x; });

        CHECK(inspected == 42);
    }

    TEST_CASE("Either - inspect_right on Left does nothing") {
        auto e = Either<int, int>::left(42);
        int inspected = 0;

        e.inspect_right([&inspected](int x) { inspected = x; });

        CHECK(inspected == 0);
    }

    TEST_CASE("Either - inspect_left") {
        auto e = Either<int, int>::left(42);
        int inspected = 0;

        e.inspect_left([&inspected](int x) { inspected = x; });

        CHECK(inspected == 42);
    }

    TEST_CASE("Either - inspect_left on Right does nothing") {
        auto e = Either<int, int>::right(42);
        int inspected = 0;

        e.inspect_left([&inspected](int x) { inspected = x; });

        CHECK(inspected == 0);
    }

    TEST_CASE("Either - equality") {
        auto e1 = Either<int, std::string>::left(42);
        auto e2 = Either<int, std::string>::left(42);
        auto e3 = Either<int, std::string>::left(100);
        auto e4 = Either<int, std::string>::right("hello");

        CHECK(e1 == e2);
        CHECK(e1 != e3);
        CHECK(e1 != e4);
    }

    TEST_CASE("Either - ordering") {
        auto e1 = Either<int, int>::left(10);
        auto e2 = Either<int, int>::left(20);
        auto e3 = Either<int, int>::right(5);

        CHECK(e1 < e2);
        CHECK(e1 <= e2);
        CHECK(e2 > e1);
        CHECK(e2 >= e1);

        // Left < Right in variant ordering
        CHECK(e1 < e3);
    }

    TEST_CASE("Either - map_right changes type") {
        auto e = Either<int, int>::right(42);
        auto result = e.map_right([](int x) { return std::to_string(x); });

        CHECK(result.is_right());
        CHECK(result.right_value() == "42");
    }

    TEST_CASE("Either - map_left changes type") {
        auto e = Either<int, std::string>::left(42);
        auto result = e.map_left([](int x) { return std::to_string(x); });

        CHECK(result.is_left());
        CHECK(result.left_value() == "42");
    }

    TEST_CASE("Either - chaining map_right") {
        auto e = Either<int, int>::right(5);
        auto result = e.map_right([](int x) { return x * 2; }).map_right([](int x) { return x + 3; });

        CHECK(result.is_right());
        CHECK(result.right_value() == 13);
    }

    TEST_CASE("Either - chaining inspect") {
        auto e = Either<int, int>::right(42);
        int count = 0;

        e.inspect_right([&count](int) { count++; }).inspect_right([&count](int) { count++; });

        CHECK(count == 2);
    }

    TEST_CASE("Either - move semantics") {
        struct NonCopyable {
            int value;
            NonCopyable(int v) : value(v) {}
            NonCopyable(NonCopyable const &) = delete;
            NonCopyable &operator=(NonCopyable const &) = delete;
            NonCopyable(NonCopyable &&) = default;
            NonCopyable &operator=(NonCopyable &&) = default;
        };

        auto e = Either<int, NonCopyable>::right(NonCopyable{42});
        CHECK(e.is_right());
        CHECK(e.right_value().value == 42);
    }

    TEST_CASE("Either - complex types") {
        struct Data {
            int x;
            std::string s;
        };

        auto e = Either<Data, int>::left(Data{42, "test"});
        CHECK(e.is_left());
        CHECK(e.left_value().x == 42);
        CHECK(e.left_value().s == "test");
    }

    TEST_CASE("Either - as error handling alternative") {
        auto divide = [](int a, int b) -> Either<std::string, int> {
            if (b == 0) {
                return Either<std::string, int>::left("Division by zero");
            }
            return Either<std::string, int>::right(a / b);
        };

        auto result1 = divide(10, 2);
        CHECK(result1.is_right());
        CHECK(result1.right_value() == 5);

        auto result2 = divide(10, 0);
        CHECK(result2.is_left());
        CHECK(result2.left_value() == "Division by zero");
    }

    TEST_CASE("Either - fold for unified return type") {
        auto e1 = Either<int, int>::left(42);
        auto e2 = Either<int, int>::right(100);

        auto result1 = e1.fold([](int x) { return x * 2; }, [](int x) { return x + 10; });

        auto result2 = e2.fold([](int x) { return x * 2; }, [](int x) { return x + 10; });

        CHECK(result1 == 84);
        CHECK(result2 == 110);
    }
}
