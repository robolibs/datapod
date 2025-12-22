#pragma once

#include <cmath>
#include <tuple>

#include "../../sequential/array.hpp"
#include "../point.hpp"

namespace datapod {

    struct Rectangle {
        Point top_left;
        Point top_right;
        Point bottom_left;
        Point bottom_right;

        auto members() noexcept { return std::tie(top_left, top_right, bottom_left, bottom_right); }
        auto members() const noexcept { return std::tie(top_left, top_right, bottom_left, bottom_right); }

        // Calculate the area of the rectangle
        inline double area() const noexcept {
            // Width is distance from bottom_left to bottom_right
            double width = bottom_left.distance_to(bottom_right);
            // Height is distance from bottom_left to top_left
            double height = bottom_left.distance_to(top_left);
            return width * height;
        }

        // Calculate the perimeter of the rectangle
        inline double perimeter() const noexcept {
            double width = bottom_left.distance_to(bottom_right);
            double height = bottom_left.distance_to(top_left);
            return 2.0 * (width + height);
        }

        // Check if a point is inside the rectangle (2D check, ignores Z)
        inline bool contains(const Point &p) const noexcept {
            // Get the min and max x and y coordinates
            double min_x = bottom_left.x;
            double max_x = bottom_right.x;
            double min_y = bottom_left.y;
            double max_y = top_left.y;

            // Ensure min/max are in correct order (handle rotated rectangles)
            if (min_x > max_x)
                std::swap(min_x, max_x);
            if (min_y > max_y)
                std::swap(min_y, max_y);

            // Check if point is within bounds
            return p.x >= min_x && p.x <= max_x && p.y >= min_y && p.y <= max_y;
        }

        // Get all four corners as an array
        inline Array<Point, 4> get_corners() const noexcept {
            Array<Point, 4> corners;
            corners[0] = bottom_left;
            corners[1] = bottom_right;
            corners[2] = top_right;
            corners[3] = top_left;
            return corners;
        }
    };

} // namespace datapod
