use std::ops::{Add, Div, Mul, Sub};

use crate::mat;

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Point {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

impl Point {
    pub fn new(x: f64, y: f64, z: f64) -> Self {
        Self { x, y, z }
    }

    pub fn magnitude(&self) -> f64 {
        (self.x * self.x + self.y * self.y + self.z * self.z).sqrt()
    }

    pub fn distance_to(&self, other: Point) -> f64 {
        (*self - other).magnitude()
    }

    pub fn distance_to_2d(&self, other: Point) -> f64 {
        let dx = self.x - other.x;
        let dy = self.y - other.y;
        (dx * dx + dy * dy).sqrt()
    }

    pub fn is_set(&self) -> bool {
        self.x != 0.0 || self.y != 0.0 || self.z != 0.0
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 3> {
        mat::Vector::from([self.x, self.y, self.z])
    }

    pub fn from_mat(value: mat::Vector<f64, 3>) -> Self {
        Self::new(value[0], value[1], value[2])
    }
}

impl Add for Point {
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        Self::new(self.x + rhs.x, self.y + rhs.y, self.z + rhs.z)
    }
}

impl Sub for Point {
    type Output = Self;

    fn sub(self, rhs: Self) -> Self::Output {
        Self::new(self.x - rhs.x, self.y - rhs.y, self.z - rhs.z)
    }
}

impl Mul<f64> for Point {
    type Output = Self;

    fn mul(self, rhs: f64) -> Self::Output {
        Self::new(self.x * rhs, self.y * rhs, self.z * rhs)
    }
}

impl Div<f64> for Point {
    type Output = Self;

    fn div(self, rhs: f64) -> Self::Output {
        Self::new(self.x / rhs, self.y / rhs, self.z / rhs)
    }
}

#[allow(dead_code)]
pub mod point {
    use super::Point;

    pub fn make(x: f64, y: f64) -> Point {
        Point::new(x, y, 0.0)
    }

    pub fn make3(x: f64, y: f64, z: f64) -> Point {
        Point::new(x, y, z)
    }

    pub fn origin() -> Point {
        Point::default()
    }
}
