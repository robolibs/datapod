pub use super::mat::{DMatrix, Dynamic, DynamicMatrix};

use std::ops::{Add, Mul, Sub};

impl<T> DMatrix<T>
where
    T: Copy + Default,
{
    pub fn zeros(rows: usize, cols: usize) -> Self {
        Self {
            rows,
            cols,
            values: vec![T::default(); rows * cols],
        }
    }

    pub fn filled(rows: usize, cols: usize, value: T) -> Self {
        Self {
            rows,
            cols,
            values: vec![value; rows * cols],
        }
    }
}

impl<T> DMatrix<T>
where
    T: Copy + Default + Add<Output = T> + Sub<Output = T>,
{
    pub fn add_mat(&self, other: &Self) -> Result<Self, &'static str> {
        if self.rows != other.rows || self.cols != other.cols {
            return Err("DMatrix::add shape mismatch");
        }
        let mut values = vec![T::default(); self.values.len()];
        for (i, v) in values.iter_mut().enumerate() {
            *v = self.values[i] + other.values[i];
        }
        Ok(Self {
            rows: self.rows,
            cols: self.cols,
            values,
        })
    }

    pub fn sub_mat(&self, other: &Self) -> Result<Self, &'static str> {
        if self.rows != other.rows || self.cols != other.cols {
            return Err("DMatrix::sub shape mismatch");
        }
        let mut values = vec![T::default(); self.values.len()];
        for (i, v) in values.iter_mut().enumerate() {
            *v = self.values[i] - other.values[i];
        }
        Ok(Self {
            rows: self.rows,
            cols: self.cols,
            values,
        })
    }
}

impl<T> DMatrix<T>
where
    T: Copy + Default + Add<Output = T> + Mul<Output = T>,
{
    pub fn mul_mat(&self, other: &Self) -> Result<Self, &'static str> {
        if self.cols != other.rows {
            return Err("DMatrix::mul shape mismatch");
        }
        let mut out = Self::zeros(self.rows, other.cols);
        for r in 0..self.rows {
            for c in 0..other.cols {
                let mut acc = T::default();
                for k in 0..self.cols {
                    acc = acc + self.values[r * self.cols + k] * other.values[k * other.cols + c];
                }
                out.values[r * out.cols + c] = acc;
            }
        }
        Ok(out)
    }
}

impl<T> DMatrix<T>
where
    T: Copy + Default,
{
    pub fn transpose(&self) -> Self {
        let mut out = Self::zeros(self.cols, self.rows);
        for r in 0..self.rows {
            for c in 0..self.cols {
                out.values[c * out.cols + r] = self.values[r * self.cols + c];
            }
        }
        out
    }
}

impl<T> DMatrix<T>
where
    T: Copy + Default + From<f32>,
{
    pub fn identity(n: usize) -> Self {
        let mut out = Self::zeros(n, n);
        let one = T::from(1.0);
        for i in 0..n {
            out.values[i * n + i] = one;
        }
        out
    }
}
