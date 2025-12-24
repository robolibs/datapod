#pragma once

#include <tuple>

#include "../../matrix/vector.hpp"
#include "../point.hpp"

namespace datapod {

    /**
     * @brief Infinite line defined by origin point and direction vector
     *
     * Line represents an infinite line in 3D space, defined by a point
     * and a direction vector. Any point on the line can be expressed as:
     * P(t) = origin + t * direction (for any real t)
     *
     * Use this for infinite lines (rays, axes, etc.)
     * For bounded line segments, use Segment (start + end points).
     *
     * Features:
     * - POD-compatible
     * - Serializable via members()
     * - Direction vector should typically be normalized
     */
    struct Line {
        Point origin;    ///< Point on the line
        Point direction; ///< Direction vector (typically normalized)

        auto members() noexcept { return std::tie(origin, direction); }
        auto members() const noexcept { return std::tie(origin, direction); }

        // Distance queries for infinite line
        inline Point closest_point(const Point &p) const noexcept {
            const Point to_p = p - origin;
            const double dir_mag_sq = direction.x * direction.x + direction.y * direction.y + direction.z * direction.z;

            if (dir_mag_sq < 1e-10) {
                return origin; // Degenerate line
            }

            const double t = (to_p.x * direction.x + to_p.y * direction.y + to_p.z * direction.z) / dir_mag_sq;

            return Point{origin.x + t * direction.x, origin.y + t * direction.y, origin.z + t * direction.z};
        }

        inline double distance_to(const Point &p) const noexcept { return p.distance_to(closest_point(p)); }

        // SIMD conversion: Line → mat::vector<double, 6> (origin(3), direction(3))
        inline mat::vector<double, 6> to_mat() const noexcept {
            mat::vector<double, 6> v;
            v[0] = origin.x;
            v[1] = origin.y;
            v[2] = origin.z;
            v[3] = direction.x;
            v[4] = direction.y;
            v[5] = direction.z;
            return v;
        }

        // SIMD conversion: mat::vector<double, 6> → Line
        static inline Line from_mat(const mat::vector<double, 6> &v) noexcept {
            return Line{Point{v[0], v[1], v[2]}, Point{v[3], v[4], v[5]}};
        }
    };

} // namespace datapod
