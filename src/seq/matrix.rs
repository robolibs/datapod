//! Row-major dense matrix Pod — holds any `T: bytemuck::Pod`.

use crate::seq::assert_element_size;
use crate::{DataPodAccess, DataPodValidate, WireError};

#[datapod::datapod]
#[dp(manual_access)]
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

/// Borrowed, validation-backed view over a `Matrix` wire payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MatrixView<'a> {
    pub header: MatrixHeader,
    pub data: &'a [u8],
}

impl<'a> MatrixView<'a> {
    pub fn rows(&self) -> u32 {
        self.header.rows
    }

    pub fn cols(&self) -> u32 {
        self.header.cols
    }

    pub fn element_size(&self) -> u32 {
        self.header.element_size
    }

    pub fn payload_bytes(&self) -> &'a [u8] {
        self.data
    }

    pub fn size(&self) -> usize {
        self.header.rows as usize * self.header.cols as usize
    }

    pub fn flat_index(&self, row: usize, col: usize) -> usize {
        row * self.header.cols as usize + col
    }

    pub fn get_unaligned<T: bytemuck::Pod + Copy>(
        &self,
        row: usize,
        col: usize,
    ) -> Result<T, WireError> {
        assert_element_size::<T>(self.header.element_size);
        if row >= self.header.rows as usize || col >= self.header.cols as usize {
            return Err(crate::wire::invalid_header::<Matrix>(format!(
                "matrix index out of bounds: ({row}, {col}) for {}x{}",
                self.header.rows, self.header.cols
            )));
        }
        let elem_size = core::mem::size_of::<T>();
        let offset = self
            .flat_index(row, col)
            .checked_mul(elem_size)
            .ok_or_else(|| crate::wire::invalid_payload::<Matrix>("element offset overflowed"))?;
        let end = offset
            .checked_add(elem_size)
            .ok_or_else(|| crate::wire::invalid_payload::<Matrix>("element end overflowed"))?;
        Ok(bytemuck::pod_read_unaligned(&self.data[offset..end]))
    }

    pub fn as_aligned_slice<T: bytemuck::Pod>(&self) -> Result<&'a [T], WireError> {
        assert_element_size::<T>(self.header.element_size);
        bytemuck::try_cast_slice(self.data)
            .map_err(|error| crate::wire::invalid_payload::<Matrix>(error.to_string()))
    }
}

impl DataPodValidate for Matrix {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        if header.rows == 0 || header.cols == 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "rows and cols must be non-zero",
            ));
        }
        if header.element_size == 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "element_size must be non-zero",
            ));
        }
        if header._pad != 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "reserved _pad field must be zero",
            ));
        }
        let expected = crate::wire::checked_product::<Self>(&[
            header.rows as usize,
            header.cols as usize,
            header.element_size as usize,
        ])?;
        if payload.len() != expected {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "got {} bytes, expected {expected}",
                payload.len()
            )));
        }
        Ok(())
    }
}

impl DataPodAccess for Matrix {
    type View<'a> = MatrixView<'a>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Self::validate_wire_parts(&header, payload)?;
        Ok(MatrixView {
            header,
            data: payload,
        })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Self::View<'a> {
        MatrixView {
            header,
            data: payload,
        }
    }
}
