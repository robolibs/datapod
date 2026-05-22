use crate::geom::Point;

#[datapod::datapod]
#[derive(Default)]
pub struct Line {
    pub origin: Point,
    pub direction: Point,
}

impl Line {
    pub fn new(origin: Point, direction: Point) -> Self {
        Self { origin, direction }
    }

    pub fn from_points(p1: Point, p2: Point) -> Self {
        Self {
            origin: p1,
            direction: p2 - p1,
        }
    }

    pub fn closest_point(&self, p: Point) -> Point {
        let to_p = p - self.origin;
        let dir_mag_sq = self.direction.x * self.direction.x
            + self.direction.y * self.direction.y
            + self.direction.z * self.direction.z;

        if dir_mag_sq < 1e-10 {
            return self.origin;
        }

        let t = (to_p.x * self.direction.x + to_p.y * self.direction.y + to_p.z * self.direction.z)
            / dir_mag_sq;

        Point::new(
            self.origin.x + t * self.direction.x,
            self.origin.y + t * self.direction.y,
            self.origin.z + t * self.direction.z,
        )
    }

    pub fn distance_to(&self, p: Point) -> f64 {
        p.distance_to(self.closest_point(p))
    }

    pub fn to_mat(&self) -> [f64; 6] {
        [
            self.origin.x,
            self.origin.y,
            self.origin.z,
            self.direction.x,
            self.direction.y,
            self.direction.z,
        ]    }

    pub fn from_mat(v: [f64; 6]) -> Self {
        Self {
            origin: Point::new(v[0], v[1], v[2]),
            direction: Point::new(v[3], v[4], v[5]),
        }
    }
}
