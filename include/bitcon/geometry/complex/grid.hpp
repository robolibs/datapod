#pragma once

#include "../pose.hpp"
#include "bitcon/containers/vector.hpp"

namespace bitcon {

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
    };

} // namespace bitcon
