use crate::geom::Point;
use crate::motion::Pose;
use crate::wire::Encoding;
use crate::{DataPodAccess, DataPodValidate, WireError};

/// 2-D occupancy / heightmap / image grid. Owns its data buffer.
///
/// All non-data fields are mirrored into the generated `GridHeader`
/// (which IS Pod and rides on the wire's `user_header` slot). The `data`
/// field is the heap byte buffer that rides as the slice payload.
#[datapod::datapod]
#[dp(manual_access)]
#[derive(Default)]
pub struct Grid {
    pub rows: u32,
    pub cols: u32,
    pub encoding: Encoding,
    /// Non-zero if the grid is centered on its pose origin.
    pub centered: u32,
    pub resolution: f64,
    pub pose: Pose,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

impl Grid {
    pub fn new(
        rows: u32,
        cols: u32,
        encoding: Encoding,
        resolution: f64,
        centered: bool,
        pose: Pose,
        data: Vec<u8>,
    ) -> Self {
        Self::try_new(rows, cols, encoding, resolution, centered, pose, data).unwrap_or_default()
    }

    pub fn try_new(
        rows: u32,
        cols: u32,
        encoding: Encoding,
        resolution: f64,
        centered: bool,
        pose: Pose,
        data: Vec<u8>,
    ) -> Result<Self, WireError> {
        let grid = Self {
            rows,
            cols,
            encoding,
            centered: if centered { 1 } else { 0 },
            resolution,
            pose,
            data,
        };
        grid.validate_owned()?;
        Ok(grid)
    }

    pub fn size(&self) -> usize {
        self.try_size().unwrap_or(0)
    }

    /// Fallible logical cell count for callers handling potentially malformed
    /// owned buffers.
    pub fn try_size(&self) -> Result<usize, WireError> {
        self.validate_owned()?;
        let (rows, cols) = self.dims_usize()?;
        crate::wire::checked_product::<Self>(&[rows, cols])
    }

    pub fn flat_index(&self, row: usize, col: usize) -> usize {
        self.try_flat_index(row, col).unwrap_or(usize::MAX)
    }

    pub fn try_flat_index(&self, row: usize, col: usize) -> Result<usize, WireError> {
        self.validate_owned()?;
        self.checked_flat_index(row, col)
    }

    /// Sanity check: payload length matches dimensions * bytes_per_cell.
    pub fn is_valid(&self, bytes_per_cell: usize) -> bool {
        self.try_is_valid(bytes_per_cell).unwrap_or(false)
    }

    /// Fallible validity check for callers handling potentially malformed
    /// owned buffers. Returns `Ok(false)` only when the owned grid is valid for
    /// its encoding but the caller-provided `bytes_per_cell` disagrees.
    pub fn try_is_valid(&self, bytes_per_cell: usize) -> Result<bool, WireError> {
        self.validate_owned()?;
        if bytes_per_cell != self.encoding.byte_width() {
            return Ok(false);
        }
        Ok(true)
    }

    pub fn get_point(&self, row: usize, col: usize) -> Point {
        let mut local_x = (col as f64 + 0.5) * self.resolution;
        let mut local_y = (row as f64 + 0.5) * self.resolution;
        if self.centered != 0 {
            local_x -= self.cols as f64 * self.resolution * 0.5;
            local_y -= self.rows as f64 * self.resolution * 0.5;
        }
        self.pose.transform_point(Point::new(local_x, local_y, 0.0))
    }

    pub fn world_to_grid(&self, world_point: Point) -> (usize, usize) {
        self.try_world_to_grid(world_point).unwrap_or((0, 0))
    }

    pub fn try_world_to_grid(&self, world_point: Point) -> Result<(usize, usize), WireError> {
        self.validate_owned()?;
        let local_point = self.pose.inverse_transform_point(world_point);
        let mut local_x = local_point.x;
        let mut local_y = local_point.y;
        if self.centered != 0 {
            local_x += self.cols as f64 * self.resolution * 0.5;
            local_y += self.rows as f64 * self.resolution * 0.5;
        }
        let max_col = self
            .cols
            .checked_sub(1)
            .ok_or_else(|| crate::wire::invalid_header::<Self>("grid cols must be non-zero"))?;
        let max_row = self
            .rows
            .checked_sub(1)
            .ok_or_else(|| crate::wire::invalid_header::<Self>("grid rows must be non-zero"))?;
        let col =
            super::checked_world_axis::<Self>(local_x, self.resolution, max_col, "grid column")?;
        let row = super::checked_world_axis::<Self>(local_y, self.resolution, max_row, "grid row")?;
        Ok((row, col))
    }

    fn checked_flat_index(&self, row: usize, col: usize) -> Result<usize, WireError> {
        let (rows, cols) = self.dims_usize()?;
        if row >= rows || col >= cols {
            return Err(crate::wire::invalid_header::<Self>(format!(
                "grid index out of bounds: ({row}, {col}) for {}x{}",
                self.rows, self.cols
            )));
        }
        row.checked_mul(cols)
            .and_then(|base| base.checked_add(col))
            .ok_or_else(|| crate::wire::invalid_payload::<Self>("cell index overflowed"))
    }

    fn dims_usize(&self) -> Result<(usize, usize), WireError> {
        grid_dims::<Self>(self.rows, self.cols)
    }

    fn validate_owned(&self) -> Result<(), WireError> {
        <Self as DataPodValidate>::validate_wire_parts(
            &GridHeader {
                rows: self.rows,
                cols: self.cols,
                encoding: self.encoding,
                centered: self.centered,
                resolution: self.resolution,
                pose: self.pose,
            },
            &self.data,
        )
    }

    pub(crate) fn validate_wire_len(
        header: &GridHeader,
        payload_len: usize,
    ) -> Result<(), WireError> {
        if header.rows == 0 || header.cols == 0 {
            return Err(crate::wire::invalid_header::<Self>(
                "rows and cols must be non-zero",
            ));
        }
        if header.centered > 1 {
            return Err(crate::wire::invalid_header::<Self>(
                "centered must be 0 or 1",
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
        let (rows, cols) = grid_dims::<Self>(header.rows, header.cols)?;
        let expected =
            crate::wire::checked_product::<Self>(&[rows, cols, header.encoding.byte_width()])?;
        if payload_len != expected {
            return Err(crate::wire::invalid_payload::<Self>(format!(
                "got {payload_len} bytes, expected {expected}"
            )));
        }
        Ok(())
    }
}

fn grid_dims<P: 'static>(rows: u32, cols: u32) -> Result<(usize, usize), WireError> {
    Ok((
        u32_to_usize::<P>(rows, "rows")?,
        u32_to_usize::<P>(cols, "cols")?,
    ))
}

fn u32_to_usize<P: 'static>(value: u32, field: &'static str) -> Result<usize, WireError> {
    usize::try_from(value)
        .map_err(|_| crate::wire::invalid_header::<P>(format!("{field} does not fit in usize")))
}

/// Borrowed, validation-backed view over a `Grid` wire payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct GridView<'a> {
    pub header: GridHeader,
    pub data: &'a [u8],
}

impl<'a> GridView<'a> {
    pub fn rows(&self) -> u32 {
        self.header.rows
    }

    pub fn cols(&self) -> u32 {
        self.header.cols
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
        Grid::validate_wire_len(&self.header, self.data.len())?;
        let (rows, cols) = grid_dims::<Grid>(self.header.rows, self.header.cols)?;
        crate::wire::checked_product::<Grid>(&[rows, cols])
    }

    pub fn flat_index(&self, row: usize, col: usize) -> usize {
        self.try_flat_index(row, col).unwrap_or(usize::MAX)
    }

    pub fn try_flat_index(&self, row: usize, col: usize) -> Result<usize, WireError> {
        Grid::validate_wire_len(&self.header, self.data.len())?;
        let (rows, cols) = grid_dims::<Grid>(self.header.rows, self.header.cols)?;
        if row >= rows || col >= cols {
            return Err(crate::wire::invalid_header::<Grid>(format!(
                "grid index out of bounds: ({row}, {col}) for {}x{}",
                self.header.rows, self.header.cols
            )));
        }
        row.checked_mul(cols)
            .and_then(|base| base.checked_add(col))
            .ok_or_else(|| crate::wire::invalid_payload::<Grid>("cell index overflowed"))
    }

    pub fn get_point(&self, row: usize, col: usize) -> Point {
        let mut local_x = (col as f64 + 0.5) * self.header.resolution;
        let mut local_y = (row as f64 + 0.5) * self.header.resolution;
        if self.header.centered != 0 {
            local_x -= self.header.cols as f64 * self.header.resolution * 0.5;
            local_y -= self.header.rows as f64 * self.header.resolution * 0.5;
        }
        self.header
            .pose
            .transform_point(Point::new(local_x, local_y, 0.0))
    }
}

impl DataPodValidate for Grid {
    fn validate_wire_parts(header: &Self::Header, payload: &[u8]) -> Result<(), WireError> {
        Self::validate_wire_len(header, payload.len())
    }
}

impl DataPodAccess for Grid {
    type View<'a> = GridView<'a>;

    fn access_wire_parts<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Result<Self::View<'a>, WireError> {
        Self::validate_wire_parts(&header, payload)?;
        Ok(GridView {
            header,
            data: payload,
        })
    }

    unsafe fn access_wire_parts_unchecked<'a>(
        header: Self::Header,
        payload: &'a [u8],
    ) -> Self::View<'a> {
        GridView {
            header,
            data: payload,
        }
    }
}
