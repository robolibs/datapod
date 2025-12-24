#pragma once

#include <cmath>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "../point.hpp"
#include "../pose.hpp"
#include "datapod/matrix/matrix.hpp"
#include "datapod/sequential/array.hpp"
#include "datapod/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief 2D grid with spatial transformation (POD)
     *
     * Pure aggregate struct with grid utility methods.
     * Stores a 2D grid of values with optional pose transform.
     *
     * Grid data is stored in row-major order: data[row * cols + col]
     *
     * Template parameter T should be a POD type for full serializability.
     * Fully serializable and reflectable when T is serializable.
     */
    template <typename T> struct Grid {
        std::size_t rows = 0;
        std::size_t cols = 0;
        double resolution = 0.0; // Cell size (inradius from concord)
        bool centered = false;   // Whether grid is centered at pose
        Pose pose;               // Spatial transform
        Vector<T> data;          // Row-major grid data

        auto members() noexcept { return std::tie(rows, cols, resolution, centered, pose, data); }
        auto members() const noexcept { return std::tie(rows, cols, resolution, centered, pose, data); }

        // Index conversion
        inline std::size_t index(std::size_t r, std::size_t c) const noexcept { return r * cols + c; }

        // Data access
        inline T &operator()(std::size_t r, std::size_t c) noexcept { return data[index(r, c)]; }

        inline const T &operator()(std::size_t r, std::size_t c) const noexcept { return data[index(r, c)]; }

        // Bounds-checked access
        inline T &at(std::size_t r, std::size_t c) {
            if (r >= rows || c >= cols) {
                throw std::out_of_range("Grid indices out of bounds");
            }
            return data[index(r, c)];
        }

        inline const T &at(std::size_t r, std::size_t c) const {
            if (r >= rows || c >= cols) {
                throw std::out_of_range("Grid indices out of bounds");
            }
            return data[index(r, c)];
        }

        // Get world point for a grid cell (center of cell)
        inline Point get_point(std::size_t r, std::size_t c) const noexcept {
            // Local coordinates (cell center)
            double local_x = (static_cast<double>(c) + 0.5) * resolution;
            double local_y = (static_cast<double>(r) + 0.5) * resolution;

            // Adjust if centered
            if (centered) {
                local_x -= (static_cast<double>(cols) * resolution) * 0.5;
                local_y -= (static_cast<double>(rows) * resolution) * 0.5;
            }

            // Transform to world coordinates using pose
            Point local_point{local_x, local_y, 0.0};
            return pose.transform_point(local_point);
        }

        // Convert world coordinates to grid indices
        inline std::pair<std::size_t, std::size_t> world_to_grid(const Point &world_point) const noexcept {
            // Transform world point to grid's local coordinate system
            Point local_point = pose.inverse_transform_point(world_point);

            // Adjust if centered
            double local_x = local_point.x;
            double local_y = local_point.y;

            if (centered) {
                local_x += (static_cast<double>(cols) * resolution) * 0.5;
                local_y += (static_cast<double>(rows) * resolution) * 0.5;
            }

            // Convert to grid indices
            double col_d = local_x / resolution - 0.5;
            double row_d = local_y / resolution - 0.5;

            // Clamp to valid range
            std::size_t col =
                static_cast<std::size_t>(std::max(0.0, std::min(static_cast<double>(cols - 1), std::round(col_d))));
            std::size_t row =
                static_cast<std::size_t>(std::max(0.0, std::min(static_cast<double>(rows - 1), std::round(row_d))));

            return {row, col};
        }

        // Get corner points of the grid
        inline Array<Point, 4> corners() const noexcept {
            if (rows == 0 || cols == 0) {
                return Array<Point, 4>{};
            }
            return Array<Point, 4>{{
                get_point(0, 0),               // top-left
                get_point(0, cols - 1),        // top-right
                get_point(rows - 1, cols - 1), // bottom-right
                get_point(rows - 1, 0)         // bottom-left
            }};
        }

        // Comparison operators
        inline bool operator==(const Grid<T> &other) const noexcept {
            return rows == other.rows && cols == other.cols && resolution == other.resolution &&
                   centered == other.centered && pose == other.pose && data == other.data;
        }

        inline bool operator!=(const Grid<T> &other) const noexcept { return !(*this == other); }

        // Data iterators
        inline auto begin() noexcept { return data.begin(); }
        inline auto end() noexcept { return data.end(); }
        inline auto begin() const noexcept { return data.begin(); }
        inline auto end() const noexcept { return data.end(); }

        // Utility
        inline std::size_t size() const noexcept { return rows * cols; }
        inline bool empty() const noexcept { return rows == 0 || cols == 0; }
        inline bool is_valid() const noexcept { return rows > 0 && cols > 0 && data.size() == rows * cols; }

        // Note: Grid has runtime dimensions, but mat::matrix requires compile-time dimensions.
        // For SIMD operations on grid data, access grid.data directly (it's a Vector<T>)
        // or use to_mat<R,C>() if dimensions are known at compile time.

        // Conversion to mat::matrix for compile-time known dimensions
        // Example: auto m = grid.to_mat<10, 10>(); // for a 10x10 grid
        template <std::size_t R, std::size_t C>
        inline mat::matrix<T, R, C> to_mat() const noexcept
        requires(std::is_arithmetic_v<T>)
        {
            mat::matrix<T, R, C> result;
            // Runtime check: only convert if sizes match
            if (rows == R && cols == C) {
                for (std::size_t r = 0; r < R; ++r) {
                    for (std::size_t c = 0; c < C; ++c) {
                        result(r, c) = data[index(r, c)];
                    }
                }
            } else {
                // Size mismatch - return zero-initialized matrix
                result.fill(T{});
            }
            return result;
        }

        // Create Grid from mat::matrix
        template <std::size_t R, std::size_t C>
        static inline Grid<T> from_mat(const mat::matrix<T, R, C> &m, double res = 1.0, bool cent = false,
                                       const Pose &p = Pose{})
        requires(std::is_arithmetic_v<T>)
        {
            Grid<T> grid;
            grid.rows = R;
            grid.cols = C;
            grid.resolution = res;
            grid.centered = cent;
            grid.pose = p;
            grid.data.resize(R * C);
            for (std::size_t r = 0; r < R; ++r) {
                for (std::size_t c = 0; c < C; ++c) {
                    grid.data[r * C + c] = m(r, c);
                }
            }
            return grid;
        }
    };

} // namespace datapod
