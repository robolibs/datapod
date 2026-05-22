#[datapod::datapod]
pub struct Utm {
    pub zone: i32,
    // `band` is an ASCII UTM grid-zone letter ('C'..='X' excluding 'I'/'O').
    // Stored as `u32` rather than `u8` so the struct has no padding (Pod
    // requires every byte to be initialized). The `char` form is exposed via
    // [`Utm::band_char`] and [`Utm::set_band_char`].
    pub band: u32,
    pub easting: f64,
    pub northing: f64,
    pub altitude: f64,
}

impl Default for Utm {
    fn default() -> Self {
        Self {
            zone: 0,
            band: b'N' as u32,
            easting: 0.0,
            northing: 0.0,
            altitude: 0.0,
        }
    }
}

impl Utm {
    pub fn band_char(&self) -> char {
        char::from_u32(self.band).unwrap_or('?')
    }

    pub fn set_band_char(&mut self, c: char) {
        self.band = c as u32;
    }

    pub fn is_set(&self) -> bool {
        self.zone != 0 || self.easting != 0.0 || self.northing != 0.0
    }

    pub fn has_altitude(&self) -> bool {
        !self.altitude.is_nan()
    }

    pub fn is_valid_zone(&self) -> bool {
        (1..=60).contains(&self.zone)
    }

    pub fn is_valid_band(&self) -> bool {
        let b = self.band;
        (b'C' as u32..=b'X' as u32).contains(&b) && b != b'I' as u32 && b != b'O' as u32
    }

    pub fn is_northern(&self) -> bool {
        self.band >= b'N' as u32
    }

    pub fn is_valid(&self) -> bool {
        self.is_valid_zone()
            && self.is_valid_band()
            && self.easting >= 100000.0
            && self.easting <= 900000.0
            && self.northing >= 0.0
            && self.northing <= 10000000.0
    }

    pub fn distance_to(&self, other: Utm) -> f64 {
        let de = self.easting - other.easting;
        let dn = self.northing - other.northing;
        (de * de + dn * dn).sqrt()
    }

    pub fn distance_to_3d(&self, other: Utm) -> f64 {
        let de = self.easting - other.easting;
        let dn = self.northing - other.northing;
        let da = self.altitude - other.altitude;
        (de * de + dn * dn + da * da).sqrt()
    }

    pub fn same_zone(&self, other: Utm) -> bool {
        self.zone == other.zone && self.band == other.band
    }

    pub fn central_meridian(&self) -> f64 {
        (self.zone - 1) as f64 * 6.0 - 180.0 + 3.0
    }
}
