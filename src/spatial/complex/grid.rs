use crate::Vector;
use crate::spatial::{Point, Pose};

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Grid<T = f64> {
    pub rows: usize,
    pub cols: usize,
    pub resolution: f64,
    pub centered: bool,
    pub pose: Pose,
    pub data: Vector<T>,
}

impl<T> Grid<T> {
    pub fn index(&self, row: usize, col: usize) -> usize {
        row * self.cols + col
    }

    pub fn size(&self) -> usize {
        self.rows * self.cols
    }

    pub fn is_valid(&self) -> bool {
        self.rows > 0 && self.cols > 0 && self.data.len() == self.size()
    }

    pub fn get_point(&self, row: usize, col: usize) -> Point {
        let mut local_x = (col as f64 + 0.5) * self.resolution;
        let mut local_y = (row as f64 + 0.5) * self.resolution;
        if self.centered {
            local_x -= self.cols as f64 * self.resolution * 0.5;
            local_y -= self.rows as f64 * self.resolution * 0.5;
        }
        self.pose.transform_point(Point::new(local_x, local_y, 0.0))
    }

    pub fn world_to_grid(&self, world_point: Point) -> (usize, usize) {
        let local_point = self.pose.inverse_transform_point(world_point);
        let mut local_x = local_point.x;
        let mut local_y = local_point.y;
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
        (row, col)
    }
}

impl<T> std::ops::Index<(usize, usize)> for Grid<T> {
    type Output = T;

    fn index(&self, index: (usize, usize)) -> &Self::Output {
        &self.data[self.index(index.0, index.1)]
    }
}

impl<T> std::ops::IndexMut<(usize, usize)> for Grid<T> {
    fn index_mut(&mut self, index: (usize, usize)) -> &mut Self::Output {
        let flat = self.index(index.0, index.1);
        &mut self.data[flat]
    }
}
