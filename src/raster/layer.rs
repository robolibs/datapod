use crate::geom::Point;
use crate::motion::Pose;
use crate::wire::Encoding;

/// 3-D voxel stack — N stacked 2-D grids sharing pose, resolution, encoding.
/// Owns its data buffer.
#[datapod::datapod]
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
        self.rows as usize * self.cols as usize * self.layers as usize
    }

    pub fn flat_index(&self, row: usize, col: usize, layer: usize) -> usize {
        layer * self.rows as usize * self.cols as usize + row * self.cols as usize + col
    }

    pub fn is_valid(&self, bytes_per_cell: usize) -> bool {
        self.rows > 0
            && self.cols > 0
            && self.layers > 0
            && self.data.len() == self.size() * bytes_per_cell
    }

    pub fn layer_count(&self) -> usize {
        self.layers as usize
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
        let local_point = self.pose.inverse_transform_point(world_point);
        let mut local_x = local_point.x;
        let mut local_y = local_point.y;
        let local_z = local_point.z;
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
        let layer = if self.layer_height > 0.0 {
            ((local_z / self.layer_height) - 0.5)
                .round()
                .clamp(0.0, (self.layers.saturating_sub(1)) as f64) as usize
        } else {
            0
        };
        (row, col, layer)
    }
}
