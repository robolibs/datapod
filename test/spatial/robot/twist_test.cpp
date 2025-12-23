#include <doctest/doctest.h>

#include <datapod/spatial/robot/twist.hpp>

using namespace datapod;

TEST_SUITE("Twist") {
    TEST_CASE("Default construction") {
        Twist t;
        CHECK(t.linear.vx == 0.0);
        CHECK(t.linear.vy == 0.0);
        CHECK(t.linear.vz == 0.0);
        CHECK(t.angular.vx == 0.0);
        CHECK(t.angular.vy == 0.0);
        CHECK(t.angular.vz == 0.0);
    }

    TEST_CASE("Aggregate initialization") {
        Velocity lin{1.0, 0.0, 0.0};
        Velocity ang{0.0, 0.0, 0.5};
        Twist t{lin, ang};

        CHECK(t.linear.vx == 1.0);
        CHECK(t.angular.vz == 0.5);
    }

    TEST_CASE("is_set - false for zero twist") {
        Twist t;
        CHECK_FALSE(t.is_set());
    }

    TEST_CASE("is_set - true with linear velocity") {
        Twist t{Velocity{1.0, 0.0, 0.0}, Velocity{}};
        CHECK(t.is_set());
    }

    TEST_CASE("is_set - true with angular velocity") {
        Twist t{Velocity{}, Velocity{0.0, 0.0, 0.5}};
        CHECK(t.is_set());
    }

    TEST_CASE("operator== equality") {
        Twist t1{Velocity{1.0, 0.0, 0.0}, Velocity{0.0, 0.0, 0.5}};
        Twist t2{Velocity{1.0, 0.0, 0.0}, Velocity{0.0, 0.0, 0.5}};
        CHECK(t1 == t2);
    }

    TEST_CASE("operator!= inequality") {
        Twist t1{Velocity{1.0, 0.0, 0.0}, Velocity{0.0, 0.0, 0.5}};
        Twist t2{Velocity{2.0, 0.0, 0.0}, Velocity{0.0, 0.0, 0.5}};
        CHECK(t1 != t2);
    }

    TEST_CASE("members() reflection") {
        Twist t;
        auto m = t.members();
        CHECK(&std::get<0>(m) == &t.linear);
        CHECK(&std::get<1>(m) == &t.angular);
    }

    TEST_CASE("POD properties") {
        CHECK(std::is_standard_layout_v<Twist>);
        CHECK(std::is_trivially_copyable_v<Twist>);
    }

    TEST_CASE("Robot velocity command use case") {
        // Move forward at 0.5 m/s, turn at 0.2 rad/s
        Twist cmd_vel{Velocity{0.5, 0.0, 0.0}, Velocity{0.0, 0.0, 0.2}};
        CHECK(cmd_vel.linear.vx == 0.5);
        CHECK(cmd_vel.angular.vz == 0.2);
    }
}
