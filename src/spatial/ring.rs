use crate::Vector;

use super::Point;

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Ring {
    pub points: Vector<Point>,
}

impl Ring {
    pub fn length(&self) -> f64 {
        self.points
            .windows(2)
            .map(|segment| segment[0].distance_to(segment[1]))
            .sum()
    }

    pub fn area(&self) -> f64 {
        if self.points.len() < 3 {
            return 0.0;
        }
        let mut sum = 0.0;
        for i in 0..self.points.len() - 1 {
            sum +=
                self.points[i].x * self.points[i + 1].y - self.points[i + 1].x * self.points[i].y;
        }
        sum.abs() * 0.5
    }

    pub fn num_points(&self) -> usize {
        self.points.len()
    }

    pub fn is_empty(&self) -> bool {
        self.points.is_empty()
    }

    pub fn is_closed(&self) -> bool {
        self.points.len() >= 3 && self.points.front() == self.points.back()
    }
}
