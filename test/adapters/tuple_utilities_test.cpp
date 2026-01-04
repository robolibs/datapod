#include <doctest/doctest.h>

#include "datapod/adapters/tuple.hpp"

using namespace datapod;

TEST_SUITE("Tuple Utilities") {

    // ========================================================================
    // swap Tests
    // ========================================================================

    TEST_CASE("Tuple - swap") {
        Tuple<int, float, double> t1(1, 2.0f, 3.0);
        Tuple<int, float, double> t2(10, 20.0f, 30.0);

        swap(t1, t2);

        CHECK(get<0>(t1) == 10);
        CHECK(get<1>(t1) == 20.0f);
        CHECK(get<2>(t1) == 30.0);

        CHECK(get<0>(t2) == 1);
        CHECK(get<1>(t2) == 2.0f);
        CHECK(get<2>(t2) == 3.0);
    }

    TEST_CASE("Tuple - swap with different types") {
        Tuple<int, const char *> t1(42, "hello");
        Tuple<int, const char *> t2(100, "world");

        swap(t1, t2);

        CHECK(get<0>(t1) == 100);
        CHECK(get<1>(t1) == std::string("world"));
        CHECK(get<0>(t2) == 42);
        CHECK(get<1>(t2) == std::string("hello"));
    }

    // ========================================================================
    // tuple_cat Tests
    // ========================================================================

    TEST_CASE("Tuple - tuple_cat with two tuples") {
        Tuple<int, float> t1(1, 2.0f);
        Tuple<double, char> t2(3.0, 'a');

        auto result = tuple_cat(t1, t2);

        CHECK(get<0>(result) == 1);
        CHECK(get<1>(result) == 2.0f);
        CHECK(get<2>(result) == 3.0);
        CHECK(get<3>(result) == 'a');
    }

    TEST_CASE("Tuple - tuple_cat with three tuples") {
        Tuple<int> t1(1);
        Tuple<float, double> t2(2.0f, 3.0);
        Tuple<char> t3('a');

        auto result = tuple_cat(t1, t2, t3);

        CHECK(get<0>(result) == 1);
        CHECK(get<1>(result) == 2.0f);
        CHECK(get<2>(result) == 3.0);
        CHECK(get<3>(result) == 'a');
    }

    // Note: Empty Tuple<> is not currently supported by the Tuple implementation
    // TEST_CASE("Tuple - tuple_cat with empty and non-empty") {
    //     Tuple<> empty;
    //     Tuple<int, float> t(42, 3.14f);
    //
    //     auto result = tuple_cat(empty, t);
    //
    //     CHECK(get<0>(result) == 42);
    //     CHECK(get<1>(result) == 3.14f);
    // }

    TEST_CASE("Tuple - tuple_cat preserves values") {
        Tuple<int, int> t1(10, 20);
        Tuple<int, int> t2(30, 40);

        auto result = tuple_cat(t1, t2);

        CHECK(get<0>(result) == 10);
        CHECK(get<1>(result) == 20);
        CHECK(get<2>(result) == 30);
        CHECK(get<3>(result) == 40);
    }

    // ========================================================================
    // make_from_tuple Tests
    // ========================================================================

    struct Point {
        int x;
        float y;
        double z;
    };

    TEST_CASE("Tuple - make_from_tuple") {
        Tuple<int, float, double> t(1, 2.0f, 3.0);
        auto point = make_from_tuple<Point>(t);

        CHECK(point.x == 1);
        CHECK(point.y == 2.0f);
        CHECK(point.z == 3.0);
    }

    struct TwoInts {
        int a;
        int b;
    };

    TEST_CASE("Tuple - make_from_tuple with same types") {
        Tuple<int, int> t(42, 100);
        auto obj = make_from_tuple<TwoInts>(t);

        CHECK(obj.a == 42);
        CHECK(obj.b == 100);
    }

    // ========================================================================
    // get by type Tests
    // ========================================================================

    TEST_CASE("Tuple - get by type (unique)") {
        Tuple<int, float, double> t(42, 3.14f, 2.718);

        CHECK(get<int>(t) == 42);
        CHECK(get<float>(t) == 3.14f);
        CHECK(get<double>(t) == 2.718);
    }

    TEST_CASE("Tuple - get by type const") {
        const Tuple<int, float, double> t(42, 3.14f, 2.718);

        CHECK(get<int>(t) == 42);
        CHECK(get<float>(t) == 3.14f);
        CHECK(get<double>(t) == 2.718);
    }

    TEST_CASE("Tuple - get by type mutation") {
        Tuple<int, float, double> t(42, 3.14f, 2.718);

        get<int>(t) = 100;
        get<float>(t) = 1.0f;

        CHECK(get<0>(t) == 100);
        CHECK(get<1>(t) == 1.0f);
    }

    TEST_CASE("Tuple - get by type with different types") {
        Tuple<char, short, int, long> t('a', 1, 2, 3);

        CHECK(get<char>(t) == 'a');
        CHECK(get<short>(t) == 1);
        CHECK(get<int>(t) == 2);
        CHECK(get<long>(t) == 3);
    }

    // ========================================================================
    // Combined Tests
    // ========================================================================

    TEST_CASE("Tuple - tuple_cat then get by type") {
        Tuple<int, float> t1(42, 3.14f);
        Tuple<double, char> t2(2.718, 'x');

        auto result = tuple_cat(t1, t2);

        CHECK(get<int>(result) == 42);
        CHECK(get<float>(result) == 3.14f);
        CHECK(get<double>(result) == 2.718);
        CHECK(get<char>(result) == 'x');
    }

    TEST_CASE("Tuple - make_from_tuple after tuple_cat") {
        Tuple<int> t1(10);
        Tuple<float, double> t2(20.0f, 30.0);

        auto combined = tuple_cat(t1, t2);
        auto point = make_from_tuple<Point>(combined);

        CHECK(point.x == 10);
        CHECK(point.y == 20.0f);
        CHECK(point.z == 30.0);
    }
}
