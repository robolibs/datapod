#include <doctest/doctest.h>

#include <datapod/reflection/for_each_field.hpp>
#include <datapod/reflection/to_tuple.hpp>
#include <datapod/spatial/linestring.hpp>
#include <datapod/spatial/multi/multi_linestring.hpp>
#include <datapod/spatial/multi/multi_point.hpp>
#include <datapod/spatial/multi/multi_polygon.hpp>
#include <datapod/spatial/primitives/line.hpp>
#include <datapod/spatial/ring.hpp>

using namespace datapod;

// ============================================================================
// TEST: Line (infinite line)
// ============================================================================

TEST_CASE("Line - Default Construction") {
    Line l;
    CHECK(l.origin.x == 0.0);
    CHECK(l.origin.y == 0.0);
    CHECK(l.origin.z == 0.0);
    CHECK(l.direction.x == 0.0);
    CHECK(l.direction.y == 0.0);
    CHECK(l.direction.z == 0.0);
}

TEST_CASE("Line - Construction with origin and direction") {
    Line l{{1.0, 2.0, 3.0}, {0.0, 0.0, 1.0}};
    CHECK(l.origin.x == 1.0);
    CHECK(l.origin.y == 2.0);
    CHECK(l.origin.z == 3.0);
    CHECK(l.direction.x == 0.0);
    CHECK(l.direction.y == 0.0);
    CHECK(l.direction.z == 1.0);
}

TEST_CASE("Line - members() reflection") {
    Line l{{1.0, 2.0, 3.0}, {0.0, 1.0, 0.0}};
    auto m = l.members();

    CHECK(&std::get<0>(m) == &l.origin);
    CHECK(&std::get<1>(m) == &l.direction);
}

TEST_CASE("Line - const members() reflection") {
    const Line l{{1.0, 2.0, 3.0}, {0.0, 1.0, 0.0}};
    auto m = l.members();

    CHECK(&std::get<0>(m) == &l.origin);
    CHECK(&std::get<1>(m) == &l.direction);
}

TEST_CASE("Line - is POD") {
    CHECK(std::is_standard_layout_v<Line>);
    CHECK(std::is_trivially_copyable_v<Line>);
}

// ============================================================================
// TEST: Linestring
// ============================================================================

TEST_CASE("Linestring - Default Construction") {
    Linestring ls;
    CHECK(ls.points.size() == 0);
}

TEST_CASE("Linestring - Construction with points") {
    Linestring ls;
    ls.points.push_back({1.0, 2.0, 3.0});
    ls.points.push_back({4.0, 5.0, 6.0});
    ls.points.push_back({7.0, 8.0, 9.0});

    CHECK(ls.points.size() == 3);
    CHECK(ls.points[0].x == 1.0);
    CHECK(ls.points[1].x == 4.0);
    CHECK(ls.points[2].x == 7.0);
}

TEST_CASE("Linestring - members() reflection") {
    Linestring ls;
    ls.points.push_back({1.0, 2.0, 3.0});

    auto m = ls.members();
    CHECK(&std::get<0>(m) == &ls.points);
}

TEST_CASE("Linestring - const members() reflection") {
    Linestring ls;
    ls.points.push_back({1.0, 2.0, 3.0});
    const auto &cls = ls;

    auto m = cls.members();
    CHECK(&std::get<0>(m) == &cls.points);
}

TEST_CASE("Linestring - for_each_field") {
    Linestring ls;
    ls.points.push_back({1.0, 2.0, 3.0});

    int count = 0;
    for_each_field(ls, [&count](auto &field) { count++; });
    CHECK(count == 1); // only points field
}

// ============================================================================
// TEST: Ring
// ============================================================================

TEST_CASE("Ring - Default Construction") {
    Ring r;
    CHECK(r.points.size() == 0);
}

TEST_CASE("Ring - Construction with closed points") {
    Ring r;
    r.points.push_back({0.0, 0.0, 0.0});
    r.points.push_back({1.0, 0.0, 0.0});
    r.points.push_back({1.0, 1.0, 0.0});
    r.points.push_back({0.0, 1.0, 0.0});
    r.points.push_back({0.0, 0.0, 0.0}); // Closed: first == last

    CHECK(r.points.size() == 5);
    CHECK(r.points[0].x == r.points[4].x);
    CHECK(r.points[0].y == r.points[4].y);
    CHECK(r.points[0].z == r.points[4].z);
}

TEST_CASE("Ring - members() reflection") {
    Ring r;
    r.points.push_back({0.0, 0.0, 0.0});

    auto m = r.members();
    CHECK(&std::get<0>(m) == &r.points);
}

TEST_CASE("Ring - const members() reflection") {
    Ring r;
    r.points.push_back({0.0, 0.0, 0.0});
    const auto &cr = r;

    auto m = cr.members();
    CHECK(&std::get<0>(m) == &cr.points);
}

// ============================================================================
// TEST: MultiPoint
// ============================================================================

TEST_CASE("MultiPoint - Default Construction") {
    MultiPoint mp;
    CHECK(mp.points.size() == 0);
}

TEST_CASE("MultiPoint - Construction with multiple points") {
    MultiPoint mp;
    mp.points.push_back({1.0, 2.0, 3.0});
    mp.points.push_back({4.0, 5.0, 6.0});
    mp.points.push_back({7.0, 8.0, 9.0});

    CHECK(mp.points.size() == 3);
    CHECK(mp.points[0].x == 1.0);
    CHECK(mp.points[1].y == 5.0);
    CHECK(mp.points[2].z == 9.0);
}

TEST_CASE("MultiPoint - members() reflection") {
    MultiPoint mp;
    mp.points.push_back({1.0, 2.0, 3.0});

    auto m = mp.members();
    CHECK(&std::get<0>(m) == &mp.points);
}

TEST_CASE("MultiPoint - const members() reflection") {
    MultiPoint mp;
    mp.points.push_back({1.0, 2.0, 3.0});
    const auto &cmp = mp;

    auto m = cmp.members();
    CHECK(&std::get<0>(m) == &cmp.points);
}

TEST_CASE("MultiPoint - to_tuple") {
    MultiPoint mp;
    mp.points.push_back({1.0, 2.0, 3.0});

    auto t = to_tuple(mp);
    auto &points = std::get<0>(t);
    CHECK(points.size() == 1);
}

// ============================================================================
// TEST: MultiLinestring
// ============================================================================

TEST_CASE("MultiLinestring - Default Construction") {
    MultiLinestring mls;
    CHECK(mls.linestrings.size() == 0);
}

TEST_CASE("MultiLinestring - Construction with multiple linestrings") {
    MultiLinestring mls;

    Linestring ls1;
    ls1.points.push_back({0.0, 0.0, 0.0});
    ls1.points.push_back({1.0, 1.0, 0.0});

    Linestring ls2;
    ls2.points.push_back({2.0, 2.0, 0.0});
    ls2.points.push_back({3.0, 3.0, 0.0});

    mls.linestrings.push_back(ls1);
    mls.linestrings.push_back(ls2);

    CHECK(mls.linestrings.size() == 2);
    CHECK(mls.linestrings[0].points.size() == 2);
    CHECK(mls.linestrings[1].points.size() == 2);
}

TEST_CASE("MultiLinestring - members() reflection") {
    MultiLinestring mls;

    auto m = mls.members();
    CHECK(&std::get<0>(m) == &mls.linestrings);
}

TEST_CASE("MultiLinestring - const members() reflection") {
    MultiLinestring mls;
    const auto &cmls = mls;

    auto m = cmls.members();
    CHECK(&std::get<0>(m) == &cmls.linestrings);
}

// ============================================================================
// TEST: MultiPolygon
// ============================================================================

TEST_CASE("MultiPolygon - Default Construction") {
    MultiPolygon mpoly;
    CHECK(mpoly.polygons.size() == 0);
}

TEST_CASE("MultiPolygon - Construction with multiple polygons") {
    MultiPolygon mpoly;

    Polygon poly1;
    poly1.vertices.push_back({0.0, 0.0, 0.0});
    poly1.vertices.push_back({1.0, 0.0, 0.0});
    poly1.vertices.push_back({0.0, 1.0, 0.0});

    Polygon poly2;
    poly2.vertices.push_back({2.0, 2.0, 0.0});
    poly2.vertices.push_back({3.0, 2.0, 0.0});
    poly2.vertices.push_back({2.0, 3.0, 0.0});

    mpoly.polygons.push_back(poly1);
    mpoly.polygons.push_back(poly2);

    CHECK(mpoly.polygons.size() == 2);
    CHECK(mpoly.polygons[0].vertices.size() == 3);
    CHECK(mpoly.polygons[1].vertices.size() == 3);
}

TEST_CASE("MultiPolygon - members() reflection") {
    MultiPolygon mpoly;

    auto m = mpoly.members();
    CHECK(&std::get<0>(m) == &mpoly.polygons);
}

TEST_CASE("MultiPolygon - const members() reflection") {
    MultiPolygon mpoly;
    const auto &cmpoly = mpoly;

    auto m = cmpoly.members();
    CHECK(&std::get<0>(m) == &cmpoly.polygons);
}

// ============================================================================
// TEST: POD Properties for all types
// ============================================================================

TEST_CASE("All geometry types are standard layout") {
    CHECK(std::is_standard_layout_v<Line>);
    CHECK(std::is_standard_layout_v<Linestring>);
    CHECK(std::is_standard_layout_v<Ring>);
    CHECK(std::is_standard_layout_v<MultiPoint>);
    CHECK(std::is_standard_layout_v<MultiLinestring>);
    CHECK(std::is_standard_layout_v<MultiPolygon>);
}
