#include <doctest/doctest.h>

#include <datapod/spatial/robot/accel.hpp>

using namespace datapod;

TEST_SUITE("Accel") {
    TEST_CASE("Default construction") {
        Accel a;
        CHECK(a.linear.ax == 0.0);
        CHECK(a.linear.ay == 0.0);
        CHECK(a.linear.az == 0.0);
        CHECK(a.angular.ax == 0.0);
        CHECK(a.angular.ay == 0.0);
        CHECK(a.angular.az == 0.0);
    }

    TEST_CASE("Aggregate initialization") {
        Acceleration lin{1.0, 0.0, -9.81};
        Acceleration ang{0.0, 0.0, 0.5};
        Accel a{lin, ang};

        CHECK(a.linear.ax == 1.0);
        CHECK(a.linear.az == -9.81);
        CHECK(a.angular.az == 0.5);
    }

    TEST_CASE("is_set - false for zero acceleration") {
        Accel a;
        CHECK_FALSE(a.is_set());
    }

    TEST_CASE("is_set - true with linear acceleration") {
        Accel a{Acceleration{0.0, -9.81, 0.0}, Acceleration{}};
        CHECK(a.is_set());
    }

    TEST_CASE("is_set - true with angular acceleration") {
        Accel a{Acceleration{}, Acceleration{0.0, 0.0, 1.0}};
        CHECK(a.is_set());
    }

    TEST_CASE("operator== equality") {
        Accel a1{Acceleration{1.0, 0.0, 0.0}, Acceleration{0.0, 0.0, 0.5}};
        Accel a2{Acceleration{1.0, 0.0, 0.0}, Acceleration{0.0, 0.0, 0.5}};
        CHECK(a1 == a2);
    }

    TEST_CASE("operator!= inequality") {
        Accel a1{Acceleration{1.0, 0.0, 0.0}, Acceleration{0.0, 0.0, 0.5}};
        Accel a2{Acceleration{2.0, 0.0, 0.0}, Acceleration{0.0, 0.0, 0.5}};
        CHECK(a1 != a2);
    }

    TEST_CASE("members() reflection") {
        Accel a;
        auto m = a.members();
        CHECK(&std::get<0>(m) == &a.linear);
        CHECK(&std::get<1>(m) == &a.angular);
    }

    TEST_CASE("POD properties") {
        CHECK(std::is_standard_layout_v<Accel>);
        CHECK(std::is_trivially_copyable_v<Accel>);
    }

    TEST_CASE("Dynamics use case") {
        // Linear acceleration 2 m/s², angular acceleration 0.1 rad/s²
        Accel dyn{Acceleration{2.0, 0.0, 0.0}, Acceleration{0.0, 0.0, 0.1}};
        CHECK(dyn.linear.ax == 2.0);
        CHECK(dyn.angular.az == 0.1);
    }
}
