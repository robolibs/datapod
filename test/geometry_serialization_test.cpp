#include <doctest/doctest.h>

#include "datagram/datagram.hpp"
#include "datagram/geometry/box.hpp"
#include "datagram/geometry/euler.hpp"
#include "datagram/geometry/point.hpp"
#include "datagram/geometry/pose.hpp"
#include "datagram/geometry/primitives/circle.hpp"
#include "datagram/geometry/primitives/line.hpp"
#include "datagram/geometry/primitives/rectangle.hpp"
#include "datagram/geometry/primitives/square.hpp"
#include "datagram/geometry/primitives/triangle.hpp"
#include "datagram/geometry/quaternion.hpp"
#include "datagram/geometry/size.hpp"

using namespace datagram;

// ============================================================================
// Point Serialization Tests
// ============================================================================

TEST_CASE("serialize - Point") {
    Point p{1.5, 2.5, 3.5};
    auto buf = serialize(p);

    auto result = deserialize<Mode::NONE, Point>(buf);
    CHECK(result.x == doctest::Approx(1.5));
    CHECK(result.y == doctest::Approx(2.5));
    CHECK(result.z == doctest::Approx(3.5));
}

TEST_CASE("serialize - Point with version") {
    Point p{10.0, 20.0, 30.0};
    auto buf = serialize<Mode::WITH_VERSION>(p);

    auto result = deserialize<Mode::WITH_VERSION, Point>(buf);
    CHECK(result.x == doctest::Approx(10.0));
    CHECK(result.y == doctest::Approx(20.0));
    CHECK(result.z == doctest::Approx(30.0));
}

TEST_CASE("serialize - Point with integrity") {
    Point p{100.0, 200.0, 300.0};
    auto buf = serialize<Mode::WITH_INTEGRITY>(p);

    auto result = deserialize<Mode::WITH_INTEGRITY, Point>(buf);
    CHECK(result.x == doctest::Approx(100.0));
    CHECK(result.y == doctest::Approx(200.0));
    CHECK(result.z == doctest::Approx(300.0));
}

TEST_CASE("serialize - Point size check") {
    Point p{1.0, 2.0, 3.0};
    auto buf = serialize(p);
    // Point has 3 doubles = 3 * 8 = 24 bytes
    CHECK(buf.size() == 24);
}

// ============================================================================
// Size Serialization Tests
// ============================================================================

TEST_CASE("serialize - Size") {
    Size s{4.0, 5.0, 6.0};
    auto buf = serialize(s);

    auto result = deserialize<Mode::NONE, Size>(buf);
    CHECK(result.x == doctest::Approx(4.0));
    CHECK(result.y == doctest::Approx(5.0));
    CHECK(result.z == doctest::Approx(6.0));
}

TEST_CASE("serialize - Size with version") {
    Size s{7.5, 8.5, 9.5};
    auto buf = serialize<Mode::WITH_VERSION>(s);

    auto result = deserialize<Mode::WITH_VERSION, Size>(buf);
    CHECK(result.x == doctest::Approx(7.5));
    CHECK(result.y == doctest::Approx(8.5));
    CHECK(result.z == doctest::Approx(9.5));
}

TEST_CASE("serialize - Size size check") {
    Size s{1.0, 1.0, 1.0};
    auto buf = serialize(s);
    CHECK(buf.size() == 24);
}

// ============================================================================
// Euler Serialization Tests
// ============================================================================

TEST_CASE("serialize - Euler") {
    Euler e{0.1, 0.2, 0.3};
    auto buf = serialize(e);

    auto result = deserialize<Mode::NONE, Euler>(buf);
    CHECK(result.roll == doctest::Approx(0.1));
    CHECK(result.pitch == doctest::Approx(0.2));
    CHECK(result.yaw == doctest::Approx(0.3));
}

TEST_CASE("serialize - Euler with integrity") {
    Euler e{1.57, 3.14, 0.785};
    auto buf = serialize<Mode::WITH_INTEGRITY>(e);

    auto result = deserialize<Mode::WITH_INTEGRITY, Euler>(buf);
    CHECK(result.roll == doctest::Approx(1.57));
    CHECK(result.pitch == doctest::Approx(3.14));
    CHECK(result.yaw == doctest::Approx(0.785));
}

// ============================================================================
// Quaternion Serialization Tests
// ============================================================================

TEST_CASE("serialize - Quaternion") {
    Quaternion q{1.0, 0.0, 0.0, 0.0};
    auto buf = serialize(q);

    auto result = deserialize<Mode::NONE, Quaternion>(buf);
    CHECK(result.w == doctest::Approx(1.0));
    CHECK(result.x == doctest::Approx(0.0));
    CHECK(result.y == doctest::Approx(0.0));
    CHECK(result.z == doctest::Approx(0.0));
}

TEST_CASE("serialize - Quaternion normalized") {
    Quaternion q{0.707, 0.707, 0.0, 0.0};
    auto buf = serialize(q);

    auto result = deserialize<Mode::NONE, Quaternion>(buf);
    CHECK(result.w == doctest::Approx(0.707));
    CHECK(result.x == doctest::Approx(0.707));
    CHECK(result.y == doctest::Approx(0.0));
    CHECK(result.z == doctest::Approx(0.0));
}

// ============================================================================
// Pose Serialization Tests
// ============================================================================

TEST_CASE("serialize - Pose") {
    Pose pose{Point{1.0, 2.0, 3.0}, Euler{0.1, 0.2, 0.3}};
    auto buf = serialize(pose);

    auto result = deserialize<Mode::NONE, Pose>(buf);
    CHECK(result.point.x == doctest::Approx(1.0));
    CHECK(result.point.y == doctest::Approx(2.0));
    CHECK(result.point.z == doctest::Approx(3.0));
    CHECK(result.angle.roll == doctest::Approx(0.1));
    CHECK(result.angle.pitch == doctest::Approx(0.2));
    CHECK(result.angle.yaw == doctest::Approx(0.3));
}

TEST_CASE("serialize - Pose with version") {
    Pose pose{Point{5.0, 6.0, 7.0}, Euler{0.5, 0.6, 0.7}};
    auto buf = serialize<Mode::WITH_VERSION>(pose);

    auto result = deserialize<Mode::WITH_VERSION, Pose>(buf);
    CHECK(result.point.x == doctest::Approx(5.0));
    CHECK(result.angle.yaw == doctest::Approx(0.7));
}

TEST_CASE("serialize - Pose size check") {
    Pose pose{Point{0.0, 0.0, 0.0}, Euler{0.0, 0.0, 0.0}};
    auto buf = serialize(pose);
    // Pose = Point(24) + Euler(24) = 48 bytes
    CHECK(buf.size() == 48);
}

// ============================================================================
// Box Serialization Tests
// ============================================================================

TEST_CASE("serialize - Box") {
    Box box{Pose{Point{0.0, 0.0, 0.0}, Euler{0.0, 0.0, 0.0}}, Size{10.0, 20.0, 30.0}};
    auto buf = serialize(box);

    auto result = deserialize<Mode::NONE, Box>(buf);
    CHECK(result.pose.point.x == doctest::Approx(0.0));
    CHECK(result.size.x == doctest::Approx(10.0));
    CHECK(result.size.y == doctest::Approx(20.0));
    CHECK(result.size.z == doctest::Approx(30.0));
}

TEST_CASE("serialize - Box with integrity") {
    Box box{Pose{Point{1.0, 2.0, 3.0}, Euler{0.1, 0.2, 0.3}}, Size{4.0, 5.0, 6.0}};
    auto buf = serialize<Mode::WITH_INTEGRITY>(box);

    auto result = deserialize<Mode::WITH_INTEGRITY, Box>(buf);
    CHECK(result.pose.point.x == doctest::Approx(1.0));
    CHECK(result.pose.angle.roll == doctest::Approx(0.1));
    CHECK(result.size.x == doctest::Approx(4.0));
}

TEST_CASE("serialize - Box size check") {
    Box box{Pose{Point{0.0, 0.0, 0.0}, Euler{0.0, 0.0, 0.0}}, Size{0.0, 0.0, 0.0}};
    auto buf = serialize(box);
    // Box = Pose(48) + Size(24) = 72 bytes
    CHECK(buf.size() == 72);
}

// ============================================================================
// Line Serialization Tests
// ============================================================================

TEST_CASE("serialize - Line") {
    Line line{Point{0.0, 0.0, 0.0}, Point{10.0, 20.0, 30.0}};
    auto buf = serialize(line);

    auto result = deserialize<Mode::NONE, Line>(buf);
    CHECK(result.start.x == doctest::Approx(0.0));
    CHECK(result.start.y == doctest::Approx(0.0));
    CHECK(result.start.z == doctest::Approx(0.0));
    CHECK(result.end.x == doctest::Approx(10.0));
    CHECK(result.end.y == doctest::Approx(20.0));
    CHECK(result.end.z == doctest::Approx(30.0));
}

TEST_CASE("serialize - Line size check") {
    Line line{Point{0.0, 0.0, 0.0}, Point{0.0, 0.0, 0.0}};
    auto buf = serialize(line);
    // Line = 2 * Point(24) = 48 bytes
    CHECK(buf.size() == 48);
}

// ============================================================================
// Circle Serialization Tests
// ============================================================================

TEST_CASE("serialize - Circle") {
    Circle circle{Point{5.0, 5.0, 0.0}, 3.5};
    auto buf = serialize(circle);

    auto result = deserialize<Mode::NONE, Circle>(buf);
    CHECK(result.center.x == doctest::Approx(5.0));
    CHECK(result.center.y == doctest::Approx(5.0));
    CHECK(result.center.z == doctest::Approx(0.0));
    CHECK(result.radius == doctest::Approx(3.5));
}

TEST_CASE("serialize - Circle with version") {
    Circle circle{Point{10.0, 20.0, 0.0}, 7.25};
    auto buf = serialize<Mode::WITH_VERSION>(circle);

    auto result = deserialize<Mode::WITH_VERSION, Circle>(buf);
    CHECK(result.center.x == doctest::Approx(10.0));
    CHECK(result.radius == doctest::Approx(7.25));
}

TEST_CASE("serialize - Circle size check") {
    Circle circle{Point{0.0, 0.0, 0.0}, 1.0};
    auto buf = serialize(circle);
    // Circle = Point(24) + double(8) = 32 bytes
    CHECK(buf.size() == 32);
}

// ============================================================================
// Rectangle Serialization Tests
// ============================================================================

TEST_CASE("serialize - Rectangle") {
    Rectangle rect{Point{0.0, 10.0, 0.0}, Point{10.0, 10.0, 0.0}, Point{0.0, 0.0, 0.0}, Point{10.0, 0.0, 0.0}};
    auto buf = serialize(rect);

    auto result = deserialize<Mode::NONE, Rectangle>(buf);
    CHECK(result.top_left.x == doctest::Approx(0.0));
    CHECK(result.top_left.y == doctest::Approx(10.0));
    CHECK(result.top_right.x == doctest::Approx(10.0));
    CHECK(result.bottom_left.y == doctest::Approx(0.0));
    CHECK(result.bottom_right.x == doctest::Approx(10.0));
    CHECK(result.bottom_right.y == doctest::Approx(0.0));
}

TEST_CASE("serialize - Rectangle size check") {
    Rectangle rect{Point{0.0, 0.0, 0.0}, Point{0.0, 0.0, 0.0}, Point{0.0, 0.0, 0.0}, Point{0.0, 0.0, 0.0}};
    auto buf = serialize(rect);
    // Rectangle = 4 * Point(24) = 96 bytes
    CHECK(buf.size() == 96);
}

// ============================================================================
// Square Serialization Tests
// ============================================================================

TEST_CASE("serialize - Square") {
    Square sq{Point{5.0, 5.0, 0.0}, 10.0};
    auto buf = serialize(sq);

    auto result = deserialize<Mode::NONE, Square>(buf);
    CHECK(result.center.x == doctest::Approx(5.0));
    CHECK(result.center.y == doctest::Approx(5.0));
    CHECK(result.center.z == doctest::Approx(0.0));
    CHECK(result.side == doctest::Approx(10.0));
}

TEST_CASE("serialize - Square with integrity") {
    Square sq{Point{3.0, 4.0, 0.0}, 6.5};
    auto buf = serialize<Mode::WITH_INTEGRITY>(sq);

    auto result = deserialize<Mode::WITH_INTEGRITY, Square>(buf);
    CHECK(result.center.x == doctest::Approx(3.0));
    CHECK(result.side == doctest::Approx(6.5));
}

TEST_CASE("serialize - Square size check") {
    Square sq{Point{0.0, 0.0, 0.0}, 1.0};
    auto buf = serialize(sq);
    // Square = Point(24) + double(8) = 32 bytes
    CHECK(buf.size() == 32);
}

// ============================================================================
// Triangle Serialization Tests
// ============================================================================

TEST_CASE("serialize - Triangle") {
    Triangle tri{Point{0.0, 0.0, 0.0}, Point{10.0, 0.0, 0.0}, Point{5.0, 8.66, 0.0}};
    auto buf = serialize(tri);

    auto result = deserialize<Mode::NONE, Triangle>(buf);
    CHECK(result.a.x == doctest::Approx(0.0));
    CHECK(result.a.y == doctest::Approx(0.0));
    CHECK(result.b.x == doctest::Approx(10.0));
    CHECK(result.b.y == doctest::Approx(0.0));
    CHECK(result.c.x == doctest::Approx(5.0));
    CHECK(result.c.y == doctest::Approx(8.66));
}

TEST_CASE("serialize - Triangle with version") {
    Triangle tri{Point{1.0, 1.0, 0.0}, Point{2.0, 1.0, 0.0}, Point{1.5, 2.0, 0.0}};
    auto buf = serialize<Mode::WITH_VERSION>(tri);

    auto result = deserialize<Mode::WITH_VERSION, Triangle>(buf);
    CHECK(result.a.x == doctest::Approx(1.0));
    CHECK(result.b.x == doctest::Approx(2.0));
    CHECK(result.c.y == doctest::Approx(2.0));
}

TEST_CASE("serialize - Triangle size check") {
    Triangle tri{Point{0.0, 0.0, 0.0}, Point{0.0, 0.0, 0.0}, Point{0.0, 0.0, 0.0}};
    auto buf = serialize(tri);
    // Triangle = 3 * Point(24) = 72 bytes
    CHECK(buf.size() == 72);
}
