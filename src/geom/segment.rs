
use super::Point;

#[datapod::datapod]
#[derive(Default)]
pub struct Segment {
    pub start: Point,
    pub end: Point,
}

impl Segment {
    pub fn new(start: Point, end: Point) -> Self {
        Self { start, end }
    }

    pub fn length(&self) -> f64 {
        self.start.distance_to(self.end)
    }

    pub fn midpoint(&self) -> Point {
        Point::new(
            (self.start.x + self.end.x) * 0.5,
            (self.start.y + self.end.y) * 0.5,
            (self.start.z + self.end.z) * 0.5,
        )
    }

    pub fn closest_point(&self, point: Point) -> Point {
        let diff = self.end - self.start;
        let len_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        if len_sq < 1e-10 {
            return self.start;
        }

        let to_point = point - self.start;
        let t = (to_point.x * diff.x + to_point.y * diff.y + to_point.z * diff.z) / len_sq;
        if t <= 0.0 {
            self.start
        } else if t >= 1.0 {
            self.end
        } else {
            Point::new(
                self.start.x + t * diff.x,
                self.start.y + t * diff.y,
                self.start.z + t * diff.z,
            )
        }
    }

    pub fn distance_to(&self, point: Point) -> f64 {
        point.distance_to(self.closest_point(point))
    }

    pub fn to_mat(&self) -> [f64; 6] {
        [
            self.start.x,
            self.start.y,
            self.start.z,
            self.end.x,
            self.end.y,
            self.end.z,
        ]    }

    pub fn from_mat(value: [f64; 6]) -> Self {
        Self::new(
            Point::new(value[0], value[1], value[2]),
            Point::new(value[3], value[4], value[5]),
        )
    }
}

#[allow(dead_code)]
pub mod segment {
    use super::{Point, Segment};

    pub fn make(start: Point, end: Point) -> Segment {
        Segment::new(start, end)
    }
}
