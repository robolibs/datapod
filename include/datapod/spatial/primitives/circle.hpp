#pragma once

#include <cmath>
#include <tuple>

#include "../point.hpp"

namespace datapod {

    struct Circle {
        Point center;
        double radius = 0.0;

        auto members() noexcept { return std::tie(center, radius); }
        auto members() const noexcept { return std::tie(center, radius); }

        // Geometric properties
        inline double area() const noexcept { return M_PI * radius * radius; }

        inline double perimeter() const noexcept { return 2.0 * M_PI * radius; }

        // Containment
        inline bool contains(const Point &p) const noexcept { return center.distance_to(p) <= radius; }
    };

} // namespace datapod
