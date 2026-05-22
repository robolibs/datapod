use crate::geom::Point;

#[datapod::datapod]
#[derive(Default)]
pub struct Circle {
    pub center: Point,
    pub radius: f64,
}

impl Circle {
    pub fn area(&self) -> f64 {
        std::f64::consts::PI * self.radius * self.radius
    }

    pub fn perimeter(&self) -> f64 {
        2.0 * std::f64::consts::PI * self.radius
    }

    pub fn contains(&self, point: Point) -> bool {
        self.center.distance_to(point) <= self.radius
    }

    pub fn to_mat(&self) -> [f64; 4] {
        [self.center.x, self.center.y, self.center.z, self.radius]
    }

    pub fn from_mat(v: [f64; 4]) -> Self {
        Self {
            center: Point::new(v[0], v[1], v[2]),
            radius: v[3],
        }
    }
}
