use crate::geom::Point;
use crate::motion::Pose;
use crate::wire::Encoding;
use crate::{DataPodAccess, DataPodValidate, WireError};

/// 3-D voxel stack — N stacked 2-D grids sharing pose, resolution, encoding.
/// Owns its data buffer.
#[datapod::datapod]
#[dp(manual_access)]
#[derive(Default)]
pub struct Layer {
    pub rows: u32,
    pub cols: u32,
    pub layers: u32,
    pub encoding: Encoding,
    /// Non-zero if the layer is centered on its pose origin.
    pub centered: u32,
    pub _pad: u32,
    pub resolution: f64,
    pub layer_height: f64,
    pub pose: Pose,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Layer {
    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    /// Fallible logical voxel count for callers handling potentially
    /// malformed owned buffers.
    pub fn try_size(&self) -> Result<usize, WireError> {
        self.validate_owned()?;
        let (rows, cols, layers) = self.dims_usize()?;
        crate::wire::checked_product::<Self>(&[rows, cols, layers])
    }

    pub fn flat_index(&self, row: usize, col: usize, layer: usize) -> usize {
        self.try_flat_index(row, col, layer).unwrap_or(usize::MAX)
    }

    pub fn try_flat_index(&self, row: usize, col: usize, layer: usize) -> Result<usize, WireError> {
        self.validate_owned()?;
        self.checked_flat_index(row, col, layer)
    }

    pub fn is_valid(&self, bytes_per_cell: usize) -> bool {
        self.try_is_valid(bytes_per_cell).unwrap_or(false)
    }

    /// Fallible validity check for callers handling potentially malformed
    /// owned buffers. Returns `Ok(false)` only when the owned layer is valid
    /// for its encoding but the caller-provided `bytes_per_cell` disagrees.
    pub fn try_is_valid(&self, bytes_per_cell: usize) -> Result<bool, WireError> {
        self.validate_owned()?;
        if bytes_per_cell != self.encoding.byte_width() {
            return Ok(false);
        }
        Ok(true)
    }

    pub fn layer_count(&self) -> usize {
        self.try_layer_count().unwrap_or(0)
    }

    pub fn try_layer_count(&self) -> Result<usize, WireError> {
        self.validate_owned()?;
        u32_to_usize::<Self>(self.layers, "layers")
    }

    pub fn get_layer_height(&self) -> f64 {
        self.layer_height
    }

    pub fn get_resolution(&self) -> f64 {
        self.resolution
    }

    pub fn shift(&self) -> &Pose {
        &self.pose
    }

    pub fn get_point(&self, row: usize, col: usize, layer: usize) -> Point {
        let mut local_x = (col as f64 + 0.5) * self.resolution;
        let mut local_y = (row as f64 + 0.5) * self.resolution;
        let local_z = (layer as f64 + 0.5) * self.layer_height;
        if self.centered != 0 {
            local_x -= self.cols as f64 * self.resolution * 0.5;
            local_y -= self.rows as f64 * self.resolution * 0.5;
        }
        self.pose
            .transform_point(Point::new(local_x, local_y, local_z))
    }

    pub fn world_to_voxel(&self, world_point: Point) -> (usize, usize, usize) {
        self.try_world_to_voxel(world_point).unwrap_or((0, 0, 0))
    }

    pub fn try_world_to_voxel(
        &self,
        world_point: Point,
    ) -> Result<(usize, usize, usize), WireError> {
        self.validate_owned()?;
        let local_point = self.pose.inverse_transform_point(world_point);
        let mut local_x = local_point.x;
        let mut local_y = local_point.y;
        let local_z = local_point.z;
        if self.centered != 0 {
            local_x += self.cols as f64 * self.resolution * 0.5;
            local_y += self.rows as f64 * self.resolution * 0.5;
        }
        let max_col = self
            .cols
            .checked_sub(1)
            .ok_or_else(|| crate::wire::invalid_header::<Self>("layer cols must be non-zero"))?;
        let max_row = self
            .rows
            .checked_sub(1)
            .ok_or_else(|| crate::wire::invalid_header::<Self>("layer rows must be non-zero"))?;
        let max_layer = self
            .layers
            .checked_sub(1)
            .ok_or_else(|| crate::wire::invalid_header::<Self>("layers must be non-zero"))?;
        let col =
            super::checked_world_axis::<Self>(local_x, self.resolution, max_col, "layer column")?;
        let row =
            super::checked_world_axis::<Self>(local_y, self.resolution, max_row, "layer row")?;
        let layer = super::checked_world_axis::<Self>(
            local_z,
            self.layer_height,
            max_layer,
            "layer index",
        )?;
        Ok((row, col, layer))
    }

    fn checked_flat_index(&self, row: usize, col: usize, layer: usize) -> Result<usize, WireError> {
        let (rows, cols, layers) = self.dims_usize()?;
        if row >= rows || col >= cols || layer >= layers {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "layer index out of bounds: ({row}, {col}, {layer}) for {}x{}x{}",
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
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("voxel index overflowed"))
    }

    fn dims_usize(&self) -> Result<(usize, usize, usize), WireError> {
        layer_dims::<Self>(self.rows, self.cols, self.layers)
    }

    fn validate_owned(&self) -> Result<(), WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &LayerHeader {
                rows: self.rows,
                cols: self.cols,
                layers: self.layers,
                encoding: self.encoding,
                centered: self.centered,
                _pad: self._pad,
                resolution: self.resolution,
                layer_height: self.layer_height,
                pose: self.pose,
            },
            &self.data,
        )
    }

    pub(crate) fn validate_wire_len(
        header: &LayerHeader,
        payload_len: usize,
    ) -> Result<(), WireError> {
        if header.rows == 0 || header.cols == 0 || header.layers == 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "rows, cols, and layers must be non-zero",
            ));
        }
        if header.centered > 1 {
            return Err(crate::wire::invalid_header::<Self>(
                "centered must be 0 or 1",
            ));
        }
        if header._pad != 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "reserved _pad field must be zero",
            ));
        }
        if !header.encoding.is_valid() {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "unknown encoding tag {}",
                header.encoding.0
            )));
        }
        if !header.resolution.is_finite() || header.resolution <= 0.0 {
            return Err(crate::wire::invalid_header::<Self>(
                "resolution must be finite and positive",
            ));
        }
        if !header.layer_height.is_finite() || header.layer_height <= 0.0 {
            return Err(crate::wire::invalid_header::<Self>(
                "layer_height must be finite and positive",
            ));
        }
        let (rows, cols, layers) = layer_dims::<Self>(header.rows, header.cols, header.layers)?;
        let expected = crate::wire::checked_product::<Self>(&[
            rows,
            cols,
            layers,
            header.encoding.byte_width(),
        ])?;
        if payload_len != expected {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "got {payload_len} bytes, expected {expected}"
            )));
        }
        Ok(())
    }
}

fn layer_dims<P: 'static>(
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

/// Borrowed, validation-backed view over a `Layer` wire payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct LayerView<'a> {
    pub header: LayerHeader,
    pub data: &'a [u8],
}

impl<'a> LayerView<'a> {
    pub fn rows(&self) -> u32 {
        self.header.rows
    }

    pub fn cols(&self) -> u32 {
        self.header.cols
    }

    pub fn layers(&self) -> u32 {
        self.header.layers
    }

    pub fn encoding(&self) -> Encoding {
        self.header.encoding
    }

    pub fn payload_bytes(&self) -> &'a [u8] {
        self.data
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    pub fn try_size(&self) -> Result<usize, WireError> {
        Layer::validate_wire_len(&self.header, self.data.len())?;
        let (rows, cols, layers) =
            layer_dims::<Layer>(self.header.rows, self.header.cols, self.header.layers)?;
        crate::wire::checked_product::<Layer>(&[rows, cols, layers])
    }

    pub fn flat_index(&self, row: usize, col: usize, layer: usize) -> usize {
        self.try_flat_index(row, col, layer).unwrap_or(usize::MAX)
    }

    pub fn try_flat_index(&self, row: usize, col: usize, layer: usize) -> Result<usize, WireError> {
        Layer::validate_wire_len(&self.header, self.data.len())?;
        let (rows, cols, layers) =
            layer_dims::<Layer>(self.header.rows, self.header.cols, self.header.layers)?;
        if row >= rows || col >= cols || layer >= layers {
            return Err(crate::wire::invalid_header::<Layer>(format!(
                "layer index out of bounds: ({row}, {col}, {layer}) for {}x{}x{}",
                self.header.rows, self.header.cols, self.header.layers
            )));
        }
        let plane = crate::wire::checked_product::<Layer>(&[rows, cols])?;
        layer
            .checked_mul(plane)
            .and_then(|base| {
                row.checked_mul(cols)
                    .and_then(|row_base| base.checked_add(row_base))
            })
            .and_then(|base| base.checked_add(col))
            .ok_or_else(|| crate::wire::invalid_payload::<Layer>("voxel index overflowed"))
    }
}

impl DataPodValidate for Layer {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        Self::validate_wire_len(header, payload.len())
    }
}

impl DataPodAccess for Layer {
    type View<'a> = LayerView<'a>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Self::validate_wire_parts(&header, payload)?;
        Ok(LayerView {
            header,
            data: payload,
        })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Self::View<'a> {
        LayerView {
            header,
            data: payload,
        }
    }
}
