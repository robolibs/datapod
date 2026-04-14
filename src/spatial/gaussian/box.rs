use crate::spatial::Box;

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct GaussianBox {
    pub r#box: Box,
    pub uncertainty: f64,
}

impl GaussianBox {
    pub fn new(r#box: Box, uncertainty: f64) -> Self {
        Self { r#box, uncertainty }
    }
}
