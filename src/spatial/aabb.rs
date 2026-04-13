use super::Point;

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Aabb {
    pub min_point: Point,
    pub max_point: Point,
}

impl Aabb {
    pub fn new(min_point: Point, max_point: Point) -> Self {
        Self {
            min_point,
            max_point,
        }
    }
}
