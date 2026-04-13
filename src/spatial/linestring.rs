use crate::Vector;

use super::Point;

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Linestring {
    pub points: Vector<Point>,
}

impl Linestring {
    pub fn length(&self) -> f64 {
        self.points
            .windows(2)
            .map(|pair| pair[0].distance_to(pair[1]))
            .sum()
    }

    pub fn num_points(&self) -> usize {
        self.points.len()
    }

    pub fn empty(&self) -> bool {
        self.points.is_empty()
    }
}
