use crate::spatial::Circle;

#[derive(Debug, Clone, Copy, PartialEq, Default)]
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
