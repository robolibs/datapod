pub use super::mat::Matrix;

use std::ops::{Add, Mul, Sub};

impl<T, const R: usize, const C: usize> Matrix<T, R, C>
where
    T: Copy + Default,
{
    pub fn zero() -> Self {
        Self::from_rows([[T::default(); C]; R])
    }

    pub fn filled(value: T) -> Self {
        Self::from_rows([[value; C]; R])
    }

    pub fn get(&self, row: usize, col: usize) -> T {
        self.as_rows()[row][col]
    }

    pub fn row_slice(&self, row: usize) -> &[T; C] {
        &self.as_rows()[row]
    }

    pub fn map<F>(&self, mut f: F) -> Self
    where
        F: FnMut(T) -> T,
    {
        let rows = self.as_rows();
        let mut out = [[T::default(); C]; R];
        for r in 0..R {
            for c in 0..C {
                out[r][c] = f(rows[r][c]);
            }
        }
        Self::from_rows(out)
    }
}

impl<T, const R: usize, const C: usize> Matrix<T, R, C>
where
    T: Copy + Default + Add<Output = T> + Sub<Output = T>,
{
    pub fn add_mat(&self, other: &Self) -> Self {
        let a = self.as_rows();
        let b = other.as_rows();
        let mut out = [[T::default(); C]; R];
        for r in 0..R {
            for c in 0..C {
                out[r][c] = a[r][c] + b[r][c];
            }
        }
        Self::from_rows(out)
    }

    pub fn sub_mat(&self, other: &Self) -> Self {
        let a = self.as_rows();
        let b = other.as_rows();
        let mut out = [[T::default(); C]; R];
        for r in 0..R {
            for c in 0..C {
                out[r][c] = a[r][c] - b[r][c];
            }
        }
        Self::from_rows(out)
    }
}

impl<T, const R: usize, const C: usize> Matrix<T, R, C>
where
    T: Copy + Default + Mul<Output = T>,
{
    pub fn mul_scalar(&self, s: T) -> Self {
        let src = self.as_rows();
        let mut out = [[T::default(); C]; R];
        for r in 0..R {
            for c in 0..C {
                out[r][c] = src[r][c] * s;
            }
        }
        Self::from_rows(out)
    }
}

impl<T, const R: usize, const C: usize> Matrix<T, R, C>
where
    T: Copy + Default + Add<Output = T> + Mul<Output = T>,
{
    pub fn mul_mat<const P: usize>(&self, other: &Matrix<T, C, P>) -> Matrix<T, R, P> {
        let a = self.as_rows();
        let b = other.as_rows();
        let mut out = [[T::default(); P]; R];
        for r in 0..R {
            for c in 0..P {
                let mut acc = T::default();
                for k in 0..C {
                    acc = acc + a[r][k] * b[k][c];
                }
                out[r][c] = acc;
            }
        }
        Matrix::from_rows(out)
    }
}

impl<T, const R: usize, const C: usize> Matrix<T, R, C>
where
    T: Copy + Default,
{
    pub fn transpose(&self) -> Matrix<T, C, R> {
        let src = self.as_rows();
        let mut out = [[T::default(); R]; C];
        for r in 0..R {
            for c in 0..C {
                out[c][r] = src[r][c];
            }
        }
        Matrix::from_rows(out)
    }
}

impl<T, const N: usize> Matrix<T, N, N>
where
    T: Copy + Default + From<f32>,
{
    pub fn identity() -> Self {
        let zero = T::default();
        let one = T::from(1.0);
        let mut out = [[zero; N]; N];
        for (i, row) in out.iter_mut().enumerate().take(N) {
            row[i] = one;
        }
        Self::from_rows(out)
    }
}

pub type Matrix2f = Matrix<f32, 2, 2>;
pub type Matrix2d = Matrix<f64, 2, 2>;
pub type Matrix3f = Matrix<f32, 3, 3>;
pub type Matrix3d = Matrix<f64, 3, 3>;
pub type Matrix4f = Matrix<f32, 4, 4>;
pub type Matrix4d = Matrix<f64, 4, 4>;
