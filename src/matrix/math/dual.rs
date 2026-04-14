use std::ops::{Add, AddAssign, Div, DivAssign, Mul, MulAssign, Neg, Sub, SubAssign};

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Dual<T> {
    pub real: T,
    pub eps: T,
}

impl<T> Dual<T> {
    pub const fn new(real: T, eps: T) -> Self {
        Self { real, eps }
    }
}

impl<T: Copy + Default + From<f32>> Dual<T> {
    pub fn variable(value: T) -> Self {
        Self {
            real: value,
            eps: T::from(1.0),
        }
    }

    pub fn constant(value: T) -> Self {
        Self {
            real: value,
            eps: T::default(),
        }
    }
}

impl<T: Copy> Dual<T> {
    pub fn value(&self) -> T {
        self.real
    }

    pub fn derivative(&self) -> T {
        self.eps
    }
}

impl<T: Copy + PartialEq + Default> Dual<T> {
    pub fn is_set(&self) -> bool {
        self.real != T::default() || self.eps != T::default()
    }
}

impl Dual<f64> {
    pub fn sqrt(&self) -> Self {
        let s = self.real.sqrt();
        Self::new(s, self.eps / (2.0 * s))
    }

    pub fn exp(&self) -> Self {
        let e = self.real.exp();
        Self::new(e, e * self.eps)
    }

    pub fn ln(&self) -> Self {
        Self::new(self.real.ln(), self.eps / self.real)
    }

    pub fn powf(&self, exp: f64) -> Self {
        let p = self.real.powf(exp);
        Self::new(p, exp * self.real.powf(exp - 1.0) * self.eps)
    }

    pub fn pow_dual(&self, exp: Self) -> Self {
        let p = self.real.powf(exp.real);
        let dp = p * (exp.eps * self.real.ln() + exp.real * self.eps / self.real);
        Self::new(p, dp)
    }

    pub fn sin(&self) -> Self {
        Self::new(self.real.sin(), self.real.cos() * self.eps)
    }

    pub fn cos(&self) -> Self {
        Self::new(self.real.cos(), -self.real.sin() * self.eps)
    }

    pub fn tan(&self) -> Self {
        let c = self.real.cos();
        Self::new(self.real.tan(), self.eps / (c * c))
    }

    pub fn asin(&self) -> Self {
        Self::new(
            self.real.asin(),
            self.eps / (1.0 - self.real * self.real).sqrt(),
        )
    }

    pub fn acos(&self) -> Self {
        Self::new(
            self.real.acos(),
            -self.eps / (1.0 - self.real * self.real).sqrt(),
        )
    }

    pub fn atan(&self) -> Self {
        Self::new(self.real.atan(), self.eps / (1.0 + self.real * self.real))
    }

    pub fn sinh(&self) -> Self {
        Self::new(self.real.sinh(), self.real.cosh() * self.eps)
    }

    pub fn cosh(&self) -> Self {
        Self::new(self.real.cosh(), self.real.sinh() * self.eps)
    }

    pub fn tanh(&self) -> Self {
        let c = self.real.cosh();
        Self::new(self.real.tanh(), self.eps / (c * c))
    }

    pub fn abs(&self) -> Self {
        if self.real >= 0.0 { *self } else { -*self }
    }
}

impl<T> Add for Dual<T>
where
    T: Copy + Add<Output = T>,
{
    type Output = Self;
    fn add(self, rhs: Self) -> Self {
        Self::new(self.real + rhs.real, self.eps + rhs.eps)
    }
}

impl<T> Sub for Dual<T>
where
    T: Copy + Sub<Output = T>,
{
    type Output = Self;
    fn sub(self, rhs: Self) -> Self {
        Self::new(self.real - rhs.real, self.eps - rhs.eps)
    }
}

impl<T> Mul for Dual<T>
where
    T: Copy + Add<Output = T> + Mul<Output = T>,
{
    type Output = Self;
    fn mul(self, rhs: Self) -> Self {
        Self::new(
            self.real * rhs.real,
            self.real * rhs.eps + self.eps * rhs.real,
        )
    }
}

impl<T> Div for Dual<T>
where
    T: Copy + Mul<Output = T> + Sub<Output = T> + Div<Output = T>,
{
    type Output = Self;
    fn div(self, rhs: Self) -> Self {
        let denom = rhs.real * rhs.real;
        Self::new(
            self.real / rhs.real,
            (self.eps * rhs.real - self.real * rhs.eps) / denom,
        )
    }
}

impl<T> Neg for Dual<T>
where
    T: Copy + Neg<Output = T>,
{
    type Output = Self;
    fn neg(self) -> Self {
        Self::new(-self.real, -self.eps)
    }
}

impl<T> Mul<T> for Dual<T>
where
    T: Copy + Mul<Output = T>,
{
    type Output = Self;
    fn mul(self, s: T) -> Self {
        Self::new(self.real * s, self.eps * s)
    }
}

impl<T> Div<T> for Dual<T>
where
    T: Copy + Div<Output = T>,
{
    type Output = Self;
    fn div(self, s: T) -> Self {
        Self::new(self.real / s, self.eps / s)
    }
}

impl<T> AddAssign for Dual<T>
where
    T: Copy + Add<Output = T>,
{
    fn add_assign(&mut self, rhs: Self) {
        *self = *self + rhs;
    }
}

impl<T> SubAssign for Dual<T>
where
    T: Copy + Sub<Output = T>,
{
    fn sub_assign(&mut self, rhs: Self) {
        *self = *self - rhs;
    }
}

impl<T> MulAssign for Dual<T>
where
    T: Copy + Add<Output = T> + Mul<Output = T>,
{
    fn mul_assign(&mut self, rhs: Self) {
        *self = *self * rhs;
    }
}

impl<T> DivAssign for Dual<T>
where
    T: Copy + Mul<Output = T> + Sub<Output = T> + Div<Output = T>,
{
    fn div_assign(&mut self, rhs: Self) {
        *self = *self / rhs;
    }
}

pub type Dualf = Dual<f32>;
pub type Duald = Dual<f64>;
