#include <doctest/doctest.h>

#include <datapod/spatial/robot/odom.hpp>

using namespace datapod;

TEST_SUITE("Odom") {
    TEST_CASE("Default construction") {
        Odom odom;
        CHECK(odom.pose.point.x == 0.0);
        CHECK(odom.pose.point.y == 0.0);
        CHECK(odom.pose.point.z == 0.0);
        CHECK(odom.pose.rotation.w == 0.0);
        CHECK(odom.twist.linear.vx == 0.0);
        CHECK(odom.twist.angular.vz == 0.0);
    }

    TEST_CASE("Aggregate initialization") {
        Pose pose{Point{1.0, 2.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
        Twist twist{Velocity{0.5, 0.0, 0.0}, Velocity{0.0, 0.0, 0.1}};
        Odom odom{pose, twist};

        CHECK(odom.pose.point.x == 1.0);
        CHECK(odom.pose.point.y == 2.0);
        CHECK(odom.twist.linear.vx == 0.5);
        CHECK(odom.twist.angular.vz == 0.1);
    }

    TEST_CASE("is_set - false for zero odometry") {
        Odom odom;
        CHECK_FALSE(odom.is_set());
    }

    TEST_CASE("is_set - true with pose") {
        Odom odom{Pose{Point{1.0, 0.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}}, Twist{}};
        CHECK(odom.is_set());
    }

    TEST_CASE("is_set - true with twist") {
        Odom odom{Pose{}, Twist{Velocity{0.5, 0.0, 0.0}, Velocity{}}};
        CHECK(odom.is_set());
    }

    TEST_CASE("operator== equality") {
        Pose pose{Point{1.0, 2.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
        Twist twist{Velocity{0.5, 0.0, 0.0}, Velocity{0.0, 0.0, 0.1}};

        Odom odom1{pose, twist};
        Odom odom2{pose, twist};
        CHECK(odom1 == odom2);
    }

    TEST_CASE("operator!= inequality - different pose") {
        Pose pose1{Point{1.0, 2.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
        Pose pose2{Point{2.0, 3.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
        Twist twist{Velocity{0.5, 0.0, 0.0}, Velocity{0.0, 0.0, 0.1}};

        Odom odom1{pose1, twist};
        Odom odom2{pose2, twist};
        CHECK(odom1 != odom2);
    }

    TEST_CASE("operator!= inequality - different twist") {
        Pose pose{Point{1.0, 2.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
        Twist twist1{Velocity{0.5, 0.0, 0.0}, Velocity{0.0, 0.0, 0.1}};
        Twist twist2{Velocity{1.0, 0.0, 0.0}, Velocity{0.0, 0.0, 0.2}};

        Odom odom1{pose, twist1};
        Odom odom2{pose, twist2};
        CHECK(odom1 != odom2);
    }

    TEST_CASE("members() reflection") {
        Odom odom;
        auto m = odom.members();
        CHECK(&std::get<0>(m) == &odom.pose);
        CHECK(&std::get<1>(m) == &odom.twist);
    }

    TEST_CASE("POD properties") {
        CHECK(std::is_standard_layout_v<Odom>);
        CHECK(std::is_trivially_copyable_v<Odom>);
    }

    TEST_CASE("Mobile robot odometry use case") {
        // Robot at position (5, 3) moving forward at 0.5 m/s and turning at 0.2 rad/s
        Odom odom{Pose{Point{5.0, 3.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}},
                  Twist{Velocity{0.5, 0.0, 0.0}, Velocity{0.0, 0.0, 0.2}}};

        CHECK(odom.pose.point.x == 5.0);
        CHECK(odom.pose.point.y == 3.0);
        CHECK(odom.twist.linear.vx == 0.5);
        CHECK(odom.twist.angular.vz == 0.2);
        CHECK(odom.is_set());
    }

    TEST_CASE("Stationary robot odometry") {
        // Robot at position (10, 5) but not moving
        Odom odom{Pose{Point{10.0, 5.0, 0.0}, Quaternion{0.7071, 0.0, 0.0, 0.7071}}, // 90 degree rotation
                  Twist{}};

        CHECK(odom.pose.point.x == 10.0);
        CHECK(odom.pose.point.y == 5.0);
        CHECK(odom.twist.linear.vx == 0.0);
        CHECK(odom.twist.angular.vz == 0.0);
        CHECK(odom.is_set()); // Still set because pose is non-zero
    }

    TEST_CASE("Odometry with 3D motion") {
        // Drone odometry with z-axis movement
        Odom odom{Pose{Point{1.0, 2.0, 5.0}, Quaternion{1.0, 0.0, 0.0, 0.0}},
                  Twist{Velocity{0.5, 0.3, 0.2}, Velocity{0.1, 0.1, 0.1}}};

        CHECK(odom.pose.point.z == 5.0);
        CHECK(odom.twist.linear.vz == 0.2);
        CHECK(odom.twist.angular.vx == 0.1);
    }
}
