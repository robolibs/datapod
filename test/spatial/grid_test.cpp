#include <doctest/doctest.h>

#include <datapod/spatial/complex/grid.hpp>
#include <datapod/spatial/euler.hpp>

using namespace datapod;

TEST_CASE("Grid - Default construction") {
    Grid<int> grid;
    CHECK(grid.rows == 0);
    CHECK(grid.cols == 0);
    CHECK(grid.resolution == 0.0);
    CHECK(grid.centered == false);
    CHECK(grid.data.empty());
}

TEST_CASE("Grid - Aggregate initialization") {
    Grid<int> grid{3, 4, 0.5, false, Pose{}, Vector<int>{}};
    CHECK(grid.rows == 3);
    CHECK(grid.cols == 4);
    CHECK(grid.resolution == 0.5);
    CHECK(grid.centered == false);
}

TEST_CASE("Grid - members() reflection") {
    Grid<int> grid{2, 2, 1.0, true, Pose{}, Vector<int>{}};
    auto m = grid.members();
    CHECK(&std::get<0>(m) == &grid.rows);
    CHECK(&std::get<1>(m) == &grid.cols);
    CHECK(&std::get<2>(m) == &grid.resolution);
    CHECK(&std::get<3>(m) == &grid.centered);
    CHECK(&std::get<4>(m) == &grid.pose);
    CHECK(&std::get<5>(m) == &grid.data);
}

TEST_CASE("Grid - const members() reflection") {
    const Grid<int> grid{2, 2, 1.0, false, Pose{}, Vector<int>{}};
    auto m = grid.members();
    CHECK(&std::get<0>(m) == &grid.rows);
    CHECK(&std::get<1>(m) == &grid.cols);
}

TEST_CASE("Grid - index conversion") {
    Grid<int> grid{3, 4, 1.0, false, Pose{}, Vector<int>{}};
    CHECK(grid.index(0, 0) == 0);
    CHECK(grid.index(0, 1) == 1);
    CHECK(grid.index(1, 0) == 4);
    CHECK(grid.index(2, 3) == 11);
}

TEST_CASE("Grid - operator() access") {
    Grid<int> grid{2, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4}};
    CHECK(grid(0, 0) == 1);
    CHECK(grid(0, 1) == 2);
    CHECK(grid(1, 0) == 3);
    CHECK(grid(1, 1) == 4);
}

TEST_CASE("Grid - operator() modification") {
    Grid<int> grid{2, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4}};
    grid(1, 1) = 99;
    CHECK(grid(1, 1) == 99);
}

TEST_CASE("Grid - at() bounds checking - valid access") {
    Grid<int> grid{2, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4}};
    CHECK(grid.at(0, 0) == 1);
    CHECK(grid.at(1, 1) == 4);
}

TEST_CASE("Grid - at() bounds checking - row out of bounds") {
    Grid<int> grid{2, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4}};
    CHECK_THROWS_AS(grid.at(2, 0), std::out_of_range);
}

TEST_CASE("Grid - at() bounds checking - col out of bounds") {
    Grid<int> grid{2, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4}};
    CHECK_THROWS_AS(grid.at(0, 2), std::out_of_range);
}

TEST_CASE("Grid - get_point for non-centered grid") {
    Grid<int> grid{2, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4}};
    Point p = grid.get_point(0, 0);
    // Cell center should be at (0.5, 0.5) for non-centered grid
    CHECK(p.x == doctest::Approx(0.5));
    CHECK(p.y == doctest::Approx(0.5));
    CHECK(p.z == doctest::Approx(0.0));
}

TEST_CASE("Grid - get_point for centered grid") {
    Grid<int> grid{2, 2, 1.0, true, Pose{}, Vector<int>{1, 2, 3, 4}};
    Point p = grid.get_point(0, 0);
    // For centered grid of 2x2 with resolution 1.0, cell (0,0) should be at (-0.5, -0.5)
    CHECK(p.x == doctest::Approx(-0.5));
    CHECK(p.y == doctest::Approx(-0.5));
    CHECK(p.z == doctest::Approx(0.0));
}

TEST_CASE("Grid - get_point with pose translation") {
    Pose pose{Point{10.0, 20.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    Grid<int> grid{2, 2, 1.0, false, pose, Vector<int>{1, 2, 3, 4}};
    Point p = grid.get_point(0, 0);
    // Should be translated by pose
    CHECK(p.x == doctest::Approx(10.5));
    CHECK(p.y == doctest::Approx(20.5));
}

TEST_CASE("Grid - world_to_grid for simple case") {
    Grid<int> grid{4, 4, 1.0, false, Pose{}, Vector<int>{}};
    // Point at (0.5, 0.5) should map to cell (0, 0)
    auto [r, c] = grid.world_to_grid(Point{0.5, 0.5, 0.0});
    CHECK(r == 0);
    CHECK(c == 0);
}

TEST_CASE("Grid - world_to_grid for centered grid") {
    Grid<int> grid{4, 4, 1.0, true, Pose{}, Vector<int>{}};
    // For centered 4x4 grid, origin is at center
    // Point at (0, 0) should map to center cells
    auto [r, c] = grid.world_to_grid(Point{0.0, 0.0, 0.0});
    CHECK(r == 2); // Center row (rounded)
    CHECK(c == 2); // Center col (rounded)
}

TEST_CASE("Grid - world_to_grid with pose translation") {
    Pose pose{Point{10.0, 20.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    Grid<int> grid{4, 4, 1.0, false, pose, Vector<int>{}};
    // Point at pose origin should map to cell (0, 0)
    auto [r, c] = grid.world_to_grid(Point{10.5, 20.5, 0.0});
    CHECK(r == 0);
    CHECK(c == 0);
}

TEST_CASE("Grid - world_to_grid clamping") {
    Grid<int> grid{4, 4, 1.0, false, Pose{}, Vector<int>{}};
    // Point outside grid should be clamped
    auto [r, c] = grid.world_to_grid(Point{100.0, 100.0, 0.0});
    CHECK(r == 3); // Max row
    CHECK(c == 3); // Max col
}

TEST_CASE("Grid - corners for non-centered grid") {
    Grid<int> grid{3, 4, 1.0, false, Pose{}, Vector<int>{}};
    auto corners = grid.corners();
    // Top-left (0, 0) -> (0.5, 0.5)
    CHECK(corners[0].x == doctest::Approx(0.5));
    CHECK(corners[0].y == doctest::Approx(0.5));
    // Top-right (0, 3) -> (3.5, 0.5)
    CHECK(corners[1].x == doctest::Approx(3.5));
    CHECK(corners[1].y == doctest::Approx(0.5));
    // Bottom-right (2, 3) -> (3.5, 2.5)
    CHECK(corners[2].x == doctest::Approx(3.5));
    CHECK(corners[2].y == doctest::Approx(2.5));
    // Bottom-left (2, 0) -> (0.5, 2.5)
    CHECK(corners[3].x == doctest::Approx(0.5));
    CHECK(corners[3].y == doctest::Approx(2.5));
}

TEST_CASE("Grid - corners for centered grid") {
    Grid<int> grid{2, 2, 1.0, true, Pose{}, Vector<int>{}};
    auto corners = grid.corners();
    // For centered 2x2 grid
    // Top-left (0, 0) -> (-0.5, -0.5)
    CHECK(corners[0].x == doctest::Approx(-0.5));
    CHECK(corners[0].y == doctest::Approx(-0.5));
}

TEST_CASE("Grid - operator== for equal grids") {
    Grid<int> g1{2, 3, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4, 5, 6}};
    Grid<int> g2{2, 3, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4, 5, 6}};
    CHECK(g1 == g2);
}

TEST_CASE("Grid - operator== for different dimensions") {
    Grid<int> g1{2, 3, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4, 5, 6}};
    Grid<int> g2{3, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4, 5, 6}};
    CHECK_FALSE(g1 == g2);
}

TEST_CASE("Grid - operator== for different data") {
    Grid<int> g1{2, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4}};
    Grid<int> g2{2, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 5}};
    CHECK_FALSE(g1 == g2);
}

TEST_CASE("Grid - operator!= ") {
    Grid<int> g1{2, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4}};
    Grid<int> g2{2, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 5}};
    CHECK(g1 != g2);
}

TEST_CASE("Grid - iterators") {
    Grid<int> grid{2, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4}};

    // Test begin/end
    auto it = grid.begin();
    CHECK(*it == 1);
    ++it;
    CHECK(*it == 2);

    // Test iteration
    int sum = 0;
    for (auto val : grid) {
        sum += val;
    }
    CHECK(sum == 10);
}

TEST_CASE("Grid - const iterators") {
    const Grid<int> grid{2, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4}};

    int sum = 0;
    for (auto val : grid) {
        sum += val;
    }
    CHECK(sum == 10);
}

TEST_CASE("Grid - size()") {
    Grid<int> grid{3, 4, 1.0, false, Pose{}, Vector<int>{}};
    CHECK(grid.size() == 12);
}

TEST_CASE("Grid - empty() - true") {
    Grid<int> grid;
    CHECK(grid.empty());
}

TEST_CASE("Grid - empty() - false") {
    Grid<int> grid{2, 2, 1.0, false, Pose{}, Vector<int>{}};
    CHECK_FALSE(grid.empty());
}

TEST_CASE("Grid - is_valid() - true") {
    Grid<int> grid{2, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3, 4}};
    CHECK(grid.is_valid());
}

TEST_CASE("Grid - is_valid() - false (empty)") {
    Grid<int> grid;
    CHECK_FALSE(grid.is_valid());
}

TEST_CASE("Grid - is_valid() - false (data size mismatch)") {
    Grid<int> grid{2, 2, 1.0, false, Pose{}, Vector<int>{1, 2, 3}}; // Missing one element
    CHECK_FALSE(grid.is_valid());
}

TEST_CASE("Grid - roundtrip get_point/world_to_grid") {
    Grid<int> grid{10, 10, 0.5, false, Pose{}, Vector<int>{}};

    // Get world point for cell (5, 7)
    Point world_point = grid.get_point(5, 7);

    // Convert back to grid indices
    auto [r, c] = grid.world_to_grid(world_point);

    // Should get back same indices
    CHECK(r == 5);
    CHECK(c == 7);
}

TEST_CASE("Grid - roundtrip with centered grid") {
    Grid<int> grid{8, 8, 1.0, true, Pose{}, Vector<int>{}};

    // Test several cells
    for (std::size_t r = 0; r < 8; ++r) {
        for (std::size_t c = 0; c < 8; ++c) {
            Point world_point = grid.get_point(r, c);
            auto [r_back, c_back] = grid.world_to_grid(world_point);
            CHECK(r_back == r);
            CHECK(c_back == c);
        }
    }
}

TEST_CASE("Grid - roundtrip with pose translation") {
    Pose pose{Point{100.0, 200.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
    Grid<int> grid{5, 5, 2.0, true, pose, Vector<int>{}};

    // Test a few cells
    Point world_point = grid.get_point(2, 3);
    auto [r, c] = grid.world_to_grid(world_point);
    CHECK(r == 2);
    CHECK(c == 3);
}
