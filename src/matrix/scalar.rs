pub use super::mat::Scalar;

use std::ops::{Add, AddAssign, Div, DivAssign, Mul, MulAssign, Neg, Sub, SubAssign};

impl<T: Copy> Scalar<T> {
    pub fn get(&self) -> T {
        self.value
    }
}

impl<T> Add for Scalar<T>
where
    T: Copy + Add<Output = T>,
{
    type Output = Self;
    fn add(self, rhs: Self) -> Self {
        Self::new(self.value + rhs.value)
    }
}

impl<T> Sub for Scalar<T>
where
    T: Copy + Sub<Output = T>,
{
    type Output = Self;
    fn sub(self, rhs: Self) -> Self {
        Self::new(self.value - rhs.value)
    }
}

impl<T> Mul for Scalar<T>
where
    T: Copy + Mul<Output = T>,
{
    type Output = Self;
    fn mul(self, rhs: Self) -> Self {
        Self::new(self.value * rhs.value)
    }
}

impl<T> Div for Scalar<T>
where
    T: Copy + Div<Output = T>,
{
    type Output = Self;
    fn div(self, rhs: Self) -> Self {
        Self::new(self.value / rhs.value)
    }
}

impl<T> Neg for Scalar<T>
where
    T: Copy + Neg<Output = T>,
{
    type Output = Self;
    fn neg(self) -> Self {
        Self::new(-self.value)
    }
}

impl<T> AddAssign for Scalar<T>
where
    T: Copy + Add<Output = T>,
{
    fn add_assign(&mut self, rhs: Self) {
        self.value = self.value + rhs.value;
    }
}

impl<T> SubAssign for Scalar<T>
where
    T: Copy + Sub<Output = T>,
{
    fn sub_assign(&mut self, rhs: Self) {
        self.value = self.value - rhs.value;
    }
}

impl<T> MulAssign for Scalar<T>
where
    T: Copy + Mul<Output = T>,
{
    fn mul_assign(&mut self, rhs: Self) {
        self.value = self.value * rhs.value;
    }
}

impl<T> DivAssign for Scalar<T>
where
    T: Copy + Div<Output = T>,
{
    fn div_assign(&mut self, rhs: Self) {
        self.value = self.value / rhs.value;
    }
}

pub type Scalarf = Scalar<f32>;
pub type Scalard = Scalar<f64>;
pub type Scalari = Scalar<i32>;
pub type Scalaru = Scalar<u32>;
