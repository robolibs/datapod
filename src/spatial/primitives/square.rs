use crate::matrix::mat;
use crate::spatial::Point;

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Square {
    pub center: Point,
    pub side: f64,
}

impl Square {
    pub fn area(&self) -> f64 {
        self.side * self.side
    }

    pub fn perimeter(&self) -> f64 {
        4.0 * self.side
    }

    pub fn diagonal(&self) -> f64 {
        self.side * 2.0_f64.sqrt()
    }

    pub fn contains(&self, point: Point) -> bool {
        let half = self.side / 2.0;
        (point.x - self.center.x).abs() <= half && (point.y - self.center.y).abs() <= half
    }

    pub fn get_corners(&self) -> [Point; 4] {
        let half = self.side / 2.0;
        [
            Point::new(self.center.x - half, self.center.y - half, self.center.z),
            Point::new(self.center.x + half, self.center.y - half, self.center.z),
            Point::new(self.center.x + half, self.center.y + half, self.center.z),
            Point::new(self.center.x - half, self.center.y + half, self.center.z),
        ]
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 4> {
        mat::Vector::from([self.center.x, self.center.y, self.center.z, self.side])
    }

    pub fn from_mat(v: mat::Vector<f64, 4>) -> Self {
        Self {
            center: Point::new(v[0], v[1], v[2]),
            side: v[3],
        }
    }
}
