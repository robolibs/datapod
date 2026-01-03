#include <doctest/doctest.h>

#include "datapod/matrix/scalar.hpp"
#include "datapod/reflection/for_each_field.hpp"
#include "datapod/reflection/to_tuple.hpp"

using namespace datapod;
// Not using datapod::mat to avoid Vector conflict

TEST_SUITE("mat::scalar") {
    TEST_CASE("construction and access") {
        mat::Scalar<double> s{42.5};
        CHECK(s.value == 42.5);
        CHECK(s.get() == 42.5);

        double val = s; // implicit conversion
        CHECK(val == 42.5);
    }

    TEST_CASE("arithmetic operations") {
        mat::Scalar<double> a{10.0};
        mat::Scalar<double> b{3.0};

        auto c = a + b;
        CHECK(c.value == 13.0);

        c = a - b;
        CHECK(c.value == 7.0);

        c = a * b;
        CHECK(c.value == 30.0);

        c = a / b;
        CHECK(c.value == doctest::Approx(10.0 / 3.0));
    }

    TEST_CASE("compound assignment") {
        mat::Scalar<double> s{10.0};
        s += mat::Scalar<double>{5.0};
        CHECK(s.value == 15.0);

        s -= 3.0; // works with raw values too
        CHECK(s.value == 12.0);
    }

    TEST_CASE("comparison") {
        mat::Scalar<int> a{10};
        mat::Scalar<int> b{20};
        mat::Scalar<int> c{10};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
    }

    TEST_CASE("reflection") {
        mat::Scalar<double> s{42.5};
        auto tuple = s.members();
        CHECK(std::get<0>(tuple) == 42.5);

        auto t2 = to_tuple(s);
        CHECK(std::get<0>(t2) == 42.5);
    }

    TEST_CASE("type traits") {
        CHECK(mat::is_scalar_v<mat::Scalar<double>>);
        CHECK_FALSE(mat::is_scalar_v<double>);
        CHECK(mat::Scalar<double>::rank == 0);
    }

    TEST_CASE("POD compatibility") {
        CHECK(std::is_trivially_copyable_v<mat::Scalar<double>>);
        CHECK(std::is_trivially_copyable_v<mat::Scalar<int>>);
    }
}
