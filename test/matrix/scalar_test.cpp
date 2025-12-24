#include <doctest/doctest.h>

#include "datapod/matrix/scalar.hpp"
#include "datapod/reflection/for_each_field.hpp"
#include "datapod/reflection/to_tuple.hpp"

using namespace datapod;
using namespace datapod::mat;

TEST_SUITE("mat::scalar") {
    TEST_CASE("construction and access") {
        scalar<double> s{42.5};
        CHECK(s.value == 42.5);
        CHECK(s.get() == 42.5);

        double val = s; // implicit conversion
        CHECK(val == 42.5);
    }

    TEST_CASE("arithmetic operations") {
        scalar<double> a{10.0};
        scalar<double> b{3.0};

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
        scalar<double> s{10.0};
        s += scalar<double>{5.0};
        CHECK(s.value == 15.0);

        s -= 3.0; // works with raw values too
        CHECK(s.value == 12.0);
    }

    TEST_CASE("comparison") {
        scalar<int> a{10};
        scalar<int> b{20};
        scalar<int> c{10};

        CHECK(a == c);
        CHECK(a != b);
        CHECK(a < b);
        CHECK(b > a);
    }

    TEST_CASE("reflection") {
        scalar<double> s{42.5};
        auto tuple = s.members();
        CHECK(std::get<0>(tuple) == 42.5);

        auto t2 = to_tuple(s);
        CHECK(std::get<0>(t2) == 42.5);
    }

    TEST_CASE("type traits") {
        CHECK(is_scalar_v<scalar<double>>);
        CHECK_FALSE(is_scalar_v<double>);
        CHECK(scalar<double>::rank == 0);
    }

    TEST_CASE("POD compatibility") {
        CHECK(std::is_trivially_copyable_v<scalar<double>>);
        CHECK(std::is_trivially_copyable_v<scalar<int>>);
    }
}
