use super::{Geo, Point};

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Loc {
    pub local: Point,
    pub origin: Geo,
}

impl Loc {
    pub fn is_set(&self) -> bool {
        self.local.is_set() || self.origin.is_set()
    }

    pub fn has_valid_origin(&self) -> bool {
        self.origin.is_valid()
    }

    pub fn distance_from_origin(&self) -> f64 {
        self.local.magnitude()
    }

    pub fn distance_from_origin_2d(&self) -> f64 {
        (self.local.x * self.local.x + self.local.y * self.local.y).sqrt()
    }

    pub fn distance_to(&self, other: Loc) -> f64 {
        self.local.distance_to(other.local)
    }

    pub fn distance_to_2d(&self, other: Loc) -> f64 {
        self.local.distance_to_2d(other.local)
    }

    pub fn same_origin(&self, other: Loc, tolerance: f64) -> bool {
        (self.origin.latitude - other.origin.latitude).abs() < tolerance
            && (self.origin.longitude - other.origin.longitude).abs() < tolerance
            && (self.origin.altitude - other.origin.altitude).abs() < tolerance
    }
}

impl std::ops::Add<Point> for Loc {
    type Output = Self;

    fn add(self, rhs: Point) -> Self::Output {
        Self {
            local: self.local + rhs,
            origin: self.origin,
        }
    }
}

impl std::ops::Sub<Point> for Loc {
    type Output = Self;

    fn sub(self, rhs: Point) -> Self::Output {
        Self {
            local: self.local - rhs,
            origin: self.origin,
        }
    }
}
