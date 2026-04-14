use std::ops::{Add, AddAssign, Div, DivAssign, Mul, MulAssign, Neg, Sub, SubAssign};

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Interval<T> {
    pub lo: T,
    pub hi: T,
}

impl<T> Interval<T> {
    pub const fn new(lo: T, hi: T) -> Self {
        Self { lo, hi }
    }
}

impl<T: Copy> Interval<T> {
    pub fn point(value: T) -> Self {
        Self {
            lo: value,
            hi: value,
        }
    }
}

impl Interval<f64> {
    pub fn entire() -> Self {
        Self::new(f64::NEG_INFINITY, f64::INFINITY)
    }

    pub fn empty() -> Self {
        Self::new(f64::INFINITY, f64::NEG_INFINITY)
    }

    pub fn with_uncertainty(value: f64, uncertainty: f64) -> Self {
        Self::new(value - uncertainty, value + uncertainty)
    }

    pub fn width(&self) -> f64 {
        self.hi - self.lo
    }

    pub fn midpoint(&self) -> f64 {
        (self.lo + self.hi) / 2.0
    }

    pub fn radius(&self) -> f64 {
        self.width() / 2.0
    }

    pub fn is_empty(&self) -> bool {
        self.lo > self.hi
    }

    pub fn is_point(&self) -> bool {
        self.lo == self.hi
    }

    pub fn is_set(&self) -> bool {
        !self.is_empty()
    }

    pub fn contains(&self, value: f64) -> bool {
        self.lo <= value && value <= self.hi
    }

    pub fn contains_interval(&self, other: &Self) -> bool {
        self.lo <= other.lo && other.hi <= self.hi
    }

    pub fn intersects(&self, other: &Self) -> bool {
        self.lo <= other.hi && other.lo <= self.hi
    }

    pub fn intersect(&self, other: &Self) -> Self {
        Self::new(self.lo.max(other.lo), self.hi.min(other.hi))
    }

    pub fn hull(&self, other: &Self) -> Self {
        Self::new(self.lo.min(other.lo), self.hi.max(other.hi))
    }

    pub fn sqrt(&self) -> Self {
        if self.hi < 0.0 {
            return Self::empty();
        }
        let lo = if self.lo > 0.0 { self.lo.sqrt() } else { 0.0 };
        Self::new(lo, self.hi.sqrt())
    }

    pub fn sqr(&self) -> Self {
        if self.lo >= 0.0 {
            Self::new(self.lo * self.lo, self.hi * self.hi)
        } else if self.hi <= 0.0 {
            Self::new(self.hi * self.hi, self.lo * self.lo)
        } else {
            Self::new(0.0, (self.lo * self.lo).max(self.hi * self.hi))
        }
    }

    pub fn abs(&self) -> Self {
        if self.lo >= 0.0 {
            *self
        } else if self.hi <= 0.0 {
            -*self
        } else {
            Self::new(0.0, (-self.lo).max(self.hi))
        }
    }

    pub fn exp(&self) -> Self {
        Self::new(self.lo.exp(), self.hi.exp())
    }

    pub fn ln(&self) -> Self {
        if self.hi <= 0.0 {
            return Self::empty();
        }
        let lo = if self.lo > 0.0 {
            self.lo.ln()
        } else {
            f64::NEG_INFINITY
        };
        Self::new(lo, self.hi.ln())
    }

    pub fn powi(&self, exp: i32) -> Self {
        if exp == 0 {
            return Self::new(1.0, 1.0);
        }
        if exp > 0 {
            if exp % 2 == 0 {
                let half = self.powi(exp / 2);
                return half.sqr();
            }
            return *self * self.powi(exp - 1);
        }
        Self::new(1.0, 1.0) / self.powi(-exp)
    }
}

impl Add for Interval<f64> {
    type Output = Self;
    fn add(self, rhs: Self) -> Self {
        Self::new(self.lo + rhs.lo, self.hi + rhs.hi)
    }
}

impl Sub for Interval<f64> {
    type Output = Self;
    fn sub(self, rhs: Self) -> Self {
        Self::new(self.lo - rhs.hi, self.hi - rhs.lo)
    }
}

impl Mul for Interval<f64> {
    type Output = Self;
    fn mul(self, rhs: Self) -> Self {
        let p1 = self.lo * rhs.lo;
        let p2 = self.lo * rhs.hi;
        let p3 = self.hi * rhs.lo;
        let p4 = self.hi * rhs.hi;
        Self::new(p1.min(p2).min(p3).min(p4), p1.max(p2).max(p3).max(p4))
    }
}

impl Div for Interval<f64> {
    type Output = Self;
    fn div(self, rhs: Self) -> Self {
        if rhs.lo <= 0.0 && 0.0 <= rhs.hi {
            return Self::entire();
        }
        self * Self::new(1.0 / rhs.hi, 1.0 / rhs.lo)
    }
}

impl Mul<f64> for Interval<f64> {
    type Output = Self;
    fn mul(self, s: f64) -> Self {
        if s >= 0.0 {
            Self::new(self.lo * s, self.hi * s)
        } else {
            Self::new(self.hi * s, self.lo * s)
        }
    }
}

impl Div<f64> for Interval<f64> {
    type Output = Self;
    fn div(self, s: f64) -> Self {
        self * (1.0 / s)
    }
}

impl Add<f64> for Interval<f64> {
    type Output = Self;
    fn add(self, s: f64) -> Self {
        Self::new(self.lo + s, self.hi + s)
    }
}

impl Sub<f64> for Interval<f64> {
    type Output = Self;
    fn sub(self, s: f64) -> Self {
        Self::new(self.lo - s, self.hi - s)
    }
}

impl Neg for Interval<f64> {
    type Output = Self;
    fn neg(self) -> Self {
        Self::new(-self.hi, -self.lo)
    }
}

impl AddAssign for Interval<f64> {
    fn add_assign(&mut self, rhs: Self) {
        *self = *self + rhs;
    }
}

impl SubAssign for Interval<f64> {
    fn sub_assign(&mut self, rhs: Self) {
        *self = *self - rhs;
    }
}

impl MulAssign for Interval<f64> {
    fn mul_assign(&mut self, rhs: Self) {
        *self = *self * rhs;
    }
}

impl DivAssign for Interval<f64> {
    fn div_assign(&mut self, rhs: Self) {
        *self = *self / rhs;
    }
}

pub type Intervalf = Interval<f32>;
pub type Intervald = Interval<f64>;
