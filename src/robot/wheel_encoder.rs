/// One wheel's reading: the angle it's turned through and how fast it's
/// turning right now. Radians and radians-per-second, not a tick count —
/// nothing upstream of this counts ticks, so a tick unit would be fake
/// precision over what's actually measured.
#[datapod::datapod]
#[derive(Default)]
pub struct WheelEncoder {
    pub wheel_id: u64,
    pub angle_rad: f64,
    pub velocity_rad_s: f64,
}

impl WheelEncoder {
    pub fn new(wheel_id: u64, angle_rad: f64, velocity_rad_s: f64) -> Self {
        Self {
            wheel_id,
            angle_rad,
            velocity_rad_s,
        }
    }
}

/// One machine's full set of wheel readings. Owns its wheel buffer, so a
/// two-wheel and a six-wheel machine both fit the same wire type.
#[datapod::datapod]
#[derive(Default)]
pub struct WheelEncoders {
    #[dp(bytes)]
    pub wheels: Vec<WheelEncoder>,
}

impl WheelEncoders {
    pub fn new(wheels: Vec<WheelEncoder>) -> Self {
        Self { wheels }
    }

    pub fn num_wheels(&self) -> usize {
        self.wheels.len()
    }
}
