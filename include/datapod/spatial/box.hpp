#pragma once

#include <cmath>
#include <tuple>

#include "../matrix/vector.hpp"
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

        // SIMD conversion: Box → mat::Vector<double, 10> (pose(7), size(3))
        inline mat::Vector<double, 10> to_mat() const noexcept {
            mat::Vector<double, 10> v;
            auto pose_v = pose.to_mat();
            v[0] = pose_v[0]; // point.x
            v[1] = pose_v[1]; // point.y
            v[2] = pose_v[2]; // point.z
            v[3] = pose_v[3]; // quat.x
            v[4] = pose_v[4]; // quat.y
            v[5] = pose_v[5]; // quat.z
            v[6] = pose_v[6]; // quat.w
            v[7] = size.x;
            v[8] = size.y;
            v[9] = size.z;
            return v;
        }

        // SIMD conversion: mat::Vector<double, 10> → Box
        static inline Box from_mat(const mat::Vector<double, 10> &v) noexcept {
            mat::Vector<double, 7> pose_v;
            pose_v[0] = v[0];
            pose_v[1] = v[1];
            pose_v[2] = v[2];
            pose_v[3] = v[3];
            pose_v[4] = v[4];
            pose_v[5] = v[5];
            pose_v[6] = v[6];
            return Box{Pose::from_mat(pose_v), Size{v[7], v[8], v[9]}};
        }
    };

} // namespace datapod
