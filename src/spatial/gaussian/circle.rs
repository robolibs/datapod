use crate::spatial::Circle;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct GaussianCircle {
    pub circle: Circle,
    pub uncertainty: f64,
}

impl GaussianCircle {
    pub fn new(circle: Circle, uncertainty: f64) -> Self {
        Self {
            circle,
            uncertainty,
        }
    }
}
