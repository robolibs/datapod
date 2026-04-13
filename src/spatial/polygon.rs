use crate::Vector;

use super::{Aabb, Euler, Obb, Point, Size};

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Polygon {
    pub vertices: Vector<Point>,
}

impl Polygon {
    pub fn perimeter(&self) -> f64 {
        if self.vertices.len() < 2 {
            return 0.0;
        }

        let mut perimeter = 0.0;
        for index in 1..self.vertices.len() {
            perimeter += self.vertices[index - 1].distance_to(self.vertices[index]);
        }
        perimeter + self.vertices.back().distance_to(*self.vertices.front())
    }

    pub fn area(&self) -> f64 {
        if self.vertices.len() < 3 {
            return 0.0;
        }

        let mut area = 0.0;
        let mut j = self.vertices.len() - 1;
        for i in 0..self.vertices.len() {
            let pi = self.vertices[i];
            let pj = self.vertices[j];
            area += (pj.x + pi.x) * (pj.y - pi.y);
            j = i;
        }
        (area * 0.5).abs()
    }

    pub fn contains(&self, point: Point) -> bool {
        if self.vertices.len() < 3 {
            return false;
        }

        let mut contains = false;
        let mut j = self.vertices.len() - 1;
        for i in 0..self.vertices.len() {
            let pi = self.vertices[i];
            let pj = self.vertices[j];
            if ((pi.y > point.y) != (pj.y > point.y))
                && (point.x < (pj.x - pi.x) * (point.y - pi.y) / (pj.y - pi.y) + pi.x)
            {
                contains = !contains;
            }
            j = i;
        }
        contains
    }

    pub fn num_vertices(&self) -> usize {
        self.vertices.len()
    }

    pub fn is_valid(&self) -> bool {
        self.vertices.len() >= 3
    }

    pub fn empty(&self) -> bool {
        self.vertices.is_empty()
    }

    pub fn get_aabb(&self) -> Aabb {
        if self.vertices.is_empty() {
            return Aabb::default();
        }

        let mut min_point = self.vertices[0];
        let mut max_point = self.vertices[0];

        for point in self.vertices.iter().skip(1) {
            min_point.x = min_point.x.min(point.x);
            min_point.y = min_point.y.min(point.y);
            min_point.z = min_point.z.min(point.z);
            max_point.x = max_point.x.max(point.x);
            max_point.y = max_point.y.max(point.y);
            max_point.z = max_point.z.max(point.z);
        }

        Aabb::new(min_point, max_point)
    }

    pub fn get_obb(&self) -> Obb {
        if self.vertices.is_empty() {
            return Obb::default();
        }

        let (sum_x, sum_y) = self
            .vertices
            .iter()
            .fold((0.0, 0.0), |(sx, sy), point| (sx + point.x, sy + point.y));
        let centroid_x = sum_x / self.vertices.len() as f64;
        let centroid_y = sum_y / self.vertices.len() as f64;

        let first = self.vertices[0];
        let orientation = (centroid_y - first.y).atan2(centroid_x - first.x);
        let cos_o = orientation.cos();
        let sin_o = orientation.sin();

        let mut min_rot_x = f64::INFINITY;
        let mut max_rot_x = f64::NEG_INFINITY;
        let mut min_rot_y = f64::INFINITY;
        let mut max_rot_y = f64::NEG_INFINITY;

        for point in &self.vertices {
            let rot_x = point.x * cos_o + point.y * sin_o;
            let rot_y = -point.x * sin_o + point.y * cos_o;
            min_rot_x = min_rot_x.min(rot_x);
            max_rot_x = max_rot_x.max(rot_x);
            min_rot_y = min_rot_y.min(rot_y);
            max_rot_y = max_rot_y.max(rot_y);
        }

        let width = max_rot_x - min_rot_x;
        let height = max_rot_y - min_rot_y;
        let center_rot_x = 0.5 * (min_rot_x + max_rot_x);
        let center_rot_y = 0.5 * (min_rot_y + max_rot_y);

        let center = Point::new(
            center_rot_x * cos_o - center_rot_y * sin_o,
            center_rot_x * sin_o + center_rot_y * cos_o,
            0.0,
        );

        Obb::new(
            center,
            Size::new(width * 0.5, height * 0.5, 0.0),
            Euler::new(0.0, 0.0, orientation),
        )
    }

    pub fn iter(&self) -> std::slice::Iter<'_, Point> {
        self.vertices.iter()
    }
}
