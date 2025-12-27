#include <doctest/doctest.h>

#include <datapod/spatial/box.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction and Members
// ============================================================================

TEST_CASE("Box - Default construction") {
    Box box;
    CHECK(box.pose.point.x == 0.0);
    CHECK(box.pose.point.y == 0.0);
    CHECK(box.pose.point.z == 0.0);
    CHECK(box.size.x == 0.0);
    CHECK(box.size.y == 0.0);
    CHECK(box.size.z == 0.0);
}

TEST_CASE("Box - Aggregate initialization") {
    Box box{{{5.0, 5.0, 5.0}, Quaternion{}}, {10.0, 20.0, 30.0}};
    CHECK(box.pose.point.x == 5.0);
    CHECK(box.pose.point.y == 5.0);
    CHECK(box.pose.point.z == 5.0);
    CHECK(box.size.x == 10.0);
    CHECK(box.size.y == 20.0);
    CHECK(box.size.z == 30.0);
}

TEST_CASE("Box - members() reflection") {
    Box box{{{5.0, 5.0, 5.0}, Quaternion{}}, {10.0, 20.0, 30.0}};
    auto m = box.members();
    CHECK(&std::get<0>(m) == &box.pose);
    CHECK(&std::get<1>(m) == &box.size);
}

TEST_CASE("Box - const members() reflection") {
    const Box box{{{5.0, 5.0, 5.0}, Quaternion{}}, {10.0, 20.0, 30.0}};
    auto m = box.members();
    CHECK(&std::get<0>(m) == &box.pose);
    CHECK(&std::get<1>(m) == &box.size);
}

// ============================================================================
// TEST: Center Point
// ============================================================================

TEST_CASE("Box - center returns pose point") {
    Box box{{{10.0, 20.0, 30.0}, Quaternion{}}, {5.0, 5.0, 5.0}};
    Point c = box.center();
    CHECK(c.x == doctest::Approx(10.0));
    CHECK(c.y == doctest::Approx(20.0));
    CHECK(c.z == doctest::Approx(30.0));
}

TEST_CASE("Box - center at origin") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {10.0, 10.0, 10.0}};
    Point c = box.center();
    CHECK(c.x == doctest::Approx(0.0));
    CHECK(c.y == doctest::Approx(0.0));
    CHECK(c.z == doctest::Approx(0.0));
}

// ============================================================================
// TEST: Volume Calculation
// ============================================================================

TEST_CASE("Box - volume of degenerate box") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {0.0, 0.0, 0.0}};
    CHECK(box.volume() == doctest::Approx(0.0));
}

TEST_CASE("Box - volume of unit cube") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {1.0, 1.0, 1.0}};
    CHECK(box.volume() == doctest::Approx(1.0));
}

TEST_CASE("Box - volume of 2x3x4 box") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {2.0, 3.0, 4.0}};
    CHECK(box.volume() == doctest::Approx(24.0));
}

TEST_CASE("Box - volume of 10x10x10 cube") {
    Box box{{{5.0, 5.0, 5.0}, Quaternion{}}, {10.0, 10.0, 10.0}};
    CHECK(box.volume() == doctest::Approx(1000.0));
}

// ============================================================================
// TEST: Surface Area Calculation
// ============================================================================

TEST_CASE("Box - surface area of degenerate box") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {0.0, 0.0, 0.0}};
    CHECK(box.surface_area() == doctest::Approx(0.0));
}

TEST_CASE("Box - surface area of unit cube") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {1.0, 1.0, 1.0}};
    CHECK(box.surface_area() == doctest::Approx(6.0));
}

TEST_CASE("Box - surface area of 2x2x2 cube") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {2.0, 2.0, 2.0}};
    CHECK(box.surface_area() == doctest::Approx(24.0));
}

TEST_CASE("Box - surface area of 2x3x4 box") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {2.0, 3.0, 4.0}};
    // 2*(2*3 + 3*4 + 4*2) = 2*(6 + 12 + 8) = 52
    CHECK(box.surface_area() == doctest::Approx(52.0));
}

// ============================================================================
// TEST: Corners Generation
// ============================================================================

TEST_CASE("Box - corners returns 8 points") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {10.0, 10.0, 10.0}};
    auto corners = box.corners();
    CHECK(corners.size() == 8);
}

TEST_CASE("Box - corners of unit cube at origin") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {1.0, 1.0, 1.0}};
    auto corners = box.corners();

    // Check corner 0: bottom-back-left (-0.5, -0.5, -0.5)
    CHECK(corners[0].x == doctest::Approx(-0.5));
    CHECK(corners[0].y == doctest::Approx(-0.5));
    CHECK(corners[0].z == doctest::Approx(-0.5));

    // Check corner 2: bottom-front-right (0.5, 0.5, -0.5)
    CHECK(corners[2].x == doctest::Approx(0.5));
    CHECK(corners[2].y == doctest::Approx(0.5));
    CHECK(corners[2].z == doctest::Approx(-0.5));

    // Check corner 6: top-front-right (0.5, 0.5, 0.5)
    CHECK(corners[6].x == doctest::Approx(0.5));
    CHECK(corners[6].y == doctest::Approx(0.5));
    CHECK(corners[6].z == doctest::Approx(0.5));
}

TEST_CASE("Box - corners of box with offset center") {
    Box box{{{10.0, 20.0, 30.0}, Quaternion{}}, {6.0, 8.0, 10.0}};
    auto corners = box.corners();

    // Center is at (10, 20, 30), half-extents are (3, 4, 5)
    // Corner 0 should be at (10-3, 20-4, 30-5) = (7, 16, 25)
    CHECK(corners[0].x == doctest::Approx(7.0));
    CHECK(corners[0].y == doctest::Approx(16.0));
    CHECK(corners[0].z == doctest::Approx(25.0));

    // Corner 6 should be at (10+3, 20+4, 30+5) = (13, 24, 35)
    CHECK(corners[6].x == doctest::Approx(13.0));
    CHECK(corners[6].y == doctest::Approx(24.0));
    CHECK(corners[6].z == doctest::Approx(35.0));
}

TEST_CASE("Box - corners symmetric around center") {
    Box box{{{5.0, 5.0, 5.0}, Quaternion{}}, {10.0, 10.0, 10.0}};
    auto corners = box.corners();

    // All corners should be equidistant from center
    Point center = box.center();
    double dist0 = center.distance_to(corners[0]);

    for (int i = 1; i < 8; ++i) {
        double dist = center.distance_to(corners[i]);
        CHECK(dist == doctest::Approx(dist0));
    }
}

// ============================================================================
// TEST: Point Containment (Axis-Aligned)
// ============================================================================

TEST_CASE("Box - contains center point") {
    Box box{{{10.0, 10.0, 10.0}, Quaternion{}}, {10.0, 10.0, 10.0}};
    CHECK(box.contains(box.center()));
}

TEST_CASE("Box - contains point inside") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {10.0, 10.0, 10.0}};
    Point inside{2.0, 3.0, 4.0};
    CHECK(box.contains(inside));
}

TEST_CASE("Box - contains point on face") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {10.0, 10.0, 10.0}};
    Point onFace{5.0, 0.0, 0.0}; // On +X face
    CHECK(box.contains(onFace));
}

TEST_CASE("Box - does not contain point outside X") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {10.0, 10.0, 10.0}};
    Point outside{6.0, 0.0, 0.0};
    CHECK_FALSE(box.contains(outside));
}

TEST_CASE("Box - does not contain point outside Y") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {10.0, 10.0, 10.0}};
    Point outside{0.0, 6.0, 0.0};
    CHECK_FALSE(box.contains(outside));
}

TEST_CASE("Box - does not contain point outside Z") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {10.0, 10.0, 10.0}};
    Point outside{0.0, 0.0, 6.0};
    CHECK_FALSE(box.contains(outside));
}

TEST_CASE("Box - does not contain far point") {
    Box box{{{0.0, 0.0, 0.0}, Quaternion{}}, {1.0, 1.0, 1.0}};
    Point outside{100.0, 100.0, 100.0};
    CHECK_FALSE(box.contains(outside));
}

TEST_CASE("Box - contains with offset center") {
    Box box{{{20.0, 30.0, 40.0}, Quaternion{}}, {10.0, 10.0, 10.0}};
    Point inside{22.0, 32.0, 42.0};
    CHECK(box.contains(inside));
}

// ============================================================================
// TEST: POD Properties
// ============================================================================

TEST_CASE("Box - is standard layout") { CHECK(std::is_standard_layout_v<Box>); }

TEST_CASE("Box - is trivially copyable") { CHECK(std::is_trivially_copyable_v<Box>); }
