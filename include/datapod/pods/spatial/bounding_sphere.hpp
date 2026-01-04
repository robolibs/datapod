#pragma once

#include <tuple>

#include "point.hpp"

namespace datapod {

    /**
     * @brief Bounding Sphere (POD)
     *
     * Pure aggregate struct - no constructors, no methods.
     * Use aggregate initialization: BoundingSphere{center, radius}
     * Fully serializable and reflectable.
     */
    struct BoundingSphere {
        Point center;
        double radius = 0.0;

        auto members() noexcept { return std::tie(center, radius); }
        auto members() const noexcept { return std::tie(center, radius); }
    };

    namespace bounding_sphere {
        /// Create a bounding sphere from center and radius
        inline BoundingSphere make(const Point &center, double radius) noexcept {
            return BoundingSphere{center, radius};
        }

        /// Create a bounding sphere from center coordinates and radius
        inline BoundingSphere make(double x, double y, double z, double radius) noexcept {
            return BoundingSphere{Point{x, y, z}, radius};
        }

        /// Create a unit sphere at origin
        inline BoundingSphere unit() noexcept { return BoundingSphere{Point{0.0, 0.0, 0.0}, 1.0}; }
    } // namespace bounding_sphere

} // namespace datapod
