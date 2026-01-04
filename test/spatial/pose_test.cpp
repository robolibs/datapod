#include <doctest/doctest.h>

#include <datapod/pods/spatial/pose.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction and Members
// ============================================================================

TEST_CASE("Pose - Default construction") {
    Pose p;
    CHECK(p.point.x == 0.0);
    CHECK(p.point.y == 0.0);
    CHECK(p.point.z == 0.0);
    CHECK(p.rotation.w == 1.0); // Identity quaternion
    CHECK(p.rotation.x == 0.0);
    CHECK(p.rotation.y == 0.0);
    CHECK(p.rotation.z == 0.0);
}

TEST_CASE("Pose - Aggregate initialization") {
    Point pt{1.0, 2.0, 3.0};
    Quaternion rot{1.0, 0.0, 0.0, 0.0};
    Pose p{pt, rot};
    CHECK(p.point.x == 1.0);
    CHECK(p.point.y == 2.0);
    CHECK(p.point.z == 3.0);
    CHECK(p.rotation.w == 1.0);
}

TEST_CASE("Pose - members() reflection") {
    Pose p;
    auto m = p.members();
    CHECK(&std::get<0>(m) == &p.point);
    CHECK(&std::get<1>(m) == &p.rotation);
}

TEST_CASE("Pose - const members() reflection") {
    const Pose p{};
    auto m = p.members();
    CHECK(&std::get<0>(m) == &p.point);
    CHECK(&std::get<1>(m) == &p.rotation);
}

// ============================================================================
// TEST: Utility
// ============================================================================

TEST_CASE("Pose - is_set returns false for default") {
    Pose p;
    CHECK_FALSE(p.is_set());
}

TEST_CASE("Pose - is_set returns true with position") {
    Pose p{Point{1.0, 0.0, 0.0}, Quaternion{0.0, 0.0, 0.0, 0.0}};
    CHECK(p.is_set());
}

TEST_CASE("Pose - is_set returns true with rotation") {
    Pose p{Point{0.0, 0.0, 0.0}, Quaternion{0.707, 0.0, 0.707, 0.0}}; // Non-identity rotation
    CHECK(p.is_set());
}

// ============================================================================
// TEST: Comparison
// ============================================================================

TEST_CASE("Pose - operator== true for same values") {
    Pose p1{Point{1.0, 2.0, 3.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    Pose p2{Point{1.0, 2.0, 3.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    CHECK(p1 == p2);
}

TEST_CASE("Pose - operator== false for different values") {
    Pose p1{Point{1.0, 2.0, 3.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    Pose p2{Point{1.0, 2.0, 4.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    CHECK_FALSE(p1 == p2);
}

TEST_CASE("Pose - operator!= false for same values") {
    Pose p1{Point{1.0, 2.0, 3.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    Pose p2{Point{1.0, 2.0, 3.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    CHECK_FALSE(p1 != p2);
}

TEST_CASE("Pose - operator!= true for different values") {
    Pose p1{Point{1.0, 2.0, 3.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    Pose p2{Point{1.0, 2.0, 4.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    CHECK(p1 != p2);
}

// ============================================================================
// TEST: POD Properties
// ============================================================================

TEST_CASE("Pose - is standard layout") { CHECK(std::is_standard_layout_v<Pose>); }

TEST_CASE("Pose - is trivially copyable") { CHECK(std::is_trivially_copyable_v<Pose>); }

// ============================================================================
// TEST: Namespace Utilities
// ============================================================================

TEST_CASE("pose::make - from position and rotation") {
    Point pt{1.0, 2.0, 3.0};
    Quaternion rot{1.0, 0.0, 0.0, 0.0};

    auto p = pose::make(pt, rot);
    CHECK(p.point.x == 1.0);
    CHECK(p.point.y == 2.0);
    CHECK(p.point.z == 3.0);
    CHECK(p.rotation.w == 1.0);
}

TEST_CASE("pose::make - from coordinates and quaternion") {
    auto p = pose::make(1.0, 2.0, 3.0, 1.0, 0.0, 0.0, 0.0);
    CHECK(p.point.x == 1.0);
    CHECK(p.point.y == 2.0);
    CHECK(p.point.z == 3.0);
    CHECK(p.rotation.w == 1.0);
}

TEST_CASE("pose::make - from position only") {
    Point pt{1.0, 2.0, 3.0};

    auto p = pose::make(pt);
    CHECK(p.point.x == 1.0);
    CHECK(p.point.y == 2.0);
    CHECK(p.point.z == 3.0);
    CHECK(p.rotation.w == 1.0);
    CHECK(p.rotation.x == 0.0);
    CHECK(p.rotation.y == 0.0);
    CHECK(p.rotation.z == 0.0);
}

TEST_CASE("pose::make - from rotation only") {
    Quaternion rot{0.707, 0.0, 0.707, 0.0};

    auto p = pose::make(rot);
    CHECK(p.point.x == 0.0);
    CHECK(p.point.y == 0.0);
    CHECK(p.point.z == 0.0);
    CHECK(p.rotation.w == doctest::Approx(0.707));
    CHECK(p.rotation.y == doctest::Approx(0.707));
}

TEST_CASE("pose::identity - creates identity pose") {
    auto p = pose::identity();
    CHECK(p.point.x == 0.0);
    CHECK(p.point.y == 0.0);
    CHECK(p.point.z == 0.0);
    CHECK(p.rotation.w == 1.0);
    CHECK(p.rotation.x == 0.0);
    CHECK(p.rotation.y == 0.0);
    CHECK(p.rotation.z == 0.0);
}
