use crate::matrix::mat;
use crate::spatial::Point;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Wrench {
    pub force: Point,
    pub torque: Point,
}

impl Wrench {
    pub fn new(force: Point, torque: Point) -> Self {
        Self { force, torque }
    }

    pub fn from_components(fx: f64, fy: f64, fz: f64, tx: f64, ty: f64, tz: f64) -> Self {
        Self {
            force: Point::new(fx, fy, fz),
            torque: Point::new(tx, ty, tz),
        }
    }

    pub fn force_only(force: Point) -> Self {
        Self {
            force,
            torque: Point::new(0.0, 0.0, 0.0),
        }
    }

    pub fn torque_only(torque: Point) -> Self {
        Self {
            force: Point::new(0.0, 0.0, 0.0),
            torque,
        }
    }

    pub fn zero() -> Self {
        Self::default()
    }

    pub fn is_set(&self) -> bool {
        self.force.is_set() || self.torque.is_set()
    }

    pub fn force_magnitude(&self) -> f64 {
        self.force.magnitude()
    }

    pub fn torque_magnitude(&self) -> f64 {
        self.torque.magnitude()
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 6> {
        mat::Vector::from([
            self.force.x,
            self.force.y,
            self.force.z,
            self.torque.x,
            self.torque.y,
            self.torque.z,
        ])
    }

    pub fn from_mat(v: mat::Vector<f64, 6>) -> Self {
        Self {
            force: Point::new(v[0], v[1], v[2]),
            torque: Point::new(v[3], v[4], v[5]),
        }
    }
}

impl std::ops::Add for Wrench {
    type Output = Self;
    fn add(self, rhs: Self) -> Self::Output {
        Self {
            force: self.force + rhs.force,
            torque: self.torque + rhs.torque,
        }
    }
}

impl std::ops::Sub for Wrench {
    type Output = Self;
    fn sub(self, rhs: Self) -> Self::Output {
        Self {
            force: self.force - rhs.force,
            torque: self.torque - rhs.torque,
        }
    }
}

impl std::ops::Mul<f64> for Wrench {
    type Output = Self;
    fn mul(self, rhs: f64) -> Self::Output {
        Self {
            force: self.force * rhs,
            torque: self.torque * rhs,
        }
    }
}

impl std::ops::Div<f64> for Wrench {
    type Output = Self;
    fn div(self, rhs: f64) -> Self::Output {
        Self {
            force: self.force / rhs,
            torque: self.torque / rhs,
        }
    }
}
