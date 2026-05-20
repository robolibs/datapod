use super::Euler;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Quaternion {
    pub w: f64,
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

impl Default for Quaternion {
    fn default() -> Self {
        Self::identity()
    }
}

impl Quaternion {
    pub const fn new(w: f64, x: f64, y: f64, z: f64) -> Self {
        Self { w, x, y, z }
    }

    pub const fn identity() -> Self {
        Self::new(1.0, 0.0, 0.0, 0.0)
    }

    pub fn is_set(&self) -> bool {
        *self != Self::identity()
    }

    pub fn norm_squared(&self) -> f64 {
        self.w * self.w + self.x * self.x + self.y * self.y + self.z * self.z
    }

    pub fn norm(&self) -> f64 {
        self.norm_squared().sqrt()
    }

    pub fn conjugate(&self) -> Self {
        Self::new(self.w, -self.x, -self.y, -self.z)
    }

    pub fn inverse(&self) -> Self {
        let n2 = self.norm_squared();
        Self::new(self.w / n2, -self.x / n2, -self.y / n2, -self.z / n2)
    }

    pub fn normalized(&self) -> Self {
        let n = self.norm();
        if n < 1e-10 {
            Self::identity()
        } else {
            Self::new(self.w / n, self.x / n, self.y / n, self.z / n)
        }
    }

    pub fn from_euler(euler: Euler) -> Self {
        let (sr, cr) = (euler.roll * 0.5).sin_cos();
        let (sp, cp) = (euler.pitch * 0.5).sin_cos();
        let (sy, cy) = (euler.yaw * 0.5).sin_cos();

        Self::new(
            cr * cp * cy + sr * sp * sy,
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
        )
    }

    pub fn to_euler(&self) -> Euler {
        let sinr_cosp = 2.0 * (self.w * self.x + self.y * self.z);
        let cosr_cosp = 1.0 - 2.0 * (self.x * self.x + self.y * self.y);
        let roll = sinr_cosp.atan2(cosr_cosp);

        let sinp = 2.0 * (self.w * self.y - self.z * self.x);
        let pitch = if sinp.abs() >= 1.0 {
            sinp.signum() * std::f64::consts::FRAC_PI_2
        } else {
            sinp.asin()
        };

        let siny_cosp = 2.0 * (self.w * self.z + self.x * self.y);
        let cosy_cosp = 1.0 - 2.0 * (self.y * self.y + self.z * self.z);
        let yaw = siny_cosp.atan2(cosy_cosp);

        Euler::new(roll, pitch, yaw)
    }

    pub fn rotate_vector(&self, x: &mut f64, y: &mut f64, z: &mut f64) {
        let tx = 2.0 * (self.y * *z - self.z * *y);
        let ty = 2.0 * (self.z * *x - self.x * *z);
        let tz = 2.0 * (self.x * *y - self.y * *x);

        *x = *x + self.w * tx + (self.y * tz - self.z * ty);
        *y = *y + self.w * ty + (self.z * tx - self.x * tz);
        *z = *z + self.w * tz + (self.x * ty - self.y * tx);
    }
}

impl std::ops::Add for Quaternion {
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        Self::new(
            self.w + rhs.w,
            self.x + rhs.x,
            self.y + rhs.y,
            self.z + rhs.z,
        )
    }
}

impl std::ops::Sub for Quaternion {
    type Output = Self;

    fn sub(self, rhs: Self) -> Self::Output {
        Self::new(
            self.w - rhs.w,
            self.x - rhs.x,
            self.y - rhs.y,
            self.z - rhs.z,
        )
    }
}

impl std::ops::Mul for Quaternion {
    type Output = Self;

    fn mul(self, rhs: Self) -> Self::Output {
        Self::new(
            self.w * rhs.w - self.x * rhs.x - self.y * rhs.y - self.z * rhs.z,
            self.w * rhs.x + self.x * rhs.w + self.y * rhs.z - self.z * rhs.y,
            self.w * rhs.y - self.x * rhs.z + self.y * rhs.w + self.z * rhs.x,
            self.w * rhs.z + self.x * rhs.y - self.y * rhs.x + self.z * rhs.w,
        )
    }
}

impl std::ops::Div for Quaternion {
    type Output = Self;

    fn div(self, rhs: Self) -> Self::Output {
        self * rhs.inverse()
    }
}

impl std::ops::Mul<f64> for Quaternion {
    type Output = Self;

    fn mul(self, rhs: f64) -> Self::Output {
        Self::new(self.w * rhs, self.x * rhs, self.y * rhs, self.z * rhs)
    }
}

impl std::ops::Div<f64> for Quaternion {
    type Output = Self;

    fn div(self, rhs: f64) -> Self::Output {
        Self::new(self.w / rhs, self.x / rhs, self.y / rhs, self.z / rhs)
    }
}

#[allow(dead_code)]
pub mod quaternion {
    use super::Quaternion;

    pub fn make(w: f64, x: f64, y: f64, z: f64) -> Quaternion {
        Quaternion::new(w, x, y, z)
    }
}
