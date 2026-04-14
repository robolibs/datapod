use crate::matrix::mat;
use crate::spatial::Point;

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Rectangle {
    pub top_left: Point,
    pub top_right: Point,
    pub bottom_left: Point,
    pub bottom_right: Point,
}

impl Rectangle {
    pub fn area(&self) -> f64 {
        self.bottom_left.distance_to(self.bottom_right)
            * self.bottom_left.distance_to(self.top_left)
    }

    pub fn perimeter(&self) -> f64 {
        let width = self.bottom_left.distance_to(self.bottom_right);
        let height = self.bottom_left.distance_to(self.top_left);
        2.0 * (width + height)
    }

    pub fn contains(&self, point: Point) -> bool {
        let mut min_x = self.bottom_left.x;
        let mut max_x = self.bottom_right.x;
        let mut min_y = self.bottom_left.y;
        let mut max_y = self.top_left.y;
        if min_x > max_x {
            std::mem::swap(&mut min_x, &mut max_x);
        }
        if min_y > max_y {
            std::mem::swap(&mut min_y, &mut max_y);
        }
        point.x >= min_x && point.x <= max_x && point.y >= min_y && point.y <= max_y
    }

    pub fn get_corners(&self) -> [Point; 4] {
        [
            self.bottom_left,
            self.bottom_right,
            self.top_right,
            self.top_left,
        ]
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 12> {
        mat::Vector::from([
            self.top_left.x,
            self.top_left.y,
            self.top_left.z,
            self.top_right.x,
            self.top_right.y,
            self.top_right.z,
            self.bottom_left.x,
            self.bottom_left.y,
            self.bottom_left.z,
            self.bottom_right.x,
            self.bottom_right.y,
            self.bottom_right.z,
        ])
    }

    pub fn from_mat(v: mat::Vector<f64, 12>) -> Self {
        Self {
            top_left: Point::new(v[0], v[1], v[2]),
            top_right: Point::new(v[3], v[4], v[5]),
            bottom_left: Point::new(v[6], v[7], v[8]),
            bottom_right: Point::new(v[9], v[10], v[11]),
        }
    }
}
