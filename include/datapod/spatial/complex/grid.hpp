#pragma once

#include <tuple>

#include "../pose.hpp"
#include "datapod/sequential/vector.hpp"

namespace datapod {

    /**
     * @brief 2D grid with spatial transformation (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
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
    };

} // namespace datapod
