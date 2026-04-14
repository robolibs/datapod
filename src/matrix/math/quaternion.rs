use std::ops::{Add, AddAssign, Div, DivAssign, Mul, MulAssign, Neg, Sub, SubAssign};

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Quaternion<T> {
    pub w: T,
    pub x: T,
    pub y: T,
    pub z: T,
}

impl<T> Quaternion<T> {
    pub const fn new(w: T, x: T, y: T, z: T) -> Self {
        Self { w, x, y, z }
    }
}

impl<T: Copy + Default + From<f32>> Quaternion<T> {
    pub fn identity() -> Self {
        Self {
            w: T::from(1.0),
            x: T::default(),
            y: T::default(),
            z: T::default(),
        }
    }

    pub fn i() -> Self {
        Self {
            w: T::default(),
            x: T::from(1.0),
            y: T::default(),
            z: T::default(),
        }
    }

    pub fn j() -> Self {
        Self {
            w: T::default(),
            x: T::default(),
            y: T::from(1.0),
            z: T::default(),
        }
    }

    pub fn k() -> Self {
        Self {
            w: T::default(),
            x: T::default(),
            y: T::default(),
            z: T::from(1.0),
        }
    }
}

impl<T: Copy> Quaternion<T> {
    pub fn scalar(&self) -> T {
        self.w
    }
}

impl<T> Quaternion<T>
where
    T: Copy + Add<Output = T> + Mul<Output = T>,
{
    pub fn norm_squared(&self) -> T {
        self.w * self.w + self.x * self.x + self.y * self.y + self.z * self.z
    }
}

impl<T> Quaternion<T>
where
    T: Copy + Neg<Output = T>,
{
    pub fn conjugate(&self) -> Self {
        Self::new(self.w, -self.x, -self.y, -self.z)
    }
}

impl<T: Copy + Default + PartialEq + From<f32>> Quaternion<T> {
    pub fn is_identity(&self) -> bool {
        self.w == T::from(1.0)
            && self.x == T::default()
            && self.y == T::default()
            && self.z == T::default()
    }

    pub fn is_set(&self) -> bool {
        !self.is_identity()
    }
}

impl<T: Copy + Default + PartialEq> Quaternion<T> {
    pub fn is_real(&self) -> bool {
        let z = T::default();
        self.x == z && self.y == z && self.z == z
    }

    pub fn is_pure(&self) -> bool {
        self.w == T::default()
    }
}

impl Quaternion<f64> {
    pub fn from_axis_angle(ax: f64, ay: f64, az: f64, angle: f64) -> Self {
        let half = angle / 2.0;
        let s = half.sin();
        Self::new(half.cos(), ax * s, ay * s, az * s)
    }

    pub fn from_euler(roll: f64, pitch: f64, yaw: f64) -> Self {
        let cr = (roll * 0.5).cos();
        let sr = (roll * 0.5).sin();
        let cp = (pitch * 0.5).cos();
        let sp = (pitch * 0.5).sin();
        let cy = (yaw * 0.5).cos();
        let sy = (yaw * 0.5).sin();
        Self::new(
            cr * cp * cy + sr * sp * sy,
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
        )
    }

    pub fn norm(&self) -> f64 {
        self.norm_squared().sqrt()
    }

    pub fn magnitude(&self) -> f64 {
        self.norm()
    }

    pub fn is_unit(&self, tolerance: f64) -> bool {
        (self.norm_squared() - 1.0).abs() < tolerance
    }

    pub fn inverse(&self) -> Self {
        let n2 = self.norm_squared();
        Self::new(self.w / n2, -self.x / n2, -self.y / n2, -self.z / n2)
    }

    pub fn unit_inverse(&self) -> Self {
        self.conjugate()
    }

    pub fn normalized(&self) -> Self {
        let n = self.norm();
        if n < 1e-10 {
            Self::identity()
        } else {
            Self::new(self.w / n, self.x / n, self.y / n, self.z / n)
        }
    }

    pub fn rotate_vector(&self, vx: &mut f64, vy: &mut f64, vz: &mut f64) {
        let tx = 2.0 * (self.y * *vz - self.z * *vy);
        let ty = 2.0 * (self.z * *vx - self.x * *vz);
        let tz = 2.0 * (self.x * *vy - self.y * *vx);
        *vx += self.w * tx + (self.y * tz - self.z * ty);
        *vy += self.w * ty + (self.z * tx - self.x * tz);
        *vz += self.w * tz + (self.x * ty - self.y * tx);
    }

    pub fn to_euler(&self) -> (f64, f64, f64) {
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
        (roll, pitch, yaw)
    }

    pub fn to_axis_angle(&self) -> (f64, f64, f64, f64) {
        let angle = 2.0 * self.w.acos();
        let s = (1.0 - self.w * self.w).sqrt();
        if s < 1e-10 {
            (1.0, 0.0, 0.0, angle)
        } else {
            (self.x / s, self.y / s, self.z / s, angle)
        }
    }

    pub fn exp(&self) -> Self {
        let vnorm = (self.x * self.x + self.y * self.y + self.z * self.z).sqrt();
        let ew = self.w.exp();
        if vnorm < 1e-10 {
            return Self::new(ew, 0.0, 0.0, 0.0);
        }
        let s = ew * vnorm.sin() / vnorm;
        Self::new(ew * vnorm.cos(), s * self.x, s * self.y, s * self.z)
    }

    pub fn ln(&self) -> Self {
        let n = self.norm();
        let vnorm = (self.x * self.x + self.y * self.y + self.z * self.z).sqrt();
        if vnorm < 1e-10 {
            return Self::new(n.ln(), 0.0, 0.0, 0.0);
        }
        let s = (self.w / n).acos() / vnorm;
        Self::new(n.ln(), s * self.x, s * self.y, s * self.z)
    }

    pub fn powf(&self, t: f64) -> Self {
        (self.ln() * t).exp()
    }
}

impl<T> Add for Quaternion<T>
where
    T: Copy + Add<Output = T>,
{
    type Output = Self;
    fn add(self, rhs: Self) -> Self {
        Self::new(
            self.w + rhs.w,
            self.x + rhs.x,
            self.y + rhs.y,
            self.z + rhs.z,
        )
    }
}

impl<T> Sub for Quaternion<T>
where
    T: Copy + Sub<Output = T>,
{
    type Output = Self;
    fn sub(self, rhs: Self) -> Self {
        Self::new(
            self.w - rhs.w,
            self.x - rhs.x,
            self.y - rhs.y,
            self.z - rhs.z,
        )
    }
}

impl<T> Mul for Quaternion<T>
where
    T: Copy + Add<Output = T> + Sub<Output = T> + Mul<Output = T>,
{
    type Output = Self;
    fn mul(self, rhs: Self) -> Self {
        Self::new(
            self.w * rhs.w - self.x * rhs.x - self.y * rhs.y - self.z * rhs.z,
            self.w * rhs.x + self.x * rhs.w + self.y * rhs.z - self.z * rhs.y,
            self.w * rhs.y - self.x * rhs.z + self.y * rhs.w + self.z * rhs.x,
            self.w * rhs.z + self.x * rhs.y - self.y * rhs.x + self.z * rhs.w,
        )
    }
}

impl Div for Quaternion<f64> {
    type Output = Self;
    fn div(self, rhs: Self) -> Self {
        self * rhs.inverse()
    }
}

impl<T> Mul<T> for Quaternion<T>
where
    T: Copy + Mul<Output = T>,
{
    type Output = Self;
    fn mul(self, s: T) -> Self {
        Self::new(self.w * s, self.x * s, self.y * s, self.z * s)
    }
}

impl<T> Div<T> for Quaternion<T>
where
    T: Copy + Div<Output = T>,
{
    type Output = Self;
    fn div(self, s: T) -> Self {
        Self::new(self.w / s, self.x / s, self.y / s, self.z / s)
    }
}

impl<T> Neg for Quaternion<T>
where
    T: Copy + Neg<Output = T>,
{
    type Output = Self;
    fn neg(self) -> Self {
        Self::new(-self.w, -self.x, -self.y, -self.z)
    }
}

impl<T> AddAssign for Quaternion<T>
where
    T: Copy + Add<Output = T>,
{
    fn add_assign(&mut self, rhs: Self) {
        *self = *self + rhs;
    }
}

impl<T> SubAssign for Quaternion<T>
where
    T: Copy + Sub<Output = T>,
{
    fn sub_assign(&mut self, rhs: Self) {
        *self = *self - rhs;
    }
}

impl<T> MulAssign for Quaternion<T>
where
    T: Copy + Add<Output = T> + Sub<Output = T> + Mul<Output = T>,
{
    fn mul_assign(&mut self, rhs: Self) {
        *self = *self * rhs;
    }
}

impl DivAssign for Quaternion<f64> {
    fn div_assign(&mut self, rhs: Self) {
        *self = *self / rhs;
    }
}

pub fn dot<T>(a: Quaternion<T>, b: Quaternion<T>) -> T
where
    T: Copy + Add<Output = T> + Mul<Output = T>,
{
    a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z
}

pub fn lerp<T>(a: Quaternion<T>, b: Quaternion<T>, t: T) -> Quaternion<T>
where
    T: Copy + Add<Output = T> + Sub<Output = T> + Mul<Output = T>,
{
    Quaternion::new(
        a.w + t * (b.w - a.w),
        a.x + t * (b.x - a.x),
        a.y + t * (b.y - a.y),
        a.z + t * (b.z - a.z),
    )
}

pub fn nlerp(a: Quaternion<f64>, b: Quaternion<f64>, t: f64) -> Quaternion<f64> {
    let b2 = if dot(a, b) < 0.0 { -b } else { b };
    lerp(a, b2, t).normalized()
}

pub fn slerp(a: Quaternion<f64>, b: Quaternion<f64>, t: f64) -> Quaternion<f64> {
    let mut d = dot(a, b);
    let b2 = if d < 0.0 {
        d = -d;
        -b
    } else {
        b
    };
    if d > 0.9995 {
        return nlerp(a, b2, t);
    }
    let theta = d.acos();
    let sin_theta = theta.sin();
    let wa = ((1.0 - t) * theta).sin() / sin_theta;
    let wb = (t * theta).sin() / sin_theta;
    Quaternion::new(
        wa * a.w + wb * b2.w,
        wa * a.x + wb * b2.x,
        wa * a.y + wb * b2.y,
        wa * a.z + wb * b2.z,
    )
}

pub type Quaternionf = Quaternion<f32>;
pub type Quaterniond = Quaternion<f64>;
