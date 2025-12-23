#include <doctest/doctest.h>

#include "datapod/datapod.hpp"
#include "datapod/spatial/aabb.hpp"
#include "datapod/spatial/bounding_sphere.hpp"
#include "datapod/spatial/box.hpp"
#include "datapod/spatial/complex/grid.hpp"
#include "datapod/spatial/complex/path.hpp"
#include "datapod/spatial/complex/polygon.hpp"
#include "datapod/spatial/complex/trajectory.hpp"
#include "datapod/spatial/euler.hpp"
#include "datapod/spatial/gaussian/box.hpp"
#include "datapod/spatial/gaussian/circle.hpp"
#include "datapod/spatial/gaussian/point.hpp"
#include "datapod/spatial/gaussian/rectangle.hpp"
#include "datapod/spatial/obb.hpp"
#include "datapod/spatial/point.hpp"
#include "datapod/spatial/pose.hpp"
#include "datapod/spatial/primitives/circle.hpp"
#include "datapod/spatial/primitives/line.hpp"
#include "datapod/spatial/primitives/rectangle.hpp"
#include "datapod/spatial/primitives/segment.hpp"
#include "datapod/spatial/primitives/square.hpp"
#include "datapod/spatial/primitives/triangle.hpp"
#include "datapod/spatial/quaternion.hpp"
#include "datapod/spatial/size.hpp"
#include "datapod/spatial/state.hpp"

using namespace datapod;

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
    Pose pose{Point{1.0, 2.0, 3.0}, Quaternion{0.9238795, 0.2209424, 0.1766636, 0.2588190}};
    auto buf = serialize(pose);

    auto result = deserialize<Mode::NONE, Pose>(buf);
    CHECK(result.point.x == doctest::Approx(1.0));
    CHECK(result.point.y == doctest::Approx(2.0));
    CHECK(result.point.z == doctest::Approx(3.0));
    CHECK(result.rotation.w == doctest::Approx(0.9238795));
    CHECK(result.rotation.x == doctest::Approx(0.2209424));
    CHECK(result.rotation.y == doctest::Approx(0.1766636));
    CHECK(result.rotation.z == doctest::Approx(0.2588190));
}

TEST_CASE("serialize - Pose with version") {
    Pose pose{Point{5.0, 6.0, 7.0}, Quaternion{0.7071, 0.0, 0.0, 0.7071}};
    auto buf = serialize<Mode::WITH_VERSION>(pose);

    auto result = deserialize<Mode::WITH_VERSION, Pose>(buf);
    CHECK(result.point.x == doctest::Approx(5.0));
    CHECK(result.rotation.w == doctest::Approx(0.7071));
}

TEST_CASE("serialize - Pose size check") {
    Pose pose{Point{0.0, 0.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    auto buf = serialize(pose);
    // Pose = Point(24) + Euler(24) = 48 bytes
    CHECK(buf.size() == 56);
}

// ============================================================================
// Box Serialization Tests
// ============================================================================

TEST_CASE("serialize - Box") {
    Box box{Pose{Point{0.0, 0.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}}, Size{10.0, 20.0, 30.0}};
    auto buf = serialize(box);

    auto result = deserialize<Mode::NONE, Box>(buf);
    CHECK(result.pose.point.x == doctest::Approx(0.0));
    CHECK(result.size.x == doctest::Approx(10.0));
    CHECK(result.size.y == doctest::Approx(20.0));
    CHECK(result.size.z == doctest::Approx(30.0));
}

TEST_CASE("serialize - Box with integrity") {
    Box box{Pose{Point{1.0, 2.0, 3.0}, Quaternion{0.9833, 0.1060, 0.1435, 0.0271}}, Size{4.0, 5.0, 6.0}};
    auto buf = serialize<Mode::WITH_INTEGRITY>(box);

    auto result = deserialize<Mode::WITH_INTEGRITY, Box>(buf);
    CHECK(result.pose.point.x == doctest::Approx(1.0));
    //     CHECK(result.pose.rotation.x == doctest::Approx(0.1));
    CHECK(result.size.x == doctest::Approx(4.0));
}

TEST_CASE("serialize - Box size check") {
    Box box{Pose{Point{0.0, 0.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}}, Size{0.0, 0.0, 0.0}};
    auto buf = serialize(box);
    // Box = Pose(48) + Size(24) = 72 bytes
    CHECK(buf.size() == 80);
}

// ============================================================================
// Line Serialization Tests
// ============================================================================

TEST_CASE("serialize - Segment") {
    Segment seg{Point{0.0, 0.0, 0.0}, Point{10.0, 20.0, 30.0}};
    auto buf = serialize(seg);

    auto result = deserialize<Mode::NONE, Segment>(buf);
    CHECK(result.start.x == doctest::Approx(0.0));
    CHECK(result.start.y == doctest::Approx(0.0));
    CHECK(result.start.z == doctest::Approx(0.0));
    CHECK(result.end.x == doctest::Approx(10.0));
    CHECK(result.end.y == doctest::Approx(20.0));
    CHECK(result.end.z == doctest::Approx(30.0));
}

TEST_CASE("serialize - Segment size check") {
    Segment seg{Point{0.0, 0.0, 0.0}, Point{0.0, 0.0, 0.0}};
    auto buf = serialize(seg);
    // Segment = 2 * Point(24) = 48 bytes
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

// ============================================================================
// Polygon Serialization Tests (Complex Type)
// ============================================================================

TEST_CASE("serialize - Polygon empty") {
    Polygon poly{Vector<Point>{}};
    auto buf = serialize(poly);

    auto result = deserialize<Mode::NONE, Polygon>(buf);
    CHECK(result.vertices.size() == 0);
}

TEST_CASE("serialize - Polygon triangle") {
    Vector<Point> vertices;
    vertices.push_back(Point{0.0, 0.0, 0.0});
    vertices.push_back(Point{10.0, 0.0, 0.0});
    vertices.push_back(Point{5.0, 8.66, 0.0});

    Polygon poly{vertices};
    auto buf = serialize(poly);

    auto result = deserialize<Mode::NONE, Polygon>(buf);
    CHECK(result.vertices.size() == 3);
    CHECK(result.vertices[0].x == doctest::Approx(0.0));
    CHECK(result.vertices[1].x == doctest::Approx(10.0));
    CHECK(result.vertices[2].y == doctest::Approx(8.66));
}

TEST_CASE("serialize - Polygon rectangle") {
    Vector<Point> vertices;
    vertices.push_back(Point{0.0, 0.0, 0.0});
    vertices.push_back(Point{10.0, 0.0, 0.0});
    vertices.push_back(Point{10.0, 5.0, 0.0});
    vertices.push_back(Point{0.0, 5.0, 0.0});

    Polygon poly{vertices};
    auto buf = serialize<Mode::WITH_INTEGRITY>(poly);

    auto result = deserialize<Mode::WITH_INTEGRITY, Polygon>(buf);
    CHECK(result.vertices.size() == 4);
    CHECK(result.vertices[0].x == doctest::Approx(0.0));
    CHECK(result.vertices[2].y == doctest::Approx(5.0));
    CHECK(result.vertices[3].x == doctest::Approx(0.0));
    CHECK(result.vertices[3].y == doctest::Approx(5.0));
}

TEST_CASE("serialize - Polygon with version") {
    Vector<Point> vertices;
    vertices.push_back(Point{1.0, 1.0, 0.0});
    vertices.push_back(Point{2.0, 1.0, 0.0});
    vertices.push_back(Point{2.0, 2.0, 0.0});
    vertices.push_back(Point{1.0, 2.0, 0.0});

    Polygon poly{vertices};
    auto buf = serialize<Mode::WITH_VERSION>(poly);

    auto result = deserialize<Mode::WITH_VERSION, Polygon>(buf);
    CHECK(result.vertices.size() == 4);
}

// ============================================================================
// Grid Serialization Tests (Complex Type)
// ============================================================================

TEST_CASE("serialize - Grid<int> 2x2") {
    Vector<int> data;
    data.push_back(1);
    data.push_back(2);
    data.push_back(3);
    data.push_back(4);

    Grid<int> grid{2, 2, 1.0, false, Pose{Point{0.0, 0.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}}, data};
    auto buf = serialize(grid);

    auto result = deserialize<Mode::NONE, Grid<int>>(buf);
    CHECK(result.rows == 2);
    CHECK(result.cols == 2);
    CHECK(result.resolution == doctest::Approx(1.0));
    CHECK(result.centered == false);
    CHECK(result.data.size() == 4);
    CHECK(result.data[0] == 1);
    CHECK(result.data[3] == 4);
}

TEST_CASE("serialize - Grid<double> with pose") {
    Vector<double> data;
    data.push_back(1.5);
    data.push_back(2.5);
    data.push_back(3.5);
    data.push_back(4.5);

    Pose pose{Point{10.0, 20.0, 0.0}, Quaternion{0.7071, 0.0, 0.0, 0.7071}};
    Grid<double> grid{2, 2, 0.5, true, pose, data};
    auto buf = serialize<Mode::WITH_VERSION>(grid);

    auto result = deserialize<Mode::WITH_VERSION, Grid<double>>(buf);
    CHECK(result.rows == 2);
    CHECK(result.cols == 2);
    CHECK(result.resolution == doctest::Approx(0.5));
    CHECK(result.centered == true);
    CHECK(result.pose.point.x == doctest::Approx(10.0));
    //     CHECK(result.pose.rotation.z == doctest::Approx(1.57));
    CHECK(result.data[1] == doctest::Approx(2.5));
}

TEST_CASE("serialize - Grid<float> empty") {
    Grid<float> grid{0, 0, 1.0, false, Pose{}, Vector<float>{}};
    auto buf = serialize(grid);

    auto result = deserialize<Mode::NONE, Grid<float>>(buf);
    CHECK(result.rows == 0);
    CHECK(result.cols == 0);
    CHECK(result.data.size() == 0);
}

TEST_CASE("serialize - Grid<uint8_t> 3x3 with integrity") {
    Vector<uint8_t> data;
    for (uint8_t i = 0; i < 9; ++i) {
        data.push_back(i * 10);
    }

    Grid<uint8_t> grid{3, 3, 0.1, false, Pose{}, data};
    auto buf = serialize<Mode::WITH_INTEGRITY>(grid);

    auto result = deserialize<Mode::WITH_INTEGRITY, Grid<uint8_t>>(buf);
    CHECK(result.rows == 3);
    CHECK(result.cols == 3);
    CHECK(result.data.size() == 9);
    CHECK(result.data[0] == 0);
    CHECK(result.data[4] == 40);
    CHECK(result.data[8] == 80);
}

// ============================================================================
// State Serialization Tests
// ============================================================================

TEST_CASE("serialize - State") {
    State state{Pose{Point{1.0, 2.0, 3.0}, Quaternion{0.9833, 0.1060, 0.1435, 0.0271}}, Velocity{5.0, 0.0, 0.0}, Velocity{0.5, 0.0, 0.0}};
    auto buf = serialize(state);

    auto result = deserialize<Mode::NONE, State>(buf);
    CHECK(result.pose.point.x == doctest::Approx(1.0));
    CHECK(result.pose.point.y == doctest::Approx(2.0));
    CHECK(result.pose.point.z == doctest::Approx(3.0));
    //     CHECK(result.pose.rotation.x == doctest::Approx(0.1));
    //     CHECK(result.pose.rotation.y == doctest::Approx(0.2));
    //     CHECK(result.pose.rotation.z == doctest::Approx(0.3));
    CHECK(result.linear_velocity.vx == doctest::Approx(5.0));
    CHECK(result.angular_velocity.vx == doctest::Approx(0.5));
}

TEST_CASE("serialize - State with version") {
    State state{Pose{Point{5.0, 6.0, 7.0}, Quaternion{0.9021, 0.2604, 0.3072, 0.1731}}, Velocity{10.0, 0.0, 0.0}, Velocity{1.0, 0.0, 0.0}};
    auto buf = serialize<Mode::WITH_VERSION>(state);

    auto result = deserialize<Mode::WITH_VERSION, State>(buf);
    CHECK(result.pose.point.x == doctest::Approx(5.0));
    CHECK(result.linear_velocity.vx == doctest::Approx(10.0));
    CHECK(result.angular_velocity.vx == doctest::Approx(1.0));
}

TEST_CASE("serialize - State size check") {
    State state{Pose{}, Velocity{}, Velocity{}};
    auto buf = serialize(state);
    // State = Pose(48) + double(8) + double(8) = 64 bytes
    CHECK(buf.size() == 104);
}

// ============================================================================
// Path Serialization Tests
// ============================================================================

TEST_CASE("serialize - Path empty") {
    Path path{Vector<Pose>{}};
    auto buf = serialize(path);

    auto result = deserialize<Mode::NONE, Path>(buf);
    CHECK(result.waypoints.size() == 0);
}

TEST_CASE("serialize - Path with waypoints") {
    Vector<Pose> waypoints;
    waypoints.push_back(Pose{Point{0.0, 0.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}});
    waypoints.push_back(Pose{Point{10.0, 0.0, 0.0}, Quaternion{0.7071, 0.0, 0.0, 0.7071}});
    waypoints.push_back(Pose{Point{10.0, 10.0, 0.0}, Quaternion{-0.001, 0.0, 0.0, 1.0}});

    Path path{waypoints};
    auto buf = serialize(path);

    auto result = deserialize<Mode::NONE, Path>(buf);
    CHECK(result.waypoints.size() == 3);
    CHECK(result.waypoints[0].point.x == doctest::Approx(0.0));
    CHECK(result.waypoints[1].point.x == doctest::Approx(10.0));
    //     CHECK(result.waypoints[1].rotation.z == doctest::Approx(1.57));
    //     CHECK(result.waypoints[2].point.y == doctest::Approx(10.0));
    //     CHECK(result.waypoints[2].rotation.z == doctest::Approx(3.14));
}

TEST_CASE("serialize - Path with version") {
    Vector<Pose> waypoints;
    waypoints.push_back(Pose{Point{1.0, 2.0, 3.0}, Quaternion{0.9833, 0.1060, 0.1435, 0.0271}});

    Path path{waypoints};
    auto buf = serialize<Mode::WITH_VERSION>(path);

    auto result = deserialize<Mode::WITH_VERSION, Path>(buf);
    CHECK(result.waypoints.size() == 1);
}

// ============================================================================
// Trajectory Serialization Tests
// ============================================================================

TEST_CASE("serialize - Trajectory empty") {
    Trajectory traj{Vector<State>{}};
    auto buf = serialize(traj);

    auto result = deserialize<Mode::NONE, Trajectory>(buf);
    CHECK(result.states.size() == 0);
}

TEST_CASE("serialize - Trajectory with states") {
    Vector<State> states;
    states.push_back(State{Pose{Point{0.0, 0.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}}, Velocity{}, Velocity{}});
    states.push_back(State{Pose{Point{5.0, 0.0, 0.0}, Quaternion{0.9689, 0.0, 0.0, 0.2474}}, Velocity{2.5, 0.0, 0.0}, Velocity{0.1, 0.0, 0.0}});
    states.push_back(State{Pose{Point{10.0, 5.0, 0.0}, Quaternion{0.8776, 0.0, 0.0, 0.4794}}, Velocity{5.0, 0.0, 0.0}, Velocity{0.2, 0.0, 0.0}});

    Trajectory traj{states};
    auto buf = serialize(traj);

    auto result = deserialize<Mode::NONE, Trajectory>(buf);
    CHECK(result.states.size() == 3);
    CHECK(result.states[0].pose.point.x == doctest::Approx(0.0));
    CHECK(result.states[0].linear_velocity.vx == doctest::Approx(0.0));
    CHECK(result.states[1].pose.point.x == doctest::Approx(5.0));
    CHECK(result.states[1].linear_velocity.vx == doctest::Approx(2.5));
    CHECK(result.states[1].angular_velocity.vx == doctest::Approx(0.1));
    CHECK(result.states[2].pose.point.y == doctest::Approx(5.0));
    CHECK(result.states[2].linear_velocity.vx == doctest::Approx(5.0));
}

TEST_CASE("serialize - Trajectory with integrity") {
    Vector<State> states;
    states.push_back(State{Pose{Point{1.0, 2.0, 3.0}, Quaternion{0.9833, 0.1060, 0.1435, 0.0271}}, Velocity{1.5, 0.3, 0.0}});

    Trajectory traj{states};
    auto buf = serialize<Mode::WITH_INTEGRITY>(traj);

    auto result = deserialize<Mode::WITH_INTEGRITY, Trajectory>(buf);
    CHECK(result.states.size() == 1);
}

// ============================================================================
// AABB Serialization Tests
// ============================================================================

TEST_CASE("serialize - AABB") {
    AABB aabb{Point{0.0, 0.0, 0.0}, Point{10.0, 20.0, 30.0}};
    auto buf = serialize(aabb);

    auto result = deserialize<Mode::NONE, AABB>(buf);
    CHECK(result.min_point.x == doctest::Approx(0.0));
    CHECK(result.min_point.y == doctest::Approx(0.0));
    CHECK(result.min_point.z == doctest::Approx(0.0));
    CHECK(result.max_point.x == doctest::Approx(10.0));
    CHECK(result.max_point.y == doctest::Approx(20.0));
    CHECK(result.max_point.z == doctest::Approx(30.0));
}

TEST_CASE("serialize - AABB with version") {
    AABB aabb{Point{-5.0, -5.0, -5.0}, Point{5.0, 5.0, 5.0}};
    auto buf = serialize<Mode::WITH_VERSION>(aabb);

    auto result = deserialize<Mode::WITH_VERSION, AABB>(buf);
    CHECK(result.min_point.x == doctest::Approx(-5.0));
    CHECK(result.max_point.x == doctest::Approx(5.0));
}

TEST_CASE("serialize - AABB size check") {
    AABB aabb{Point{}, Point{}};
    auto buf = serialize(aabb);
    // AABB = 2 * Point(24) = 48 bytes
    CHECK(buf.size() == 48);
}

// ============================================================================
// OBB Serialization Tests
// ============================================================================

TEST_CASE("serialize - OBB") {
    OBB obb{Point{5.0, 5.0, 5.0}, Size{2.5, 3.0, 4.0}, Euler{0.1, 0.2, 0.3}};
    auto buf = serialize(obb);

    auto result = deserialize<Mode::NONE, OBB>(buf);
    CHECK(result.center.x == doctest::Approx(5.0));
    CHECK(result.center.y == doctest::Approx(5.0));
    CHECK(result.center.z == doctest::Approx(5.0));
    CHECK(result.half_extents.x == doctest::Approx(2.5));
    CHECK(result.half_extents.y == doctest::Approx(3.0));
    CHECK(result.half_extents.z == doctest::Approx(4.0));
    CHECK(result.orientation.roll == doctest::Approx(0.1));
    CHECK(result.orientation.pitch == doctest::Approx(0.2));
    CHECK(result.orientation.yaw == doctest::Approx(0.3));
}

TEST_CASE("serialize - OBB with integrity") {
    OBB obb{Point{1.0, 2.0, 3.0}, Size{0.5, 0.5, 0.5}, Euler{0.0, 0.0, 1.57}};
    auto buf = serialize<Mode::WITH_INTEGRITY>(obb);

    auto result = deserialize<Mode::WITH_INTEGRITY, OBB>(buf);
    CHECK(result.center.x == doctest::Approx(1.0));
    CHECK(result.half_extents.x == doctest::Approx(0.5));
    CHECK(result.orientation.yaw == doctest::Approx(1.57));
}

TEST_CASE("serialize - OBB size check") {
    OBB obb{Point{}, Size{}, Euler{}};
    auto buf = serialize(obb);
    // OBB = Point(24) + Size(24) + Euler(24) = 72 bytes
    CHECK(buf.size() == 72);
}

// ============================================================================
// BoundingSphere Serialization Tests
// ============================================================================

TEST_CASE("serialize - BoundingSphere") {
    BoundingSphere sphere{Point{10.0, 20.0, 30.0}, 15.5};
    auto buf = serialize(sphere);

    auto result = deserialize<Mode::NONE, BoundingSphere>(buf);
    CHECK(result.center.x == doctest::Approx(10.0));
    CHECK(result.center.y == doctest::Approx(20.0));
    CHECK(result.center.z == doctest::Approx(30.0));
    CHECK(result.radius == doctest::Approx(15.5));
}

TEST_CASE("serialize - BoundingSphere with version") {
    BoundingSphere sphere{Point{0.0, 0.0, 0.0}, 1.0};
    auto buf = serialize<Mode::WITH_VERSION>(sphere);

    auto result = deserialize<Mode::WITH_VERSION, BoundingSphere>(buf);
    CHECK(result.center.x == doctest::Approx(0.0));
    CHECK(result.radius == doctest::Approx(1.0));
}

TEST_CASE("serialize - BoundingSphere size check") {
    BoundingSphere sphere{Point{}, 0.0};
    auto buf = serialize(sphere);
    // BoundingSphere = Point(24) + double(8) = 32 bytes
    CHECK(buf.size() == 32);
}

// ============================================================================
// Gaussian::Point Serialization Tests
// ============================================================================

TEST_CASE("serialize - Gaussian::Point") {
    gaussian::Point gpoint{Point{1.0, 2.0, 3.0}, 0.5};
    auto buf = serialize(gpoint);

    auto result = deserialize<Mode::NONE, gaussian::Point>(buf);
    CHECK(result.point.x == doctest::Approx(1.0));
    CHECK(result.point.y == doctest::Approx(2.0));
    CHECK(result.point.z == doctest::Approx(3.0));
    CHECK(result.uncertainty == doctest::Approx(0.5));
}

TEST_CASE("serialize - Gaussian::Point with version") {
    gaussian::Point gpoint{Point{5.0, 6.0, 7.0}, 1.2};
    auto buf = serialize<Mode::WITH_VERSION>(gpoint);

    auto result = deserialize<Mode::WITH_VERSION, gaussian::Point>(buf);
    CHECK(result.point.x == doctest::Approx(5.0));
    CHECK(result.uncertainty == doctest::Approx(1.2));
}

TEST_CASE("serialize - Gaussian::Point size check") {
    gaussian::Point gpoint{Point{}, 0.0};
    auto buf = serialize(gpoint);
    // Gaussian::Point = Point(24) + double(8) = 32 bytes
    CHECK(buf.size() == 32);
}

// ============================================================================
// Gaussian::Circle Serialization Tests
// ============================================================================

TEST_CASE("serialize - Gaussian::Circle") {
    gaussian::Circle gcircle{Circle{Point{5.0, 5.0, 0.0}, 3.5}, 0.8};
    auto buf = serialize(gcircle);

    auto result = deserialize<Mode::NONE, gaussian::Circle>(buf);
    CHECK(result.circle.center.x == doctest::Approx(5.0));
    CHECK(result.circle.center.y == doctest::Approx(5.0));
    CHECK(result.circle.radius == doctest::Approx(3.5));
    CHECK(result.uncertainty == doctest::Approx(0.8));
}

TEST_CASE("serialize - Gaussian::Circle with integrity") {
    gaussian::Circle gcircle{Circle{Point{10.0, 20.0, 0.0}, 7.25}, 1.5};
    auto buf = serialize<Mode::WITH_INTEGRITY>(gcircle);

    auto result = deserialize<Mode::WITH_INTEGRITY, gaussian::Circle>(buf);
    CHECK(result.circle.center.x == doctest::Approx(10.0));
    CHECK(result.circle.radius == doctest::Approx(7.25));
    CHECK(result.uncertainty == doctest::Approx(1.5));
}

TEST_CASE("serialize - Gaussian::Circle size check") {
    gaussian::Circle gcircle{Circle{}, 0.0};
    auto buf = serialize(gcircle);
    // Gaussian::Circle = Circle(32) + double(8) = 40 bytes
    CHECK(buf.size() == 40);
}

// ============================================================================
// Gaussian::Box Serialization Tests
// ============================================================================

TEST_CASE("serialize - Gaussian::Box") {
    gaussian::Box gbox{Box{Pose{Point{1.0, 2.0, 3.0}, Quaternion{0.9833, 0.1060, 0.1435, 0.0271}}, Size{4.0, 5.0, 6.0}}, 2.0};
    auto buf = serialize(gbox);

    auto result = deserialize<Mode::NONE, gaussian::Box>(buf);
    CHECK(result.box.pose.point.x == doctest::Approx(1.0));
    //     CHECK(result.box.pose.rotation.x == doctest::Approx(0.1));
    CHECK(result.box.size.x == doctest::Approx(4.0));
    CHECK(result.box.size.y == doctest::Approx(5.0));
    CHECK(result.box.size.z == doctest::Approx(6.0));
    CHECK(result.uncertainty == doctest::Approx(2.0));
}

TEST_CASE("serialize - Gaussian::Box with version") {
    gaussian::Box gbox{Box{Pose{Point{0.0, 0.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}}, Size{1.0, 1.0, 1.0}}, 0.5};
    auto buf = serialize<Mode::WITH_VERSION>(gbox);

    auto result = deserialize<Mode::WITH_VERSION, gaussian::Box>(buf);
    CHECK(result.box.size.x == doctest::Approx(1.0));
    CHECK(result.uncertainty == doctest::Approx(0.5));
}

TEST_CASE("serialize - Gaussian::Box size check") {
    gaussian::Box gbox{Box{}, 0.0};
    auto buf = serialize(gbox);
    // Gaussian::Box = Box(72) + double(8) = 80 bytes
    CHECK(buf.size() == 88);
}

// ============================================================================
// Gaussian::Rectangle Serialization Tests
// ============================================================================

TEST_CASE("serialize - Gaussian::Rectangle") {
    gaussian::Rectangle grect{
        Rectangle{Point{0.0, 10.0, 0.0}, Point{10.0, 10.0, 0.0}, Point{0.0, 0.0, 0.0}, Point{10.0, 0.0, 0.0}}, 1.0};
    auto buf = serialize(grect);

    auto result = deserialize<Mode::NONE, gaussian::Rectangle>(buf);
    CHECK(result.rectangle.top_left.x == doctest::Approx(0.0));
    CHECK(result.rectangle.top_left.y == doctest::Approx(10.0));
    CHECK(result.rectangle.top_right.x == doctest::Approx(10.0));
    CHECK(result.rectangle.bottom_left.y == doctest::Approx(0.0));
    CHECK(result.rectangle.bottom_right.x == doctest::Approx(10.0));
    CHECK(result.uncertainty == doctest::Approx(1.0));
}

TEST_CASE("serialize - Gaussian::Rectangle with integrity") {
    gaussian::Rectangle grect{
        Rectangle{Point{1.0, 1.0, 0.0}, Point{2.0, 1.0, 0.0}, Point{1.0, 0.0, 0.0}, Point{2.0, 0.0, 0.0}}, 0.3};
    auto buf = serialize<Mode::WITH_INTEGRITY>(grect);

    auto result = deserialize<Mode::WITH_INTEGRITY, gaussian::Rectangle>(buf);
    CHECK(result.rectangle.top_left.x == doctest::Approx(1.0));
    CHECK(result.uncertainty == doctest::Approx(0.3));
}

TEST_CASE("serialize - Gaussian::Rectangle size check") {
    gaussian::Rectangle grect{Rectangle{}, 0.0};
    auto buf = serialize(grect);
    // Gaussian::Rectangle = Rectangle(96) + double(8) = 104 bytes
    CHECK(buf.size() == 104);
}
