use std::ops::{Add, AddAssign, Div, DivAssign, Mul, MulAssign, Neg, Sub, SubAssign};

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Complex<T> {
    pub real: T,
    pub imag: T,
}

impl<T> Complex<T> {
    pub const fn new(real: T, imag: T) -> Self {
        Self { real, imag }
    }
}

impl<T: Copy + Default> Complex<T> {
    pub fn from_real(real: T) -> Self {
        Self {
            real,
            imag: T::default(),
        }
    }
}

impl Complex<f64> {
    pub fn i() -> Self {
        Self::new(0.0, 1.0)
    }

    pub fn from_polar(magnitude: f64, phase: f64) -> Self {
        Self::new(magnitude * phase.cos(), magnitude * phase.sin())
    }

    pub fn magnitude_squared(&self) -> f64 {
        self.real * self.real + self.imag * self.imag
    }

    pub fn magnitude(&self) -> f64 {
        self.magnitude_squared().sqrt()
    }

    pub fn abs(&self) -> f64 {
        self.magnitude()
    }

    pub fn phase(&self) -> f64 {
        self.imag.atan2(self.real)
    }

    pub fn arg(&self) -> f64 {
        self.phase()
    }

    pub fn is_set(&self) -> bool {
        self.real != 0.0 || self.imag != 0.0
    }

    pub fn is_real(&self) -> bool {
        self.imag == 0.0
    }

    pub fn is_imaginary(&self) -> bool {
        self.real == 0.0
    }

    pub fn is_zero(&self) -> bool {
        self.real == 0.0 && self.imag == 0.0
    }

    pub fn conjugate(&self) -> Self {
        Self::new(self.real, -self.imag)
    }

    pub fn inverse(&self) -> Self {
        let d = self.magnitude_squared();
        Self::new(self.real / d, -self.imag / d)
    }

    pub fn normalized(&self) -> Self {
        let m = self.magnitude();
        Self::new(self.real / m, self.imag / m)
    }

    pub fn exp(&self) -> Self {
        let ea = self.real.exp();
        Self::new(ea * self.imag.cos(), ea * self.imag.sin())
    }

    pub fn ln(&self) -> Self {
        Self::new(self.magnitude().ln(), self.phase())
    }

    pub fn sqrt(&self) -> Self {
        let r = self.magnitude().sqrt();
        let half = self.phase() / 2.0;
        Self::new(r * half.cos(), r * half.sin())
    }

    pub fn powf(&self, exp: f64) -> Self {
        if self.is_zero() {
            return Self::default();
        }
        let r = self.magnitude().powf(exp);
        let theta = self.phase() * exp;
        Self::new(r * theta.cos(), r * theta.sin())
    }

    pub fn sin(&self) -> Self {
        Self::new(
            self.real.sin() * self.imag.cosh(),
            self.real.cos() * self.imag.sinh(),
        )
    }

    pub fn cos(&self) -> Self {
        Self::new(
            self.real.cos() * self.imag.cosh(),
            -self.real.sin() * self.imag.sinh(),
        )
    }

    pub fn tan(&self) -> Self {
        self.sin() / self.cos()
    }

    pub fn sinh(&self) -> Self {
        Self::new(
            self.real.sinh() * self.imag.cos(),
            self.real.cosh() * self.imag.sin(),
        )
    }

    pub fn cosh(&self) -> Self {
        Self::new(
            self.real.cosh() * self.imag.cos(),
            self.real.sinh() * self.imag.sin(),
        )
    }

    pub fn tanh(&self) -> Self {
        self.sinh() / self.cosh()
    }
}

impl<T> Add for Complex<T>
where
    T: Copy + Add<Output = T>,
{
    type Output = Self;
    fn add(self, rhs: Self) -> Self {
        Self::new(self.real + rhs.real, self.imag + rhs.imag)
    }
}

impl<T> Sub for Complex<T>
where
    T: Copy + Sub<Output = T>,
{
    type Output = Self;
    fn sub(self, rhs: Self) -> Self {
        Self::new(self.real - rhs.real, self.imag - rhs.imag)
    }
}

impl<T> Mul for Complex<T>
where
    T: Copy + Add<Output = T> + Sub<Output = T> + Mul<Output = T>,
{
    type Output = Self;
    fn mul(self, rhs: Self) -> Self {
        Self::new(
            self.real * rhs.real - self.imag * rhs.imag,
            self.real * rhs.imag + self.imag * rhs.real,
        )
    }
}

impl Div for Complex<f64> {
    type Output = Self;
    fn div(self, rhs: Self) -> Self {
        self * rhs.inverse()
    }
}

impl<T> Mul<T> for Complex<T>
where
    T: Copy + Mul<Output = T>,
{
    type Output = Self;
    fn mul(self, s: T) -> Self {
        Self::new(self.real * s, self.imag * s)
    }
}

impl<T> Div<T> for Complex<T>
where
    T: Copy + Div<Output = T>,
{
    type Output = Self;
    fn div(self, s: T) -> Self {
        Self::new(self.real / s, self.imag / s)
    }
}

impl<T> Neg for Complex<T>
where
    T: Copy + Neg<Output = T>,
{
    type Output = Self;
    fn neg(self) -> Self {
        Self::new(-self.real, -self.imag)
    }
}

impl<T> AddAssign for Complex<T>
where
    T: Copy + Add<Output = T>,
{
    fn add_assign(&mut self, rhs: Self) {
        *self = *self + rhs;
    }
}

impl<T> SubAssign for Complex<T>
where
    T: Copy + Sub<Output = T>,
{
    fn sub_assign(&mut self, rhs: Self) {
        *self = *self - rhs;
    }
}

impl<T> MulAssign for Complex<T>
where
    T: Copy + Add<Output = T> + Sub<Output = T> + Mul<Output = T>,
{
    fn mul_assign(&mut self, rhs: Self) {
        *self = *self * rhs;
    }
}

impl DivAssign for Complex<f64> {
    fn div_assign(&mut self, rhs: Self) {
        *self = *self / rhs;
    }
}

pub type Complexf = Complex<f32>;
pub type Complexd = Complex<f64>;
