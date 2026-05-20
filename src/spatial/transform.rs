use super::{Point, Quaternion};

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Transform {
    pub rw: f64,
    pub rx: f64,
    pub ry: f64,
    pub rz: f64,
    pub dw: f64,
    pub dx: f64,
    pub dy: f64,
    pub dz: f64,
}

impl Default for Transform {
    fn default() -> Self {
        Self::identity()
    }
}

impl Transform {
    pub const fn identity() -> Self {
        Self {
            rw: 1.0,
            rx: 0.0,
            ry: 0.0,
            rz: 0.0,
            dw: 0.0,
            dx: 0.0,
            dy: 0.0,
            dz: 0.0,
        }
    }

    pub const fn from_rotation(qw: f64, qx: f64, qy: f64, qz: f64) -> Self {
        Self {
            rw: qw,
            rx: qx,
            ry: qy,
            rz: qz,
            ..Self::identity()
        }
    }

    pub const fn from_translation(tx: f64, ty: f64, tz: f64) -> Self {
        Self {
            rw: 1.0,
            rx: 0.0,
            ry: 0.0,
            rz: 0.0,
            dw: 0.0,
            dx: tx / 2.0,
            dy: ty / 2.0,
            dz: tz / 2.0,
        }
    }

    pub fn from_rotation_translation(
        qw: f64,
        qx: f64,
        qy: f64,
        qz: f64,
        tx: f64,
        ty: f64,
        tz: f64,
    ) -> Self {
        Self {
            rw: qw,
            rx: qx,
            ry: qy,
            rz: qz,
            dw: 0.5 * (-tx * qx - ty * qy - tz * qz),
            dx: 0.5 * (tx * qw + ty * qz - tz * qy),
            dy: 0.5 * (-tx * qz + ty * qw + tz * qx),
            dz: 0.5 * (tx * qy - ty * qx + tz * qw),
        }
    }

    pub fn get_rotation(&self) -> Quaternion {
        Quaternion::new(self.rw, self.rx, self.ry, self.rz)
    }

    pub fn get_translation(&self) -> Point {
        Point::new(
            2.0 * (self.dx * self.rw - self.dw * self.rx + self.dz * self.ry - self.dy * self.rz),
            2.0 * (self.dy * self.rw - self.dz * self.rx - self.dw * self.ry + self.dx * self.rz),
            2.0 * (self.dz * self.rw + self.dy * self.rx - self.dx * self.ry - self.dw * self.rz),
        )
    }

    pub fn rotation_norm(&self) -> f64 {
        (self.rw * self.rw + self.rx * self.rx + self.ry * self.ry + self.rz * self.rz).sqrt()
    }

    pub fn is_set(&self) -> bool {
        *self != Self::identity()
    }

    pub fn conjugate(&self) -> Self {
        Self {
            rw: self.rw,
            rx: -self.rx,
            ry: -self.ry,
            rz: -self.rz,
            dw: self.dw,
            dx: -self.dx,
            dy: -self.dy,
            dz: -self.dz,
        }
    }

    pub fn inverse(&self) -> Self {
        let mut inv = self.conjugate();
        inv.dw = -inv.dw;
        inv.dx = -inv.dx;
        inv.dy = -inv.dy;
        inv.dz = -inv.dz;
        inv
    }

    pub fn normalized(&self) -> Self {
        let norm = self.rotation_norm();
        let inv_norm = 1.0 / norm;
        let dot = self.rw * self.dw + self.rx * self.dx + self.ry * self.dy + self.rz * self.dz;
        Self {
            rw: self.rw * inv_norm,
            rx: self.rx * inv_norm,
            ry: self.ry * inv_norm,
            rz: self.rz * inv_norm,
            dw: (self.dw - self.rw * dot * inv_norm * inv_norm) * inv_norm,
            dx: (self.dx - self.rx * dot * inv_norm * inv_norm) * inv_norm,
            dy: (self.dy - self.ry * dot * inv_norm * inv_norm) * inv_norm,
            dz: (self.dz - self.rz * dot * inv_norm * inv_norm) * inv_norm,
        }
    }

    pub fn apply(&self, px: &mut f64, py: &mut f64, pz: &mut f64) {
        let translation = self.get_translation();
        let t0 = 2.0 * (self.ry * *pz - self.rz * *py);
        let t1 = 2.0 * (self.rz * *px - self.rx * *pz);
        let t2 = 2.0 * (self.rx * *py - self.ry * *px);
        let rx = *px + self.rw * t0 + (self.ry * t2 - self.rz * t1);
        let ry = *py + self.rw * t1 + (self.rz * t0 - self.rx * t2);
        let rz = *pz + self.rw * t2 + (self.rx * t1 - self.ry * t0);
        *px = rx + translation.x;
        *py = ry + translation.y;
        *pz = rz + translation.z;
    }
}

impl std::ops::Mul for Transform {
    type Output = Self;

    fn mul(self, other: Self) -> Self::Output {
        Self {
            rw: self.rw * other.rw - self.rx * other.rx - self.ry * other.ry - self.rz * other.rz,
            rx: self.rw * other.rx + self.rx * other.rw + self.ry * other.rz - self.rz * other.ry,
            ry: self.rw * other.ry - self.rx * other.rz + self.ry * other.rw + self.rz * other.rx,
            rz: self.rw * other.rz + self.rx * other.ry - self.ry * other.rx + self.rz * other.rw,
            dw: self.rw * other.dw - self.rx * other.dx - self.ry * other.dy - self.rz * other.dz
                + self.dw * other.rw
                - self.dx * other.rx
                - self.dy * other.ry
                - self.dz * other.rz,
            dx: self.rw * other.dx + self.rx * other.dw + self.ry * other.dz - self.rz * other.dy
                + self.dw * other.rx
                + self.dx * other.rw
                + self.dy * other.rz
                - self.dz * other.ry,
            dy: self.rw * other.dy - self.rx * other.dz
                + self.ry * other.dw
                + self.rz * other.dx
                + self.dw * other.ry
                - self.dx * other.rz
                + self.dy * other.rw
                + self.dz * other.rx,
            dz: self.rw * other.dz + self.rx * other.dy - self.ry * other.dx
                + self.rz * other.dw
                + self.dw * other.rz
                + self.dx * other.ry
                - self.dy * other.rx
                + self.dz * other.rw,
        }
    }
}
