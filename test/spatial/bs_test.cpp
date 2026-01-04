#include <doctest/doctest.h>

#include <datapod/pods/spatial/bs.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction and Members
// ============================================================================

TEST_CASE("BS - Default construction") {
    BS bs;
    CHECK(bs.center.x == 0.0);
    CHECK(bs.center.y == 0.0);
    CHECK(bs.center.z == 0.0);
    CHECK(bs.radius == 0.0);
}

TEST_CASE("BS - Aggregate initialization") {
    BS bs{{5.0, 5.0, 5.0}, 10.0};
    CHECK(bs.center.x == 5.0);
    CHECK(bs.center.y == 5.0);
    CHECK(bs.center.z == 5.0);
    CHECK(bs.radius == 10.0);
}

TEST_CASE("BS - members() reflection") {
    BS bs{{5.0, 5.0, 5.0}, 10.0};
    auto m = bs.members();
    CHECK(&std::get<0>(m) == &bs.center);
    CHECK(&std::get<1>(m) == &bs.radius);
}

TEST_CASE("BS - const members() reflection") {
    const BS bs{{5.0, 5.0, 5.0}, 10.0};
    auto m = bs.members();
    CHECK(&std::get<0>(m) == &bs.center);
    CHECK(&std::get<1>(m) == &bs.radius);
}

// ============================================================================
// TEST: Volume Calculation
// ============================================================================

TEST_CASE("BS - volume of zero radius") {
    BS bs{{0.0, 0.0, 0.0}, 0.0};
    CHECK(bs.volume() == doctest::Approx(0.0));
}

TEST_CASE("BS - volume of unit sphere") {
    BS bs{{0.0, 0.0, 0.0}, 1.0};
    // Volume = 4/3 * π * r³ = 4/3 * π ≈ 4.18879
    CHECK(bs.volume() == doctest::Approx(4.18879).epsilon(0.0001));
}

TEST_CASE("BS - volume of radius 2") {
    BS bs{{0.0, 0.0, 0.0}, 2.0};
    // Volume = 4/3 * π * 8 = 32π/3 ≈ 33.5103
    CHECK(bs.volume() == doctest::Approx(33.5103).epsilon(0.0001));
}

TEST_CASE("BS - volume of radius 5") {
    BS bs{{5.0, 5.0, 5.0}, 5.0};
    // Volume = 4/3 * π * 125 = 500π/3 ≈ 523.599
    CHECK(bs.volume() == doctest::Approx(523.599).epsilon(0.001));
}

// ============================================================================
// TEST: Surface Area Calculation
// ============================================================================

TEST_CASE("BS - surface area of zero radius") {
    BS bs{{0.0, 0.0, 0.0}, 0.0};
    CHECK(bs.surface_area() == doctest::Approx(0.0));
}

TEST_CASE("BS - surface area of unit sphere") {
    BS bs{{0.0, 0.0, 0.0}, 1.0};
    // Surface area = 4 * π * r² = 4π ≈ 12.5664
    CHECK(bs.surface_area() == doctest::Approx(12.5664).epsilon(0.0001));
}

TEST_CASE("BS - surface area of radius 2") {
    BS bs{{0.0, 0.0, 0.0}, 2.0};
    // Surface area = 4 * π * 4 = 16π ≈ 50.2655
    CHECK(bs.surface_area() == doctest::Approx(50.2655).epsilon(0.0001));
}

TEST_CASE("BS - surface area of radius 5") {
    BS bs{{10.0, 10.0, 10.0}, 5.0};
    // Surface area = 4 * π * 25 = 100π ≈ 314.159
    CHECK(bs.surface_area() == doctest::Approx(314.159).epsilon(0.001));
}

// ============================================================================
// TEST: Point Containment
// ============================================================================

TEST_CASE("BS - contains center point") {
    BS bs{{5.0, 5.0, 5.0}, 10.0};
    CHECK(bs.contains(bs.center));
}

TEST_CASE("BS - contains point inside") {
    BS bs{{0.0, 0.0, 0.0}, 10.0};
    Point inside{3.0, 4.0, 0.0}; // Distance = 5
    CHECK(bs.contains(inside));
}

TEST_CASE("BS - contains point on surface") {
    BS bs{{0.0, 0.0, 0.0}, 5.0};
    Point onSurface{3.0, 4.0, 0.0}; // Distance = 5 exactly
    CHECK(bs.contains(onSurface));
}

TEST_CASE("BS - does not contain point outside") {
    BS bs{{0.0, 0.0, 0.0}, 5.0};
    Point outside{10.0, 0.0, 0.0};
    CHECK_FALSE(bs.contains(outside));
}

TEST_CASE("BS - does not contain point far outside") {
    BS bs{{0.0, 0.0, 0.0}, 1.0};
    Point outside{100.0, 100.0, 100.0};
    CHECK_FALSE(bs.contains(outside));
}

TEST_CASE("BS - contains in 3D space") {
    BS bs{{5.0, 5.0, 5.0}, 10.0};
    Point inside{5.0, 5.0, 10.0}; // Distance = 5
    CHECK(bs.contains(inside));
}

// ============================================================================
// TEST: Sphere Intersection
// ============================================================================

TEST_CASE("BS - intersects with itself") {
    BS bs{{0.0, 0.0, 0.0}, 10.0};
    CHECK(bs.intersects(bs));
}

TEST_CASE("BS - intersects with overlapping sphere") {
    BS bs1{{0.0, 0.0, 0.0}, 10.0};
    BS bs2{{5.0, 0.0, 0.0}, 10.0};
    CHECK(bs1.intersects(bs2));
    CHECK(bs2.intersects(bs1));
}

TEST_CASE("BS - intersects with contained sphere") {
    BS outer{{0.0, 0.0, 0.0}, 10.0};
    BS inner{{0.0, 0.0, 0.0}, 5.0};
    CHECK(outer.intersects(inner));
    CHECK(inner.intersects(outer));
}

TEST_CASE("BS - does not intersect separated spheres") {
    BS bs1{{0.0, 0.0, 0.0}, 5.0};
    BS bs2{{20.0, 0.0, 0.0}, 5.0};
    CHECK_FALSE(bs1.intersects(bs2));
    CHECK_FALSE(bs2.intersects(bs1));
}

TEST_CASE("BS - intersects touching spheres") {
    BS bs1{{0.0, 0.0, 0.0}, 5.0};
    BS bs2{{10.0, 0.0, 0.0}, 5.0}; // Exactly touching
    CHECK(bs1.intersects(bs2));
}

TEST_CASE("BS - intersects in 3D space") {
    BS bs1{{0.0, 0.0, 0.0}, 10.0};
    BS bs2{{10.0, 10.0, 10.0}, 10.0};
    // Distance between centers: sqrt(300) ≈ 17.32, combined radius: 20
    CHECK(bs1.intersects(bs2));
}

// ============================================================================
// TEST: AABB Generation
// ============================================================================

TEST_CASE("BS - get_aabb of zero radius") {
    BS bs{{5.0, 5.0, 5.0}, 0.0};
    AABB aabb = bs.get_aabb();
    CHECK(aabb.min_point.x == doctest::Approx(5.0));
    CHECK(aabb.min_point.y == doctest::Approx(5.0));
    CHECK(aabb.min_point.z == doctest::Approx(5.0));
    CHECK(aabb.max_point.x == doctest::Approx(5.0));
    CHECK(aabb.max_point.y == doctest::Approx(5.0));
    CHECK(aabb.max_point.z == doctest::Approx(5.0));
}

TEST_CASE("BS - get_aabb of unit sphere at origin") {
    BS bs{{0.0, 0.0, 0.0}, 1.0};
    AABB aabb = bs.get_aabb();
    CHECK(aabb.min_point.x == doctest::Approx(-1.0));
    CHECK(aabb.min_point.y == doctest::Approx(-1.0));
    CHECK(aabb.min_point.z == doctest::Approx(-1.0));
    CHECK(aabb.max_point.x == doctest::Approx(1.0));
    CHECK(aabb.max_point.y == doctest::Approx(1.0));
    CHECK(aabb.max_point.z == doctest::Approx(1.0));
}

TEST_CASE("BS - get_aabb with offset center") {
    BS bs{{10.0, 20.0, 30.0}, 5.0};
    AABB aabb = bs.get_aabb();
    CHECK(aabb.min_point.x == doctest::Approx(5.0));
    CHECK(aabb.min_point.y == doctest::Approx(15.0));
    CHECK(aabb.min_point.z == doctest::Approx(25.0));
    CHECK(aabb.max_point.x == doctest::Approx(15.0));
    CHECK(aabb.max_point.y == doctest::Approx(25.0));
    CHECK(aabb.max_point.z == doctest::Approx(35.0));
}

// ============================================================================
// TEST: Expand with Point
// ============================================================================

TEST_CASE("BS - expand with point inside does not change radius") {
    BS bs{{0.0, 0.0, 0.0}, 10.0};
    Point inside{3.0, 4.0, 0.0}; // Distance = 5
    bs.expand(inside);
    CHECK(bs.radius == doctest::Approx(10.0));
}

TEST_CASE("BS - expand with point outside increases radius") {
    BS bs{{0.0, 0.0, 0.0}, 5.0};
    Point outside{10.0, 0.0, 0.0}; // Distance = 10
    bs.expand(outside);
    CHECK(bs.radius == doctest::Approx(10.0));
}

TEST_CASE("BS - expand with far point") {
    BS bs{{0.0, 0.0, 0.0}, 1.0};
    Point far{20.0, 0.0, 0.0};
    bs.expand(far);
    CHECK(bs.radius == doctest::Approx(20.0));
}

// ============================================================================
// TEST: Expand with Sphere
// ============================================================================

TEST_CASE("BS - expand with contained sphere does not change radius") {
    BS bs{{0.0, 0.0, 0.0}, 10.0};
    BS inner{{0.0, 0.0, 0.0}, 5.0};
    bs.expand(inner);
    CHECK(bs.radius == doctest::Approx(10.0));
}

TEST_CASE("BS - expand with overlapping sphere increases radius") {
    BS bs{{0.0, 0.0, 0.0}, 5.0};
    BS other{{10.0, 0.0, 0.0}, 8.0};
    // Distance = 10, new radius = 10 + 8 = 18
    bs.expand(other);
    CHECK(bs.radius == doctest::Approx(18.0));
}

TEST_CASE("BS - expand with non-overlapping sphere") {
    BS bs{{0.0, 0.0, 0.0}, 2.0};
    BS other{{20.0, 0.0, 0.0}, 5.0};
    // Distance = 20, new radius = 20 + 5 = 25
    bs.expand(other);
    CHECK(bs.radius == doctest::Approx(25.0));
}

// ============================================================================
// TEST: Diameter
// ============================================================================

TEST_CASE("BS - diameter of zero radius") {
    BS bs{{0.0, 0.0, 0.0}, 0.0};
    CHECK(bs.diameter() == doctest::Approx(0.0));
}

TEST_CASE("BS - diameter of unit sphere") {
    BS bs{{0.0, 0.0, 0.0}, 1.0};
    CHECK(bs.diameter() == doctest::Approx(2.0));
}

TEST_CASE("BS - diameter of radius 5") {
    BS bs{{10.0, 10.0, 10.0}, 5.0};
    CHECK(bs.diameter() == doctest::Approx(10.0));
}

TEST_CASE("BS - diameter of radius 7.5") {
    BS bs{{0.0, 0.0, 0.0}, 7.5};
    CHECK(bs.diameter() == doctest::Approx(15.0));
}

// ============================================================================
// TEST: POD Properties
// ============================================================================

TEST_CASE("BS - is standard layout") { CHECK(std::is_standard_layout_v<BS>); }

TEST_CASE("BS - is trivially copyable") { CHECK(std::is_trivially_copyable_v<BS>); }
