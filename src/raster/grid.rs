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
        Self {
            rows,
            cols,
            encoding,
            centered: if centered { 1 } else { 0 },
            resolution,
            pose,
            data,
        }
    }

    pub fn size(&self) -> usize {
        self.rows as usize * self.cols as usize
    }

    pub fn flat_index(&self, row: usize, col: usize) -> usize {
        row * self.cols as usize + col
    }

    /// Sanity check: payload length matches dimensions * bytes_per_cell.
    pub fn is_valid(&self, bytes_per_cell: usize) -> bool {
        self.rows > 0 && self.cols > 0 && self.data.len() == self.size() * bytes_per_cell
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
        let local_point = self.pose.inverse_transform_point(world_point);
        let mut local_x = local_point.x;
        let mut local_y = local_point.y;
        if self.centered != 0 {
            local_x += self.cols as f64 * self.resolution * 0.5;
            local_y += self.rows as f64 * self.resolution * 0.5;
        }
        let col = ((local_x / self.resolution) - 0.5)
            .round()
            .clamp(0.0, (self.cols.saturating_sub(1)) as f64) as usize;
        let row = ((local_y / self.resolution) - 0.5)
            .round()
            .clamp(0.0, (self.rows.saturating_sub(1)) as f64) as usize;
        (row, col)
    }
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
        self.header.rows as usize * self.header.cols as usize
    }

    pub fn flat_index(&self, row: usize, col: usize) -> usize {
        row * self.header.cols as usize + col
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
        if !header.resolution.is_finite() || header.resolution <= 0.0 {
            return Err(crate::wire::invalid_header::<Self>(
                "resolution must be finite and positive",
            ));
        }
        let expected = crate::wire::checked_product::<Self>(&[
            header.rows as usize,
            header.cols as usize,
            header.encoding.byte_width(),
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
