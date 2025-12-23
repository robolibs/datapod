#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <datapod/spatial/state.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction and Members
// ============================================================================

TEST_CASE("State - Default construction") {
    State s;
    CHECK(s.pose.point.x == 0.0);
    CHECK(s.linear_velocity.vx == 0.0);
    CHECK(s.linear_velocity.vy == 0.0);
    CHECK(s.linear_velocity.vz == 0.0);
    CHECK(s.angular_velocity.vx == 0.0);
    CHECK(s.angular_velocity.vy == 0.0);
    CHECK(s.angular_velocity.vz == 0.0);
}

TEST_CASE("State - Aggregate initialization") {
    Pose pose{Point{1.0, 2.0, 3.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    Velocity linear_vel{5.0, 0.5, 0.1};
    Velocity angular_vel{0.1, 0.2, 0.3};
    State s{pose, linear_vel, angular_vel};
    CHECK(s.pose.point.x == 1.0);
    CHECK(s.linear_velocity.vx == 5.0);
    CHECK(s.linear_velocity.vy == 0.5);
    CHECK(s.linear_velocity.vz == 0.1);
    CHECK(s.angular_velocity.vx == 0.1);
    CHECK(s.angular_velocity.vy == 0.2);
    CHECK(s.angular_velocity.vz == 0.3);
}

TEST_CASE("State - members() reflection") {
    State s;
    auto m = s.members();
    CHECK(&std::get<0>(m) == &s.pose);
    CHECK(&std::get<1>(m) == &s.linear_velocity);
    CHECK(&std::get<2>(m) == &s.angular_velocity);
}

TEST_CASE("State - const members() reflection") {
    const State s{};
    auto m = s.members();
    CHECK(&std::get<0>(m) == &s.pose);
    CHECK(&std::get<1>(m) == &s.linear_velocity);
    CHECK(&std::get<2>(m) == &s.angular_velocity);
}

// ============================================================================
// TEST: Utility
// ============================================================================

TEST_CASE("State - is_set returns false for default") {
    State s;
    CHECK_FALSE(s.is_set());
}

TEST_CASE("State - is_set returns true with pose") {
    State s{Pose{Point{1.0, 0.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}}, Velocity{}, Velocity{}};
    CHECK(s.is_set());
}

TEST_CASE("State - is_set returns true with linear velocity") {
    State s{Pose{}, Velocity{5.0, 0.0, 0.0}, Velocity{}};
    CHECK(s.is_set());
}

TEST_CASE("State - is_set returns true with angular velocity") {
    State s{Pose{}, Velocity{}, Velocity{0.1, 0.2, 0.3}};
    CHECK(s.is_set());
}

TEST_CASE("State - is_set returns true with all fields") {
    State s{Pose{Point{1.0, 2.0, 3.0}, Quaternion{0.7071, 0.0, 0.0, 0.7071}}, Velocity{10.0, 1.5, 0.5},
            Velocity{0.1, 0.2, 0.3}};
    CHECK(s.is_set());
}

// ============================================================================
// TEST: POD Properties
// ============================================================================

TEST_CASE("State - is standard layout") { CHECK(std::is_standard_layout_v<State>); }

TEST_CASE("State - is trivially copyable") { CHECK(std::is_trivially_copyable_v<State>); }
