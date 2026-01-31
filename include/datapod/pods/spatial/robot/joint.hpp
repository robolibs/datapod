#pragma once

#include <tuple>

#include "datapod/pods/adapters/optional.hpp"
#include "datapod/pods/associative/map.hpp"
#include "datapod/pods/sequential/array.hpp"
#include "datapod/pods/sequential/string.hpp"
#include "datapod/pods/spatial/pose.hpp"
#include "datapod/types/types.hpp"

namespace datapod {
    namespace robot {

        /// Sentinel value for invalid link/joint IDs
        static constexpr u32 kInvalidId = ~u32{0};

        /**
         * @brief Joint - Robot joint description (POD)
         *
         * Represents a joint connecting two links in a robot model.
         * Used in URDF/SDF-style robot definitions.
         */
        struct Joint {
            enum class Type : u8 { Fixed, Revolute, Continuous, Prismatic, Floating, Planar };

            /// Joint limits for bounded joint types
            struct Limits {
                f64 lower = 0.0;
                f64 upper = 0.0;
                f64 effort = 0.0;
                f64 velocity = 0.0;

                auto members() noexcept { return std::tie(lower, upper, effort, velocity); }
                auto members() const noexcept { return std::tie(lower, upper, effort, velocity); }

                inline bool operator==(const Limits &other) const noexcept {
                    return lower == other.lower && upper == other.upper && effort == other.effort &&
                           velocity == other.velocity;
                }
                inline bool operator!=(const Limits &other) const noexcept { return !(*this == other); }
            };

            /// Joint dynamics parameters (URDF <dynamics>)
            ///
            /// - damping: viscous friction coefficient (torque ~ -damping * qdot)
            /// - friction: Coulomb friction coefficient (torque ~ -friction * sign(qdot))
            struct Dynamics {
                f64 damping = 0.0;
                f64 friction = 0.0;

                auto members() noexcept { return std::tie(damping, friction); }
                auto members() const noexcept { return std::tie(damping, friction); }

                inline bool operator==(const Dynamics &other) const noexcept {
                    return damping == other.damping && friction == other.friction;
                }
                inline bool operator!=(const Dynamics &other) const noexcept { return !(*this == other); }
            };

            /// Joint mimic parameters (URDF <mimic>)
            ///
            /// Makes this joint mimic another joint: value = multiplier * other_joint + offset
            struct Mimic {
                String joint; // Name of joint to mimic
                f64 multiplier = 1.0;
                f64 offset = 0.0;

                auto members() noexcept { return std::tie(joint, multiplier, offset); }
                auto members() const noexcept { return std::tie(joint, multiplier, offset); }

                inline bool operator==(const Mimic &other) const noexcept {
                    return joint == other.joint && multiplier == other.multiplier && offset == other.offset;
                }
                inline bool operator!=(const Mimic &other) const noexcept { return !(*this == other); }
            };

            /// Joint safety controller parameters (URDF <safety_controller>)
            struct SafetyController {
                f64 soft_lower_limit = 0.0;
                f64 soft_upper_limit = 0.0;
                f64 k_position = 0.0;
                f64 k_velocity = 0.0;

                auto members() noexcept { return std::tie(soft_lower_limit, soft_upper_limit, k_position, k_velocity); }
                auto members() const noexcept {
                    return std::tie(soft_lower_limit, soft_upper_limit, k_position, k_velocity);
                }

                inline bool operator==(const SafetyController &other) const noexcept {
                    return soft_lower_limit == other.soft_lower_limit && soft_upper_limit == other.soft_upper_limit &&
                           k_position == other.k_position && k_velocity == other.k_velocity;
                }
                inline bool operator!=(const SafetyController &other) const noexcept { return !(*this == other); }
            };

            /// Joint calibration parameters (URDF <calibration>)
            struct Calibration {
                Optional<f64> rising;
                Optional<f64> falling;

                auto members() noexcept { return std::tie(rising, falling); }
                auto members() const noexcept { return std::tie(rising, falling); }

                inline bool operator==(const Calibration &other) const noexcept {
                    return rising == other.rising && falling == other.falling;
                }
                inline bool operator!=(const Calibration &other) const noexcept { return !(*this == other); }
            };

            String name;
            Type type = Type::Fixed;
            Pose origin;
            Array<f64, 3> axis{1.0, 0.0, 0.0};
            Optional<Limits> limits;
            Optional<Dynamics> dynamics;
            Optional<Mimic> mimic;
            Optional<SafetyController> safety_controller;
            Optional<Calibration> calibration;
            u32 parent = kInvalidId;
            u32 child = kInvalidId;
            /// Non-core URDF extensions flattened into key/value properties.
            Map<String, String> props;

            auto members() noexcept {
                return std::tie(name, type, origin, axis, limits, dynamics, mimic, safety_controller, calibration,
                                parent, child, props);
            }
            auto members() const noexcept {
                return std::tie(name, type, origin, axis, limits, dynamics, mimic, safety_controller, calibration,
                                parent, child, props);
            }

            inline bool is_fixed() const noexcept { return type == Type::Fixed; }
            inline bool is_revolute() const noexcept { return type == Type::Revolute; }
            inline bool is_continuous() const noexcept { return type == Type::Continuous; }
            inline bool is_prismatic() const noexcept { return type == Type::Prismatic; }
            inline bool is_floating() const noexcept { return type == Type::Floating; }
            inline bool is_planar() const noexcept { return type == Type::Planar; }

            inline bool operator==(const Joint &other) const noexcept {
                return name == other.name && type == other.type && origin == other.origin && axis == other.axis &&
                       limits == other.limits && dynamics == other.dynamics && mimic == other.mimic &&
                       safety_controller == other.safety_controller && calibration == other.calibration &&
                       parent == other.parent && child == other.child && props == other.props;
            }
            inline bool operator!=(const Joint &other) const noexcept { return !(*this == other); }
        };

        namespace joint {
            inline Joint fixed(const String &name, const Pose &origin = pose::identity()) noexcept {
                return Joint{name,       Joint::Type::Fixed,
                             origin,     Array<f64, 3>{1.0, 0.0, 0.0},
                             nullopt,    nullopt,
                             nullopt,    nullopt,
                             nullopt,    kInvalidId,
                             kInvalidId, {}};
            }

            inline Joint revolute(const String &name, const Array<f64, 3> &axis, const Joint::Limits &limits,
                                  const Pose &origin = pose::identity()) noexcept {
                return Joint{name,       Joint::Type::Revolute,
                             origin,     axis,
                             limits,     nullopt,
                             nullopt,    nullopt,
                             nullopt,    kInvalidId,
                             kInvalidId, {}};
            }

            inline Joint continuous(const String &name, const Array<f64, 3> &axis,
                                    const Pose &origin = pose::identity()) noexcept {
                return Joint{name,       Joint::Type::Continuous,
                             origin,     axis,
                             nullopt,    nullopt,
                             nullopt,    nullopt,
                             nullopt,    kInvalidId,
                             kInvalidId, {}};
            }

            inline Joint prismatic(const String &name, const Array<f64, 3> &axis, const Joint::Limits &limits,
                                   const Pose &origin = pose::identity()) noexcept {
                return Joint{name,       Joint::Type::Prismatic,
                             origin,     axis,
                             limits,     nullopt,
                             nullopt,    nullopt,
                             nullopt,    kInvalidId,
                             kInvalidId, {}};
            }
        } // namespace joint

    } // namespace robot
} // namespace datapod
