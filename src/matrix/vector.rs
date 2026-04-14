pub use super::mat::{
    DVector, DynamicVector, Vector, Vector1, Vector2, Vector3, Vector3d, Vector3f, Vector4,
    Vector4d, Vector4f, Vector6, Vector6d, Vector6f,
};

use std::ops::{Add, Mul, Sub};

impl<T, const N: usize> Vector<T, N>
where
    T: Copy + Default + Add<Output = T> + Mul<Output = T>,
{
    pub fn dot(&self, other: &Self) -> T {
        let mut acc = T::default();
        for i in 0..N {
            acc = acc + self[i] * other[i];
        }
        acc
    }

    pub fn squared_norm(&self) -> T {
        self.dot(self)
    }
}

impl<T, const N: usize> Vector<T, N>
where
    T: Copy + Default + Add<Output = T> + Sub<Output = T> + Mul<Output = T>,
{
    pub fn map_zip<F>(&self, other: &Self, mut f: F) -> Self
    where
        F: FnMut(T, T) -> T,
    {
        let mut out = [T::default(); N];
        for i in 0..N {
            out[i] = f(self[i], other[i]);
        }
        Self::new(out)
    }

    pub fn add_vec(&self, other: &Self) -> Self {
        self.map_zip(other, |a, b| a + b)
    }

    pub fn sub_vec(&self, other: &Self) -> Self {
        self.map_zip(other, |a, b| a - b)
    }

    pub fn mul_scalar(&self, s: T) -> Self {
        let mut out = [T::default(); N];
        for i in 0..N {
            out[i] = self[i] * s;
        }
        Self::new(out)
    }
}

impl Vector<f64, 3> {
    pub fn cross(&self, other: &Self) -> Self {
        Self::new([
            self[1] * other[2] - self[2] * other[1],
            self[2] * other[0] - self[0] * other[2],
            self[0] * other[1] - self[1] * other[0],
        ])
    }

    pub fn norm(&self) -> f64 {
        self.squared_norm().sqrt()
    }

    pub fn length_f(&self) -> f64 {
        self.norm()
    }

    pub fn normalize(&self) -> Self {
        let n = self.norm();
        if n == 0.0 {
            *self
        } else {
            self.mul_scalar(1.0 / n)
        }
    }

    pub fn distance(&self, other: &Self) -> f64 {
        self.sub_vec(other).norm()
    }
}

impl Vector<f32, 3> {
    pub fn cross(&self, other: &Self) -> Self {
        Self::new([
            self[1] * other[2] - self[2] * other[1],
            self[2] * other[0] - self[0] * other[2],
            self[0] * other[1] - self[1] * other[0],
        ])
    }

    pub fn norm(&self) -> f32 {
        self.squared_norm().sqrt()
    }

    pub fn normalize(&self) -> Self {
        let n = self.norm();
        if n == 0.0 {
            *self
        } else {
            self.mul_scalar(1.0 / n)
        }
    }

    pub fn distance(&self, other: &Self) -> f32 {
        self.sub_vec(other).norm()
    }
}

impl<T> DVector<T>
where
    T: Copy + Default + Add<Output = T> + Mul<Output = T>,
{
    pub fn dot(&self, other: &Self) -> T {
        let n = self.values.len().min(other.values.len());
        let mut acc = T::default();
        for i in 0..n {
            acc = acc + self.values[i] * other.values[i];
        }
        acc
    }

    pub fn squared_norm(&self) -> T {
        self.dot(self)
    }
}

impl DVector<f64> {
    pub fn norm(&self) -> f64 {
        self.squared_norm().sqrt()
    }
}

pub type Vector2f = Vector<f32, 2>;
pub type Vector2d = Vector<f64, 2>;
pub type Vector1f = Vector<f32, 1>;
pub type Vector1d = Vector<f64, 1>;
