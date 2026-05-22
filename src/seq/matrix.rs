//! Row-major dense matrix Pod — holds any `T: bytemuck::Pod`.

use crate::seq::assert_element_size;

#[datapod::datapod]
#[derive(Default)]
pub struct Matrix {
    pub rows: u32,
    pub cols: u32,
    pub element_size: u32,
    pub _pad: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Matrix {
    pub fn new<T: bytemuck::Pod>(rows: u32, cols: u32) -> Self {
        let es = std::mem::size_of::<T>();
        let bytes = rows as usize * cols as usize * es;
        Self {
            rows,
            cols,
            element_size: es as u32,
            _pad: 0,
            data: vec![0u8; bytes],
        }
    }

    pub fn from_bytes<T: bytemuck::Pod>(rows: u32, cols: u32, data: Vec<u8>) -> Self {
        let es = std::mem::size_of::<T>();
        debug_assert_eq!(data.len(), rows as usize * cols as usize * es);
        Self {
            rows,
            cols,
            element_size: es as u32,
            _pad: 0,
            data,
        }
    }

    pub fn size(&self) -> usize {
        self.rows as usize * self.cols as usize
    }

    pub fn is_empty(&self) -> bool {
        self.rows == 0 || self.cols == 0
    }

    pub fn flat_index(&self, row: usize, col: usize) -> usize {
        row * self.cols as usize + col
    }

    pub fn is_valid(&self) -> bool {
        self.rows > 0
            && self.cols > 0
            && self.data.len() == self.size() * self.element_size as usize
    }

    pub fn as_slice<T: bytemuck::Pod>(&self) -> &[T] {
        assert_element_size::<T>(self.element_size);
        bytemuck::cast_slice(&self.data)
    }

    pub fn as_mut_slice<T: bytemuck::Pod>(&mut self) -> &mut [T] {
        assert_element_size::<T>(self.element_size);
        bytemuck::cast_slice_mut(&mut self.data)
    }

    pub fn row<T: bytemuck::Pod>(&self, row: usize) -> &[T] {
        let cols = self.cols as usize;
        &self.as_slice::<T>()[row * cols..(row + 1) * cols]
    }

    pub fn get<T: bytemuck::Pod>(&self, row: usize, col: usize) -> T {
        self.as_slice::<T>()[self.flat_index(row, col)]
    }

    pub fn set<T: bytemuck::Pod>(&mut self, row: usize, col: usize, value: T) {
        let idx = self.flat_index(row, col);
        self.as_mut_slice::<T>()[idx] = value;
    }
}
