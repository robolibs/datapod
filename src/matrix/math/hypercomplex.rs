use std::ops::{Add, AddAssign, Div, Mul, MulAssign, Neg, Sub, SubAssign};

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Hypercomplex<T> {
    pub e0: T,
    pub e1: T,
    pub e2: T,
    pub e3: T,
    pub e4: T,
    pub e5: T,
    pub e6: T,
    pub e7: T,
}

pub type Octonion<T> = Hypercomplex<T>;

impl<T> Hypercomplex<T> {
    #[allow(clippy::too_many_arguments)]
    pub const fn new(e0: T, e1: T, e2: T, e3: T, e4: T, e5: T, e6: T, e7: T) -> Self {
        Self {
            e0,
            e1,
            e2,
            e3,
            e4,
            e5,
            e6,
            e7,
        }
    }
}

impl<T: Copy + Default> Hypercomplex<T> {
    pub fn from_scalar(s: T) -> Self {
        Self {
            e0: s,
            e1: T::default(),
            e2: T::default(),
            e3: T::default(),
            e4: T::default(),
            e5: T::default(),
            e6: T::default(),
            e7: T::default(),
        }
    }

    pub fn scalar(&self) -> T {
        self.e0
    }
}

impl<T: Copy + Default + From<f32>> Hypercomplex<T> {
    pub fn unit(idx: usize) -> Self {
        let mut o = Self::default();
        let one = T::from(1.0);
        match idx {
            0 => o.e0 = one,
            1 => o.e1 = one,
            2 => o.e2 = one,
            3 => o.e3 = one,
            4 => o.e4 = one,
            5 => o.e5 = one,
            6 => o.e6 = one,
            7 => o.e7 = one,
            _ => {}
        }
        o
    }
}

impl<T> Hypercomplex<T>
where
    T: Copy + Add<Output = T> + Mul<Output = T>,
{
    pub fn norm_squared(&self) -> T {
        self.e0 * self.e0
            + self.e1 * self.e1
            + self.e2 * self.e2
            + self.e3 * self.e3
            + self.e4 * self.e4
            + self.e5 * self.e5
            + self.e6 * self.e6
            + self.e7 * self.e7
    }
}

impl<T> Hypercomplex<T>
where
    T: Copy + Neg<Output = T>,
{
    pub fn conjugate(&self) -> Self {
        Self::new(
            self.e0, -self.e1, -self.e2, -self.e3, -self.e4, -self.e5, -self.e6, -self.e7,
        )
    }
}

impl<T: Copy + Default + PartialEq> Hypercomplex<T> {
    pub fn is_real(&self) -> bool {
        let z = T::default();
        self.e1 == z
            && self.e2 == z
            && self.e3 == z
            && self.e4 == z
            && self.e5 == z
            && self.e6 == z
            && self.e7 == z
    }

    pub fn is_set(&self) -> bool {
        let z = T::default();
        self.e0 != z
            || self.e1 != z
            || self.e2 != z
            || self.e3 != z
            || self.e4 != z
            || self.e5 != z
            || self.e6 != z
            || self.e7 != z
    }
}

impl Hypercomplex<f64> {
    pub fn norm(&self) -> f64 {
        self.norm_squared().sqrt()
    }

    pub fn magnitude(&self) -> f64 {
        self.norm()
    }

    pub fn inverse(&self) -> Self {
        let n2 = self.norm_squared();
        Self::new(
            self.e0 / n2,
            -self.e1 / n2,
            -self.e2 / n2,
            -self.e3 / n2,
            -self.e4 / n2,
            -self.e5 / n2,
            -self.e6 / n2,
            -self.e7 / n2,
        )
    }

    pub fn normalized(&self) -> Self {
        let n = self.norm();
        Self::new(
            self.e0 / n,
            self.e1 / n,
            self.e2 / n,
            self.e3 / n,
            self.e4 / n,
            self.e5 / n,
            self.e6 / n,
            self.e7 / n,
        )
    }

    pub fn exp(&self) -> Self {
        let vnorm = (self.e1 * self.e1
            + self.e2 * self.e2
            + self.e3 * self.e3
            + self.e4 * self.e4
            + self.e5 * self.e5
            + self.e6 * self.e6
            + self.e7 * self.e7)
            .sqrt();
        let ea = self.e0.exp();
        if vnorm < 1e-10 {
            return Self::new(ea, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        }
        let s = ea * vnorm.sin() / vnorm;
        let c = ea * vnorm.cos();
        Self::new(
            c,
            s * self.e1,
            s * self.e2,
            s * self.e3,
            s * self.e4,
            s * self.e5,
            s * self.e6,
            s * self.e7,
        )
    }

    pub fn ln(&self) -> Self {
        let n = self.norm();
        let vnorm = (self.e1 * self.e1
            + self.e2 * self.e2
            + self.e3 * self.e3
            + self.e4 * self.e4
            + self.e5 * self.e5
            + self.e6 * self.e6
            + self.e7 * self.e7)
            .sqrt();
        if vnorm < 1e-10 {
            return Self::new(n.ln(), 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        }
        let s = (self.e0 / n).acos() / vnorm;
        Self::new(
            n.ln(),
            s * self.e1,
            s * self.e2,
            s * self.e3,
            s * self.e4,
            s * self.e5,
            s * self.e6,
            s * self.e7,
        )
    }
}

impl<T> Add for Hypercomplex<T>
where
    T: Copy + Add<Output = T>,
{
    type Output = Self;
    fn add(self, rhs: Self) -> Self {
        Self::new(
            self.e0 + rhs.e0,
            self.e1 + rhs.e1,
            self.e2 + rhs.e2,
            self.e3 + rhs.e3,
            self.e4 + rhs.e4,
            self.e5 + rhs.e5,
            self.e6 + rhs.e6,
            self.e7 + rhs.e7,
        )
    }
}

impl<T> Sub for Hypercomplex<T>
where
    T: Copy + Sub<Output = T>,
{
    type Output = Self;
    fn sub(self, rhs: Self) -> Self {
        Self::new(
            self.e0 - rhs.e0,
            self.e1 - rhs.e1,
            self.e2 - rhs.e2,
            self.e3 - rhs.e3,
            self.e4 - rhs.e4,
            self.e5 - rhs.e5,
            self.e6 - rhs.e6,
            self.e7 - rhs.e7,
        )
    }
}

impl<T> Mul for Hypercomplex<T>
where
    T: Copy + Add<Output = T> + Sub<Output = T> + Mul<Output = T>,
{
    type Output = Self;
    fn mul(self, b: Self) -> Self {
        let a = self;
        let n0 = a.e0 * b.e0
            - a.e1 * b.e1
            - a.e2 * b.e2
            - a.e3 * b.e3
            - a.e4 * b.e4
            - a.e5 * b.e5
            - a.e6 * b.e6
            - a.e7 * b.e7;
        let n1 = a.e0 * b.e1 + a.e1 * b.e0 + a.e2 * b.e3 - a.e3 * b.e2 + a.e4 * b.e5 - a.e5 * b.e4
            - a.e6 * b.e7
            + a.e7 * b.e6;
        let n2 = a.e0 * b.e2 - a.e1 * b.e3 + a.e2 * b.e0 + a.e3 * b.e1 + a.e4 * b.e6 + a.e5 * b.e7
            - a.e6 * b.e4
            - a.e7 * b.e5;
        let n3 = a.e0 * b.e3 + a.e1 * b.e2 - a.e2 * b.e1 + a.e3 * b.e0 + a.e4 * b.e7 - a.e5 * b.e6
            + a.e6 * b.e5
            - a.e7 * b.e4;
        let n4 = a.e0 * b.e4
            - a.e1 * b.e5
            - a.e2 * b.e6
            - a.e3 * b.e7
            + a.e4 * b.e0
            + a.e5 * b.e1
            + a.e6 * b.e2
            + a.e7 * b.e3;
        let n5 = a.e0 * b.e5 + a.e1 * b.e4 - a.e2 * b.e7 + a.e3 * b.e6 - a.e4 * b.e1 + a.e5 * b.e0
            - a.e6 * b.e3
            + a.e7 * b.e2;
        let n6 = a.e0 * b.e6 + a.e1 * b.e7 + a.e2 * b.e4 - a.e3 * b.e5 - a.e4 * b.e2 + a.e5 * b.e3
            + a.e6 * b.e0
            - a.e7 * b.e1;
        let n7 = a.e0 * b.e7 - a.e1 * b.e6 + a.e2 * b.e5 + a.e3 * b.e4 - a.e4 * b.e3 - a.e5 * b.e2
            + a.e6 * b.e1
            + a.e7 * b.e0;
        Self::new(n0, n1, n2, n3, n4, n5, n6, n7)
    }
}

impl Div for Hypercomplex<f64> {
    type Output = Self;
    fn div(self, rhs: Self) -> Self {
        self * rhs.inverse()
    }
}

impl<T> Mul<T> for Hypercomplex<T>
where
    T: Copy + Mul<Output = T>,
{
    type Output = Self;
    fn mul(self, s: T) -> Self {
        Self::new(
            self.e0 * s,
            self.e1 * s,
            self.e2 * s,
            self.e3 * s,
            self.e4 * s,
            self.e5 * s,
            self.e6 * s,
            self.e7 * s,
        )
    }
}

impl<T> Div<T> for Hypercomplex<T>
where
    T: Copy + Div<Output = T>,
{
    type Output = Self;
    fn div(self, s: T) -> Self {
        Self::new(
            self.e0 / s,
            self.e1 / s,
            self.e2 / s,
            self.e3 / s,
            self.e4 / s,
            self.e5 / s,
            self.e6 / s,
            self.e7 / s,
        )
    }
}

impl<T> Neg for Hypercomplex<T>
where
    T: Copy + Neg<Output = T>,
{
    type Output = Self;
    fn neg(self) -> Self {
        Self::new(
            -self.e0, -self.e1, -self.e2, -self.e3, -self.e4, -self.e5, -self.e6, -self.e7,
        )
    }
}

impl<T> AddAssign for Hypercomplex<T>
where
    T: Copy + Add<Output = T>,
{
    fn add_assign(&mut self, rhs: Self) {
        *self = *self + rhs;
    }
}

impl<T> SubAssign for Hypercomplex<T>
where
    T: Copy + Sub<Output = T>,
{
    fn sub_assign(&mut self, rhs: Self) {
        *self = *self - rhs;
    }
}

impl<T> MulAssign for Hypercomplex<T>
where
    T: Copy + Add<Output = T> + Sub<Output = T> + Mul<Output = T>,
{
    fn mul_assign(&mut self, rhs: Self) {
        *self = *self * rhs;
    }
}

pub type Octonionf = Hypercomplex<f32>;
pub type Octoniond = Hypercomplex<f64>;
