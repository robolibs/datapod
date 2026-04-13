#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Size {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

impl Size {
    pub fn new(x: f64, y: f64, z: f64) -> Self {
        Self { x, y, z }
    }
}
