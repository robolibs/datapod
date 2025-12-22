#pragma once

#include <tuple>

#include "pose.hpp"
#include "size.hpp"

namespace datapod {

    struct Box {
        Pose pose;
        Size size;

        auto members() noexcept { return std::tie(pose, size); }
        auto members() const noexcept { return std::tie(pose, size); }
    };

} // namespace datapod
