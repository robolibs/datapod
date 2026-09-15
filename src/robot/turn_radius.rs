/// A machine's current turn radius, in meters. Positive and negative mean
/// opposite steer directions; a straight line is represented as `f64::INFINITY`
/// rather than a very large number, so "driving straight" is exact rather than
/// a threshold check.
#[datapod::datapod]
pub struct TurnRadius {
    pub radius_m: f64,
}

impl Default for TurnRadius {
    fn default() -> Self {
        Self::straight()
    }
}

impl TurnRadius {
    pub fn new(radius_m: f64) -> Self {
        Self { radius_m }
    }

    pub fn straight() -> Self {
        Self {
            radius_m: f64::INFINITY,
        }
    }

    pub fn is_straight(&self) -> bool {
        !self.radius_m.is_finite()
    }
}
