#include <doctest/doctest.h>

#include <datapod/pods/spatial/acceleration.hpp>

using namespace datapod;

TEST_SUITE("Acceleration") {
    TEST_CASE("Default construction") {
        Acceleration a;
        CHECK(a.ax == 0.0);
        CHECK(a.ay == 0.0);
        CHECK(a.az == 0.0);
    }

    TEST_CASE("Aggregate initialization") {
        Acceleration a{1.5, 2.5, -9.81};
        CHECK(a.ax == 1.5);
        CHECK(a.ay == 2.5);
        CHECK(a.az == -9.81);
    }

    TEST_CASE("magnitude calculation") {
        Acceleration a{3.0, 4.0, 0.0};
        CHECK(a.magnitude() == doctest::Approx(5.0));
    }

    TEST_CASE("magnitude_2d calculation") {
        Acceleration a{3.0, 4.0, 12.0}; // Ignore z
        CHECK(a.magnitude_2d() == doctest::Approx(5.0));
    }

    TEST_CASE("magnitude_squared") {
        Acceleration a{3.0, 4.0, 0.0};
        CHECK(a.magnitude_squared() == 25.0);
    }

    TEST_CASE("is_set - false for zero acceleration") {
        Acceleration a;
        CHECK_FALSE(a.is_set());
    }

    TEST_CASE("is_set - true for non-zero acceleration") {
        Acceleration a{0.0, -9.81, 0.0}; // Gravity
        CHECK(a.is_set());
    }

    TEST_CASE("operator+ addition") {
        Acceleration a1{1.0, 2.0, 3.0};
        Acceleration a2{4.0, 5.0, 6.0};
        auto result = a1 + a2;
        CHECK(result.ax == 5.0);
        CHECK(result.ay == 7.0);
        CHECK(result.az == 9.0);
    }

    TEST_CASE("operator- subtraction") {
        Acceleration a1{10.0, 8.0, 6.0};
        Acceleration a2{1.0, 2.0, 3.0};
        auto result = a1 - a2;
        CHECK(result.ax == 9.0);
        CHECK(result.ay == 6.0);
        CHECK(result.az == 3.0);
    }

    TEST_CASE("operator* scaling") {
        Acceleration a{1.0, 2.0, 3.0};
        auto result = a * 2.0;
        CHECK(result.ax == 2.0);
        CHECK(result.ay == 4.0);
        CHECK(result.az == 6.0);
    }

    TEST_CASE("operator/ division") {
        Acceleration a{10.0, 20.0, 30.0};
        auto result = a / 10.0;
        CHECK(result.ax == 1.0);
        CHECK(result.ay == 2.0);
        CHECK(result.az == 3.0);
    }

    TEST_CASE("operator== equality") {
        Acceleration a1{1.0, 2.0, 3.0};
        Acceleration a2{1.0, 2.0, 3.0};
        CHECK(a1 == a2);
    }

    TEST_CASE("operator!= inequality") {
        Acceleration a1{1.0, 2.0, 3.0};
        Acceleration a2{1.0, 2.0, 4.0};
        CHECK(a1 != a2);
    }

    TEST_CASE("members() reflection") {
        Acceleration a{1.0, 2.0, 3.0};
        auto m = a.members();
        CHECK(&std::get<0>(m) == &a.ax);
        CHECK(&std::get<1>(m) == &a.ay);
        CHECK(&std::get<2>(m) == &a.az);
    }

    TEST_CASE("POD properties") {
        CHECK(std::is_standard_layout_v<Acceleration>);
        CHECK(std::is_trivially_copyable_v<Acceleration>);
    }
}
