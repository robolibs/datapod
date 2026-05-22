use crate::geom::Point;
use crate::motion::Pose;
use crate::wire::Encoding;

/// 2-D occupancy / heightmap / image grid. Owns its data buffer.
///
/// All non-data fields are mirrored into the generated `GridHeader`
/// (which IS Pod and rides on the wire's `user_header` slot). The `data`
/// field is the heap byte buffer that rides as the slice payload.
#[datapod::datapod]
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
