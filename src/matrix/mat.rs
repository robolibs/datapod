use std::ops::{Index, IndexMut};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Scalar<T> {
    pub value: T,
}

impl<T> Scalar<T> {
    pub const fn new(value: T) -> Self {
        Self { value }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(align(32))]
pub struct Vector<T, const N: usize> {
    data: [T; N],
}

impl<T: Default + Copy, const N: usize> Default for Vector<T, N> {
    fn default() -> Self {
        Self {
            data: [T::default(); N],
        }
    }
}

impl<T, const N: usize> Vector<T, N> {
    #[allow(non_upper_case_globals)]
    pub const rank: usize = 1;
    pub const SIZE: usize = N;

    pub const fn new(data: [T; N]) -> Self {
        Self { data }
    }

    pub fn data(&self) -> *const T {
        self.data.as_ptr()
    }

    pub fn data_mut(&mut self) -> *mut T {
        self.data.as_mut_ptr()
    }

    pub const fn size(&self) -> usize {
        N
    }

    pub const fn length(&self) -> usize {
        N
    }

    pub const fn empty(&self) -> bool {
        N == 0
    }

    pub fn as_slice(&self) -> &[T] {
        &self.data
    }

    pub fn as_mut_slice(&mut self) -> &mut [T] {
        &mut self.data
    }

    pub fn iter(&self) -> std::slice::Iter<'_, T> {
        self.data.iter()
    }

    pub fn iter_mut(&mut self) -> std::slice::IterMut<'_, T> {
        self.data.iter_mut()
    }

    pub fn front(&self) -> &T {
        &self.data[0]
    }

    pub fn back(&self) -> &T {
        &self.data[N - 1]
    }

    pub fn at(&self, index: usize) -> Result<&T, &'static str> {
        self.data.get(index).ok_or("Vector::at")
    }

    pub fn at_mut(&mut self, index: usize) -> Result<&mut T, &'static str> {
        self.data.get_mut(index).ok_or("Vector::at")
    }

    pub fn fill(&mut self, value: T)
    where
        T: Copy,
    {
        self.data.fill(value);
    }

    pub fn swap(&mut self, other: &mut Self) {
        std::mem::swap(&mut self.data, &mut other.data);
    }
}

impl<T, const N: usize> Index<usize> for Vector<T, N> {
    type Output = T;

    fn index(&self, index: usize) -> &Self::Output {
        &self.data[index]
    }
}

impl<T, const N: usize> IndexMut<usize> for Vector<T, N> {
    fn index_mut(&mut self, index: usize) -> &mut Self::Output {
        &mut self.data[index]
    }
}

impl<T, const N: usize> From<[T; N]> for Vector<T, N> {
    fn from(value: [T; N]) -> Self {
        Self::new(value)
    }
}

impl<T, const N: usize> IntoIterator for Vector<T, N> {
    type Item = T;
    type IntoIter = std::array::IntoIter<T, N>;

    fn into_iter(self) -> Self::IntoIter {
        self.data.into_iter()
    }
}

impl<'a, T, const N: usize> IntoIterator for &'a Vector<T, N> {
    type Item = &'a T;
    type IntoIter = std::slice::Iter<'a, T>;

    fn into_iter(self) -> Self::IntoIter {
        self.data.iter()
    }
}

impl<'a, T, const N: usize> IntoIterator for &'a mut Vector<T, N> {
    type Item = &'a mut T;
    type IntoIter = std::slice::IterMut<'a, T>;

    fn into_iter(self) -> Self::IntoIter {
        self.data.iter_mut()
    }
}

pub type Vector1<T> = Vector<T, 1>;
pub type Vector2<T> = Vector<T, 2>;
pub type Vector3<T> = Vector<T, 3>;
pub type Vector4<T> = Vector<T, 4>;
pub type Vector6<T> = Vector<T, 6>;

pub type Vector3f = Vector<f32, 3>;
pub type Vector3d = Vector<f64, 3>;
pub type Vector4f = Vector<f32, 4>;
pub type Vector4d = Vector<f64, 4>;
pub type Vector6f = Vector<f32, 6>;
pub type Vector6d = Vector<f64, 6>;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(align(32))]
pub struct Matrix<T, const R: usize, const C: usize> {
    data: [[T; C]; R],
}

impl<T: Default + Copy, const R: usize, const C: usize> Default for Matrix<T, R, C> {
    fn default() -> Self {
        Self {
            data: [[T::default(); C]; R],
        }
    }
}

impl<T, const R: usize, const C: usize> Matrix<T, R, C> {
    pub fn from_rows(data: [[T; C]; R]) -> Self {
        Self { data }
    }

    pub fn rows(&self) -> usize {
        R
    }

    pub fn cols(&self) -> usize {
        C
    }

    pub fn as_rows(&self) -> &[[T; C]; R] {
        &self.data
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Dynamic<T> {
    pub values: Vec<T>,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Tensor<T> {
    pub shape: Vec<usize>,
    pub values: Vec<T>,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Complex<T> {
    pub re: T,
    pub im: T,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Fraction<T> {
    pub numerator: T,
    pub denominator: T,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Modular<T> {
    pub value: T,
    pub modulus: T,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Interval<T> {
    pub min: T,
    pub max: T,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Dual<T> {
    pub real: T,
    pub dual: T,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Phasor<T> {
    pub magnitude: T,
    pub phase: T,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Polynomial<T> {
    pub coefficients: Vec<T>,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Quaternion<T> {
    pub w: T,
    pub x: T,
    pub y: T,
    pub z: T,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Hypercomplex<T> {
    pub values: Vec<T>,
}

pub type BigInt = i128;
