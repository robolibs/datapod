use crate::geom::Point;

#[datapod::datapod]
#[derive(Default)]
pub struct Aabb {
    pub min_point: Point,
    pub max_point: Point,
}

impl Aabb {
    pub fn new(min_point: Point, max_point: Point) -> Self {
        Self {
            min_point,
            max_point,
        }
    }

    pub fn center(&self) -> Point {
        Point::new(
            (self.min_point.x + self.max_point.x) / 2.0,
            (self.min_point.y + self.max_point.y) / 2.0,
            (self.min_point.z + self.max_point.z) / 2.0,
        )
    }

    pub fn volume(&self) -> f64 {
        let dx = self.max_point.x - self.min_point.x;
        let dy = self.max_point.y - self.min_point.y;
        let dz = self.max_point.z - self.min_point.z;
        dx * dy * dz
    }

    pub fn surface_area(&self) -> f64 {
        let dx = self.max_point.x - self.min_point.x;
        let dy = self.max_point.y - self.min_point.y;
        let dz = self.max_point.z - self.min_point.z;
        2.0 * (dx * dy + dy * dz + dz * dx)
    }

    pub fn contains(&self, point: Point) -> bool {
        point.x >= self.min_point.x
            && point.x <= self.max_point.x
            && point.y >= self.min_point.y
            && point.y <= self.max_point.y
            && point.z >= self.min_point.z
            && point.z <= self.max_point.z
    }

    pub fn intersects(&self, other: Aabb) -> bool {
        !(self.max_point.x < other.min_point.x
            || self.min_point.x > other.max_point.x
            || self.max_point.y < other.min_point.y
            || self.min_point.y > other.max_point.y
            || self.max_point.z < other.min_point.z
            || self.min_point.z > other.max_point.z)
    }

    pub fn expand_point(&mut self, point: Point) {
        self.min_point.x = self.min_point.x.min(point.x);
        self.min_point.y = self.min_point.y.min(point.y);
        self.min_point.z = self.min_point.z.min(point.z);
        self.max_point.x = self.max_point.x.max(point.x);
        self.max_point.y = self.max_point.y.max(point.y);
        self.max_point.z = self.max_point.z.max(point.z);
    }

    pub fn expand_box(&mut self, other: Aabb) {
        self.expand_point(other.min_point);
        self.expand_point(other.max_point);
    }

    pub fn distance_to_point(&self, point: Point) -> f64 {
        let dx = (self.min_point.x - point.x)
            .max(0.0)
            .max(point.x - self.max_point.x);
        let dy = (self.min_point.y - point.y)
            .max(0.0)
            .max(point.y - self.max_point.y);
        let dz = (self.min_point.z - point.z)
            .max(0.0)
            .max(point.z - self.max_point.z);
        (dx * dx + dy * dy + dz * dz).sqrt()
    }

    pub fn to_mat(&self) -> [f64; 6] {
        [
            self.min_point.x,
            self.min_point.y,
            self.min_point.z,
            self.max_point.x,
            self.max_point.y,
            self.max_point.z,
        ]
    }

    pub fn from_mat(v: [f64; 6]) -> Self {
        Self::new(Point::new(v[0], v[1], v[2]), Point::new(v[3], v[4], v[5]))
    }
}
