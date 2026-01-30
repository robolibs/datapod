#pragma once

#include <tuple>

#include "datapod/pods/spatial/pose.hpp"
#include "geometry.hpp"

namespace datapod {

    /**
     * @brief Collision - Collision geometry of a robot link (POD)
     *
     * Represents a collision element with origin transform and geometry.
     * Used in URDF/SDF-style robot definitions for collision detection.
     */
    struct Collision {
        Pose origin;
        Geometry geom;

        auto members() noexcept { return std::tie(origin, geom); }
        auto members() const noexcept { return std::tie(origin, geom); }

        inline bool is_set() const noexcept { return origin.is_set(); }

        inline bool operator==(const Collision &other) const noexcept {
            return origin == other.origin && geom == other.geom;
        }
        inline bool operator!=(const Collision &other) const noexcept { return !(*this == other); }
    };

    namespace collision {
        inline Collision make(const Geometry &geom) noexcept { return Collision{pose::identity(), geom}; }
        inline Collision make(const Pose &origin, const Geometry &geom) noexcept { return Collision{origin, geom}; }
    } // namespace collision

} // namespace datapod
