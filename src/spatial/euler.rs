use std::f64::consts::PI;
use std::ops::{Add, Mul, Sub};

use crate::mat;

use super::Quaternion;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Euler {
    pub roll: f64,
    pub pitch: f64,
    pub yaw: f64,
}

impl Euler {
    pub fn new(roll: f64, pitch: f64, yaw: f64) -> Self {
        Self { roll, pitch, yaw }
    }

    pub fn is_set(&self) -> bool {
        self.roll != 0.0 || self.pitch != 0.0 || self.yaw != 0.0
    }

    pub fn yaw_cos(&self) -> f64 {
        self.yaw.cos()
    }

    pub fn yaw_sin(&self) -> f64 {
        self.yaw.sin()
    }

    pub fn normalized(&self) -> Self {
        fn normalize_angle(angle: f64) -> f64 {
            let two_pi = 2.0 * PI;
            let mut result = (angle + PI) % two_pi;
            if result < 0.0 {
                result += two_pi;
            }
            result - PI
        }

        Self::new(
            normalize_angle(self.roll),
            normalize_angle(self.pitch),
            normalize_angle(self.yaw),
        )
    }

    pub fn to_quaternion(&self) -> Quaternion {
        Quaternion::from_euler(*self)
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 3> {
        mat::Vector::from([self.roll, self.pitch, self.yaw])
    }

    pub fn from_mat(value: mat::Vector<f64, 3>) -> Self {
        Self::new(value[0], value[1], value[2])
    }
}

impl Add for Euler {
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        Self::new(
            self.roll + rhs.roll,
            self.pitch + rhs.pitch,
            self.yaw + rhs.yaw,
        )
    }
}

impl Sub for Euler {
    type Output = Self;

    fn sub(self, rhs: Self) -> Self::Output {
        Self::new(
            self.roll - rhs.roll,
            self.pitch - rhs.pitch,
            self.yaw - rhs.yaw,
        )
    }
}

impl Mul<f64> for Euler {
    type Output = Self;

    fn mul(self, rhs: f64) -> Self::Output {
        Self::new(self.roll * rhs, self.pitch * rhs, self.yaw * rhs)
    }
}

#[allow(dead_code)]
pub mod euler {
    use super::Euler;

    pub fn make(roll: f64, pitch: f64, yaw: f64) -> Euler {
        Euler::new(roll, pitch, yaw)
    }
}
