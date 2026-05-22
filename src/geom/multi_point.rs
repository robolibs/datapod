use crate::geom::Point;

/// Unordered set / bag of points. Owns the buffer.
#[datapod::datapod]
#[derive(Default)]
pub struct MultiPoint {
    #[dp(bytes)]
    pub points: Vec<Point>,
}

impl MultiPoint {
    pub fn new(points: Vec<Point>) -> Self {
        Self { points }
    }

    pub fn num_points(&self) -> usize {
        self.points.len()
    }

    pub fn is_empty(&self) -> bool {
        self.points.is_empty()
    }

    /// Bounding box helper. Returns `None` for an empty set.
    pub fn bbox(&self) -> Option<(Point, Point)> {
        let first = *self.points.first()?;
        let mut min = first;
        let mut max = first;
        for p in &self.points[1..] {
            min.x = min.x.min(p.x);
            min.y = min.y.min(p.y);
            min.z = min.z.min(p.z);
            max.x = max.x.max(p.x);
            max.y = max.y.max(p.y);
            max.z = max.z.max(p.z);
        }
        Some((min, max))
    }
}
