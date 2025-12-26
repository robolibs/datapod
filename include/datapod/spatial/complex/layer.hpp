#pragma once

#include <cmath>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "../point.hpp"
#include "../pose.hpp"
#include "datapod/sequential/vector.hpp"
#include "grid.hpp"

namespace datapod {

    /**
     * @brief 3D voxel grid with spatial transformation (POD)
     *
     * Pure aggregate struct with voxel grid utility methods.
     * Stores a 3D grid of values (rows x cols x layers) with optional pose transform.
     *
     * Data is stored in layer-major, row-major order:
     *   data[layer * rows * cols + row * cols + col]
     *
     * Template parameter T should be a POD type for full serializability.
     * Fully serializable and reflectable when T is serializable.
     */
    template <typename T> struct Layer {
        std::size_t rows = 0;      // Y dimension (height in cells)
        std::size_t cols = 0;      // X dimension (width in cells)
        std::size_t layers = 0;    // Z dimension (number of layers)
        double resolution = 0.0;   // XY cell size (meters per cell)
        double layer_height = 0.0; // Z spacing between layers (meters)
        bool centered = false;     // Whether grid is centered at pose
        Pose pose;                 // Spatial transform (position + orientation)
        Vector<T> data;            // Layer-major, row-major data

        auto members() noexcept { return std::tie(rows, cols, layers, resolution, layer_height, centered, pose, data); }
        auto members() const noexcept {
            return std::tie(rows, cols, layers, resolution, layer_height, centered, pose, data);
        }

        // Index conversion (layer-major, row-major)
        inline std::size_t index(std::size_t r, std::size_t c, std::size_t l) const noexcept {
            return l * rows * cols + r * cols + c;
        }

        // Data access
        inline T &operator()(std::size_t r, std::size_t c, std::size_t l) noexcept { return data[index(r, c, l)]; }

        inline const T &operator()(std::size_t r, std::size_t c, std::size_t l) const noexcept {
            return data[index(r, c, l)];
        }

        // Bounds-checked access
        inline T &at(std::size_t r, std::size_t c, std::size_t l) {
            if (r >= rows || c >= cols || l >= layers) {
                throw std::out_of_range("Layer indices out of bounds");
            }
            return data[index(r, c, l)];
        }

        inline const T &at(std::size_t r, std::size_t c, std::size_t l) const {
            if (r >= rows || c >= cols || l >= layers) {
                throw std::out_of_range("Layer indices out of bounds");
            }
            return data[index(r, c, l)];
        }

        // Get world point for a voxel (center of voxel)
        inline Point get_point(std::size_t r, std::size_t c, std::size_t l) const noexcept {
            // Local coordinates (voxel center)
            double local_x = (static_cast<double>(c) + 0.5) * resolution;
            double local_y = (static_cast<double>(r) + 0.5) * resolution;
            double local_z = (static_cast<double>(l) + 0.5) * layer_height;

            // Adjust if centered (XY only, Z starts from pose.point.z)
            if (centered) {
                local_x -= (static_cast<double>(cols) * resolution) * 0.5;
                local_y -= (static_cast<double>(rows) * resolution) * 0.5;
            }

            // Transform to world coordinates using pose
            Point local_point{local_x, local_y, local_z};
            return pose.transform_point(local_point);
        }

        // Convert world coordinates to voxel indices
        inline std::tuple<std::size_t, std::size_t, std::size_t>
        world_to_voxel(const Point &world_point) const noexcept {
            // Transform world point to grid's local coordinate system
            Point local_point = pose.inverse_transform_point(world_point);

            // Adjust if centered
            double local_x = local_point.x;
            double local_y = local_point.y;
            double local_z = local_point.z;

            if (centered) {
                local_x += (static_cast<double>(cols) * resolution) * 0.5;
                local_y += (static_cast<double>(rows) * resolution) * 0.5;
            }

            // Convert to voxel indices
            double col_d = local_x / resolution - 0.5;
            double row_d = local_y / resolution - 0.5;
            double layer_d = (layer_height > 0.0) ? (local_z / layer_height - 0.5) : 0.0;

            // Clamp to valid range
            std::size_t col =
                static_cast<std::size_t>(std::max(0.0, std::min(static_cast<double>(cols - 1), std::round(col_d))));
            std::size_t row =
                static_cast<std::size_t>(std::max(0.0, std::min(static_cast<double>(rows - 1), std::round(row_d))));
            std::size_t layer =
                static_cast<std::size_t>(std::max(0.0, std::min(static_cast<double>(layers - 1), std::round(layer_d))));

            return {row, col, layer};
        }

        // Extract a 2D grid slice at a specific layer index
        inline Grid<T> extract_grid(std::size_t layer_idx) const {
            if (layer_idx >= layers) {
                throw std::out_of_range("Layer index out of bounds");
            }

            Grid<T> grid;
            grid.rows = rows;
            grid.cols = cols;
            grid.resolution = resolution;
            grid.centered = centered;

            // Compute pose for this layer (offset in Z)
            double z_offset = (static_cast<double>(layer_idx) + 0.5) * layer_height;
            Point layer_offset{0.0, 0.0, z_offset};
            Point world_offset = pose.transform_point(layer_offset) - pose.point;
            grid.pose =
                Pose{Point{pose.point.x + world_offset.x, pose.point.y + world_offset.y, pose.point.z + world_offset.z},
                     pose.rotation};

            // Copy layer data
            grid.data.resize(rows * cols);
            std::size_t layer_start = layer_idx * rows * cols;
            for (std::size_t i = 0; i < rows * cols; ++i) {
                grid.data[i] = data[layer_start + i];
            }

            return grid;
        }

        // Set a 2D grid slice at a specific layer index
        inline void set_grid(std::size_t layer_idx, const Grid<T> &grid) {
            if (layer_idx >= layers) {
                throw std::out_of_range("Layer index out of bounds");
            }
            if (grid.rows != rows || grid.cols != cols) {
                throw std::invalid_argument("Grid dimensions must match layer dimensions");
            }

            std::size_t layer_start = layer_idx * rows * cols;
            for (std::size_t i = 0; i < rows * cols; ++i) {
                data[layer_start + i] = grid.data[i];
            }
        }

        // Comparison operators
        inline bool operator==(const Layer<T> &other) const noexcept {
            return rows == other.rows && cols == other.cols && layers == other.layers &&
                   resolution == other.resolution && layer_height == other.layer_height && centered == other.centered &&
                   pose == other.pose && data == other.data;
        }

        inline bool operator!=(const Layer<T> &other) const noexcept { return !(*this == other); }

        // Data iterators
        inline auto begin() noexcept { return data.begin(); }
        inline auto end() noexcept { return data.end(); }
        inline auto begin() const noexcept { return data.begin(); }
        inline auto end() const noexcept { return data.end(); }

        // Utility
        inline std::size_t size() const noexcept { return rows * cols * layers; }
        inline bool empty() const noexcept { return rows == 0 || cols == 0 || layers == 0; }
        inline bool is_valid() const noexcept {
            return rows > 0 && cols > 0 && layers > 0 && data.size() == rows * cols * layers;
        }

        // Dimension accessors (for compatibility with old concord API)
        inline std::size_t layer_count() const noexcept { return layers; }
        inline double get_layer_height() const noexcept { return layer_height; }
        inline double get_resolution() const noexcept { return resolution; }
        inline const Pose &shift() const noexcept { return pose; }
    };

    // Factory function to create a properly initialized Layer
    template <typename T>
    inline Layer<T> make_layer(std::size_t rows, std::size_t cols, std::size_t layers, double resolution,
                               double layer_height, bool centered = false, const Pose &pose = Pose{},
                               const T &default_value = T{}) {
        Layer<T> layer;
        layer.rows = rows;
        layer.cols = cols;
        layer.layers = layers;
        layer.resolution = resolution;
        layer.layer_height = layer_height;
        layer.centered = centered;
        layer.pose = pose;
        layer.data.resize(rows * cols * layers, default_value);
        return layer;
    }

    // Factory function to create a properly initialized Grid
    template <typename T>
    inline Grid<T> make_grid(std::size_t rows, std::size_t cols, double resolution, bool centered = false,
                             const Pose &pose = Pose{}, const T &default_value = T{}) {
        Grid<T> grid;
        grid.rows = rows;
        grid.cols = cols;
        grid.resolution = resolution;
        grid.centered = centered;
        grid.pose = pose;
        grid.data.resize(rows * cols, default_value);
        return grid;
    }

} // namespace datapod
