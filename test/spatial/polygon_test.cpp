#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <datapod/spatial/complex/polygon.hpp>

using namespace datapod;

TEST_CASE("Polygon - Default construction") {
    Polygon poly;
    CHECK(poly.vertices.empty());
}

TEST_CASE("Polygon - Aggregate initialization") {
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{0.5, 1.0, 0.0}}}};
    CHECK(poly.vertices.size() == 3);
}

TEST_CASE("Polygon - members() reflection") {
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}}}};
    auto m = poly.members();
    CHECK(&std::get<0>(m) == &poly.vertices);
}

TEST_CASE("Polygon - const members() reflection") {
    const Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}}}};
    auto m = poly.members();
    CHECK(&std::get<0>(m) == &poly.vertices);
}

TEST_CASE("Polygon - num_vertices") {
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{0.5, 1.0, 0.0}}}};
    CHECK(poly.num_vertices() == 3);
}

TEST_CASE("Polygon - is_valid - true") {
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{0.5, 1.0, 0.0}}}};
    CHECK(poly.is_valid());
}

TEST_CASE("Polygon - is_valid - false (too few vertices)") {
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}}}};
    CHECK_FALSE(poly.is_valid());
}

TEST_CASE("Polygon - empty - true") {
    Polygon poly;
    CHECK(poly.empty());
}

TEST_CASE("Polygon - empty - false") {
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}}}};
    CHECK_FALSE(poly.empty());
}

TEST_CASE("Polygon - perimeter of empty polygon") {
    Polygon poly;
    CHECK(poly.perimeter() == doctest::Approx(0.0));
}

TEST_CASE("Polygon - perimeter of single point") {
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}}}};
    CHECK(poly.perimeter() == doctest::Approx(0.0));
}

TEST_CASE("Polygon - perimeter of triangle") {
    // Right triangle with sides 3, 4, 5
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{3.0, 0.0, 0.0}, Point{0.0, 4.0, 0.0}}}};
    CHECK(poly.perimeter() == doctest::Approx(12.0)); // 3 + 4 + 5
}

TEST_CASE("Polygon - perimeter of square") {
    // Unit square
    Polygon poly{
        Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{1.0, 1.0, 0.0}, Point{0.0, 1.0, 0.0}}}};
    CHECK(poly.perimeter() == doctest::Approx(4.0));
}

TEST_CASE("Polygon - area of empty polygon") {
    Polygon poly;
    CHECK(poly.area() == doctest::Approx(0.0));
}

TEST_CASE("Polygon - area of two points") {
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}}}};
    CHECK(poly.area() == doctest::Approx(0.0));
}

TEST_CASE("Polygon - area of triangle") {
    // Right triangle with base 3, height 4 -> area = 6
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{3.0, 0.0, 0.0}, Point{0.0, 4.0, 0.0}}}};
    CHECK(poly.area() == doctest::Approx(6.0));
}

TEST_CASE("Polygon - area of unit square") {
    Polygon poly{
        Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{1.0, 1.0, 0.0}, Point{0.0, 1.0, 0.0}}}};
    CHECK(poly.area() == doctest::Approx(1.0));
}

TEST_CASE("Polygon - area of rectangle") {
    // 2x3 rectangle
    Polygon poly{
        Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{2.0, 0.0, 0.0}, Point{2.0, 3.0, 0.0}, Point{0.0, 3.0, 0.0}}}};
    CHECK(poly.area() == doctest::Approx(6.0));
}

TEST_CASE("Polygon - contains point inside triangle") {
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{3.0, 0.0, 0.0}, Point{1.5, 2.0, 0.0}}}};
    CHECK(poly.contains(Point{1.5, 0.5, 0.0}));
}

TEST_CASE("Polygon - contains point outside triangle") {
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{3.0, 0.0, 0.0}, Point{1.5, 2.0, 0.0}}}};
    CHECK_FALSE(poly.contains(Point{5.0, 5.0, 0.0}));
}

TEST_CASE("Polygon - contains point on vertex") {
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{3.0, 0.0, 0.0}, Point{1.5, 2.0, 0.0}}}};
    // Point on vertex - behavior depends on ray casting
    // Usually treated as outside or on boundary
    Point vertex_point = poly.vertices[0];
    // Just check it doesn't crash
    poly.contains(vertex_point);
}

TEST_CASE("Polygon - contains point inside square") {
    Polygon poly{
        Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{2.0, 0.0, 0.0}, Point{2.0, 2.0, 0.0}, Point{0.0, 2.0, 0.0}}}};
    CHECK(poly.contains(Point{1.0, 1.0, 0.0}));
}

TEST_CASE("Polygon - contains point outside square") {
    Polygon poly{
        Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{2.0, 0.0, 0.0}, Point{2.0, 2.0, 0.0}, Point{0.0, 2.0, 0.0}}}};
    CHECK_FALSE(poly.contains(Point{3.0, 3.0, 0.0}));
}

TEST_CASE("Polygon - contains with too few vertices") {
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}}}};
    CHECK_FALSE(poly.contains(Point{0.5, 0.0, 0.0}));
}

TEST_CASE("Polygon - get_aabb for empty polygon") {
    Polygon poly;
    AABB aabb = poly.get_aabb();
    CHECK(aabb.min_point.x == doctest::Approx(0.0));
    CHECK(aabb.min_point.y == doctest::Approx(0.0));
    CHECK(aabb.max_point.x == doctest::Approx(0.0));
    CHECK(aabb.max_point.y == doctest::Approx(0.0));
}

TEST_CASE("Polygon - get_aabb for single point") {
    Polygon poly{Vector<Point>{{Point{1.0, 2.0, 3.0}}}};
    AABB aabb = poly.get_aabb();
    CHECK(aabb.min_point.x == doctest::Approx(1.0));
    CHECK(aabb.min_point.y == doctest::Approx(2.0));
    CHECK(aabb.min_point.z == doctest::Approx(3.0));
    CHECK(aabb.max_point.x == doctest::Approx(1.0));
    CHECK(aabb.max_point.y == doctest::Approx(2.0));
    CHECK(aabb.max_point.z == doctest::Approx(3.0));
}

TEST_CASE("Polygon - get_aabb for triangle") {
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{3.0, 0.0, 0.0}, Point{1.5, 2.0, 0.0}}}};
    AABB aabb = poly.get_aabb();
    CHECK(aabb.min_point.x == doctest::Approx(0.0));
    CHECK(aabb.min_point.y == doctest::Approx(0.0));
    CHECK(aabb.max_point.x == doctest::Approx(3.0));
    CHECK(aabb.max_point.y == doctest::Approx(2.0));
}

TEST_CASE("Polygon - get_aabb for square") {
    Polygon poly{
        Vector<Point>{{Point{1.0, 1.0, 0.0}, Point{3.0, 1.0, 0.0}, Point{3.0, 3.0, 0.0}, Point{1.0, 3.0, 0.0}}}};
    AABB aabb = poly.get_aabb();
    CHECK(aabb.min_point.x == doctest::Approx(1.0));
    CHECK(aabb.min_point.y == doctest::Approx(1.0));
    CHECK(aabb.max_point.x == doctest::Approx(3.0));
    CHECK(aabb.max_point.y == doctest::Approx(3.0));
}

TEST_CASE("Polygon - get_obb for empty polygon") {
    Polygon poly;
    OBB obb = poly.get_obb();
    CHECK(obb.center.x == doctest::Approx(0.0));
    CHECK(obb.center.y == doctest::Approx(0.0));
    CHECK(obb.half_extents.x == doctest::Approx(0.0));
    CHECK(obb.half_extents.y == doctest::Approx(0.0));
}

TEST_CASE("Polygon - get_obb for square") {
    // Axis-aligned square centered at origin
    Polygon poly{
        Vector<Point>{{Point{-1.0, -1.0, 0.0}, Point{1.0, -1.0, 0.0}, Point{1.0, 1.0, 0.0}, Point{-1.0, 1.0, 0.0}}}};
    OBB obb = poly.get_obb();

    // Center should be near origin (centroid)
    CHECK(obb.center.x == doctest::Approx(0.0).epsilon(0.1));
    CHECK(obb.center.y == doctest::Approx(0.0).epsilon(0.1));

    // Half extents should be ~1.0 in each direction
    CHECK(obb.half_extents.x > 0.0);
    CHECK(obb.half_extents.y > 0.0);
}

TEST_CASE("Polygon - get_obb for triangle") {
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{4.0, 0.0, 0.0}, Point{2.0, 3.0, 0.0}}}};
    OBB obb = poly.get_obb();

    // Just verify it returns valid values
    CHECK(obb.half_extents.x > 0.0);
    CHECK(obb.half_extents.y > 0.0);
}

TEST_CASE("Polygon - iterators") {
    Polygon poly{Vector<Point>{{Point{1.0, 2.0, 0.0}, Point{3.0, 4.0, 0.0}, Point{5.0, 6.0, 0.0}}}};

    // Test begin/end
    auto it = poly.begin();
    CHECK(it->x == doctest::Approx(1.0));
    ++it;
    CHECK(it->x == doctest::Approx(3.0));

    // Test range-for
    double sum_x = 0.0;
    for (const auto &pt : poly) {
        sum_x += pt.x;
    }
    CHECK(sum_x == doctest::Approx(9.0)); // 1 + 3 + 5
}

TEST_CASE("Polygon - const iterators") {
    const Polygon poly{Vector<Point>{{Point{1.0, 2.0, 0.0}, Point{3.0, 4.0, 0.0}}}};

    double sum_y = 0.0;
    for (const auto &pt : poly) {
        sum_y += pt.y;
    }
    CHECK(sum_y == doctest::Approx(6.0)); // 2 + 4
}

TEST_CASE("Polygon - operator== for equal polygons") {
    Polygon p1{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{0.5, 1.0, 0.0}}}};
    Polygon p2{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{0.5, 1.0, 0.0}}}};
    CHECK(p1 == p2);
}

TEST_CASE("Polygon - operator== for different polygons") {
    Polygon p1{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{0.5, 1.0, 0.0}}}};
    Polygon p2{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{2.0, 0.0, 0.0}, Point{1.0, 2.0, 0.0}}}};
    CHECK_FALSE(p1 == p2);
}

TEST_CASE("Polygon - operator== for different number of vertices") {
    Polygon p1{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{0.5, 1.0, 0.0}}}};
    Polygon p2{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}}}};
    CHECK_FALSE(p1 == p2);
}

TEST_CASE("Polygon - operator!=") {
    Polygon p1{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{0.5, 1.0, 0.0}}}};
    Polygon p2{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{2.0, 0.0, 0.0}, Point{1.0, 2.0, 0.0}}}};
    CHECK(p1 != p2);
}

TEST_CASE("Polygon - complex polygon perimeter") {
    // Pentagon
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{2.0, 0.0, 0.0}, Point{2.5, 1.5, 0.0}, Point{1.0, 2.5, 0.0},
                                Point{-0.5, 1.5, 0.0}}}};
    double perimeter = poly.perimeter();
    CHECK(perimeter > 0.0);
}

TEST_CASE("Polygon - complex polygon area") {
    // Pentagon
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{2.0, 0.0, 0.0}, Point{2.5, 1.5, 0.0}, Point{1.0, 2.5, 0.0},
                                Point{-0.5, 1.5, 0.0}}}};
    double area = poly.area();
    CHECK(area > 0.0);
}

TEST_CASE("Polygon - winding order doesn't affect area") {
    // Clockwise
    Polygon p1{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{1.0, 1.0, 0.0}, Point{0.0, 1.0, 0.0}}}};

    // Counter-clockwise
    Polygon p2{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{0.0, 1.0, 0.0}, Point{1.0, 1.0, 0.0}, Point{1.0, 0.0, 0.0}}}};

    // Area should be the same (shoelace formula uses abs)
    CHECK(p1.area() == doctest::Approx(p2.area()));
}

TEST_CASE("Polygon - contains with concave polygon") {
    // L-shaped polygon (concave)
    Polygon poly{Vector<Point>{{Point{0.0, 0.0, 0.0}, Point{2.0, 0.0, 0.0}, Point{2.0, 1.0, 0.0}, Point{1.0, 1.0, 0.0},
                                Point{1.0, 2.0, 0.0}, Point{0.0, 2.0, 0.0}}}};

    // Point inside the L
    CHECK(poly.contains(Point{0.5, 0.5, 0.0}));

    // Point in the concave notch (should be outside)
    CHECK_FALSE(poly.contains(Point{1.5, 1.5, 0.0}));
}
