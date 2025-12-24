#include <doctest/doctest.h>

#include <datapod/spatial/obb.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction and Members
// ============================================================================

TEST_CASE("OBB - Default construction") {
    OBB obb;
    CHECK(obb.center.x == 0.0);
    CHECK(obb.center.y == 0.0);
    CHECK(obb.center.z == 0.0);
    CHECK(obb.half_extents.x == 0.0);
    CHECK(obb.half_extents.y == 0.0);
    CHECK(obb.half_extents.z == 0.0);
    CHECK(obb.orientation.roll == 0.0);
    CHECK(obb.orientation.pitch == 0.0);
    CHECK(obb.orientation.yaw == 0.0);
}

TEST_CASE("OBB - Aggregate initialization") {
    OBB obb{{5.0, 5.0, 5.0}, {2.5, 3.5, 4.5}, {0.0, 0.0, 0.0}};
    CHECK(obb.center.x == 5.0);
    CHECK(obb.center.y == 5.0);
    CHECK(obb.center.z == 5.0);
    CHECK(obb.half_extents.x == 2.5);
    CHECK(obb.half_extents.y == 3.5);
    CHECK(obb.half_extents.z == 4.5);
}

TEST_CASE("OBB - members() reflection") {
    OBB obb{{5.0, 5.0, 5.0}, {2.5, 3.5, 4.5}, {0.0, 0.0, 0.0}};
    auto m = obb.members();
    CHECK(&std::get<0>(m) == &obb.center);
    CHECK(&std::get<1>(m) == &obb.half_extents);
    CHECK(&std::get<2>(m) == &obb.orientation);
}

TEST_CASE("OBB - const members() reflection") {
    const OBB obb{{5.0, 5.0, 5.0}, {2.5, 3.5, 4.5}, {0.0, 0.0, 0.0}};
    auto m = obb.members();
    CHECK(&std::get<0>(m) == &obb.center);
    CHECK(&std::get<1>(m) == &obb.half_extents);
    CHECK(&std::get<2>(m) == &obb.orientation);
}

// ============================================================================
// TEST: Volume Calculation
// ============================================================================

TEST_CASE("OBB - volume of degenerate OBB") {
    OBB obb{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    CHECK(obb.volume() == doctest::Approx(0.0));
}

TEST_CASE("OBB - volume of unit cube (half-extents 0.5)") {
    OBB obb{{0.0, 0.0, 0.0}, {0.5, 0.5, 0.5}, {0.0, 0.0, 0.0}};
    CHECK(obb.volume() == doctest::Approx(1.0));
}

TEST_CASE("OBB - volume with half-extents (1, 1.5, 2)") {
    // Full size: 2 x 3 x 4, volume = 24
    OBB obb{{0.0, 0.0, 0.0}, {1.0, 1.5, 2.0}, {0.0, 0.0, 0.0}};
    CHECK(obb.volume() == doctest::Approx(24.0));
}

TEST_CASE("OBB - volume with half-extents (5, 5, 5)") {
    // Full size: 10 x 10 x 10, volume = 1000
    OBB obb{{5.0, 5.0, 5.0}, {5.0, 5.0, 5.0}, {0.0, 0.0, 0.0}};
    CHECK(obb.volume() == doctest::Approx(1000.0));
}

// ============================================================================
// TEST: Surface Area Calculation
// ============================================================================

TEST_CASE("OBB - surface area of degenerate OBB") {
    OBB obb{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    CHECK(obb.surface_area() == doctest::Approx(0.0));
}

TEST_CASE("OBB - surface area of unit cube (half-extents 0.5)") {
    OBB obb{{0.0, 0.0, 0.0}, {0.5, 0.5, 0.5}, {0.0, 0.0, 0.0}};
    CHECK(obb.surface_area() == doctest::Approx(6.0));
}

TEST_CASE("OBB - surface area with half-extents (1, 1, 1)") {
    // Full size: 2 x 2 x 2, surface area = 24
    OBB obb{{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}};
    CHECK(obb.surface_area() == doctest::Approx(24.0));
}

TEST_CASE("OBB - surface area with half-extents (1, 1.5, 2)") {
    // Full size: 2 x 3 x 4
    // Surface area: 2*(2*3 + 3*4 + 4*2) = 2*(6 + 12 + 8) = 52
    OBB obb{{0.0, 0.0, 0.0}, {1.0, 1.5, 2.0}, {0.0, 0.0, 0.0}};
    CHECK(obb.surface_area() == doctest::Approx(52.0));
}

// ============================================================================
// TEST: Corners Generation
// ============================================================================

TEST_CASE("OBB - corners returns 8 points") {
    OBB obb{{0.0, 0.0, 0.0}, {5.0, 5.0, 5.0}, {0.0, 0.0, 0.0}};
    auto corners = obb.corners();
    CHECK(corners.size() == 8);
}

TEST_CASE("OBB - corners of unit cube at origin (half-extents 0.5)") {
    OBB obb{{0.0, 0.0, 0.0}, {0.5, 0.5, 0.5}, {0.0, 0.0, 0.0}};
    auto corners = obb.corners();

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

TEST_CASE("OBB - corners with offset center") {
    OBB obb{{10.0, 20.0, 30.0}, {3.0, 4.0, 5.0}, {0.0, 0.0, 0.0}};
    auto corners = obb.corners();

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

TEST_CASE("OBB - corners symmetric around center") {
    OBB obb{{5.0, 5.0, 5.0}, {5.0, 5.0, 5.0}, {0.0, 0.0, 0.0}};
    auto corners = obb.corners();

    // All corners should be equidistant from center
    double dist0 = obb.center.distance_to(corners[0]);

    for (int i = 1; i < 8; ++i) {
        double dist = obb.center.distance_to(corners[i]);
        CHECK(dist == doctest::Approx(dist0));
    }
}

// ============================================================================
// TEST: Point Containment (Axis-Aligned)
// ============================================================================

TEST_CASE("OBB - contains center point") {
    OBB obb{{10.0, 10.0, 10.0}, {5.0, 5.0, 5.0}, {0.0, 0.0, 0.0}};
    CHECK(obb.contains(obb.center));
}

TEST_CASE("OBB - contains point inside") {
    OBB obb{{0.0, 0.0, 0.0}, {5.0, 5.0, 5.0}, {0.0, 0.0, 0.0}};
    Point inside{2.0, 3.0, 4.0};
    CHECK(obb.contains(inside));
}

TEST_CASE("OBB - contains point on face") {
    OBB obb{{0.0, 0.0, 0.0}, {5.0, 5.0, 5.0}, {0.0, 0.0, 0.0}};
    Point onFace{5.0, 0.0, 0.0}; // On +X face
    CHECK(obb.contains(onFace));
}

TEST_CASE("OBB - does not contain point outside X") {
    OBB obb{{0.0, 0.0, 0.0}, {5.0, 5.0, 5.0}, {0.0, 0.0, 0.0}};
    Point outside{6.0, 0.0, 0.0};
    CHECK_FALSE(obb.contains(outside));
}

TEST_CASE("OBB - does not contain point outside Y") {
    OBB obb{{0.0, 0.0, 0.0}, {5.0, 5.0, 5.0}, {0.0, 0.0, 0.0}};
    Point outside{0.0, 6.0, 0.0};
    CHECK_FALSE(obb.contains(outside));
}

TEST_CASE("OBB - does not contain point outside Z") {
    OBB obb{{0.0, 0.0, 0.0}, {5.0, 5.0, 5.0}, {0.0, 0.0, 0.0}};
    Point outside{0.0, 0.0, 6.0};
    CHECK_FALSE(obb.contains(outside));
}

TEST_CASE("OBB - does not contain far point") {
    OBB obb{{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {0.0, 0.0, 0.0}};
    Point outside{100.0, 100.0, 100.0};
    CHECK_FALSE(obb.contains(outside));
}

TEST_CASE("OBB - contains with offset center") {
    OBB obb{{20.0, 30.0, 40.0}, {5.0, 5.0, 5.0}, {0.0, 0.0, 0.0}};
    Point inside{22.0, 32.0, 42.0};
    CHECK(obb.contains(inside));
}

// ============================================================================
// TEST: Full Size Calculation
// ============================================================================

TEST_CASE("OBB - full_size from half-extents") {
    OBB obb{{0.0, 0.0, 0.0}, {5.0, 7.0, 9.0}, {0.0, 0.0, 0.0}};
    Size size = obb.full_size();
    CHECK(size.x == doctest::Approx(10.0));
    CHECK(size.y == doctest::Approx(14.0));
    CHECK(size.z == doctest::Approx(18.0));
}

TEST_CASE("OBB - full_size of unit cube") {
    OBB obb{{0.0, 0.0, 0.0}, {0.5, 0.5, 0.5}, {0.0, 0.0, 0.0}};
    Size size = obb.full_size();
    CHECK(size.x == doctest::Approx(1.0));
    CHECK(size.y == doctest::Approx(1.0));
    CHECK(size.z == doctest::Approx(1.0));
}

TEST_CASE("OBB - full_size with different extents") {
    OBB obb{{5.0, 5.0, 5.0}, {1.0, 2.0, 3.0}, {0.0, 0.0, 0.0}};
    Size size = obb.full_size();
    CHECK(size.x == doctest::Approx(2.0));
    CHECK(size.y == doctest::Approx(4.0));
    CHECK(size.z == doctest::Approx(6.0));
}

// ============================================================================
// TEST: POD Properties
// ============================================================================

TEST_CASE("OBB - is standard layout") { CHECK(std::is_standard_layout_v<OBB>); }

TEST_CASE("OBB - is trivially copyable") { CHECK(std::is_trivially_copyable_v<OBB>); }
