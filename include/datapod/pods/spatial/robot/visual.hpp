#pragma once

#include <tuple>

#include "datapod/pods/spatial/pose.hpp"
#include "geometry.hpp"

namespace datapod {

    /**
     * @brief Visual - Visual representation of a robot link (POD)
     *
     * Represents a visual element with origin transform and geometry.
     * Used in URDF/SDF-style robot definitions.
     */
    struct Visual {
        Pose origin;
        Geometry geom;

        auto members() noexcept { return std::tie(origin, geom); }
        auto members() const noexcept { return std::tie(origin, geom); }

        inline bool is_set() const noexcept { return origin.is_set(); }

        inline bool operator==(const Visual &other) const noexcept {
            return origin == other.origin && geom == other.geom;
        }
        inline bool operator!=(const Visual &other) const noexcept { return !(*this == other); }
    };

    namespace visual {
        inline Visual make(const Geometry &geom) noexcept { return Visual{pose::identity(), geom}; }
        inline Visual make(const Pose &origin, const Geometry &geom) noexcept { return Visual{origin, geom}; }
    } // namespace visual

} // namespace datapod
