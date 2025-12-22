#pragma once

#include <cmath>
#include <tuple>

#include "../sequential/array.hpp"
#include "euler.hpp"
#include "point.hpp"
#include "size.hpp"

namespace datapod {

    /**
     * @brief Oriented Bounding Box (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Use aggregate initialization: OBB{center, half_extents, orientation}
     * Fully serializable and reflectable.
     *
     * Note: Similar to Box, but uses half_extents instead of full size
     * and separate orientation instead of full Pose.
     */
    struct OBB {
        Point center;
        Size half_extents;
        Euler orientation;

        auto members() noexcept { return std::tie(center, half_extents, orientation); }
        auto members() const noexcept { return std::tie(center, half_extents, orientation); }

        // Calculate the volume of the OBB
        inline double volume() const noexcept {
            // Volume = (2 * half_extents.x) * (2 * half_extents.y) * (2 * half_extents.z)
            return 8.0 * half_extents.x * half_extents.y * half_extents.z;
        }

        // Calculate the surface area of the OBB
        inline double surface_area() const noexcept {
            // Full dimensions
            double width = 2.0 * half_extents.x;
            double height = 2.0 * half_extents.y;
            double depth = 2.0 * half_extents.z;
            return 2.0 * (width * height + height * depth + depth * width);
        }

        // Get all 8 corners of the OBB in world coordinates
        // Order: bottom face (CCW from -x,-y), then top face (CCW from -x,-y)
        inline Array<Point, 8> corners() const noexcept {
            Array<Point, 8> pts;

            // Local coordinates (before rotation) using half-extents
            double hx = half_extents.x;
            double hy = half_extents.y;
            double hz = half_extents.z;

            Point local[8] = {
                {-hx, -hy, -hz}, // 0: bottom-back-left
                {hx, -hy, -hz},  // 1: bottom-back-right
                {hx, hy, -hz},   // 2: bottom-front-right
                {-hx, hy, -hz},  // 3: bottom-front-left
                // Top face (z = +hz)
                {-hx, -hy, hz}, // 4: top-back-left
                {hx, -hy, hz},  // 5: top-back-right
                {hx, hy, hz},   // 6: top-front-right
                {-hx, hy, hz}   // 7: top-front-left
            };

            // For now, simplified: just translate by center (no rotation)
            // TODO: Apply rotation using orientation (Euler angles)
            for (int i = 0; i < 8; ++i) {
                pts[i] = Point{local[i].x + center.x, local[i].y + center.y, local[i].z + center.z};
            }

            return pts;
        }

        // Simple containment check (axis-aligned, ignores rotation for now)
        // TODO: Implement proper OBB containment with rotation
        inline bool contains(const Point &p) const noexcept {
            // Check if point is within box bounds (axis-aligned)
            double dx = std::abs(p.x - center.x);
            double dy = std::abs(p.y - center.y);
            double dz = std::abs(p.z - center.z);

            return dx <= half_extents.x && dy <= half_extents.y && dz <= half_extents.z;
        }

        // Get the full dimensions (not half-extents)
        inline Size full_size() const noexcept {
            return Size{2.0 * half_extents.x, 2.0 * half_extents.y, 2.0 * half_extents.z};
        }
    };

} // namespace datapod
