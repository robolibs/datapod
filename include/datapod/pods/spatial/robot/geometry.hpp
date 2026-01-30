#pragma once

#include <tuple>
#include <variant>

#include "datapod/pods/sequential/array.hpp"
#include "datapod/pods/sequential/string.hpp"
#include "datapod/pods/spatial/size.hpp"

namespace datapod {
    namespace robot {

        /**
         * @brief Geometry shapes for robot visual/collision representation (POD)
         *
         * Represents primitive shapes and meshes for URDF/SDF-style robot definitions.
         * Uses std::variant to hold one of: BoxShape, SphereShape, CylinderShape, MeshShape.
         */

        /// Box shape defined by dimensions (width, height, depth)
        struct BoxShape {
            Size size;

            auto members() noexcept { return std::tie(size); }
            auto members() const noexcept { return std::tie(size); }

            inline bool operator==(const BoxShape &other) const noexcept { return size == other.size; }
            inline bool operator!=(const BoxShape &other) const noexcept { return !(*this == other); }
        };

        /// Sphere shape defined by radius
        struct SphereShape {
            f64 radius = 0.0;

            auto members() noexcept { return std::tie(radius); }
            auto members() const noexcept { return std::tie(radius); }

            inline bool operator==(const SphereShape &other) const noexcept { return radius == other.radius; }
            inline bool operator!=(const SphereShape &other) const noexcept { return !(*this == other); }
        };

        /// Cylinder shape defined by radius and length (axis along Z)
        struct CylinderShape {
            f64 radius = 0.0;
            f64 length = 0.0;

            auto members() noexcept { return std::tie(radius, length); }
            auto members() const noexcept { return std::tie(radius, length); }

            inline bool operator==(const CylinderShape &other) const noexcept {
                return radius == other.radius && length == other.length;
            }
            inline bool operator!=(const CylinderShape &other) const noexcept { return !(*this == other); }
        };

        /// Mesh shape defined by URI and optional scale
        struct MeshShape {
            String uri;
            Array<f64, 3> scale{1.0, 1.0, 1.0};

            auto members() noexcept { return std::tie(uri, scale); }
            auto members() const noexcept { return std::tie(uri, scale); }

            inline bool operator==(const MeshShape &other) const noexcept {
                return uri == other.uri && scale == other.scale;
            }
            inline bool operator!=(const MeshShape &other) const noexcept { return !(*this == other); }
        };

        /// Geometry variant holding one of the shape types
        struct Geometry {
            std::variant<BoxShape, SphereShape, CylinderShape, MeshShape> shape;

            auto members() noexcept { return std::tie(shape); }
            auto members() const noexcept { return std::tie(shape); }

            inline bool is_box() const noexcept { return std::holds_alternative<BoxShape>(shape); }
            inline bool is_sphere() const noexcept { return std::holds_alternative<SphereShape>(shape); }
            inline bool is_cylinder() const noexcept { return std::holds_alternative<CylinderShape>(shape); }
            inline bool is_mesh() const noexcept { return std::holds_alternative<MeshShape>(shape); }

            inline const BoxShape *as_box() const noexcept { return std::get_if<BoxShape>(&shape); }
            inline const SphereShape *as_sphere() const noexcept { return std::get_if<SphereShape>(&shape); }
            inline const CylinderShape *as_cylinder() const noexcept { return std::get_if<CylinderShape>(&shape); }
            inline const MeshShape *as_mesh() const noexcept { return std::get_if<MeshShape>(&shape); }

            inline BoxShape *as_box() noexcept { return std::get_if<BoxShape>(&shape); }
            inline SphereShape *as_sphere() noexcept { return std::get_if<SphereShape>(&shape); }
            inline CylinderShape *as_cylinder() noexcept { return std::get_if<CylinderShape>(&shape); }
            inline MeshShape *as_mesh() noexcept { return std::get_if<MeshShape>(&shape); }

            inline bool operator==(const Geometry &other) const noexcept { return shape == other.shape; }
            inline bool operator!=(const Geometry &other) const noexcept { return !(*this == other); }
        };

        namespace geometry {
            inline Geometry box(const Size &s) noexcept { return Geometry{BoxShape{s}}; }
            inline Geometry box(f64 x, f64 y, f64 z) noexcept { return Geometry{BoxShape{Size{x, y, z}}}; }
            inline Geometry sphere(f64 radius) noexcept { return Geometry{SphereShape{radius}}; }
            inline Geometry cylinder(f64 radius, f64 length) noexcept {
                return Geometry{CylinderShape{radius, length}};
            }
            inline Geometry mesh(const String &uri,
                                 const Array<f64, 3> &scale = Array<f64, 3>{1.0, 1.0, 1.0}) noexcept {
                return Geometry{MeshShape{uri, scale}};
            }
        } // namespace geometry

    } // namespace robot
} // namespace datapod
