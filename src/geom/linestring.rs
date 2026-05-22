use super::Point;

/// Polyline — an ordered chain of points. Owns its vertex buffer.
#[datapod::datapod]
#[derive(Default)]
pub struct Linestring {
    #[dp(bytes)]
    pub points: Vec<Point>,
}

impl Linestring {
    pub fn new(points: Vec<Point>) -> Self {
        Self { points }
    }

    pub fn num_points(&self) -> usize {
        self.points.len()
    }

    pub fn empty(&self) -> bool {
        self.points.is_empty()
    }

    pub fn length(&self) -> f64 {
        self.points
            .windows(2)
            .map(|pair| pair[0].distance_to(pair[1]))
            .sum()
    }
}
