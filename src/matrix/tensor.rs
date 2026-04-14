pub use super::mat::{DTensor, DynamicTensor, Tensor};

use std::ops::{Add, Mul, Sub};

impl<T> DTensor<T>
where
    T: Copy + Default,
{
    pub fn zeros(shape: Vec<usize>) -> Self {
        let size = shape.iter().product::<usize>();
        let strides = compute_strides(&shape);
        Self {
            shape,
            strides,
            values: vec![T::default(); size],
        }
    }

    pub fn filled(shape: Vec<usize>, value: T) -> Self {
        let size = shape.iter().product::<usize>();
        let strides = compute_strides(&shape);
        Self {
            shape,
            strides,
            values: vec![value; size],
        }
    }

    pub fn reshape(&mut self, shape: Vec<usize>) -> Result<(), &'static str> {
        let new_size: usize = shape.iter().product();
        if new_size != self.values.len() {
            return Err("DTensor::reshape size mismatch");
        }
        self.strides = compute_strides(&shape);
        self.shape = shape;
        Ok(())
    }
}

impl<T> DTensor<T>
where
    T: Copy + Default + Add<Output = T> + Sub<Output = T>,
{
    pub fn add_tensor(&self, other: &Self) -> Result<Self, &'static str> {
        if self.shape != other.shape {
            return Err("DTensor::add shape mismatch");
        }
        let mut values = vec![T::default(); self.values.len()];
        for (i, v) in values.iter_mut().enumerate() {
            *v = self.values[i] + other.values[i];
        }
        Ok(Self {
            shape: self.shape.clone(),
            strides: self.strides.clone(),
            values,
        })
    }

    pub fn sub_tensor(&self, other: &Self) -> Result<Self, &'static str> {
        if self.shape != other.shape {
            return Err("DTensor::sub shape mismatch");
        }
        let mut values = vec![T::default(); self.values.len()];
        for (i, v) in values.iter_mut().enumerate() {
            *v = self.values[i] - other.values[i];
        }
        Ok(Self {
            shape: self.shape.clone(),
            strides: self.strides.clone(),
            values,
        })
    }
}

impl<T> DTensor<T>
where
    T: Copy + Default + Mul<Output = T>,
{
    pub fn mul_scalar(&self, s: T) -> Self {
        let mut values = vec![T::default(); self.values.len()];
        for (i, v) in values.iter_mut().enumerate() {
            *v = self.values[i] * s;
        }
        Self {
            shape: self.shape.clone(),
            strides: self.strides.clone(),
            values,
        }
    }
}

fn compute_strides(shape: &[usize]) -> Vec<usize> {
    if shape.is_empty() {
        return Vec::new();
    }
    let mut strides = vec![0; shape.len()];
    strides[0] = 1;
    for i in 1..shape.len() {
        strides[i] = strides[i - 1] * shape[i - 1];
    }
    strides
}
