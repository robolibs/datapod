use crate::Vector;
use crate::spatial::{Grid, Point, Pose};

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Layer<T = f64> {
    pub rows: usize,
    pub cols: usize,
    pub layers: usize,
    pub resolution: f64,
    pub layer_height: f64,
    pub centered: bool,
    pub pose: Pose,
    pub data: Vector<T>,
}

impl<T: Clone> Layer<T> {
    pub fn extract_grid(&self, layer_idx: usize) -> Grid<T> {
        assert!(layer_idx < self.layers, "layer index out of bounds");
        let mut grid = Grid {
            rows: self.rows,
            cols: self.cols,
            resolution: self.resolution,
            centered: self.centered,
            pose: self.pose,
            data: Vector::from_elem(
                self.rows * self.cols,
                self.data[self.index(0, 0, layer_idx)].clone(),
            ),
        };
        let z_offset = (layer_idx as f64 + 0.5) * self.layer_height;
        let layer_offset = Point::new(0.0, 0.0, z_offset);
        let world_offset = self.pose.transform_point(layer_offset) - self.pose.point;
        grid.pose = Pose {
            point: Point::new(
                self.pose.point.x + world_offset.x,
                self.pose.point.y + world_offset.y,
                self.pose.point.z + world_offset.z,
            ),
            rotation: self.pose.rotation,
        };
        for i in 0..self.rows * self.cols {
            grid.data[i] = self.data[layer_idx * self.rows * self.cols + i].clone();
        }
        grid
    }

    pub fn set_grid(&mut self, layer_idx: usize, grid: &Grid<T>) {
        assert!(layer_idx < self.layers, "layer index out of bounds");
        assert!(
            grid.rows == self.rows && grid.cols == self.cols,
            "grid shape mismatch"
        );
        let start = layer_idx * self.rows * self.cols;
        for i in 0..self.rows * self.cols {
            self.data[start + i] = grid.data[i].clone();
        }
    }
}

impl<T> Layer<T> {
    pub fn index(&self, row: usize, col: usize, layer: usize) -> usize {
        layer * self.rows * self.cols + row * self.cols + col
    }

    pub fn size(&self) -> usize {
        self.rows * self.cols * self.layers
    }

    pub fn is_valid(&self) -> bool {
        self.rows > 0 && self.cols > 0 && self.layers > 0 && self.data.len() == self.size()
    }

    pub fn get_point(&self, row: usize, col: usize, layer: usize) -> Point {
        let mut local_x = (col as f64 + 0.5) * self.resolution;
        let mut local_y = (row as f64 + 0.5) * self.resolution;
        let local_z = (layer as f64 + 0.5) * self.layer_height;
        if self.centered {
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
        if self.centered {
            local_x += self.cols as f64 * self.resolution * 0.5;
            local_y += self.rows as f64 * self.resolution * 0.5;
        }
        let col = ((local_x / self.resolution) - 0.5)
            .round()
            .clamp(0.0, self.cols.saturating_sub(1) as f64) as usize;
        let row = ((local_y / self.resolution) - 0.5)
            .round()
            .clamp(0.0, self.rows.saturating_sub(1) as f64) as usize;
        let layer = if self.layer_height > 0.0 {
            ((local_z / self.layer_height) - 0.5)
                .round()
                .clamp(0.0, self.layers.saturating_sub(1) as f64) as usize
        } else {
            0
        };
        (row, col, layer)
    }

    pub fn layer_count(&self) -> usize {
        self.layers
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
}

impl<T> std::ops::Index<(usize, usize, usize)> for Layer<T> {
    type Output = T;

    fn index(&self, index: (usize, usize, usize)) -> &Self::Output {
        &self.data[self.index(index.0, index.1, index.2)]
    }
}

impl<T> std::ops::IndexMut<(usize, usize, usize)> for Layer<T> {
    fn index_mut(&mut self, index: (usize, usize, usize)) -> &mut Self::Output {
        let flat = self.index(index.0, index.1, index.2);
        &mut self.data[flat]
    }
}
