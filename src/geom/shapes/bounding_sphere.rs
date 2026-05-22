
use super::Aabb;
use crate::geom::Point;

/// Short alias — kept for back-compat with consumers that imported `Bs`.
pub type Bs = BoundingSphere;

#[datapod::datapod]
#[derive(Default)]
pub struct BoundingSphere {
    pub center: Point,
    pub radius: f64,
}

impl BoundingSphere {
    pub fn unit() -> Self {
        Self {
            center: Point::default(),
            radius: 1.0,
        }
    }

    pub fn volume(&self) -> f64 {
        (4.0 / 3.0) * std::f64::consts::PI * self.radius * self.radius * self.radius
    }

    pub fn surface_area(&self) -> f64 {
        4.0 * std::f64::consts::PI * self.radius * self.radius
    }

    pub fn contains(&self, point: Point) -> bool {
        self.center.distance_to(point) <= self.radius
    }

    pub fn intersects(&self, other: BoundingSphere) -> bool {
        let dist = self.center.distance_to(other.center);
        dist <= self.radius + other.radius
    }

    pub fn get_aabb(&self) -> Aabb {
        Aabb::new(
            Point::new(
                self.center.x - self.radius,
                self.center.y - self.radius,
                self.center.z - self.radius,
            ),
            Point::new(
                self.center.x + self.radius,
                self.center.y + self.radius,
                self.center.z + self.radius,
            ),
        )
    }

    pub fn expand(&mut self, point: Point) {
        let dist = self.center.distance_to(point);
        if dist > self.radius {
            self.radius = dist;
        }
    }

    pub fn expand_sphere(&mut self, other: BoundingSphere) {
        let dist = self.center.distance_to(other.center);
        let new_radius = dist + other.radius;
        if new_radius > self.radius {
            self.radius = new_radius;
        }
    }

    pub fn diameter(&self) -> f64 {
        2.0 * self.radius
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
