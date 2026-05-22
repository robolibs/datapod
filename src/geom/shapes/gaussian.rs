//--- point ---
use crate::geom::Point;

#[datapod::datapod]
#[derive(Default)]
pub struct GaussianPoint {
    pub point: Point,
    pub uncertainty: f64,
}

impl GaussianPoint {
    pub fn new(point: Point, uncertainty: f64) -> Self {
        Self { point, uncertainty }
    }
}

//--- circle ---
use crate::geom::shapes::Circle;

#[datapod::datapod]
#[derive(Default)]
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

//--- rectangle ---
use crate::geom::shapes::Rectangle;

#[datapod::datapod]
#[derive(Default)]
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

//--- box ---
use crate::geom::shapes::Box;

#[datapod::datapod]
#[derive(Default)]
pub struct GaussianBox {
    pub r#box: Box,
    pub uncertainty: f64,
}

impl GaussianBox {
    pub fn new(r#box: Box, uncertainty: f64) -> Self {
        Self { r#box, uncertainty }
    }
}

