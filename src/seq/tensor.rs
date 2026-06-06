//! Dense 3-D tensor Pod — holds any `T: bytemuck::Pod`.

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
        Self::try_new::<T>(rows, cols, layers).unwrap_or_default()
    }

    pub fn try_new<T: bytemuck::Pod>(rows: u32, cols: u32, layers: u32) -> Result<Self, WireError> {
        let es = std::mem::size_of::<T>();
        let element_size = super::checked_pod_element_size::<Self, T>()?;
        let (rows_usize, cols_usize, layers_usize) = tensor_dims::<Self>(rows, cols, layers)?;
        let bytes =
            crate::wire::checked_product::<Self>(&[rows_usize, cols_usize, layers_usize, es])?;
        let mut data = Vec::new();
        data.try_reserve_exact(bytes).map_err(|err| {
            crate::wire::invalid_payload::<Self>(format!("tensor payload allocation failed: {err}"))
        })?;
        data.resize(bytes, 0);
        let tensor = Self {
            rows,
            cols,
            layers,
            element_size,
            data,
        };
        tensor.validate_owned()?;
        Ok(tensor)
    }

    pub fn from_bytes<T: bytemuck::Pod>(rows: u32, cols: u32, layers: u32, data: Vec<u8>) -> Self {
        Self::try_from_bytes::<T>(rows, cols, layers, data).unwrap_or_default()
    }

    pub fn try_from_bytes<T: bytemuck::Pod>(
        rows: u32,
        cols: u32,
        layers: u32,
        data: Vec<u8>,
    ) -> Result<Self, WireError> {
        let tensor = Self {
            rows,
            cols,
            layers,
            element_size: super::checked_pod_element_size::<Self, T>()?,
            data,
        };
        tensor.validate_owned()?;
        Ok(tensor)
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    /// Fallible logical element count for callers handling potentially
    /// malformed owned buffers.
    pub fn try_size(&self) -> Result<usize, WireError> {
        self.validate_owned()?;
        let (rows, cols, layers) = self.dims_usize()?;
        crate::wire::checked_product::<Self>(&[rows, cols, layers])
    }

    pub fn is_empty(&self) -> bool {
        self.try_is_empty().unwrap_or(true)
    }

    /// Fallible emptiness check for callers handling potentially malformed
    /// owned buffers.
    pub fn try_is_empty(&self) -> Result<bool, WireError> {
        self.validate_owned()?;
        Ok(self.rows == 0 || self.cols == 0 || self.layers == 0)
    }

    pub fn flat_index(&self, row: usize, col: usize, layer: usize) -> usize {
        self.try_flat_index(row, col, layer).unwrap_or(usize::MAX)
    }

    pub fn try_flat_index(&self, row: usize, col: usize, layer: usize) -> Result<usize, WireError> {
        self.validate_owned()?;
        self.checked_flat_index(row, col, layer)
    }

    pub fn is_valid(&self) -> bool {
        self.validate_owned().is_ok()
    }

    pub fn as_slice<T: bytemuck::Pod>(&self) -> &[T] {
        self.try_as_slice::<T>().unwrap_or(&[])
    }

    pub fn try_as_slice<T: bytemuck::Pod>(&self) -> Result<&[T], WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        bytemuck::try_cast_slice(&self.data)
            .map_err(|error| crate::wire::invalid_payload::<Self>(error.to_string()))
    }

    pub fn as_mut_slice<T: bytemuck::Pod>(&mut self) -> &mut [T] {
        self.try_as_mut_slice::<T>().unwrap_or(&mut [])
    }

    pub fn try_as_mut_slice<T: bytemuck::Pod>(&mut self) -> Result<&mut [T], WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        bytemuck::try_cast_slice_mut(&mut self.data)
            .map_err(|error| crate::wire::invalid_payload::<Self>(error.to_string()))
    }

    pub fn layer<T: bytemuck::Pod>(&self, layer: usize) -> &[T] {
        self.try_layer(layer).unwrap_or(&[])
    }

    pub fn try_layer<T: bytemuck::Pod>(&self, layer: usize) -> Result<&[T], WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        let (rows, cols, layers) = self.dims_usize()?;
        if layer >= layers {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "tensor layer out of bounds: {layer} for {} layers",
                self.layers
            )));
        }
        let stride = crate::wire::checked_product::<Self>(&[rows, cols])?;
        let start = layer
            .checked_mul(stride)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("layer offset overflowed"))?;
        let end = start
            .checked_add(stride)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("layer end overflowed"))?;
        let slice = self.try_as_slice::<T>()?;
        slice
            .get(start..end)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("layer range is out of bounds"))
    }

    pub fn get<T: bytemuck::Pod>(&self, row: usize, col: usize, layer: usize) -> T {
        self.try_get(row, col, layer)
            .unwrap_or_else(|_| bytemuck::Zeroable::zeroed())
    }

    pub fn try_get<T: bytemuck::Pod>(
        &self,
        row: usize,
        col: usize,
        layer: usize,
    ) -> Result<T, WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        let range = self.element_byte_range::<T>(row, col, layer)?;
        let bytes = self.data.get(range).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("element range is out of bounds")
        })?;
        Ok(bytemuck::pod_read_unaligned(bytes))
    }

    pub fn set<T: bytemuck::Pod>(&mut self, row: usize, col: usize, layer: usize, value: T) {
        let _ = self.try_set(row, col, layer, value);
    }

    pub fn try_set<T: bytemuck::Pod>(
        &mut self,
        row: usize,
        col: usize,
        layer: usize,
        value: T,
    ) -> Result<(), WireError> {
        check_element_size::<Self, T>(self.element_size)?;
        self.validate_owned()?;
        let range = self.element_byte_range::<T>(row, col, layer)?;
        let slot = self.data.get_mut(range).ok_or_else(|| {
            crate::wire::invalid_payload::<Self>("element range is out of bounds")
        })?;
        slot.copy_from_slice(bytemuck::bytes_of(&value));
        Ok(())
    }

    fn element_byte_range<T: bytemuck::Pod>(
        &self,
        row: usize,
        col: usize,
        layer: usize,
    ) -> Result<std::ops::Range<usize>, WireError> {
        let index = self.checked_flat_index(row, col, layer)?;
        let elem_size = core::mem::size_of::<T>();
        let start = index
            .checked_mul(elem_size)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("element offset overflowed"))?;
        let end = start
            .checked_add(elem_size)
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("element end overflowed"))?;
        Ok(start..end)
    }

    fn checked_flat_index(&self, row: usize, col: usize, layer: usize) -> Result<usize, WireError> {
        let (rows, cols, layers) = self.dims_usize()?;
        if row >= rows || col >= cols || layer >= layers {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "tensor index out of bounds: ({row}, {col}, {layer}) for {}x{}x{}",
                self.rows, self.cols, self.layers
            )));
        }
        let plane = crate::wire::checked_product::<Self>(&[rows, cols])?;
        layer
            .checked_mul(plane)
            .and_then(|base| {
                row.checked_mul(cols)
                    .and_then(|row_base| base.checked_add(row_base))
            })
            .and_then(|base| base.checked_add(col))
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("element index overflowed"))
    }

    fn dims_usize(&self) -> Result<(usize, usize, usize), WireError> {
        tensor_dims::<Self>(self.rows, self.cols, self.layers)
    }

    fn validate_owned(&self) -> Result<(), WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &TensorHeader {
                rows: self.rows,
                cols: self.cols,
                layers: self.layers,
                element_size: self.element_size,
            },
            &self.data,
        )
    }

    pub(crate) fn validate_wire_len(
        header: &TensorHeader,
        payload_len: usize,
    ) -> Result<(), WireError> {
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
        let (rows, cols, layers) = tensor_dims::<Self>(header.rows, header.cols, header.layers)?;
        let element_size = u32_to_usize::<Self>(header.element_size, "element_size")?;
        let expected = crate::wire::checked_product::<Self>(&[rows, cols, layers, element_size])?;
        if payload_len != expected {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "got {payload_len} bytes, expected {expected}"
            )));
        }
        Ok(())
    }
}

/// Borrowed, validation-backed view over a `Tensor` wire payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct TensorView<'a> {
    pub header: TensorHeader,
    pub data: &'a [u8],
}

fn check_element_size<P: 'static, T>(stored: u32) -> Result<(), WireError> {
    let actual = std::mem::size_of::<T>();
    if actual == 0 {
        return Err(crate::wire::invalid_header::<P>(
            "zero-sized Pod elements cannot be represented in byte-counted datapod containers",
        ));
    }
    let stored = u32_to_usize::<P>(stored, "element_size")?;
    if actual != stored {
        return Err(crate::wire::invalid_header::<P>(format!(
            "element size mismatch: T is {actual}, container expects {stored}"
        )));
    }
    Ok(())
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
        self.try_flat_index(row, col, layer).unwrap_or(usize::MAX)
    }

    pub fn try_size(&self) -> Result<usize, WireError> {
        Tensor::validate_wire_len(&self.header, self.data.len())?;
        let (rows, cols, layers) =
            tensor_dims::<Tensor>(self.header.rows, self.header.cols, self.header.layers)?;
        crate::wire::checked_product::<Tensor>(&[rows, cols, layers])
    }

    pub fn try_flat_index(&self, row: usize, col: usize, layer: usize) -> Result<usize, WireError> {
        Tensor::validate_wire_len(&self.header, self.data.len())?;
        let (rows, cols, layers) =
            tensor_dims::<Tensor>(self.header.rows, self.header.cols, self.header.layers)?;
        if row >= rows || col >= cols || layer >= layers {
            return Err(crate::wire::invalid_header::<Tensor>(format!(
                "tensor index out of bounds: ({row}, {col}, {layer}) for {}x{}x{}",
                self.header.rows, self.header.cols, self.header.layers
            )));
        }
        let plane = crate::wire::checked_product::<Tensor>(&[rows, cols])?;
        layer
            .checked_mul(plane)
            .and_then(|base| {
                row.checked_mul(cols)
                    .and_then(|row_base| base.checked_add(row_base))
            })
            .and_then(|base| base.checked_add(col))
            .ok_or_else(|| crate::wire::invalid_payload::<Tensor>("element index overflowed"))
    }

    pub fn get_unaligned<T: bytemuck::Pod + Copy>(
        &self,
        row: usize,
        col: usize,
        layer: usize,
    ) -> Result<T, WireError> {
        check_element_size::<Tensor, T>(self.header.element_size)?;
        Tensor::validate_wire_len(&self.header, self.data.len())?;
        let elem_size = core::mem::size_of::<T>();
        let offset = self
            .try_flat_index(row, col, layer)?
            .checked_mul(elem_size)
            .ok_or_else(|| crate::wire::invalid_payload::<Tensor>("element offset overflowed"))?;
        let end = offset
            .checked_add(elem_size)
            .ok_or_else(|| crate::wire::invalid_payload::<Tensor>("element end overflowed"))?;
        let bytes = self.data.get(offset..end).ok_or_else(|| {
            crate::wire::invalid_payload::<Tensor>("element range is out of bounds")
        })?;
        Ok(bytemuck::pod_read_unaligned(bytes))
    }

    pub fn as_aligned_slice<T: bytemuck::Pod>(&self) -> Result<&'a [T], WireError> {
        check_element_size::<Tensor, T>(self.header.element_size)?;
        Tensor::validate_wire_len(&self.header, self.data.len())?;
        bytemuck::try_cast_slice(self.data)
            .map_err(|error| crate::wire::invalid_payload::<Tensor>(error.to_string()))
    }
}

fn tensor_dims<P: 'static>(
    rows: u32,
    cols: u32,
    layers: u32,
) -> Result<(usize, usize, usize), WireError> {
    Ok((
        u32_to_usize::<P>(rows, "rows")?,
        u32_to_usize::<P>(cols, "cols")?,
        u32_to_usize::<P>(layers, "layers")?,
    ))
}

fn u32_to_usize<P: 'static>(value: u32, field: &'static str) -> Result<usize, WireError> {
    usize::try_from(value)
        .map_err(|_| crate::wire::invalid_header::<P>(format!("{field} does not fit in usize")))
}

impl DataPodValidate for Tensor {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        Self::validate_wire_len(header, payload.len())
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
