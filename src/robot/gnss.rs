use crate::world::Geo;

/// A machine's GNSS fix: where it is on the planet and which way it's
/// facing, compass-style (0 = true north, positive = clockwise) rather
/// than the math-style heading used by `Odom`/`Imu`.
#[datapod::datapod]
#[derive(Default)]
pub struct Gnss {
    pub fix: Geo,
    pub heading_rad: f64,
}

impl Gnss {
    pub fn new(fix: Geo, heading_rad: f64) -> Self {
        Self { fix, heading_rad }
    }

    pub fn is_set(&self) -> bool {
        self.fix.is_set()
    }
}
