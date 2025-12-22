#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <datapod/reflection/for_each_field.hpp>
#include <datapod/reflection/to_tuple.hpp>
#include <datapod/spatial/primitives/segment.hpp>

using namespace datapod;

// ============================================================================
// TEST: Construction
// ============================================================================

TEST_CASE("Segment - Default Construction") {
    Segment s;
    CHECK(s.start.x == 0.0);
    CHECK(s.start.y == 0.0);
    CHECK(s.start.z == 0.0);
    CHECK(s.end.x == 0.0);
    CHECK(s.end.y == 0.0);
    CHECK(s.end.z == 0.0);
}

TEST_CASE("Segment - Construction with points") {
    Segment s{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
    CHECK(s.start.x == 1.0);
    CHECK(s.start.y == 2.0);
    CHECK(s.start.z == 3.0);
    CHECK(s.end.x == 4.0);
    CHECK(s.end.y == 5.0);
    CHECK(s.end.z == 6.0);
}

// ============================================================================
// TEST: Reflection
// ============================================================================

TEST_CASE("Segment - members() reflection") {
    Segment s{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
    auto m = s.members();

    CHECK(&std::get<0>(m) == &s.start);
    CHECK(&std::get<1>(m) == &s.end);
}

TEST_CASE("Segment - const members() reflection") {
    const Segment s{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
    auto m = s.members();

    CHECK(&std::get<0>(m) == &s.start);
    CHECK(&std::get<1>(m) == &s.end);
}

TEST_CASE("Segment - to_tuple conversion") {
    Segment s{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
    auto t = to_tuple(s);

    auto &[start, end] = t;
    CHECK(start.x == 1.0);
    CHECK(end.x == 4.0);
}

TEST_CASE("Segment - for_each_field iteration") {
    Segment s{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};

    int field_count = 0;
    for_each_field(s, [&field_count](auto &field) { field_count++; });

    CHECK(field_count == 2); // start and end
}

// ============================================================================
// TEST: POD properties
// ============================================================================

TEST_CASE("Segment - is standard layout") { CHECK(std::is_standard_layout_v<Segment>); }

TEST_CASE("Segment - is trivially copyable") { CHECK(std::is_trivially_copyable_v<Segment>); }
