use crate::matrix::mat;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Velocity {
    pub vx: f64,
    pub vy: f64,
    pub vz: f64,
}

impl Velocity {
    pub fn speed(&self) -> f64 {
        (self.vx * self.vx + self.vy * self.vy + self.vz * self.vz).sqrt()
    }

    pub fn speed_2d(&self) -> f64 {
        (self.vx * self.vx + self.vy * self.vy).sqrt()
    }

    pub fn speed_squared(&self) -> f64 {
        self.vx * self.vx + self.vy * self.vy + self.vz * self.vz
    }

    pub fn is_set(&self) -> bool {
        self.vx != 0.0 || self.vy != 0.0 || self.vz != 0.0
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 3> {
        mat::Vector::from([self.vx, self.vy, self.vz])
    }

    pub fn from_mat(v: mat::Vector<f64, 3>) -> Self {
        Self {
            vx: v[0],
            vy: v[1],
            vz: v[2],
        }
    }
}

impl std::ops::Add for Velocity {
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        Self {
            vx: self.vx + rhs.vx,
            vy: self.vy + rhs.vy,
            vz: self.vz + rhs.vz,
        }
    }
}

impl std::ops::Sub for Velocity {
    type Output = Self;

    fn sub(self, rhs: Self) -> Self::Output {
        Self {
            vx: self.vx - rhs.vx,
            vy: self.vy - rhs.vy,
            vz: self.vz - rhs.vz,
        }
    }
}

impl std::ops::Mul<f64> for Velocity {
    type Output = Self;

    fn mul(self, rhs: f64) -> Self::Output {
        Self {
            vx: self.vx * rhs,
            vy: self.vy * rhs,
            vz: self.vz * rhs,
        }
    }
}

impl std::ops::Div<f64> for Velocity {
    type Output = Self;

    fn div(self, rhs: f64) -> Self::Output {
        Self {
            vx: self.vx / rhs,
            vy: self.vy / rhs,
            vz: self.vz / rhs,
        }
    }
}
