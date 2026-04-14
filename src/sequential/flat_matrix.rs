//! Row-major flat matrix matching `datapod::FlatMatrix<T>`.
//!
//! Mirrors the C++ layout of `n_rows * n_columns` contiguous entries in a
//! `Vector<T>`, with `(i, j)` access, row views and a no-op default-constructor.

use crate::Vector;

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct FlatMatrix<T> {
    pub n_rows: usize,
    pub n_columns: usize,
    pub entries: Vector<T>,
    // Legacy field names for backwards compatibility with existing code.
    pub rows: usize,
    pub cols: usize,
    pub values: Vector<T>,
}

impl<T> FlatMatrix<T> {
    pub fn new() -> Self
    where
        T: Default,
    {
        Self::default()
    }

    pub fn n_rows(&self) -> usize {
        self.n_rows
    }

    pub fn n_columns(&self) -> usize {
        self.n_columns
    }

    pub fn size(&self) -> usize {
        self.n_rows * self.n_columns
    }

    pub fn is_empty(&self) -> bool {
        self.entries.is_empty()
    }

    pub fn get(&self, i: usize, j: usize) -> Option<&T> {
        if i >= self.n_rows || j >= self.n_columns {
            return None;
        }
        self.entries.as_slice().get(i * self.n_columns + j)
    }

    pub fn get_mut(&mut self, i: usize, j: usize) -> Option<&mut T> {
        if i >= self.n_rows || j >= self.n_columns {
            return None;
        }
        let idx = i * self.n_columns + j;
        self.entries.as_mut_slice().get_mut(idx)
    }

    pub fn at(&self, i: usize, j: usize) -> &T {
        self.get(i, j).expect("FlatMatrix::at: out of range")
    }

    pub fn at_mut(&mut self, i: usize, j: usize) -> &mut T {
        self.get_mut(i, j).expect("FlatMatrix::at: out of range")
    }

    pub fn row(&self, i: usize) -> &[T] {
        assert!(i < self.n_rows, "FlatMatrix::row: out of range");
        let start = i * self.n_columns;
        &self.entries.as_slice()[start..start + self.n_columns]
    }

    pub fn row_mut(&mut self, i: usize) -> &mut [T] {
        assert!(i < self.n_rows, "FlatMatrix::row_mut: out of range");
        let start = i * self.n_columns;
        let end = start + self.n_columns;
        &mut self.entries.as_mut_slice()[start..end]
    }
}

impl<T: Clone + Default> FlatMatrix<T> {
    pub fn with_shape(n_rows: usize, n_columns: usize) -> Self {
        let mut entries = Vector::new();
        entries.resize(n_rows * n_columns, T::default());
        Self {
            n_rows,
            n_columns,
            entries,
            rows: n_rows,
            cols: n_columns,
            values: Vector::new(),
        }
    }

    pub fn with_shape_value(n_rows: usize, n_columns: usize, init: T) -> Self {
        let mut entries = Vector::new();
        entries.resize(n_rows * n_columns, init);
        Self {
            n_rows,
            n_columns,
            entries,
            rows: n_rows,
            cols: n_columns,
            values: Vector::new(),
        }
    }

    pub fn resize(&mut self, n_rows: usize, n_columns: usize) {
        self.n_rows = n_rows;
        self.n_columns = n_columns;
        self.rows = n_rows;
        self.cols = n_columns;
        self.entries.resize(n_rows * n_columns, T::default());
    }

    pub fn reset(&mut self, value: T) {
        for slot in self.entries.iter_mut() {
            *slot = value.clone();
        }
    }
}

impl<T> std::ops::Index<(usize, usize)> for FlatMatrix<T> {
    type Output = T;
    fn index(&self, (i, j): (usize, usize)) -> &T {
        self.at(i, j)
    }
}

impl<T> std::ops::IndexMut<(usize, usize)> for FlatMatrix<T> {
    fn index_mut(&mut self, (i, j): (usize, usize)) -> &mut T {
        self.at_mut(i, j)
    }
}
