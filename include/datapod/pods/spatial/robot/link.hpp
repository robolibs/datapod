#pragma once

#include <tuple>

#include "collision.hpp"
#include "datapod/pods/adapters/optional.hpp"
#include "datapod/pods/sequential/string.hpp"
#include "datapod/pods/sequential/vector.hpp"
#include "inertia.hpp"
#include "visual.hpp"

namespace datapod {

    /**
     * @brief Link - Robot link description (POD)
     *
     * Represents a link (rigid body) in a robot model.
     * Used in URDF/SDF-style robot definitions.
     */
    struct Link {
        String name;
        Optional<Inertia> inertial;
        Vector<Visual> visuals;
        Vector<Collision> collisions;

        auto members() noexcept { return std::tie(name, inertial, visuals, collisions); }
        auto members() const noexcept { return std::tie(name, inertial, visuals, collisions); }

        inline bool has_inertial() const noexcept { return inertial.has_value(); }
        inline bool has_visuals() const noexcept { return !visuals.empty(); }
        inline bool has_collisions() const noexcept { return !collisions.empty(); }

        inline bool operator==(const Link &other) const noexcept {
            return name == other.name && inertial == other.inertial && visuals == other.visuals &&
                   collisions == other.collisions;
        }
        inline bool operator!=(const Link &other) const noexcept { return !(*this == other); }
    };

    namespace link {
        inline Link make(const String &name) noexcept { return Link{name, nullopt, {}, {}}; }

        inline Link make(const String &name, const Inertia &inertial) noexcept { return Link{name, inertial, {}, {}}; }
    } // namespace link

} // namespace datapod
