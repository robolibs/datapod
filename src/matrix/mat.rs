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
pub struct DynamicVector<T> {
    pub values: Vec<T>,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Dynamic<T> {
    pub rows: usize,
    pub cols: usize,
    pub values: Vec<T>,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Tensor<T> {
    pub shape: Vec<usize>,
    pub strides: Vec<usize>,
    pub values: Vec<T>,
}

pub type DynamicTensor<T> = Tensor<T>;

impl<T> DynamicVector<T> {
    pub fn from_vec(values: Vec<T>) -> Self {
        Self { values }
    }

    pub fn len(&self) -> usize {
        self.values.len()
    }

    pub fn size(&self) -> usize {
        self.values.len()
    }

    pub fn length(&self) -> usize {
        self.values.len()
    }

    pub fn empty(&self) -> bool {
        self.values.is_empty()
    }

    pub fn as_slice(&self) -> &[T] {
        &self.values
    }

    pub fn as_mut_slice(&mut self) -> &mut [T] {
        &mut self.values
    }

    pub fn data(&self) -> *const T {
        self.values.as_ptr()
    }

    pub fn data_mut(&mut self) -> *mut T {
        self.values.as_mut_ptr()
    }

    pub fn front(&self) -> Option<&T> {
        self.values.first()
    }

    pub fn back(&self) -> Option<&T> {
        self.values.last()
    }

    pub fn at(&self, index: usize) -> Result<&T, &'static str> {
        self.values.get(index).ok_or("DynamicVector::at")
    }

    pub fn at_mut(&mut self, index: usize) -> Result<&mut T, &'static str> {
        self.values.get_mut(index).ok_or("DynamicVector::at")
    }

    pub fn iter(&self) -> std::slice::Iter<'_, T> {
        self.values.iter()
    }

    pub fn iter_mut(&mut self) -> std::slice::IterMut<'_, T> {
        self.values.iter_mut()
    }

    pub fn resize(&mut self, new_len: usize, value: T)
    where
        T: Clone,
    {
        self.values.resize(new_len, value);
    }

    pub fn push(&mut self, value: T) {
        self.values.push(value);
    }

    pub fn pop(&mut self) -> Option<T> {
        self.values.pop()
    }

    pub fn fill(&mut self, value: T)
    where
        T: Clone,
    {
        self.values.fill(value);
    }

    pub fn swap(&mut self, other: &mut Self) {
        std::mem::swap(&mut self.values, &mut other.values);
    }
}

impl<T: Default + Clone> DynamicVector<T> {
    pub fn new(size: usize) -> Self {
        Self {
            values: vec![T::default(); size],
        }
    }
}

impl<T> From<Vec<T>> for DynamicVector<T> {
    fn from(values: Vec<T>) -> Self {
        Self::from_vec(values)
    }
}

impl<T> Index<usize> for DynamicVector<T> {
    type Output = T;

    fn index(&self, index: usize) -> &Self::Output {
        &self.values[index]
    }
}

impl<T> IndexMut<usize> for DynamicVector<T> {
    fn index_mut(&mut self, index: usize) -> &mut Self::Output {
        &mut self.values[index]
    }
}

impl<T> Dynamic<T> {
    pub fn from_vec(rows: usize, cols: usize, values: Vec<T>) -> Result<Self, &'static str> {
        if rows * cols != values.len() {
            return Err("Dynamic::from_vec shape mismatch");
        }
        Ok(Self { rows, cols, values })
    }

    pub fn rows(&self) -> usize {
        self.rows
    }

    pub fn cols(&self) -> usize {
        self.cols
    }

    pub fn size(&self) -> usize {
        self.values.len()
    }

    pub fn empty(&self) -> bool {
        self.rows == 0 || self.cols == 0
    }

    pub fn is_valid(&self) -> bool {
        self.rows * self.cols == self.values.len()
    }

    pub fn data(&self) -> *const T {
        self.values.as_ptr()
    }

    pub fn data_mut(&mut self) -> *mut T {
        self.values.as_mut_ptr()
    }

    pub fn as_slice(&self) -> &[T] {
        &self.values
    }

    pub fn as_mut_slice(&mut self) -> &mut [T] {
        &mut self.values
    }

    pub fn index_of(&self, row: usize, col: usize) -> usize {
        row * self.cols + col
    }

    pub fn at(&self, row: usize, col: usize) -> Result<&T, &'static str> {
        if row >= self.rows || col >= self.cols {
            return Err("Dynamic::at");
        }
        Ok(&self.values[self.index_of(row, col)])
    }

    pub fn at_mut(&mut self, row: usize, col: usize) -> Result<&mut T, &'static str> {
        if row >= self.rows || col >= self.cols {
            return Err("Dynamic::at");
        }
        let index = self.index_of(row, col);
        Ok(&mut self.values[index])
    }

    pub fn iter(&self) -> std::slice::Iter<'_, T> {
        self.values.iter()
    }

    pub fn iter_mut(&mut self) -> std::slice::IterMut<'_, T> {
        self.values.iter_mut()
    }

    pub fn fill(&mut self, value: T)
    where
        T: Clone,
    {
        self.values.fill(value);
    }

    pub fn swap(&mut self, other: &mut Self) {
        std::mem::swap(self, other);
    }

    pub fn row(&self, row: usize) -> Result<&[T], &'static str> {
        if row >= self.rows {
            return Err("Dynamic::row");
        }
        let start = row * self.cols;
        Ok(&self.values[start..start + self.cols])
    }
}

impl<T: Default + Clone> Dynamic<T> {
    pub fn new(rows: usize, cols: usize) -> Self {
        Self {
            rows,
            cols,
            values: vec![T::default(); rows * cols],
        }
    }

    pub fn resize(&mut self, rows: usize, cols: usize) {
        self.rows = rows;
        self.cols = cols;
        self.values.resize(rows * cols, T::default());
    }
}

impl<T> Index<(usize, usize)> for Dynamic<T> {
    type Output = T;

    fn index(&self, index: (usize, usize)) -> &Self::Output {
        &self.values[self.index_of(index.0, index.1)]
    }
}

impl<T> IndexMut<(usize, usize)> for Dynamic<T> {
    fn index_mut(&mut self, index: (usize, usize)) -> &mut Self::Output {
        let flat = self.index_of(index.0, index.1);
        &mut self.values[flat]
    }
}

impl<T> Tensor<T> {
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

    fn compute_size(shape: &[usize]) -> usize {
        if shape.is_empty() {
            0
        } else {
            shape.iter().product()
        }
    }

    pub fn from_vec(shape: Vec<usize>, values: Vec<T>) -> Result<Self, &'static str> {
        let expected = Self::compute_size(&shape);
        if expected != values.len() {
            return Err("Tensor::from_vec shape mismatch");
        }
        let strides = Self::compute_strides(&shape);
        Ok(Self {
            shape,
            strides,
            values,
        })
    }

    pub fn rank(&self) -> usize {
        self.shape.len()
    }

    pub fn size(&self) -> usize {
        self.values.len()
    }

    pub fn empty(&self) -> bool {
        self.values.is_empty()
    }

    pub fn dim(&self, i: usize) -> Option<usize> {
        self.shape.get(i).copied()
    }

    pub fn shape(&self) -> &[usize] {
        &self.shape
    }

    pub fn strides(&self) -> &[usize] {
        &self.strides
    }

    pub fn data(&self) -> *const T {
        self.values.as_ptr()
    }

    pub fn data_mut(&mut self) -> *mut T {
        self.values.as_mut_ptr()
    }

    pub fn as_slice(&self) -> &[T] {
        &self.values
    }

    pub fn as_mut_slice(&mut self) -> &mut [T] {
        &mut self.values
    }

    pub fn linear_index(&self, indices: &[usize]) -> Result<usize, &'static str> {
        if indices.len() != self.shape.len() {
            return Err("Tensor::linear_index wrong rank");
        }
        let mut linear = 0;
        for (axis, &index) in indices.iter().enumerate() {
            if index >= self.shape[axis] {
                return Err("Tensor::linear_index");
            }
            linear += index * self.strides[axis];
        }
        Ok(linear)
    }

    pub fn at(&self, indices: &[usize]) -> Result<&T, &'static str> {
        Ok(&self.values[self.linear_index(indices)?])
    }

    pub fn at_mut(&mut self, indices: &[usize]) -> Result<&mut T, &'static str> {
        let linear = self.linear_index(indices)?;
        Ok(&mut self.values[linear])
    }

    pub fn fill(&mut self, value: T)
    where
        T: Clone,
    {
        self.values.fill(value);
    }

    pub fn swap(&mut self, other: &mut Self) {
        std::mem::swap(self, other);
    }
}

impl<T: Default + Clone> Tensor<T> {
    pub fn new(shape: Vec<usize>) -> Self {
        let size = Self::compute_size(&shape);
        let strides = Self::compute_strides(&shape);
        Self {
            shape,
            strides,
            values: vec![T::default(); size],
        }
    }

    pub fn resize(&mut self, shape: Vec<usize>) {
        let size = Self::compute_size(&shape);
        self.shape = shape;
        self.strides = Self::compute_strides(&self.shape);
        self.values.resize(size, T::default());
    }
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

impl<T> Quaternion<T>
where
    T: Copy
        + Default
        + PartialOrd
        + std::ops::Add<Output = T>
        + std::ops::Sub<Output = T>
        + std::ops::Mul<Output = T>
        + std::ops::Div<Output = T>
        + std::ops::Neg<Output = T>
        + From<f32>,
{
    pub fn new(w: T, x: T, y: T, z: T) -> Self {
        Self { w, x, y, z }
    }

    pub fn identity() -> Self {
        Self::new(1.0f32.into(), T::default(), T::default(), T::default())
    }

    pub fn i() -> Self {
        Self::new(T::default(), 1.0f32.into(), T::default(), T::default())
    }

    pub fn j() -> Self {
        Self::new(T::default(), T::default(), 1.0f32.into(), T::default())
    }

    pub fn k() -> Self {
        Self::new(T::default(), T::default(), T::default(), 1.0f32.into())
    }

    pub fn scalar(&self) -> T {
        self.w
    }

    pub fn norm_squared(&self) -> T {
        self.w * self.w + self.x * self.x + self.y * self.y + self.z * self.z
    }

    pub fn is_identity(&self) -> bool
    where
        T: PartialEq,
    {
        *self == Self::identity()
    }

    pub fn is_real(&self) -> bool
    where
        T: PartialEq,
    {
        self.x == T::default() && self.y == T::default() && self.z == T::default()
    }

    pub fn is_pure(&self) -> bool
    where
        T: PartialEq,
    {
        self.w == T::default()
    }

    pub fn is_set(&self) -> bool
    where
        T: PartialEq,
    {
        !self.is_identity()
    }

    pub fn conjugate(&self) -> Self {
        Self::new(self.w, -self.x, -self.y, -self.z)
    }

    pub fn unit_inverse(&self) -> Self {
        self.conjugate()
    }
}

impl Quaternion<f64> {
    pub fn from_axis_angle(ax: f64, ay: f64, az: f64, angle: f64) -> Self {
        let half = angle / 2.0;
        let s = half.sin();
        Self::new(half.cos(), ax * s, ay * s, az * s)
    }

    pub fn from_euler(roll: f64, pitch: f64, yaw: f64) -> Self {
        let cr = (roll * 0.5).cos();
        let sr = (roll * 0.5).sin();
        let cp = (pitch * 0.5).cos();
        let sp = (pitch * 0.5).sin();
        let cy = (yaw * 0.5).cos();
        let sy = (yaw * 0.5).sin();
        Self::new(
            cr * cp * cy + sr * sp * sy,
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
        )
    }

    pub fn norm(&self) -> f64 {
        self.norm_squared().sqrt()
    }

    pub fn magnitude(&self) -> f64 {
        self.norm()
    }

    pub fn is_unit(&self, tolerance: f64) -> bool {
        (self.norm_squared() - 1.0).abs() < tolerance
    }

    pub fn inverse(&self) -> Self {
        let n2 = self.norm_squared();
        Self::new(self.w / n2, -self.x / n2, -self.y / n2, -self.z / n2)
    }

    pub fn normalized(&self) -> Self {
        let n = self.norm();
        if n < 1e-10 {
            Self::identity()
        } else {
            Self::new(self.w / n, self.x / n, self.y / n, self.z / n)
        }
    }

    pub fn rotate_vector(&self, vx: &mut f64, vy: &mut f64, vz: &mut f64) {
        let tx = 2.0 * (self.y * *vz - self.z * *vy);
        let ty = 2.0 * (self.z * *vx - self.x * *vz);
        let tz = 2.0 * (self.x * *vy - self.y * *vx);
        *vx = *vx + self.w * tx + (self.y * tz - self.z * ty);
        *vy = *vy + self.w * ty + (self.z * tx - self.x * tz);
        *vz = *vz + self.w * tz + (self.x * ty - self.y * tx);
    }

    pub fn to_euler(&self) -> (f64, f64, f64) {
        let sinr_cosp = 2.0 * (self.w * self.x + self.y * self.z);
        let cosr_cosp = 1.0 - 2.0 * (self.x * self.x + self.y * self.y);
        let roll = sinr_cosp.atan2(cosr_cosp);
        let sinp = 2.0 * (self.w * self.y - self.z * self.x);
        let pitch = if sinp.abs() >= 1.0 {
            sinp.signum() * std::f64::consts::FRAC_PI_2
        } else {
            sinp.asin()
        };
        let siny_cosp = 2.0 * (self.w * self.z + self.x * self.y);
        let cosy_cosp = 1.0 - 2.0 * (self.y * self.y + self.z * self.z);
        let yaw = siny_cosp.atan2(cosy_cosp);
        (roll, pitch, yaw)
    }

    pub fn to_axis_angle(&self) -> (f64, f64, f64, f64) {
        let angle = 2.0 * self.w.acos();
        let s = (1.0 - self.w * self.w).sqrt();
        if s < 1e-10 {
            (1.0, 0.0, 0.0, angle)
        } else {
            (self.x / s, self.y / s, self.z / s, angle)
        }
    }
}

impl<T> std::ops::Add for Quaternion<T>
where
    T: Copy + std::ops::Add<Output = T>,
{
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        Self {
            w: self.w + rhs.w,
            x: self.x + rhs.x,
            y: self.y + rhs.y,
            z: self.z + rhs.z,
        }
    }
}

impl<T> std::ops::Sub for Quaternion<T>
where
    T: Copy + std::ops::Sub<Output = T>,
{
    type Output = Self;

    fn sub(self, rhs: Self) -> Self::Output {
        Self {
            w: self.w - rhs.w,
            x: self.x - rhs.x,
            y: self.y - rhs.y,
            z: self.z - rhs.z,
        }
    }
}

impl<T> std::ops::Mul for Quaternion<T>
where
    T: Copy + std::ops::Add<Output = T> + std::ops::Sub<Output = T> + std::ops::Mul<Output = T>,
{
    type Output = Self;

    fn mul(self, rhs: Self) -> Self::Output {
        Self {
            w: self.w * rhs.w - self.x * rhs.x - self.y * rhs.y - self.z * rhs.z,
            x: self.w * rhs.x + self.x * rhs.w + self.y * rhs.z - self.z * rhs.y,
            y: self.w * rhs.y - self.x * rhs.z + self.y * rhs.w + self.z * rhs.x,
            z: self.w * rhs.z + self.x * rhs.y - self.y * rhs.x + self.z * rhs.w,
        }
    }
}

impl<T> std::ops::Mul<T> for Quaternion<T>
where
    T: Copy + std::ops::Mul<Output = T>,
{
    type Output = Self;

    fn mul(self, rhs: T) -> Self::Output {
        Self {
            w: self.w * rhs,
            x: self.x * rhs,
            y: self.y * rhs,
            z: self.z * rhs,
        }
    }
}

impl<T> std::ops::Div<T> for Quaternion<T>
where
    T: Copy + std::ops::Div<Output = T>,
{
    type Output = Self;

    fn div(self, rhs: T) -> Self::Output {
        Self {
            w: self.w / rhs,
            x: self.x / rhs,
            y: self.y / rhs,
            z: self.z / rhs,
        }
    }
}

impl<T> std::ops::Neg for Quaternion<T>
where
    T: Copy + std::ops::Neg<Output = T>,
{
    type Output = Self;

    fn neg(self) -> Self::Output {
        Self {
            w: -self.w,
            x: -self.x,
            y: -self.y,
            z: -self.z,
        }
    }
}

pub fn dot<T>(a: Quaternion<T>, b: Quaternion<T>) -> T
where
    T: Copy + std::ops::Add<Output = T> + std::ops::Mul<Output = T>,
{
    a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z
}

pub fn lerp<T>(a: Quaternion<T>, b: Quaternion<T>, t: T) -> Quaternion<T>
where
    T: Copy + std::ops::Add<Output = T> + std::ops::Sub<Output = T> + std::ops::Mul<Output = T>,
{
    Quaternion {
        w: a.w + t * (b.w - a.w),
        x: a.x + t * (b.x - a.x),
        y: a.y + t * (b.y - a.y),
        z: a.z + t * (b.z - a.z),
    }
}

pub fn nlerp(a: Quaternion<f64>, b: Quaternion<f64>, t: f64) -> Quaternion<f64> {
    let b2 = if dot(a, b) < 0.0 { -b } else { b };
    lerp(a, b2, t).normalized()
}

pub fn slerp(a: Quaternion<f64>, b: Quaternion<f64>, t: f64) -> Quaternion<f64> {
    let mut d = dot(a, b);
    let b2 = if d < 0.0 {
        d = -d;
        -b
    } else {
        b
    };
    if d > 0.9995 {
        return nlerp(a, b2, t);
    }
    let theta = d.acos();
    let sin_theta = theta.sin();
    let wa = ((1.0 - t) * theta).sin() / sin_theta;
    let wb = (t * theta).sin() / sin_theta;
    Quaternion::new(
        wa * a.w + wb * b2.w,
        wa * a.x + wb * b2.x,
        wa * a.y + wb * b2.y,
        wa * a.z + wb * b2.z,
    )
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Hypercomplex<T> {
    pub values: Vec<T>,
}

pub type BigInt = i128;
