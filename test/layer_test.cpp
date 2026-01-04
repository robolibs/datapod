#include <doctest/doctest.h>

#include "datapod/datapod.hpp"
#include "datapod/pods/spatial/complex/layer.hpp"

using namespace datapod;

// ============================================================================
// Layer Construction Tests
// ============================================================================

TEST_CASE("Layer - make_layer factory") {
    auto layer = make_layer<uint8_t>(10, 20, 5, 0.5, 1.0);

    CHECK(layer.rows == 10);
    CHECK(layer.cols == 20);
    CHECK(layer.layers == 5);
    CHECK(layer.resolution == doctest::Approx(0.5));
    CHECK(layer.layer_height == doctest::Approx(1.0));
    CHECK(layer.centered == false);
    CHECK(layer.data.size() == 10 * 20 * 5);
}

TEST_CASE("Layer - make_layer centered") {
    Pose pose{Point{100.0, 200.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    auto layer = make_layer<double>(5, 5, 3, 1.0, 2.0, true, pose, 0.5);

    CHECK(layer.rows == 5);
    CHECK(layer.cols == 5);
    CHECK(layer.layers == 3);
    CHECK(layer.resolution == doctest::Approx(1.0));
    CHECK(layer.layer_height == doctest::Approx(2.0));
    CHECK(layer.centered == true);
    CHECK(layer.pose.point.x == doctest::Approx(100.0));
    CHECK(layer.pose.point.y == doctest::Approx(200.0));
    CHECK(layer.data.size() == 75);
    // Check default value was applied
    CHECK(layer.data[0] == doctest::Approx(0.5));
    CHECK(layer.data[74] == doctest::Approx(0.5));
}

TEST_CASE("Layer - aggregate initialization") {
    Layer<int> layer;
    layer.rows = 2;
    layer.cols = 3;
    layer.layers = 2;
    layer.resolution = 1.0;
    layer.layer_height = 0.5;
    layer.centered = false;
    layer.pose = Pose{};
    layer.data.resize(12);
    for (int i = 0; i < 12; ++i) {
        layer.data[i] = i;
    }

    CHECK(layer.is_valid());
    CHECK(layer.size() == 12);
}

// ============================================================================
// Layer Index and Access Tests
// ============================================================================

TEST_CASE("Layer - index calculation") {
    auto layer = make_layer<int>(3, 4, 2, 1.0, 1.0);

    // Layer-major, row-major: index = l * rows * cols + r * cols + c
    CHECK(layer.index(0, 0, 0) == 0);
    CHECK(layer.index(0, 1, 0) == 1);
    CHECK(layer.index(0, 3, 0) == 3);
    CHECK(layer.index(1, 0, 0) == 4);
    CHECK(layer.index(2, 3, 0) == 11);
    CHECK(layer.index(0, 0, 1) == 12);
    CHECK(layer.index(2, 3, 1) == 23);
}

TEST_CASE("Layer - operator() access") {
    auto layer = make_layer<int>(3, 4, 2, 1.0, 1.0);

    // Write values
    layer(0, 0, 0) = 100;
    layer(1, 2, 0) = 200;
    layer(2, 3, 1) = 300;

    // Read values
    CHECK(layer(0, 0, 0) == 100);
    CHECK(layer(1, 2, 0) == 200);
    CHECK(layer(2, 3, 1) == 300);
}

TEST_CASE("Layer - at() bounds checking") {
    auto layer = make_layer<int>(3, 4, 2, 1.0, 1.0);

    // Valid access
    CHECK_NOTHROW(layer.at(0, 0, 0));
    CHECK_NOTHROW(layer.at(2, 3, 1));

    // Out of bounds
    CHECK_THROWS_AS(layer.at(3, 0, 0), std::out_of_range);
    CHECK_THROWS_AS(layer.at(0, 4, 0), std::out_of_range);
    CHECK_THROWS_AS(layer.at(0, 0, 2), std::out_of_range);
}

// ============================================================================
// Layer Spatial Coordinate Tests
// ============================================================================

TEST_CASE("Layer - get_point non-centered") {
    auto layer = make_layer<uint8_t>(10, 10, 5, 1.0, 2.0, false);

    // Cell (0,0,0) center should be at (0.5, 0.5, 1.0)
    Point p000 = layer.get_point(0, 0, 0);
    CHECK(p000.x == doctest::Approx(0.5));
    CHECK(p000.y == doctest::Approx(0.5));
    CHECK(p000.z == doctest::Approx(1.0));

    // Cell (0,1,0) center should be at (1.5, 0.5, 1.0)
    Point p010 = layer.get_point(0, 1, 0);
    CHECK(p010.x == doctest::Approx(1.5));
    CHECK(p010.y == doctest::Approx(0.5));
    CHECK(p010.z == doctest::Approx(1.0));

    // Cell (1,0,0) center should be at (0.5, 1.5, 1.0)
    Point p100 = layer.get_point(1, 0, 0);
    CHECK(p100.x == doctest::Approx(0.5));
    CHECK(p100.y == doctest::Approx(1.5));
    CHECK(p100.z == doctest::Approx(1.0));

    // Cell (0,0,1) center should be at (0.5, 0.5, 3.0)
    Point p001 = layer.get_point(0, 0, 1);
    CHECK(p001.x == doctest::Approx(0.5));
    CHECK(p001.y == doctest::Approx(0.5));
    CHECK(p001.z == doctest::Approx(3.0));
}

TEST_CASE("Layer - get_point centered") {
    auto layer = make_layer<uint8_t>(10, 10, 5, 1.0, 2.0, true);

    // For centered grid: offset by -half_width, -half_height
    // Cell (5,5,0) should be near origin in XY
    Point p550 = layer.get_point(5, 5, 0);
    CHECK(p550.x == doctest::Approx(0.5));
    CHECK(p550.y == doctest::Approx(0.5));
    CHECK(p550.z == doctest::Approx(1.0));

    // Cell (0,0,0) should be at top-left corner
    Point p000 = layer.get_point(0, 0, 0);
    CHECK(p000.x == doctest::Approx(-4.5));
    CHECK(p000.y == doctest::Approx(-4.5));
}

TEST_CASE("Layer - get_point with pose offset") {
    Pose pose{Point{100.0, 200.0, 50.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    auto layer = make_layer<uint8_t>(10, 10, 5, 1.0, 2.0, false, pose);

    // Cell (0,0,0) center should be offset by pose
    Point p000 = layer.get_point(0, 0, 0);
    CHECK(p000.x == doctest::Approx(100.5));
    CHECK(p000.y == doctest::Approx(200.5));
    CHECK(p000.z == doctest::Approx(51.0));
}

TEST_CASE("Layer - world_to_voxel") {
    auto layer = make_layer<uint8_t>(10, 10, 5, 1.0, 2.0, false);

    // Point at cell center (0,0,0)
    auto [r1, c1, l1] = layer.world_to_voxel(Point{0.5, 0.5, 1.0});
    CHECK(r1 == 0);
    CHECK(c1 == 0);
    CHECK(l1 == 0);

    // Point at cell center (5,5,2)
    auto [r2, c2, l2] = layer.world_to_voxel(Point{5.5, 5.5, 5.0});
    CHECK(r2 == 5);
    CHECK(c2 == 5);
    CHECK(l2 == 2);
}

// ============================================================================
// Layer Extract/Set Grid Tests
// ============================================================================

TEST_CASE("Layer - extract_grid") {
    auto layer = make_layer<int>(3, 4, 2, 0.5, 1.0);

    // Fill layer with values
    for (std::size_t l = 0; l < 2; ++l) {
        for (std::size_t r = 0; r < 3; ++r) {
            for (std::size_t c = 0; c < 4; ++c) {
                layer(r, c, l) = static_cast<int>(l * 100 + r * 10 + c);
            }
        }
    }

    // Extract layer 0
    Grid<int> grid0 = layer.extract_grid(0);
    CHECK(grid0.rows == 3);
    CHECK(grid0.cols == 4);
    CHECK(grid0.resolution == doctest::Approx(0.5));
    CHECK(grid0(0, 0) == 0);
    CHECK(grid0(1, 2) == 12);
    CHECK(grid0(2, 3) == 23);

    // Extract layer 1
    Grid<int> grid1 = layer.extract_grid(1);
    CHECK(grid1(0, 0) == 100);
    CHECK(grid1(1, 2) == 112);
    CHECK(grid1(2, 3) == 123);
}

TEST_CASE("Layer - extract_grid out of bounds") {
    auto layer = make_layer<int>(3, 4, 2, 1.0, 1.0);
    CHECK_THROWS_AS(layer.extract_grid(2), std::out_of_range);
}

TEST_CASE("Layer - set_grid") {
    auto layer = make_layer<int>(3, 4, 2, 0.5, 1.0);

    // Create a grid to set
    auto grid = make_grid<int>(3, 4, 0.5);
    for (std::size_t r = 0; r < 3; ++r) {
        for (std::size_t c = 0; c < 4; ++c) {
            grid(r, c) = static_cast<int>(r * 10 + c + 500);
        }
    }

    // Set layer 1
    layer.set_grid(1, grid);

    // Verify
    CHECK(layer(0, 0, 1) == 500);
    CHECK(layer(1, 2, 1) == 512);
    CHECK(layer(2, 3, 1) == 523);

    // Layer 0 should still be default (0)
    CHECK(layer(0, 0, 0) == 0);
}

TEST_CASE("Layer - set_grid out of bounds") {
    auto layer = make_layer<int>(3, 4, 2, 1.0, 1.0);
    auto grid = make_grid<int>(3, 4, 1.0);
    CHECK_THROWS_AS(layer.set_grid(2, grid), std::out_of_range);
}

TEST_CASE("Layer - set_grid dimension mismatch") {
    auto layer = make_layer<int>(3, 4, 2, 1.0, 1.0);
    auto grid = make_grid<int>(4, 4, 1.0); // Wrong rows
    CHECK_THROWS_AS(layer.set_grid(0, grid), std::invalid_argument);
}

// ============================================================================
// Layer Utility Tests
// ============================================================================

TEST_CASE("Layer - size and empty") {
    auto layer = make_layer<int>(3, 4, 5, 1.0, 1.0);
    CHECK(layer.size() == 60);
    CHECK_FALSE(layer.empty());

    Layer<int> empty_layer;
    CHECK(empty_layer.size() == 0);
    CHECK(empty_layer.empty());
}

TEST_CASE("Layer - is_valid") {
    auto layer = make_layer<int>(3, 4, 5, 1.0, 1.0);
    CHECK(layer.is_valid());

    Layer<int> invalid_layer;
    invalid_layer.rows = 3;
    invalid_layer.cols = 4;
    invalid_layer.layers = 5;
    // data not resized
    CHECK_FALSE(invalid_layer.is_valid());
}

TEST_CASE("Layer - compatibility accessors") {
    auto layer = make_layer<int>(3, 4, 5, 0.5, 1.5, false);

    CHECK(layer.layer_count() == 5);
    CHECK(layer.get_layer_height() == doctest::Approx(1.5));
    CHECK(layer.get_resolution() == doctest::Approx(0.5));
    CHECK(layer.shift().point.x == doctest::Approx(0.0));
}

TEST_CASE("Layer - comparison operators") {
    auto layer1 = make_layer<int>(3, 4, 2, 1.0, 1.0);
    auto layer2 = make_layer<int>(3, 4, 2, 1.0, 1.0);
    auto layer3 = make_layer<int>(3, 4, 3, 1.0, 1.0); // Different layers

    CHECK(layer1 == layer2);
    CHECK_FALSE(layer1 != layer2);
    CHECK(layer1 != layer3);
}

TEST_CASE("Layer - iterators") {
    auto layer = make_layer<int>(2, 2, 2, 1.0, 1.0, false, Pose{}, 5);

    int sum = 0;
    for (const auto &val : layer) {
        sum += val;
    }
    CHECK(sum == 5 * 8);

    // Modify via iterator
    for (auto &val : layer) {
        val = 10;
    }
    CHECK(layer(0, 0, 0) == 10);
    CHECK(layer(1, 1, 1) == 10);
}

// ============================================================================
// Layer Serialization Tests
// ============================================================================

TEST_CASE("serialize - Layer<int> basic") {
    auto layer = make_layer<int>(2, 3, 2, 0.5, 1.0);
    for (std::size_t i = 0; i < layer.size(); ++i) {
        layer.data[i] = static_cast<int>(i * 10);
    }

    auto buf = serialize(layer);
    auto result = deserialize<Mode::NONE, Layer<int>>(buf);

    CHECK(result.rows == 2);
    CHECK(result.cols == 3);
    CHECK(result.layers == 2);
    CHECK(result.resolution == doctest::Approx(0.5));
    CHECK(result.layer_height == doctest::Approx(1.0));
    CHECK(result.centered == false);
    CHECK(result.data.size() == 12);
    CHECK(result.data[0] == 0);
    CHECK(result.data[5] == 50);
    CHECK(result.data[11] == 110);
}

TEST_CASE("serialize - Layer<double> with pose") {
    Pose pose{Point{10.0, 20.0, 30.0}, Quaternion{0.7071, 0.0, 0.0, 0.7071}};
    auto layer = make_layer<double>(3, 3, 3, 1.0, 2.0, true, pose, 1.5);

    auto buf = serialize<Mode::WITH_VERSION>(layer);
    auto result = deserialize<Mode::WITH_VERSION, Layer<double>>(buf);

    CHECK(result.rows == 3);
    CHECK(result.cols == 3);
    CHECK(result.layers == 3);
    CHECK(result.resolution == doctest::Approx(1.0));
    CHECK(result.layer_height == doctest::Approx(2.0));
    CHECK(result.centered == true);
    CHECK(result.pose.point.x == doctest::Approx(10.0));
    CHECK(result.pose.point.y == doctest::Approx(20.0));
    CHECK(result.pose.point.z == doctest::Approx(30.0));
    CHECK(result.data[0] == doctest::Approx(1.5));
}

TEST_CASE("serialize - Layer<uint8_t> with integrity") {
    auto layer = make_layer<uint8_t>(4, 4, 4, 0.25, 0.5);
    for (std::size_t i = 0; i < layer.size(); ++i) {
        layer.data[i] = static_cast<uint8_t>(i % 256);
    }

    auto buf = serialize<Mode::WITH_INTEGRITY>(layer);
    auto result = deserialize<Mode::WITH_INTEGRITY, Layer<uint8_t>>(buf);

    CHECK(result.rows == 4);
    CHECK(result.cols == 4);
    CHECK(result.layers == 4);
    CHECK(result.data.size() == 64);
    CHECK(result.data[0] == 0);
    CHECK(result.data[63] == 63);
}

TEST_CASE("serialize - Layer<float> empty") {
    Layer<float> layer;
    auto buf = serialize(layer);

    auto result = deserialize<Mode::NONE, Layer<float>>(buf);
    CHECK(result.rows == 0);
    CHECK(result.cols == 0);
    CHECK(result.layers == 0);
    CHECK(result.data.size() == 0);
}

// ============================================================================
// make_grid Factory Tests
// ============================================================================

TEST_CASE("make_grid - basic") {
    auto grid = make_grid<int>(10, 20, 0.5);

    CHECK(grid.rows == 10);
    CHECK(grid.cols == 20);
    CHECK(grid.resolution == doctest::Approx(0.5));
    CHECK(grid.centered == false);
    CHECK(grid.data.size() == 200);
}

TEST_CASE("make_grid - with all parameters") {
    Pose pose{Point{5.0, 10.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    auto grid = make_grid<double>(5, 5, 1.0, true, pose, 3.14);

    CHECK(grid.rows == 5);
    CHECK(grid.cols == 5);
    CHECK(grid.resolution == doctest::Approx(1.0));
    CHECK(grid.centered == true);
    CHECK(grid.pose.point.x == doctest::Approx(5.0));
    CHECK(grid.data.size() == 25);
    CHECK(grid.data[0] == doctest::Approx(3.14));
    CHECK(grid.data[24] == doctest::Approx(3.14));
}
