#include <doctest/doctest.h>

#include <datapod/pods/spatial/primitives/rectangle.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction and Members
// ============================================================================

TEST_CASE("Rectangle - Default construction") {
    Rectangle r;
    CHECK(r.top_left.x == 0.0);
    CHECK(r.top_left.y == 0.0);
    CHECK(r.top_right.x == 0.0);
    CHECK(r.top_right.y == 0.0);
    CHECK(r.bottom_left.x == 0.0);
    CHECK(r.bottom_left.y == 0.0);
    CHECK(r.bottom_right.x == 0.0);
    CHECK(r.bottom_right.y == 0.0);
}

TEST_CASE("Rectangle - Aggregate initialization") {
    Rectangle r{{0.0, 2.0, 0.0}, {3.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}};
    CHECK(r.top_left.x == 0.0);
    CHECK(r.top_left.y == 2.0);
    CHECK(r.top_right.x == 3.0);
    CHECK(r.top_right.y == 2.0);
    CHECK(r.bottom_left.x == 0.0);
    CHECK(r.bottom_left.y == 0.0);
    CHECK(r.bottom_right.x == 3.0);
    CHECK(r.bottom_right.y == 0.0);
}

TEST_CASE("Rectangle - members() reflection") {
    Rectangle r{{0.0, 2.0, 0.0}, {3.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}};
    auto m = r.members();
    CHECK(&std::get<0>(m) == &r.top_left);
    CHECK(&std::get<1>(m) == &r.top_right);
    CHECK(&std::get<2>(m) == &r.bottom_left);
    CHECK(&std::get<3>(m) == &r.bottom_right);
}

TEST_CASE("Rectangle - const members() reflection") {
    const Rectangle r{{0.0, 2.0, 0.0}, {3.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}};
    auto m = r.members();
    CHECK(&std::get<0>(m) == &r.top_left);
    CHECK(&std::get<1>(m) == &r.top_right);
    CHECK(&std::get<2>(m) == &r.bottom_left);
    CHECK(&std::get<3>(m) == &r.bottom_right);
}

// ============================================================================
// TEST: Area Calculation
// ============================================================================

TEST_CASE("Rectangle - area of degenerate rectangle (all points same)") {
    Rectangle r{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    CHECK(r.area() == doctest::Approx(0.0));
}

TEST_CASE("Rectangle - area of unit square") {
    Rectangle r{{0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
    CHECK(r.area() == doctest::Approx(1.0));
}

TEST_CASE("Rectangle - area of 3x2 rectangle") {
    Rectangle r{{0.0, 2.0, 0.0}, {3.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}};
    CHECK(r.area() == doctest::Approx(6.0));
}

TEST_CASE("Rectangle - area of 5x10 rectangle") {
    Rectangle r{{0.0, 10.0, 0.0}, {5.0, 10.0, 0.0}, {0.0, 0.0, 0.0}, {5.0, 0.0, 0.0}};
    CHECK(r.area() == doctest::Approx(50.0));
}

TEST_CASE("Rectangle - area with negative coordinates") {
    Rectangle r{{-2.0, 3.0, 0.0}, {2.0, 3.0, 0.0}, {-2.0, 0.0, 0.0}, {2.0, 0.0, 0.0}};
    CHECK(r.area() == doctest::Approx(12.0)); // 4 x 3
}

// ============================================================================
// TEST: Perimeter Calculation
// ============================================================================

TEST_CASE("Rectangle - perimeter of degenerate rectangle") {
    Rectangle r{{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    CHECK(r.perimeter() == doctest::Approx(0.0));
}

TEST_CASE("Rectangle - perimeter of unit square") {
    Rectangle r{{0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
    CHECK(r.perimeter() == doctest::Approx(4.0));
}

TEST_CASE("Rectangle - perimeter of 3x2 rectangle") {
    Rectangle r{{0.0, 2.0, 0.0}, {3.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}};
    CHECK(r.perimeter() == doctest::Approx(10.0)); // 2*(3+2)
}

TEST_CASE("Rectangle - perimeter of 5x10 rectangle") {
    Rectangle r{{0.0, 10.0, 0.0}, {5.0, 10.0, 0.0}, {0.0, 0.0, 0.0}, {5.0, 0.0, 0.0}};
    CHECK(r.perimeter() == doctest::Approx(30.0)); // 2*(5+10)
}

// ============================================================================
// TEST: Point Containment
// ============================================================================

TEST_CASE("Rectangle - contains bottom_left corner") {
    Rectangle r{{0.0, 2.0, 0.0}, {3.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}};
    CHECK(r.contains(r.bottom_left));
}

TEST_CASE("Rectangle - contains top_right corner") {
    Rectangle r{{0.0, 2.0, 0.0}, {3.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}};
    CHECK(r.contains(r.top_right));
}

TEST_CASE("Rectangle - contains center point") {
    Rectangle r{{0.0, 2.0, 0.0}, {4.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {4.0, 0.0, 0.0}};
    Point center{2.0, 1.0, 0.0};
    CHECK(r.contains(center));
}

TEST_CASE("Rectangle - contains point on edge") {
    Rectangle r{{0.0, 2.0, 0.0}, {3.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}};
    Point onEdge{1.5, 0.0, 0.0}; // On bottom edge
    CHECK(r.contains(onEdge));
}

TEST_CASE("Rectangle - does not contain point outside") {
    Rectangle r{{0.0, 2.0, 0.0}, {3.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}};
    Point outside{5.0, 5.0, 0.0};
    CHECK_FALSE(r.contains(outside));
}

TEST_CASE("Rectangle - does not contain point below") {
    Rectangle r{{0.0, 2.0, 0.0}, {3.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}};
    Point below{1.5, -1.0, 0.0};
    CHECK_FALSE(r.contains(below));
}

TEST_CASE("Rectangle - does not contain point to the left") {
    Rectangle r{{0.0, 2.0, 0.0}, {3.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}};
    Point left{-1.0, 1.0, 0.0};
    CHECK_FALSE(r.contains(left));
}

TEST_CASE("Rectangle - contains with negative coordinates") {
    Rectangle r{{-2.0, 2.0, 0.0}, {2.0, 2.0, 0.0}, {-2.0, -2.0, 0.0}, {2.0, -2.0, 0.0}};
    Point center{0.0, 0.0, 0.0};
    CHECK(r.contains(center));
}

// ============================================================================
// TEST: get_corners
// ============================================================================

TEST_CASE("Rectangle - get_corners returns all four corners") {
    Rectangle r{{0.0, 2.0, 0.0}, {3.0, 2.0, 0.0}, {0.0, 0.0, 0.0}, {3.0, 0.0, 0.0}};
    auto corners = r.get_corners();
    CHECK(corners.size() == 4);
    CHECK(corners[0].x == r.bottom_left.x);
    CHECK(corners[0].y == r.bottom_left.y);
    CHECK(corners[1].x == r.bottom_right.x);
    CHECK(corners[1].y == r.bottom_right.y);
    CHECK(corners[2].x == r.top_right.x);
    CHECK(corners[2].y == r.top_right.y);
    CHECK(corners[3].x == r.top_left.x);
    CHECK(corners[3].y == r.top_left.y);
}

TEST_CASE("Rectangle - get_corners maintains order") {
    Rectangle r{{1.0, 5.0, 0.0}, {4.0, 5.0, 0.0}, {1.0, 2.0, 0.0}, {4.0, 2.0, 0.0}};
    auto corners = r.get_corners();
    // Order: bottom_left, bottom_right, top_right, top_left
    CHECK(corners[0].y == 2.0); // bottom
    CHECK(corners[1].y == 2.0); // bottom
    CHECK(corners[2].y == 5.0); // top
    CHECK(corners[3].y == 5.0); // top
}

// ============================================================================
// TEST: POD Properties
// ============================================================================

TEST_CASE("Rectangle - is standard layout") { CHECK(std::is_standard_layout_v<Rectangle>); }

TEST_CASE("Rectangle - is trivially copyable") { CHECK(std::is_trivially_copyable_v<Rectangle>); }
