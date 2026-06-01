use crate::geom::Point;

#[datapod::datapod]
#[derive(Default)]
pub struct Triangle {
    pub a: Point,
    pub b: Point,
    pub c: Point,
}

impl Triangle {
    pub fn area(&self) -> f64 {
        let ab = self.b - self.a;
        let ac = self.c - self.a;
        let cross_x = ab.y * ac.z - ab.z * ac.y;
        let cross_y = ab.z * ac.x - ab.x * ac.z;
        let cross_z = ab.x * ac.y - ab.y * ac.x;
        0.5 * (cross_x * cross_x + cross_y * cross_y + cross_z * cross_z).sqrt()
    }

    pub fn perimeter(&self) -> f64 {
        self.a.distance_to(self.b) + self.b.distance_to(self.c) + self.c.distance_to(self.a)
    }

    pub fn contains(&self, point: Point) -> bool {
        let sign = |p1: Point, p2: Point, p3: Point| -> f64 {
            (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y)
        };
        let d1 = sign(point, self.a, self.b);
        let d2 = sign(point, self.b, self.c);
        let d3 = sign(point, self.c, self.a);
        let has_neg = d1 < 0.0 || d2 < 0.0 || d3 < 0.0;
        let has_pos = d1 > 0.0 || d2 > 0.0 || d3 > 0.0;
        !(has_neg && has_pos)
    }

    pub fn to_mat(&self) -> [f64; 9] {
        [
            self.a.x, self.a.y, self.a.z, self.b.x, self.b.y, self.b.z, self.c.x, self.c.y,
            self.c.z,
        ]
    }

    pub fn from_mat(v: [f64; 9]) -> Self {
        Self {
            a: Point::new(v[0], v[1], v[2]),
            b: Point::new(v[3], v[4], v[5]),
            c: Point::new(v[6], v[7], v[8]),
        }
    }
}
