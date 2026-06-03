//! Dense 3-D tensor Pod — holds any `T: bytemuck::Pod`.

use crate::seq::assert_element_size;
use crate::{DataPodAccess, DataPodValidate, WireError};

#[datapod::datapod]
#[dp(manual_access)]
#[derive(Default)]
pub struct Tensor {
    pub rows: u32,
    pub cols: u32,
    pub layers: u32,
    pub element_size: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Tensor {
    pub fn new<T: bytemuck::Pod>(rows: u32, cols: u32, layers: u32) -> Self {
        let es = std::mem::size_of::<T>();
        let bytes = rows as usize * cols as usize * layers as usize * es;
        Self {
            rows,
            cols,
            layers,
            element_size: es as u32,
            data: vec![0u8; bytes],
        }
    }

    pub fn from_bytes<T: bytemuck::Pod>(rows: u32, cols: u32, layers: u32, data: Vec<u8>) -> Self {
        let es = std::mem::size_of::<T>();
        debug_assert_eq!(
            data.len(),
            rows as usize * cols as usize * layers as usize * es
        );
        Self {
            rows,
            cols,
            layers,
            element_size: es as u32,
            data,
        }
    }

    pub fn size(&self) -> usize {
        self.rows as usize * self.cols as usize * self.layers as usize
    }

    pub fn is_empty(&self) -> bool {
        self.rows == 0 || self.cols == 0 || self.layers == 0
    }

    pub fn flat_index(&self, row: usize, col: usize, layer: usize) -> usize {
        layer * (self.rows as usize * self.cols as usize) + row * self.cols as usize + col
    }

    pub fn is_valid(&self) -> bool {
        !self.is_empty() && self.data.len() == self.size() * self.element_size as usize
    }

    pub fn as_slice<T: bytemuck::Pod>(&self) -> &[T] {
        assert_element_size::<T>(self.element_size);
        bytemuck::cast_slice(&self.data)
    }

    pub fn as_mut_slice<T: bytemuck::Pod>(&mut self) -> &mut [T] {
        assert_element_size::<T>(self.element_size);
        bytemuck::cast_slice_mut(&mut self.data)
    }

    pub fn layer<T: bytemuck::Pod>(&self, layer: usize) -> &[T] {
        let stride = self.rows as usize * self.cols as usize;
        &self.as_slice::<T>()[layer * stride..(layer + 1) * stride]
    }

    pub fn get<T: bytemuck::Pod>(&self, row: usize, col: usize, layer: usize) -> T {
        self.as_slice::<T>()[self.flat_index(row, col, layer)]
    }

    pub fn set<T: bytemuck::Pod>(&mut self, row: usize, col: usize, layer: usize, value: T) {
        let idx = self.flat_index(row, col, layer);
        self.as_mut_slice::<T>()[idx] = value;
    }
}

/// Borrowed, validation-backed view over a `Tensor` wire payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct TensorView<'a> {
    pub header: TensorHeader,
    pub data: &'a [u8],
}

impl<'a> TensorView<'a> {
    pub fn rows(&self) -> u32 {
        self.header.rows
    }

    pub fn cols(&self) -> u32 {
        self.header.cols
    }

    pub fn layers(&self) -> u32 {
        self.header.layers
    }

    pub fn element_size(&self) -> u32 {
        self.header.element_size
    }

    pub fn payload_bytes(&self) -> &'a [u8] {
        self.data
    }

    pub fn flat_index(&self, row: usize, col: usize, layer: usize) -> usize {
        layer * (self.header.rows as usize * self.header.cols as usize)
            + row * self.header.cols as usize
            + col
    }

    pub fn get_unaligned<T: bytemuck::Pod + Copy>(
        &self,
        row: usize,
        col: usize,
        layer: usize,
    ) -> Result<T, WireError> {
        assert_element_size::<T>(self.header.element_size);
        if row >= self.header.rows as usize
            || col >= self.header.cols as usize
            || layer >= self.header.layers as usize
        {
            return Err(crate::wire::invalid_header::<Tensor>(format!(
                "tensor index out of bounds: ({row}, {col}, {layer}) for {}x{}x{}",
                self.header.rows, self.header.cols, self.header.layers
            )));
        }
        let elem_size = core::mem::size_of::<T>();
        let offset = self
            .flat_index(row, col, layer)
            .checked_mul(elem_size)
            .ok_or_else(|| crate::wire::invalid_payload::<Tensor>("element offset overflowed"))?;
        let end = offset
            .checked_add(elem_size)
            .ok_or_else(|| crate::wire::invalid_payload::<Tensor>("element end overflowed"))?;
        Ok(bytemuck::pod_read_unaligned(&self.data[offset..end]))
    }

    pub fn as_aligned_slice<T: bytemuck::Pod>(&self) -> Result<&'a [T], WireError> {
        assert_element_size::<T>(self.header.element_size);
        bytemuck::try_cast_slice(self.data)
            .map_err(|error| crate::wire::invalid_payload::<Tensor>(error.to_string()))
    }
}

impl DataPodValidate for Tensor {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        if header.rows == 0 || header.cols == 0 || header.layers == 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "rows, cols, and layers must be non-zero",
            ));
        }
        if header.element_size == 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "element_size must be non-zero",
            ));
        }
        let expected = crate::wire::checked_product::<Self>(&[
            header.rows as usize,
            header.cols as usize,
            header.layers as usize,
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

impl DataPodAccess for Tensor {
    type View<'a> = TensorView<'a>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Self::validate_wire_parts(&header, payload)?;
        Ok(TensorView {
            header,
            data: payload,
        })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Self::View<'a> {
        TensorView {
            header,
            data: payload,
        }
    }
}
