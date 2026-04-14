use crate::spatial::Rectangle;

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct GaussianRectangle {
    pub rectangle: Rectangle,
    pub uncertainty: f64,
}

impl GaussianRectangle {
    pub fn new(rectangle: Rectangle, uncertainty: f64) -> Self {
        Self {
            rectangle,
            uncertainty,
        }
    }
}
