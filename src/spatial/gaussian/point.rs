use crate::spatial::Point;

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct GaussianPoint {
    pub point: Point,
    pub uncertainty: f64,
}

impl GaussianPoint {
    pub fn new(point: Point, uncertainty: f64) -> Self {
        Self { point, uncertainty }
    }
}
