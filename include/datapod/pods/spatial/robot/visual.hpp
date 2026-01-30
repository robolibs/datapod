#pragma once

#include <tuple>

#include "datapod/pods/adapters/optional.hpp"
#include "datapod/pods/sequential/array.hpp"
#include "datapod/pods/sequential/string.hpp"
#include "datapod/pods/spatial/pose.hpp"
#include "datapod/types/types.hpp"
#include "geometry.hpp"

namespace datapod {
    namespace robot {

        /**
         * @brief Material - Visual material properties (POD)
         *
         * Represents material properties for visual elements.
         * Used in URDF/SDF-style robot definitions.
         */
        struct Material {
            String name;
            Array<f64, 4> rgba{1.0, 1.0, 1.0, 1.0}; // RGBA color (default white)
            String texture;                         // Optional texture URI

            auto members() noexcept { return std::tie(name, rgba, texture); }
            auto members() const noexcept { return std::tie(name, rgba, texture); }

            inline bool has_texture() const noexcept { return !texture.empty(); }

            inline bool operator==(const Material &other) const noexcept {
                return name == other.name && rgba == other.rgba && texture == other.texture;
            }
            inline bool operator!=(const Material &other) const noexcept { return !(*this == other); }
        };

        namespace material {
            inline Material make(const String &name, f64 r, f64 g, f64 b, f64 a = 1.0) noexcept {
                return Material{name, Array<f64, 4>{r, g, b, a}, String{}};
            }
            inline Material color(f64 r, f64 g, f64 b, f64 a = 1.0) noexcept {
                return Material{String{}, Array<f64, 4>{r, g, b, a}, String{}};
            }
            inline Material textured(const String &texture_uri) noexcept {
                return Material{String{}, Array<f64, 4>{1.0, 1.0, 1.0, 1.0}, texture_uri};
            }
        } // namespace material

        /**
         * @brief Visual - Visual representation of a robot link (POD)
         *
         * Represents a visual element with origin transform, geometry, and material.
         * Used in URDF/SDF-style robot definitions.
         */
        struct Visual {
            String name; // Optional name for round-tripping
            Pose origin;
            Geometry geom;
            Optional<Material> material;

            auto members() noexcept { return std::tie(name, origin, geom, material); }
            auto members() const noexcept { return std::tie(name, origin, geom, material); }

            inline bool is_set() const noexcept { return origin.is_set() || !name.empty(); }

            inline bool operator==(const Visual &other) const noexcept {
                return name == other.name && origin == other.origin && geom == other.geom && material == other.material;
            }
            inline bool operator!=(const Visual &other) const noexcept { return !(*this == other); }
        };

        namespace visual {
            inline Visual make(const Geometry &geom) noexcept {
                return Visual{String{}, pose::identity(), geom, nullopt};
            }
            inline Visual make(const Pose &origin, const Geometry &geom) noexcept {
                return Visual{String{}, origin, geom, nullopt};
            }
            inline Visual make(const Geometry &geom, const Material &mat) noexcept {
                return Visual{String{}, pose::identity(), geom, mat};
            }
            inline Visual make(const Pose &origin, const Geometry &geom, const Material &mat) noexcept {
                return Visual{String{}, origin, geom, mat};
            }
            inline Visual make(const String &name, const Pose &origin, const Geometry &geom,
                               const Optional<Material> &mat = nullopt) noexcept {
                return Visual{name, origin, geom, mat};
            }
        } // namespace visual

    } // namespace robot
} // namespace datapod
