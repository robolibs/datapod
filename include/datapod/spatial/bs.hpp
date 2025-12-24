#pragma once

#include <cmath>
#include <tuple>

#include "../matrix/vector.hpp"
#include "aabb.hpp"
#include "point.hpp"

namespace datapod {

    /**
     * @brief Bounding Sphere (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Use aggregate initialization: BS{center, radius}
     * Fully serializable and reflectable.
     *
     * Short name for BoundingSphere - commonly used in collision detection.
     */
    struct BS {
        Point center;
        double radius = 0.0;

        auto members() noexcept { return std::tie(center, radius); }
        auto members() const noexcept { return std::tie(center, radius); }

        // Calculate the volume of the sphere (4/3 * π * r³)
        inline double volume() const noexcept {
            constexpr double four_thirds = 4.0 / 3.0;
            return four_thirds * M_PI * radius * radius * radius;
        }

        // Calculate the surface area of the sphere (4 * π * r²)
        inline double surface_area() const noexcept { return 4.0 * M_PI * radius * radius; }

        // Check if a point is inside the sphere
        inline bool contains(const Point &p) const noexcept {
            double dist = center.distance_to(p);
            return dist <= radius;
        }

        // Check if this sphere intersects with another sphere
        inline bool intersects(const BS &other) const noexcept {
            double dist = center.distance_to(other.center);
            double combined_radius = radius + other.radius;
            return dist <= combined_radius;
        }

        // Get the axis-aligned bounding box that contains this sphere
        inline AABB get_aabb() const noexcept {
            return AABB{{center.x - radius, center.y - radius, center.z - radius},
                        {center.x + radius, center.y + radius, center.z + radius}};
        }

        // Expand the sphere to include a point (mutating operation)
        inline void expand(const Point &p) noexcept {
            double dist = center.distance_to(p);
            if (dist > radius) {
                radius = dist;
            }
        }

        // Expand the sphere to include another sphere (mutating operation)
        inline void expand(const BS &other) noexcept {
            double dist = center.distance_to(other.center);
            double new_radius = dist + other.radius;
            if (new_radius > radius) {
                radius = new_radius;
            }
        }

        // Get the diameter of the sphere
        inline double diameter() const noexcept { return 2.0 * radius; }

        // SIMD conversion: BS → mat::vector<double, 4> (center(3), radius)
        inline mat::vector<double, 4> to_mat() const noexcept {
            mat::vector<double, 4> v;
            v[0] = center.x;
            v[1] = center.y;
            v[2] = center.z;
            v[3] = radius;
            return v;
        }

        // SIMD conversion: mat::vector<double, 4> → BS
        static inline BS from_mat(const mat::vector<double, 4> &v) noexcept {
            return BS{Point{v[0], v[1], v[2]}, v[3]};
        }
    };

} // namespace datapod
