#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Utm {
    pub zone: i32,
    pub band: char,
    pub easting: f64,
    pub northing: f64,
    pub altitude: f64,
}

impl Default for Utm {
    fn default() -> Self {
        Self {
            zone: 0,
            band: 'N',
            easting: 0.0,
            northing: 0.0,
            altitude: 0.0,
        }
    }
}

impl Utm {
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
        ('C'..='X').contains(&self.band) && self.band != 'I' && self.band != 'O'
    }

    pub fn is_northern(&self) -> bool {
        self.band >= 'N'
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
