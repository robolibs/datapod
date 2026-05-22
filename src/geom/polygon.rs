use super::Point;
use super::shapes::{Aabb, Obb, Size};
use crate::motion::Euler;

/// Single-ring polygon (no holes). Owns its vertex buffer.
///
/// The `vertices` field is the heap-allocated payload that gets cast to
/// bytes when the polygon rides the wire. The macro generates a sibling
/// `PolygonHeader` Pod struct (containing no fields, since `Polygon` has
/// no fixed metadata beyond the vertices). When sending, the count is
/// reconstructed on the receive side from `payload.len() / size_of::<Point>()`.
#[datapod::datapod]
#[derive(Default)]
pub struct Polygon {
    #[dp(bytes)]
    pub vertices: Vec<Point>,
}

impl Polygon {
    pub fn new(vertices: Vec<Point>) -> Self {
        Self { vertices }
    }

    pub fn num_vertices(&self) -> usize {
        self.vertices.len()
    }

    pub fn empty(&self) -> bool {
        self.vertices.is_empty()
    }

    pub fn is_valid(&self) -> bool {
        self.vertices.len() >= 3
    }

    pub fn perimeter(&self) -> f64 {
        if self.vertices.len() < 2 {
            return 0.0;
        }
        let mut p = 0.0;
        for i in 1..self.vertices.len() {
            p += self.vertices[i - 1].distance_to(self.vertices[i]);
        }
        p + self
            .vertices
            .last()
            .unwrap()
            .distance_to(*self.vertices.first().unwrap())
    }

    pub fn area(&self) -> f64 {
        if self.vertices.len() < 3 {
            return 0.0;
        }
        let mut a = 0.0;
        let mut j = self.vertices.len() - 1;
        for i in 0..self.vertices.len() {
            let pi = self.vertices[i];
            let pj = self.vertices[j];
            a += (pj.x + pi.x) * (pj.y - pi.y);
            j = i;
        }
        (a * 0.5).abs()
    }

    pub fn contains(&self, point: Point) -> bool {
        if self.vertices.len() < 3 {
            return false;
        }
        let mut c = false;
        let mut j = self.vertices.len() - 1;
        for i in 0..self.vertices.len() {
            let pi = self.vertices[i];
            let pj = self.vertices[j];
            if ((pi.y > point.y) != (pj.y > point.y))
                && (point.x < (pj.x - pi.x) * (point.y - pi.y) / (pj.y - pi.y) + pi.x)
            {
                c = !c;
            }
            j = i;
        }
        c
    }

    pub fn get_aabb(&self) -> Aabb {
        if self.vertices.is_empty() {
            return Aabb::default();
        }
        let mut min = self.vertices[0];
        let mut max = self.vertices[0];
        for p in self.vertices.iter().skip(1) {
            min.x = min.x.min(p.x);
            min.y = min.y.min(p.y);
            min.z = min.z.min(p.z);
            max.x = max.x.max(p.x);
            max.y = max.y.max(p.y);
            max.z = max.z.max(p.z);
        }
        Aabb::new(min, max)
    }

    pub fn get_obb(&self) -> Obb {
        if self.vertices.is_empty() {
            return Obb::default();
        }
        let (sx, sy) = self
            .vertices
            .iter()
            .fold((0.0, 0.0), |(sx, sy), p| (sx + p.x, sy + p.y));
        let cx = sx / self.vertices.len() as f64;
        let cy = sy / self.vertices.len() as f64;
        let first = self.vertices[0];
        let theta = (cy - first.y).atan2(cx - first.x);
        let c = theta.cos();
        let s = theta.sin();
        let (mut nx, mut my, mut ny, mut mxx) = (
            f64::INFINITY,
            f64::INFINITY,
            f64::NEG_INFINITY,
            f64::NEG_INFINITY,
        );
        for p in &self.vertices {
            let rx = p.x * c + p.y * s;
            let ry = -p.x * s + p.y * c;
            nx = nx.min(rx);
            mxx = mxx.max(rx);
            my = my.min(ry);
            ny = ny.max(ry);
        }
        let w = mxx - nx;
        let h = ny - my;
        let crx = 0.5 * (nx + mxx);
        let cry = 0.5 * (my + ny);
        let center = Point::new(crx * c - cry * s, crx * s + cry * c, 0.0);
        Obb::new(
            center,
            Size::new(w * 0.5, h * 0.5, 0.0),
            Euler::new(0.0, 0.0, theta),
        )
    }

    pub fn iter(&self) -> std::slice::Iter<'_, Point> {
        self.vertices.iter()
    }
}
