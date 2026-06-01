use super::Size;
use crate::geom::Point;
use crate::motion::Pose;

#[datapod::datapod]
#[derive(Default)]
pub struct Box {
    pub pose: Pose,
    pub size: Size,
}

impl Box {
    pub fn center(&self) -> Point {
        self.pose.point
    }

    pub fn volume(&self) -> f64 {
        self.size.x * self.size.y * self.size.z
    }

    pub fn surface_area(&self) -> f64 {
        2.0 * (self.size.x * self.size.y + self.size.y * self.size.z + self.size.z * self.size.x)
    }

    pub fn corners(&self) -> [Point; 8] {
        let hx = self.size.x / 2.0;
        let hy = self.size.y / 2.0;
        let hz = self.size.z / 2.0;
        [
            Point::new(
                self.pose.point.x - hx,
                self.pose.point.y - hy,
                self.pose.point.z - hz,
            ),
            Point::new(
                self.pose.point.x + hx,
                self.pose.point.y - hy,
                self.pose.point.z - hz,
            ),
            Point::new(
                self.pose.point.x + hx,
                self.pose.point.y + hy,
                self.pose.point.z - hz,
            ),
            Point::new(
                self.pose.point.x - hx,
                self.pose.point.y + hy,
                self.pose.point.z - hz,
            ),
            Point::new(
                self.pose.point.x - hx,
                self.pose.point.y - hy,
                self.pose.point.z + hz,
            ),
            Point::new(
                self.pose.point.x + hx,
                self.pose.point.y - hy,
                self.pose.point.z + hz,
            ),
            Point::new(
                self.pose.point.x + hx,
                self.pose.point.y + hy,
                self.pose.point.z + hz,
            ),
            Point::new(
                self.pose.point.x - hx,
                self.pose.point.y + hy,
                self.pose.point.z + hz,
            ),
        ]
    }

    pub fn contains(&self, point: Point) -> bool {
        let hx = self.size.x / 2.0;
        let hy = self.size.y / 2.0;
        let hz = self.size.z / 2.0;
        (point.x - self.pose.point.x).abs() <= hx
            && (point.y - self.pose.point.y).abs() <= hy
            && (point.z - self.pose.point.z).abs() <= hz
    }

    pub fn to_mat(&self) -> [f64; 10] {
        let p = self.pose.to_mat();
        [
            p[0],
            p[1],
            p[2],
            p[3],
            p[4],
            p[5],
            p[6],
            self.size.x,
            self.size.y,
            self.size.z,
        ]
    }

    pub fn from_mat(v: [f64; 10]) -> Self {
        Self {
            pose: Pose::from_mat([v[0], v[1], v[2], v[3], v[4], v[5], v[6]]),
            size: Size::new(v[7], v[8], v[9]),
        }
    }
}
