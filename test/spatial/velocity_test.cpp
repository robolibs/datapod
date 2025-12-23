#include <doctest/doctest.h>

#include <datapod/spatial/velocity.hpp>

using namespace datapod;

TEST_SUITE("Velocity") {
    TEST_CASE("Default construction") {
        Velocity v;
        CHECK(v.vx == 0.0);
        CHECK(v.vy == 0.0);
        CHECK(v.vz == 0.0);
    }

    TEST_CASE("Aggregate initialization") {
        Velocity v{1.5, 2.5, 3.5};
        CHECK(v.vx == 1.5);
        CHECK(v.vy == 2.5);
        CHECK(v.vz == 3.5);
    }

    TEST_CASE("speed calculation") {
        Velocity v{3.0, 4.0, 0.0};
        CHECK(v.speed() == doctest::Approx(5.0));
    }

    TEST_CASE("speed_2d calculation") {
        Velocity v{3.0, 4.0, 12.0}; // Ignore z
        CHECK(v.speed_2d() == doctest::Approx(5.0));
    }

    TEST_CASE("speed_squared") {
        Velocity v{3.0, 4.0, 0.0};
        CHECK(v.speed_squared() == 25.0);
    }

    TEST_CASE("is_set - false for zero velocity") {
        Velocity v;
        CHECK_FALSE(v.is_set());
    }

    TEST_CASE("is_set - true for non-zero velocity") {
        Velocity v{1.0, 0.0, 0.0};
        CHECK(v.is_set());
    }

    TEST_CASE("operator+ addition") {
        Velocity v1{1.0, 2.0, 3.0};
        Velocity v2{4.0, 5.0, 6.0};
        auto result = v1 + v2;
        CHECK(result.vx == 5.0);
        CHECK(result.vy == 7.0);
        CHECK(result.vz == 9.0);
    }

    TEST_CASE("operator- subtraction") {
        Velocity v1{10.0, 8.0, 6.0};
        Velocity v2{1.0, 2.0, 3.0};
        auto result = v1 - v2;
        CHECK(result.vx == 9.0);
        CHECK(result.vy == 6.0);
        CHECK(result.vz == 3.0);
    }

    TEST_CASE("operator* scaling") {
        Velocity v{1.0, 2.0, 3.0};
        auto result = v * 2.0;
        CHECK(result.vx == 2.0);
        CHECK(result.vy == 4.0);
        CHECK(result.vz == 6.0);
    }

    TEST_CASE("operator/ division") {
        Velocity v{10.0, 20.0, 30.0};
        auto result = v / 10.0;
        CHECK(result.vx == 1.0);
        CHECK(result.vy == 2.0);
        CHECK(result.vz == 3.0);
    }

    TEST_CASE("operator== equality") {
        Velocity v1{1.0, 2.0, 3.0};
        Velocity v2{1.0, 2.0, 3.0};
        CHECK(v1 == v2);
    }

    TEST_CASE("operator!= inequality") {
        Velocity v1{1.0, 2.0, 3.0};
        Velocity v2{1.0, 2.0, 4.0};
        CHECK(v1 != v2);
    }

    TEST_CASE("members() reflection") {
        Velocity v{1.0, 2.0, 3.0};
        auto m = v.members();
        CHECK(&std::get<0>(m) == &v.vx);
        CHECK(&std::get<1>(m) == &v.vy);
        CHECK(&std::get<2>(m) == &v.vz);
    }

    TEST_CASE("POD properties") {
        CHECK(std::is_standard_layout_v<Velocity>);
        CHECK(std::is_trivially_copyable_v<Velocity>);
    }
}
