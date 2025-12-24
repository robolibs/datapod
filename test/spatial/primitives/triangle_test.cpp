#include <doctest/doctest.h>

#include <datapod/spatial/primitives/triangle.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction and Members
// ============================================================================

TEST_CASE("Triangle - Default construction") {
    Triangle t;
    CHECK(t.a.x == 0.0);
    CHECK(t.a.y == 0.0);
    CHECK(t.a.z == 0.0);
    CHECK(t.b.x == 0.0);
    CHECK(t.b.y == 0.0);
    CHECK(t.b.z == 0.0);
    CHECK(t.c.x == 0.0);
    CHECK(t.c.y == 0.0);
    CHECK(t.c.z == 0.0);
}

TEST_CASE("Triangle - Aggregate initialization") {
    Triangle t{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    CHECK(t.a.x == 0.0);
    CHECK(t.a.y == 0.0);
    CHECK(t.b.x == 1.0);
    CHECK(t.b.y == 0.0);
    CHECK(t.c.x == 0.0);
    CHECK(t.c.y == 1.0);
}

TEST_CASE("Triangle - members() reflection") {
    Triangle t{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    auto m = t.members();
    CHECK(&std::get<0>(m) == &t.a);
    CHECK(&std::get<1>(m) == &t.b);
    CHECK(&std::get<2>(m) == &t.c);
}

TEST_CASE("Triangle - const members() reflection") {
    const Triangle t{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    auto m = t.members();
    CHECK(&std::get<0>(m) == &t.a);
    CHECK(&std::get<1>(m) == &t.b);
    CHECK(&std::get<2>(m) == &t.c);
}

// ============================================================================
// TEST: Area Calculation
// ============================================================================

TEST_CASE("Triangle - area of degenerate triangle (all points same)") {
    Triangle t{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    CHECK(t.area() == doctest::Approx(0.0));
}

TEST_CASE("Triangle - area of degenerate triangle (collinear points)") {
    Triangle t{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {2.0, 0.0, 0.0}};
    CHECK(t.area() == doctest::Approx(0.0));
}

TEST_CASE("Triangle - area of right triangle (3-4-5)") {
    // Right triangle with base 3, height 4, area = 6
    Triangle t{{0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}, {0.0, 4.0, 0.0}};
    CHECK(t.area() == doctest::Approx(6.0));
}

TEST_CASE("Triangle - area of unit right triangle") {
    Triangle t{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    CHECK(t.area() == doctest::Approx(0.5));
}

TEST_CASE("Triangle - area of equilateral triangle") {
    // Equilateral triangle with side length 2
    // Area = sqrt(3)
    Triangle t{{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {1.0, std::sqrt(3.0), 0.0}};
    CHECK(t.area() == doctest::Approx(std::sqrt(3.0)));
}

TEST_CASE("Triangle - area in 3D space") {
    // Triangle in 3D (not in XY plane)
    Triangle t{{0.0, 0.0, 0.0}, {1.0, 0.0, 1.0}, {0.0, 1.0, 1.0}};
    // Should still calculate correct area
    double expected = 0.5 * std::sqrt(3.0);
    CHECK(t.area() == doctest::Approx(expected));
}

// ============================================================================
// TEST: Perimeter Calculation
// ============================================================================

TEST_CASE("Triangle - perimeter of degenerate triangle") {
    Triangle t{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    CHECK(t.perimeter() == doctest::Approx(0.0));
}

TEST_CASE("Triangle - perimeter of unit right triangle") {
    Triangle t{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    // Sides: 1, 1, sqrt(2)
    CHECK(t.perimeter() == doctest::Approx(2.0 + std::sqrt(2.0)));
}

TEST_CASE("Triangle - perimeter of 3-4-5 right triangle") {
    Triangle t{{0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}, {0.0, 4.0, 0.0}};
    CHECK(t.perimeter() == doctest::Approx(12.0)); // 3 + 4 + 5
}

TEST_CASE("Triangle - perimeter of equilateral triangle") {
    Triangle t{{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {1.0, std::sqrt(3.0), 0.0}};
    CHECK(t.perimeter() == doctest::Approx(6.0)); // 3 sides of length 2
}

// ============================================================================
// TEST: Point Containment
// ============================================================================

TEST_CASE("Triangle - contains vertex a") {
    Triangle t{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    CHECK(t.contains(t.a));
}

TEST_CASE("Triangle - contains vertex b") {
    Triangle t{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    CHECK(t.contains(t.b));
}

TEST_CASE("Triangle - contains vertex c") {
    Triangle t{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    CHECK(t.contains(t.c));
}

TEST_CASE("Triangle - contains centroid") {
    Triangle t{{0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}, {0.0, 3.0, 0.0}};
    Point centroid{1.0, 1.0, 0.0};
    CHECK(t.contains(centroid));
}

TEST_CASE("Triangle - contains point on edge") {
    Triangle t{{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {0.0, 2.0, 0.0}};
    Point midpoint{1.0, 0.0, 0.0}; // Midpoint of edge a-b
    CHECK(t.contains(midpoint));
}

TEST_CASE("Triangle - does not contain point outside") {
    Triangle t{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    Point outside{2.0, 2.0, 0.0};
    CHECK_FALSE(t.contains(outside));
}

TEST_CASE("Triangle - does not contain point far outside") {
    Triangle t{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    Point outside{-10.0, -10.0, 0.0};
    CHECK_FALSE(t.contains(outside));
}

TEST_CASE("Triangle - contains for different vertex ordering") {
    // Test with clockwise ordering
    Triangle t{{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}};
    Point inside{0.25, 0.25, 0.0};
    CHECK(t.contains(inside));
}

TEST_CASE("Triangle - containment boundary case") {
    Triangle t{{0.0, 0.0, 0.0}, {4.0, 0.0, 0.0}, {0.0, 4.0, 0.0}};
    Point onEdge{2.0, 2.0, 0.0}; // On the hypotenuse
    CHECK(t.contains(onEdge));
}

// ============================================================================
// TEST: POD Properties
// ============================================================================

TEST_CASE("Triangle - is standard layout") { CHECK(std::is_standard_layout_v<Triangle>); }

TEST_CASE("Triangle - is trivially copyable") { CHECK(std::is_trivially_copyable_v<Triangle>); }
