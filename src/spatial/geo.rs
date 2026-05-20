use std::f64::consts::PI;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Default, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Geo {
    pub latitude: f64,
    pub longitude: f64,
    pub altitude: f64,
}

impl Geo {
    pub fn new(latitude: f64, longitude: f64, altitude: f64) -> Self {
        Self {
            latitude,
            longitude,
            altitude,
        }
    }

    pub fn is_set(&self) -> bool {
        self.latitude != 0.0 || self.longitude != 0.0 || self.altitude != 0.0
    }

    pub fn has_altitude(&self) -> bool {
        !self.altitude.is_nan()
    }

    pub fn is_valid(&self) -> bool {
        (-90.0..=90.0).contains(&self.latitude) && (-180.0..=180.0).contains(&self.longitude)
    }

    pub fn distance_to(&self, other: Geo) -> f64 {
        const EARTH_RADIUS: f64 = 6_371_000.0;
        let lat1_rad = self.latitude.to_radians();
        let lat2_rad = other.latitude.to_radians();
        let dlat = (other.latitude - self.latitude).to_radians();
        let dlon = (other.longitude - self.longitude).to_radians();

        let a = (dlat / 2.0).sin().powi(2)
            + lat1_rad.cos() * lat2_rad.cos() * (dlon / 2.0).sin().powi(2);
        let c = 2.0 * a.sqrt().atan2((1.0 - a).sqrt());

        EARTH_RADIUS * c
    }

    pub fn bearing_to(&self, other: Geo) -> f64 {
        let lat1_rad = self.latitude.to_radians();
        let lat2_rad = other.latitude.to_radians();
        let dlon = (other.longitude - self.longitude).to_radians();

        let y = dlon.sin() * lat2_rad.cos();
        let x = lat1_rad.cos() * lat2_rad.sin() - lat1_rad.sin() * lat2_rad.cos() * dlon.cos();

        let mut bearing = y.atan2(x);
        if bearing < 0.0 {
            bearing += 2.0 * PI;
        }
        bearing
    }
}

#[allow(dead_code)]
pub mod geo {
    use super::Geo;

    pub fn make(latitude: f64, longitude: f64) -> Geo {
        Geo::new(latitude, longitude, 0.0)
    }

    pub fn make3(latitude: f64, longitude: f64, altitude: f64) -> Geo {
        Geo::new(latitude, longitude, altitude)
    }

    pub fn origin() -> Geo {
        Geo::default()
    }

    pub fn without_altitude(latitude: f64, longitude: f64) -> Geo {
        Geo::new(latitude, longitude, f64::NAN)
    }
}
