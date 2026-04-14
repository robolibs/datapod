use std::ops::{Add, AddAssign, Div, Mul, MulAssign, Neg, Sub, SubAssign};

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Polynomial<T> {
    pub coeffs: Vec<T>,
}

impl<T> Polynomial<T> {
    pub fn new(coeffs: Vec<T>) -> Self {
        Self { coeffs }
    }

    pub fn degree(&self) -> usize {
        self.coeffs.len().saturating_sub(1)
    }

    pub fn size(&self) -> usize {
        self.coeffs.len()
    }
}

impl<T: Copy + Default + PartialEq> Polynomial<T> {
    pub fn is_zero(&self) -> bool {
        let z = T::default();
        self.coeffs.iter().all(|c| *c == z)
    }

    pub fn is_set(&self) -> bool {
        !self.is_zero()
    }

    pub fn actual_degree(&self) -> usize {
        let z = T::default();
        for i in (0..self.coeffs.len()).rev() {
            if self.coeffs[i] != z {
                return i;
            }
        }
        0
    }
}

impl<T> Polynomial<T>
where
    T: Copy + Default + Add<Output = T> + Mul<Output = T>,
{
    pub fn eval(&self, x: T) -> T {
        if self.coeffs.is_empty() {
            return T::default();
        }
        let n = self.coeffs.len();
        let mut result = self.coeffs[n - 1];
        for i in (0..n - 1).rev() {
            result = result * x + self.coeffs[i];
        }
        result
    }
}

impl Polynomial<f64> {
    pub fn derivative(&self) -> Self {
        if self.coeffs.len() <= 1 {
            return Self::new(vec![0.0]);
        }
        let mut out = Vec::with_capacity(self.coeffs.len() - 1);
        for i in 1..self.coeffs.len() {
            out.push(self.coeffs[i] * i as f64);
        }
        Self::new(out)
    }

    pub fn integral(&self, constant: f64) -> Self {
        let mut out = Vec::with_capacity(self.coeffs.len() + 1);
        out.push(constant);
        for (i, c) in self.coeffs.iter().enumerate() {
            out.push(*c / (i + 1) as f64);
        }
        Self::new(out)
    }

    pub fn integrate(&self, a: f64, b: f64) -> f64 {
        let anti = self.integral(0.0);
        anti.eval(b) - anti.eval(a)
    }
}

impl<T> Add for Polynomial<T>
where
    T: Copy + Default + Add<Output = T>,
{
    type Output = Self;
    fn add(self, rhs: Self) -> Self {
        let n = self.coeffs.len().max(rhs.coeffs.len());
        let mut out = vec![T::default(); n];
        for (i, c) in self.coeffs.iter().enumerate() {
            out[i] = *c;
        }
        for (i, c) in rhs.coeffs.iter().enumerate() {
            out[i] = out[i] + *c;
        }
        Self::new(out)
    }
}

impl<T> Sub for Polynomial<T>
where
    T: Copy + Default + Add<Output = T> + Sub<Output = T>,
{
    type Output = Self;
    fn sub(self, rhs: Self) -> Self {
        let n = self.coeffs.len().max(rhs.coeffs.len());
        let mut out = vec![T::default(); n];
        for (i, c) in self.coeffs.iter().enumerate() {
            out[i] = *c;
        }
        for (i, c) in rhs.coeffs.iter().enumerate() {
            out[i] = out[i] - *c;
        }
        Self::new(out)
    }
}

impl<T> Mul for Polynomial<T>
where
    T: Copy + Default + Add<Output = T> + Mul<Output = T>,
{
    type Output = Self;
    fn mul(self, rhs: Self) -> Self {
        if self.coeffs.is_empty() || rhs.coeffs.is_empty() {
            return Self::new(Vec::new());
        }
        let n = self.coeffs.len() + rhs.coeffs.len() - 1;
        let mut out = vec![T::default(); n];
        for (i, ai) in self.coeffs.iter().enumerate() {
            for (j, bj) in rhs.coeffs.iter().enumerate() {
                out[i + j] = out[i + j] + (*ai * *bj);
            }
        }
        Self::new(out)
    }
}

impl<T> Mul<T> for Polynomial<T>
where
    T: Copy + Mul<Output = T>,
{
    type Output = Self;
    fn mul(self, s: T) -> Self {
        Self::new(self.coeffs.iter().map(|c| *c * s).collect())
    }
}

impl<T> Div<T> for Polynomial<T>
where
    T: Copy + Div<Output = T>,
{
    type Output = Self;
    fn div(self, s: T) -> Self {
        Self::new(self.coeffs.iter().map(|c| *c / s).collect())
    }
}

impl<T> Neg for Polynomial<T>
where
    T: Copy + Neg<Output = T>,
{
    type Output = Self;
    fn neg(self) -> Self {
        Self::new(self.coeffs.iter().map(|c| -*c).collect())
    }
}

impl<T> AddAssign for Polynomial<T>
where
    T: Copy + Default + Add<Output = T>,
{
    fn add_assign(&mut self, rhs: Self) {
        *self = self.clone() + rhs;
    }
}

impl<T> SubAssign for Polynomial<T>
where
    T: Copy + Default + Add<Output = T> + Sub<Output = T>,
{
    fn sub_assign(&mut self, rhs: Self) {
        *self = self.clone() - rhs;
    }
}

impl<T> MulAssign for Polynomial<T>
where
    T: Copy + Default + Add<Output = T> + Mul<Output = T>,
{
    fn mul_assign(&mut self, rhs: Self) {
        *self = self.clone() * rhs;
    }
}

pub type Polynomialf = Polynomial<f32>;
pub type Polynomiald = Polynomial<f64>;
