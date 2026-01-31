#pragma once

#include <tuple>

#include "datapod/pods/associative/map.hpp"
#include "datapod/pods/sequential/string.hpp"
#include "datapod/pods/spatial/pose.hpp"

namespace datapod {
    namespace robot {

        /**
         * @brief Sensor - Generic simulated sensor description (POD)
         *
         * A minimal representation of sensors commonly described in URDF/Gazebo:
         * - name/type identify the sensor
         * - origin is the pose relative to the parent link
         * - props stores all additional (backend-specific) parameters as flattened key/value strings
         */
        struct Sensor {
            String name;
            String type;
            Pose origin;
            Map<String, String> props;

            auto members() noexcept { return std::tie(name, type, origin, props); }
            auto members() const noexcept { return std::tie(name, type, origin, props); }

            inline bool operator==(const Sensor &other) const noexcept {
                return name == other.name && type == other.type && origin == other.origin && props == other.props;
            }
            inline bool operator!=(const Sensor &other) const noexcept { return !(*this == other); }
        };

        namespace sensor {
            inline Sensor make(const String &name, const String &type, const Pose &origin = pose::identity()) noexcept {
                return Sensor{name, type, origin, {}};
            }
        } // namespace sensor

    } // namespace robot
} // namespace datapod
