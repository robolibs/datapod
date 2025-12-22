#pragma once

#include <cmath>
#include <tuple>

#include "../sequential/array.hpp"
#include "pose.hpp"
#include "size.hpp"

namespace datapod {

    struct Box {
        Pose pose;
        Size size;

        auto members() noexcept { return std::tie(pose, size); }
        auto members() const noexcept { return std::tie(pose, size); }

        // Get the center point of the box (from pose)
        inline Point center() const noexcept { return pose.point; }

        // Calculate the volume of the box
        inline double volume() const noexcept { return size.x * size.y * size.z; }

        // Calculate the surface area of the box
        inline double surface_area() const noexcept {
            return 2.0 * (size.x * size.y + size.y * size.z + size.z * size.x);
        }

        // Get all 8 corners of the box in world coordinates
        // Order: bottom face (CCW from -x,-y), then top face (CCW from -x,-y)
        inline Array<Point, 8> corners() const noexcept {
            Array<Point, 8> pts;

            // Half extents from center
            double hx = size.x / 2.0;
            double hy = size.y / 2.0;
            double hz = size.z / 2.0;

            // Local coordinates (before rotation)
            // Bottom face (z = -hz)
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

            // For now, simplified: just translate by pose.point (no rotation)
            // TODO: Apply rotation using pose.angle (Euler angles)
            for (int i = 0; i < 8; ++i) {
                pts[i] = Point{local[i].x + pose.point.x, local[i].y + pose.point.y, local[i].z + pose.point.z};
            }

            return pts;
        }

        // Simple containment check (axis-aligned, ignores rotation for now)
        // TODO: Implement proper OBB containment with rotation
        inline bool contains(const Point &p) const noexcept {
            // Get half extents
            double hx = size.x / 2.0;
            double hy = size.y / 2.0;
            double hz = size.z / 2.0;

            // Check if point is within box bounds (axis-aligned)
            double dx = std::abs(p.x - pose.point.x);
            double dy = std::abs(p.y - pose.point.y);
            double dz = std::abs(p.z - pose.point.z);

            return dx <= hx && dy <= hy && dz <= hz;
        }
    };

} // namespace datapod
