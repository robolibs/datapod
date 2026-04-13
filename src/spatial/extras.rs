use crate::Vector;
use crate::associative::{Map, Set};
use crate::matrix::mat;

use super::{Aabb, Geo, Linestring, Obb, Point, Polygon, Quaternion, Size};

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Velocity {
    pub vx: f64,
    pub vy: f64,
    pub vz: f64,
}

impl Velocity {
    pub fn speed(&self) -> f64 {
        (self.vx * self.vx + self.vy * self.vy + self.vz * self.vz).sqrt()
    }

    pub fn speed_2d(&self) -> f64 {
        (self.vx * self.vx + self.vy * self.vy).sqrt()
    }

    pub fn speed_squared(&self) -> f64 {
        self.vx * self.vx + self.vy * self.vy + self.vz * self.vz
    }

    pub fn is_set(&self) -> bool {
        self.vx != 0.0 || self.vy != 0.0 || self.vz != 0.0
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 3> {
        mat::Vector::from([self.vx, self.vy, self.vz])
    }

    pub fn from_mat(v: mat::Vector<f64, 3>) -> Self {
        Self {
            vx: v[0],
            vy: v[1],
            vz: v[2],
        }
    }
}

impl std::ops::Add for Velocity {
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        Self {
            vx: self.vx + rhs.vx,
            vy: self.vy + rhs.vy,
            vz: self.vz + rhs.vz,
        }
    }
}

impl std::ops::Sub for Velocity {
    type Output = Self;

    fn sub(self, rhs: Self) -> Self::Output {
        Self {
            vx: self.vx - rhs.vx,
            vy: self.vy - rhs.vy,
            vz: self.vz - rhs.vz,
        }
    }
}

impl std::ops::Mul<f64> for Velocity {
    type Output = Self;

    fn mul(self, rhs: f64) -> Self::Output {
        Self {
            vx: self.vx * rhs,
            vy: self.vy * rhs,
            vz: self.vz * rhs,
        }
    }
}

impl std::ops::Div<f64> for Velocity {
    type Output = Self;

    fn div(self, rhs: f64) -> Self::Output {
        Self {
            vx: self.vx / rhs,
            vy: self.vy / rhs,
            vz: self.vz / rhs,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Acceleration {
    pub ax: f64,
    pub ay: f64,
    pub az: f64,
}

impl Acceleration {
    pub fn magnitude(&self) -> f64 {
        (self.ax * self.ax + self.ay * self.ay + self.az * self.az).sqrt()
    }

    pub fn magnitude_2d(&self) -> f64 {
        (self.ax * self.ax + self.ay * self.ay).sqrt()
    }

    pub fn magnitude_squared(&self) -> f64 {
        self.ax * self.ax + self.ay * self.ay + self.az * self.az
    }

    pub fn is_set(&self) -> bool {
        self.ax != 0.0 || self.ay != 0.0 || self.az != 0.0
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 3> {
        mat::Vector::from([self.ax, self.ay, self.az])
    }

    pub fn from_mat(v: mat::Vector<f64, 3>) -> Self {
        Self {
            ax: v[0],
            ay: v[1],
            az: v[2],
        }
    }
}

impl std::ops::Add for Acceleration {
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        Self {
            ax: self.ax + rhs.ax,
            ay: self.ay + rhs.ay,
            az: self.az + rhs.az,
        }
    }
}

impl std::ops::Sub for Acceleration {
    type Output = Self;

    fn sub(self, rhs: Self) -> Self::Output {
        Self {
            ax: self.ax - rhs.ax,
            ay: self.ay - rhs.ay,
            az: self.az - rhs.az,
        }
    }
}

impl std::ops::Mul<f64> for Acceleration {
    type Output = Self;

    fn mul(self, rhs: f64) -> Self::Output {
        Self {
            ax: self.ax * rhs,
            ay: self.ay * rhs,
            az: self.az * rhs,
        }
    }
}

impl std::ops::Div<f64> for Acceleration {
    type Output = Self;

    fn div(self, rhs: f64) -> Self::Output {
        Self {
            ax: self.ax / rhs,
            ay: self.ay / rhs,
            az: self.az / rhs,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Pose {
    pub point: Point,
    pub rotation: Quaternion,
}

impl Pose {
    pub fn is_set(&self) -> bool {
        self.point.is_set() || self.rotation.is_set()
    }

    pub fn transform_point(&self, local_point: Point) -> Point {
        let q = self.rotation;
        let p = Quaternion::new(0.0, local_point.x, local_point.y, local_point.z);
        let rotated = q * p * q.conjugate();
        Point::new(
            self.point.x + rotated.x,
            self.point.y + rotated.y,
            self.point.z + rotated.z,
        )
    }

    pub fn inverse_transform_point(&self, world_point: Point) -> Point {
        let translated = Point::new(
            world_point.x - self.point.x,
            world_point.y - self.point.y,
            world_point.z - self.point.z,
        );
        let p = Quaternion::new(0.0, translated.x, translated.y, translated.z);
        let rotated = self.rotation.conjugate() * p * self.rotation;
        Point::new(rotated.x, rotated.y, rotated.z)
    }

    pub fn inverse(&self) -> Self {
        let q_inv = self.rotation.conjugate();
        let neg_pos = Quaternion::new(0.0, -self.point.x, -self.point.y, -self.point.z);
        let rotated = q_inv * neg_pos * self.rotation;
        Self {
            point: Point::new(rotated.x, rotated.y, rotated.z),
            rotation: q_inv,
        }
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 7> {
        mat::Vector::from([
            self.point.x,
            self.point.y,
            self.point.z,
            self.rotation.w,
            self.rotation.x,
            self.rotation.y,
            self.rotation.z,
        ])
    }

    pub fn from_mat(v: mat::Vector<f64, 7>) -> Self {
        Self {
            point: Point::new(v[0], v[1], v[2]),
            rotation: Quaternion::new(v[3], v[4], v[5], v[6]),
        }
    }
}

impl std::ops::Mul for Pose {
    type Output = Self;

    fn mul(self, rhs: Self) -> Self::Output {
        Self {
            point: self.transform_point(rhs.point),
            rotation: self.rotation * rhs.rotation,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
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

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct State {
    pub pose: Pose,
    pub linear_velocity: Velocity,
    pub angular_velocity: Velocity,
}

impl State {
    pub fn is_set(&self) -> bool {
        self.pose.is_set() || self.linear_velocity.is_set() || self.angular_velocity.is_set()
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 13> {
        mat::Vector::from([
            self.pose.point.x,
            self.pose.point.y,
            self.pose.point.z,
            self.pose.rotation.w,
            self.pose.rotation.x,
            self.pose.rotation.y,
            self.pose.rotation.z,
            self.linear_velocity.vx,
            self.linear_velocity.vy,
            self.linear_velocity.vz,
            self.angular_velocity.vx,
            self.angular_velocity.vy,
            self.angular_velocity.vz,
        ])
    }

    pub fn from_mat(v: mat::Vector<f64, 13>) -> Self {
        Self {
            pose: Pose {
                point: Point::new(v[0], v[1], v[2]),
                rotation: Quaternion::new(v[3], v[4], v[5], v[6]),
            },
            linear_velocity: Velocity {
                vx: v[7],
                vy: v[8],
                vz: v[9],
            },
            angular_velocity: Velocity {
                vx: v[10],
                vy: v[11],
                vz: v[12],
            },
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Utm {
    pub zone: i32,
    pub band: char,
    pub easting: f64,
    pub northing: f64,
    pub altitude: f64,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Loc {
    pub local: Point,
    pub origin: Geo,
}

impl Loc {
    pub fn is_set(&self) -> bool {
        self.local.is_set() || self.origin.is_set()
    }

    pub fn has_valid_origin(&self) -> bool {
        self.origin.is_valid()
    }

    pub fn distance_from_origin(&self) -> f64 {
        self.local.magnitude()
    }

    pub fn distance_from_origin_2d(&self) -> f64 {
        (self.local.x * self.local.x + self.local.y * self.local.y).sqrt()
    }

    pub fn distance_to(&self, other: Loc) -> f64 {
        self.local.distance_to(other.local)
    }

    pub fn distance_to_2d(&self, other: Loc) -> f64 {
        self.local.distance_to_2d(other.local)
    }

    pub fn same_origin(&self, other: Loc, tolerance: f64) -> bool {
        (self.origin.latitude - other.origin.latitude).abs() < tolerance
            && (self.origin.longitude - other.origin.longitude).abs() < tolerance
            && (self.origin.altitude - other.origin.altitude).abs() < tolerance
    }
}

impl std::ops::Add<Point> for Loc {
    type Output = Self;

    fn add(self, rhs: Point) -> Self::Output {
        Self {
            local: self.local + rhs,
            origin: self.origin,
        }
    }
}

impl std::ops::Sub<Point> for Loc {
    type Output = Self;

    fn sub(self, rhs: Point) -> Self::Output {
        Self {
            local: self.local - rhs,
            origin: self.origin,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct BoundingSphere {
    pub center: Point,
    pub radius: f64,
}

impl BoundingSphere {
    pub fn unit() -> Self {
        Self {
            center: Point::default(),
            radius: 1.0,
        }
    }
}

pub type Bs = BoundingSphere;

#[derive(Debug, Clone, Copy, PartialEq, Default)]
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

    pub fn to_mat(&self) -> mat::Vector<f64, 10> {
        let p = self.pose.to_mat();
        mat::Vector::from([
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
        ])
    }

    pub fn from_mat(v: mat::Vector<f64, 10>) -> Self {
        Self {
            pose: Pose::from_mat(mat::Vector::from([
                v[0], v[1], v[2], v[3], v[4], v[5], v[6],
            ])),
            size: Size::new(v[7], v[8], v[9]),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Circle {
    pub center: Point,
    pub radius: f64,
}

impl Circle {
    pub fn area(&self) -> f64 {
        std::f64::consts::PI * self.radius * self.radius
    }

    pub fn perimeter(&self) -> f64 {
        2.0 * std::f64::consts::PI * self.radius
    }

    pub fn contains(&self, point: Point) -> bool {
        self.center.distance_to(point) <= self.radius
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 4> {
        mat::Vector::from([self.center.x, self.center.y, self.center.z, self.radius])
    }

    pub fn from_mat(v: mat::Vector<f64, 4>) -> Self {
        Self {
            center: Point::new(v[0], v[1], v[2]),
            radius: v[3],
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Line {
    pub point: Point,
    pub direction: Point,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Rectangle {
    pub top_left: Point,
    pub top_right: Point,
    pub bottom_left: Point,
    pub bottom_right: Point,
}

impl Rectangle {
    pub fn area(&self) -> f64 {
        self.bottom_left.distance_to(self.bottom_right)
            * self.bottom_left.distance_to(self.top_left)
    }

    pub fn perimeter(&self) -> f64 {
        let width = self.bottom_left.distance_to(self.bottom_right);
        let height = self.bottom_left.distance_to(self.top_left);
        2.0 * (width + height)
    }

    pub fn contains(&self, point: Point) -> bool {
        let mut min_x = self.bottom_left.x;
        let mut max_x = self.bottom_right.x;
        let mut min_y = self.bottom_left.y;
        let mut max_y = self.top_left.y;
        if min_x > max_x {
            std::mem::swap(&mut min_x, &mut max_x);
        }
        if min_y > max_y {
            std::mem::swap(&mut min_y, &mut max_y);
        }
        point.x >= min_x && point.x <= max_x && point.y >= min_y && point.y <= max_y
    }

    pub fn get_corners(&self) -> [Point; 4] {
        [
            self.bottom_left,
            self.bottom_right,
            self.top_right,
            self.top_left,
        ]
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 12> {
        mat::Vector::from([
            self.top_left.x,
            self.top_left.y,
            self.top_left.z,
            self.top_right.x,
            self.top_right.y,
            self.top_right.z,
            self.bottom_left.x,
            self.bottom_left.y,
            self.bottom_left.z,
            self.bottom_right.x,
            self.bottom_right.y,
            self.bottom_right.z,
        ])
    }

    pub fn from_mat(v: mat::Vector<f64, 12>) -> Self {
        Self {
            top_left: Point::new(v[0], v[1], v[2]),
            top_right: Point::new(v[3], v[4], v[5]),
            bottom_left: Point::new(v[6], v[7], v[8]),
            bottom_right: Point::new(v[9], v[10], v[11]),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Square {
    pub center: Point,
    pub side: f64,
}

impl Square {
    pub fn area(&self) -> f64 {
        self.side * self.side
    }

    pub fn perimeter(&self) -> f64 {
        4.0 * self.side
    }

    pub fn diagonal(&self) -> f64 {
        self.side * 2.0_f64.sqrt()
    }

    pub fn contains(&self, point: Point) -> bool {
        let half = self.side / 2.0;
        (point.x - self.center.x).abs() <= half && (point.y - self.center.y).abs() <= half
    }

    pub fn get_corners(&self) -> [Point; 4] {
        let half = self.side / 2.0;
        [
            Point::new(self.center.x - half, self.center.y - half, self.center.z),
            Point::new(self.center.x + half, self.center.y - half, self.center.z),
            Point::new(self.center.x + half, self.center.y + half, self.center.z),
            Point::new(self.center.x - half, self.center.y + half, self.center.z),
        ]
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 4> {
        mat::Vector::from([self.center.x, self.center.y, self.center.z, self.side])
    }

    pub fn from_mat(v: mat::Vector<f64, 4>) -> Self {
        Self {
            center: Point::new(v[0], v[1], v[2]),
            side: v[3],
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
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

    pub fn to_mat(&self) -> mat::Vector<f64, 9> {
        mat::Vector::from([
            self.a.x, self.a.y, self.a.z, self.b.x, self.b.y, self.b.z, self.c.x, self.c.y,
            self.c.z,
        ])
    }

    pub fn from_mat(v: mat::Vector<f64, 9>) -> Self {
        Self {
            a: Point::new(v[0], v[1], v[2]),
            b: Point::new(v[3], v[4], v[5]),
            c: Point::new(v[6], v[7], v[8]),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Ring {
    pub points: Vector<Point>,
}

impl Ring {
    pub fn length(&self) -> f64 {
        self.points
            .windows(2)
            .map(|segment| segment[0].distance_to(segment[1]))
            .sum()
    }

    pub fn area(&self) -> f64 {
        if self.points.len() < 3 {
            return 0.0;
        }
        let mut sum = 0.0;
        for i in 0..self.points.len() - 1 {
            sum +=
                self.points[i].x * self.points[i + 1].y - self.points[i + 1].x * self.points[i].y;
        }
        sum.abs() * 0.5
    }

    pub fn num_points(&self) -> usize {
        self.points.len()
    }

    pub fn is_closed(&self) -> bool {
        self.points.len() >= 3 && self.points.front() == self.points.back()
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Grid<T = f64> {
    pub rows: usize,
    pub cols: usize,
    pub resolution: f64,
    pub centered: bool,
    pub pose: Pose,
    pub data: Vector<T>,
}

impl<T> Grid<T> {
    pub fn index(&self, row: usize, col: usize) -> usize {
        row * self.cols + col
    }

    pub fn size(&self) -> usize {
        self.rows * self.cols
    }

    pub fn is_valid(&self) -> bool {
        self.rows > 0 && self.cols > 0 && self.data.len() == self.size()
    }

    pub fn get_point(&self, row: usize, col: usize) -> Point {
        let mut local_x = (col as f64 + 0.5) * self.resolution;
        let mut local_y = (row as f64 + 0.5) * self.resolution;
        if self.centered {
            local_x -= self.cols as f64 * self.resolution * 0.5;
            local_y -= self.rows as f64 * self.resolution * 0.5;
        }
        self.pose.transform_point(Point::new(local_x, local_y, 0.0))
    }

    pub fn world_to_grid(&self, world_point: Point) -> (usize, usize) {
        let local_point = self.pose.inverse_transform_point(world_point);
        let mut local_x = local_point.x;
        let mut local_y = local_point.y;
        if self.centered {
            local_x += self.cols as f64 * self.resolution * 0.5;
            local_y += self.rows as f64 * self.resolution * 0.5;
        }
        let col = ((local_x / self.resolution) - 0.5)
            .round()
            .clamp(0.0, self.cols.saturating_sub(1) as f64) as usize;
        let row = ((local_y / self.resolution) - 0.5)
            .round()
            .clamp(0.0, self.rows.saturating_sub(1) as f64) as usize;
        (row, col)
    }
}

impl<T> std::ops::Index<(usize, usize)> for Grid<T> {
    type Output = T;

    fn index(&self, index: (usize, usize)) -> &Self::Output {
        &self.data[self.index(index.0, index.1)]
    }
}

impl<T> std::ops::IndexMut<(usize, usize)> for Grid<T> {
    fn index_mut(&mut self, index: (usize, usize)) -> &mut Self::Output {
        let flat = self.index(index.0, index.1);
        &mut self.data[flat]
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Layer<T = f64> {
    pub rows: usize,
    pub cols: usize,
    pub layers: usize,
    pub resolution: f64,
    pub layer_height: f64,
    pub centered: bool,
    pub pose: Pose,
    pub data: Vector<T>,
}

impl<T: Clone> Layer<T> {
    pub fn extract_grid(&self, layer_idx: usize) -> Grid<T> {
        assert!(layer_idx < self.layers, "layer index out of bounds");
        let mut grid = Grid {
            rows: self.rows,
            cols: self.cols,
            resolution: self.resolution,
            centered: self.centered,
            pose: self.pose,
            data: Vector::from_elem(
                self.rows * self.cols,
                self.data[self.index(0, 0, layer_idx)].clone(),
            ),
        };
        let z_offset = (layer_idx as f64 + 0.5) * self.layer_height;
        let layer_offset = Point::new(0.0, 0.0, z_offset);
        let world_offset = self.pose.transform_point(layer_offset) - self.pose.point;
        grid.pose = Pose {
            point: Point::new(
                self.pose.point.x + world_offset.x,
                self.pose.point.y + world_offset.y,
                self.pose.point.z + world_offset.z,
            ),
            rotation: self.pose.rotation,
        };
        for i in 0..self.rows * self.cols {
            grid.data[i] = self.data[layer_idx * self.rows * self.cols + i].clone();
        }
        grid
    }

    pub fn set_grid(&mut self, layer_idx: usize, grid: &Grid<T>) {
        assert!(layer_idx < self.layers, "layer index out of bounds");
        assert!(
            grid.rows == self.rows && grid.cols == self.cols,
            "grid shape mismatch"
        );
        let start = layer_idx * self.rows * self.cols;
        for i in 0..self.rows * self.cols {
            self.data[start + i] = grid.data[i].clone();
        }
    }
}

impl<T> Layer<T> {
    pub fn index(&self, row: usize, col: usize, layer: usize) -> usize {
        layer * self.rows * self.cols + row * self.cols + col
    }

    pub fn size(&self) -> usize {
        self.rows * self.cols * self.layers
    }

    pub fn is_valid(&self) -> bool {
        self.rows > 0 && self.cols > 0 && self.layers > 0 && self.data.len() == self.size()
    }

    pub fn get_point(&self, row: usize, col: usize, layer: usize) -> Point {
        let mut local_x = (col as f64 + 0.5) * self.resolution;
        let mut local_y = (row as f64 + 0.5) * self.resolution;
        let local_z = (layer as f64 + 0.5) * self.layer_height;
        if self.centered {
            local_x -= self.cols as f64 * self.resolution * 0.5;
            local_y -= self.rows as f64 * self.resolution * 0.5;
        }
        self.pose
            .transform_point(Point::new(local_x, local_y, local_z))
    }

    pub fn world_to_voxel(&self, world_point: Point) -> (usize, usize, usize) {
        let local_point = self.pose.inverse_transform_point(world_point);
        let mut local_x = local_point.x;
        let mut local_y = local_point.y;
        let local_z = local_point.z;
        if self.centered {
            local_x += self.cols as f64 * self.resolution * 0.5;
            local_y += self.rows as f64 * self.resolution * 0.5;
        }
        let col = ((local_x / self.resolution) - 0.5)
            .round()
            .clamp(0.0, self.cols.saturating_sub(1) as f64) as usize;
        let row = ((local_y / self.resolution) - 0.5)
            .round()
            .clamp(0.0, self.rows.saturating_sub(1) as f64) as usize;
        let layer = if self.layer_height > 0.0 {
            ((local_z / self.layer_height) - 0.5)
                .round()
                .clamp(0.0, self.layers.saturating_sub(1) as f64) as usize
        } else {
            0
        };
        (row, col, layer)
    }

    pub fn layer_count(&self) -> usize {
        self.layers
    }

    pub fn get_layer_height(&self) -> f64 {
        self.layer_height
    }

    pub fn get_resolution(&self) -> f64 {
        self.resolution
    }

    pub fn shift(&self) -> &Pose {
        &self.pose
    }
}

impl<T> std::ops::Index<(usize, usize, usize)> for Layer<T> {
    type Output = T;

    fn index(&self, index: (usize, usize, usize)) -> &Self::Output {
        &self.data[self.index(index.0, index.1, index.2)]
    }
}

impl<T> std::ops::IndexMut<(usize, usize, usize)> for Layer<T> {
    fn index_mut(&mut self, index: (usize, usize, usize)) -> &mut Self::Output {
        let flat = self.index(index.0, index.1, index.2);
        &mut self.data[flat]
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Path {
    pub waypoints: Vector<Pose>,
}

impl Path {
    pub fn size(&self) -> usize {
        self.waypoints.len()
    }

    pub fn is_empty(&self) -> bool {
        self.waypoints.is_empty()
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Trajectory {
    pub states: Vector<State>,
}

impl Trajectory {
    pub fn size(&self) -> usize {
        self.states.len()
    }

    pub fn is_empty(&self) -> bool {
        self.states.is_empty()
    }
}

pub type MultiPoint = Vector<Point>;
pub type MultiLinestring = Vector<Linestring>;
pub type MultiPolygon = Vector<Polygon>;

#[derive(Debug, Clone, PartialEq, Default)]
pub struct GaussianPoint {
    pub mean: Point,
    pub covariance: mat::Matrix<f64, 3, 3>,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct GaussianBox {
    pub mean: Box,
    pub covariance: mat::Matrix<f64, 6, 6>,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct GaussianCircle {
    pub mean: Circle,
    pub variance: f64,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct GaussianRectangle {
    pub mean: Rectangle,
    pub covariance: mat::Matrix<f64, 4, 4>,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct RTree<T> {
    pub items: Vector<T>,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Quadtree<T> {
    pub items: Vector<T>,
}

#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Identity {
    pub name: String,
    pub uuid: crate::sugar::Uuid,
    pub ip: crate::sugar::Ip,
    pub rci: u8,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct BoxShape {
    pub size: Size,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct SphereShape {
    pub radius: f64,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct CylinderShape {
    pub radius: f64,
    pub length: f64,
}

#[derive(Debug, Clone, PartialEq)]
pub struct MeshShape {
    pub uri: String,
    pub scale: [f64; 3],
}

impl Default for MeshShape {
    fn default() -> Self {
        Self {
            uri: String::default(),
            scale: [1.0, 1.0, 1.0],
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum Geometry {
    Box(BoxShape),
    Sphere(SphereShape),
    Cylinder(CylinderShape),
    Mesh(MeshShape),
}

impl Default for Geometry {
    fn default() -> Self {
        Self::Box(BoxShape::default())
    }
}

impl Geometry {
    pub fn box_shape(size: Size) -> Self {
        Self::Box(BoxShape { size })
    }

    pub fn sphere(radius: f64) -> Self {
        Self::Sphere(SphereShape { radius })
    }

    pub fn cylinder(radius: f64, length: f64) -> Self {
        Self::Cylinder(CylinderShape { radius, length })
    }

    pub fn mesh(uri: impl Into<String>, scale: [f64; 3]) -> Self {
        Self::Mesh(MeshShape {
            uri: uri.into(),
            scale,
        })
    }

    pub fn is_box(&self) -> bool {
        matches!(self, Self::Box(_))
    }

    pub fn is_sphere(&self) -> bool {
        matches!(self, Self::Sphere(_))
    }

    pub fn is_cylinder(&self) -> bool {
        matches!(self, Self::Cylinder(_))
    }

    pub fn is_mesh(&self) -> bool {
        matches!(self, Self::Mesh(_))
    }

    pub fn as_box(&self) -> Option<&BoxShape> {
        match self {
            Self::Box(shape) => Some(shape),
            _ => None,
        }
    }

    pub fn as_sphere(&self) -> Option<&SphereShape> {
        match self {
            Self::Sphere(shape) => Some(shape),
            _ => None,
        }
    }

    pub fn as_cylinder(&self) -> Option<&CylinderShape> {
        match self {
            Self::Cylinder(shape) => Some(shape),
            _ => None,
        }
    }

    pub fn as_mesh(&self) -> Option<&MeshShape> {
        match self {
            Self::Mesh(shape) => Some(shape),
            _ => None,
        }
    }

    pub fn as_box_mut(&mut self) -> Option<&mut BoxShape> {
        match self {
            Self::Box(shape) => Some(shape),
            _ => None,
        }
    }

    pub fn as_sphere_mut(&mut self) -> Option<&mut SphereShape> {
        match self {
            Self::Sphere(shape) => Some(shape),
            _ => None,
        }
    }

    pub fn as_cylinder_mut(&mut self) -> Option<&mut CylinderShape> {
        match self {
            Self::Cylinder(shape) => Some(shape),
            _ => None,
        }
    }

    pub fn as_mesh_mut(&mut self) -> Option<&mut MeshShape> {
        match self {
            Self::Mesh(shape) => Some(shape),
            _ => None,
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct Material {
    pub name: String,
    pub rgba: [f64; 4],
    pub texture: String,
}

impl Default for Material {
    fn default() -> Self {
        Self {
            name: String::default(),
            rgba: [1.0, 1.0, 1.0, 1.0],
            texture: String::default(),
        }
    }
}

impl Material {
    pub fn has_texture(&self) -> bool {
        !self.texture.is_empty()
    }

    pub fn named(name: impl Into<String>, rgba: [f64; 4]) -> Self {
        Self {
            name: name.into(),
            rgba,
            texture: String::default(),
        }
    }

    pub fn color(rgba: [f64; 4]) -> Self {
        Self {
            name: String::default(),
            rgba,
            texture: String::default(),
        }
    }

    pub fn textured(texture: impl Into<String>) -> Self {
        Self {
            name: String::default(),
            rgba: [1.0, 1.0, 1.0, 1.0],
            texture: texture.into(),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Visual {
    pub name: String,
    pub origin: Pose,
    pub geom: Geometry,
    pub material: Option<Material>,
}

impl Visual {
    pub fn new(geom: Geometry) -> Self {
        Self {
            name: String::default(),
            origin: Pose::default(),
            geom,
            material: None,
        }
    }

    pub fn with_origin(origin: Pose, geom: Geometry) -> Self {
        Self {
            name: String::default(),
            origin,
            geom,
            material: None,
        }
    }

    pub fn with_material(geom: Geometry, material: Material) -> Self {
        Self {
            name: String::default(),
            origin: Pose::default(),
            geom,
            material: Some(material),
        }
    }

    pub fn is_set(&self) -> bool {
        self.origin.is_set() || !self.name.is_empty()
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Collision {
    pub name: String,
    pub origin: Pose,
    pub geom: Geometry,
}

impl Collision {
    pub fn new(geom: Geometry) -> Self {
        Self {
            name: String::default(),
            origin: Pose::default(),
            geom,
        }
    }

    pub fn with_origin(origin: Pose, geom: Geometry) -> Self {
        Self {
            name: String::default(),
            origin,
            geom,
        }
    }

    pub fn is_set(&self) -> bool {
        self.origin.is_set() || !self.name.is_empty()
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Inertial {
    pub origin: Pose,
    pub mass: f64,
    pub ixx: f64,
    pub ixy: f64,
    pub ixz: f64,
    pub iyy: f64,
    pub iyz: f64,
    pub izz: f64,
}

impl Inertial {
    pub fn is_set(&self) -> bool {
        self.mass != 0.0
            || self.origin.is_set()
            || self.ixx != 0.0
            || self.iyy != 0.0
            || self.izz != 0.0
    }

    pub fn trace(&self) -> f64 {
        self.ixx + self.iyy + self.izz
    }

    pub fn is_diagonal(&self) -> bool {
        self.ixy == 0.0 && self.ixz == 0.0 && self.iyz == 0.0
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Sensor {
    pub name: String,
    pub r#type: String,
    pub origin: Pose,
    pub props: Map<String, String>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum JointType {
    #[default]
    Fixed,
    Revolute,
    Continuous,
    Prismatic,
    Floating,
    Planar,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct JointLimits {
    pub lower: f64,
    pub upper: f64,
    pub effort: f64,
    pub velocity: f64,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct JointDynamics {
    pub damping: f64,
    pub friction: f64,
}

#[derive(Debug, Clone, PartialEq)]
pub struct JointMimic {
    pub name: String,
    pub multiplier: f64,
    pub offset: f64,
}

impl Default for JointMimic {
    fn default() -> Self {
        Self {
            name: String::default(),
            multiplier: 1.0,
            offset: 0.0,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct JointSafetyController {
    pub soft_lower_limit: f64,
    pub soft_upper_limit: f64,
    pub k_position: f64,
    pub k_velocity: f64,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct JointCalibration {
    pub rising: Option<f64>,
    pub falling: Option<f64>,
}

pub const INVALID_ID: u32 = u32::MAX;

#[derive(Debug, Clone, PartialEq)]
pub struct Joint {
    pub name: String,
    pub r#type: JointType,
    pub origin: Pose,
    pub axis: [f64; 3],
    pub limits: Option<JointLimits>,
    pub dynamics: Option<JointDynamics>,
    pub mimic: Option<JointMimic>,
    pub safety_controller: Option<JointSafetyController>,
    pub calibration: Option<JointCalibration>,
    pub parent: u32,
    pub child: u32,
    pub props: Map<String, String>,
}

impl Default for Joint {
    fn default() -> Self {
        Self {
            name: String::default(),
            r#type: JointType::Fixed,
            origin: Pose::default(),
            axis: [1.0, 0.0, 0.0],
            limits: None,
            dynamics: None,
            mimic: None,
            safety_controller: None,
            calibration: None,
            parent: INVALID_ID,
            child: INVALID_ID,
            props: Map::default(),
        }
    }
}

impl Joint {
    pub fn fixed(name: impl Into<String>, origin: Pose) -> Self {
        Self {
            name: name.into(),
            origin,
            ..Self::default()
        }
    }

    pub fn revolute(
        name: impl Into<String>,
        axis: [f64; 3],
        limits: JointLimits,
        origin: Pose,
    ) -> Self {
        Self {
            name: name.into(),
            r#type: JointType::Revolute,
            origin,
            axis,
            limits: Some(limits),
            ..Self::default()
        }
    }

    pub fn continuous(name: impl Into<String>, axis: [f64; 3], origin: Pose) -> Self {
        Self {
            name: name.into(),
            r#type: JointType::Continuous,
            origin,
            axis,
            ..Self::default()
        }
    }

    pub fn prismatic(
        name: impl Into<String>,
        axis: [f64; 3],
        limits: JointLimits,
        origin: Pose,
    ) -> Self {
        Self {
            name: name.into(),
            r#type: JointType::Prismatic,
            origin,
            axis,
            limits: Some(limits),
            ..Self::default()
        }
    }

    pub fn is_fixed(&self) -> bool {
        self.r#type == JointType::Fixed
    }

    pub fn is_revolute(&self) -> bool {
        self.r#type == JointType::Revolute
    }

    pub fn is_continuous(&self) -> bool {
        self.r#type == JointType::Continuous
    }

    pub fn is_prismatic(&self) -> bool {
        self.r#type == JointType::Prismatic
    }

    pub fn is_floating(&self) -> bool {
        self.r#type == JointType::Floating
    }

    pub fn is_planar(&self) -> bool {
        self.r#type == JointType::Planar
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Link {
    pub name: String,
    pub inertial: Option<Inertial>,
    pub visuals: Vector<Visual>,
    pub collisions: Vector<Collision>,
    pub props: Map<String, String>,
    pub sensor: Option<Sensor>,
}

impl Link {
    pub fn new(name: impl Into<String>) -> Self {
        Self {
            name: name.into(),
            ..Self::default()
        }
    }

    pub fn with_inertial(name: impl Into<String>, inertial: Inertial) -> Self {
        Self {
            name: name.into(),
            inertial: Some(inertial),
            ..Self::default()
        }
    }

    pub fn has_inertial(&self) -> bool {
        self.inertial.is_some()
    }

    pub fn has_visuals(&self) -> bool {
        !self.visuals.is_empty()
    }

    pub fn has_collisions(&self) -> bool {
        !self.collisions.is_empty()
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Actuator {
    pub name: String,
    pub mechanical_reduction: Option<f64>,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct TransmissionJoint {
    pub name: String,
    pub mechanical_reduction: Option<f64>,
    pub offset: Option<f64>,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Transmission {
    pub name: String,
    pub r#type: String,
    pub joints: Vector<TransmissionJoint>,
    pub actuators: Vector<Actuator>,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Twist {
    pub linear: Velocity,
    pub angular: Velocity,
}

impl Twist {
    pub fn is_set(&self) -> bool {
        self.linear.is_set() || self.angular.is_set()
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 6> {
        mat::Vector::from([
            self.linear.vx,
            self.linear.vy,
            self.linear.vz,
            self.angular.vx,
            self.angular.vy,
            self.angular.vz,
        ])
    }

    pub fn from_mat(v: mat::Vector<f64, 6>) -> Self {
        Self {
            linear: Velocity {
                vx: v[0],
                vy: v[1],
                vz: v[2],
            },
            angular: Velocity {
                vx: v[3],
                vy: v[4],
                vz: v[5],
            },
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Wrench {
    pub force: Point,
    pub torque: Point,
}

impl Wrench {
    pub fn is_set(&self) -> bool {
        self.force.is_set() || self.torque.is_set()
    }

    pub fn force_magnitude(&self) -> f64 {
        self.force.magnitude()
    }

    pub fn torque_magnitude(&self) -> f64 {
        self.torque.magnitude()
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 6> {
        mat::Vector::from([
            self.force.x,
            self.force.y,
            self.force.z,
            self.torque.x,
            self.torque.y,
            self.torque.z,
        ])
    }

    pub fn from_mat(v: mat::Vector<f64, 6>) -> Self {
        Self {
            force: Point::new(v[0], v[1], v[2]),
            torque: Point::new(v[3], v[4], v[5]),
        }
    }
}

impl std::ops::Add for Wrench {
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        Self {
            force: self.force + rhs.force,
            torque: self.torque + rhs.torque,
        }
    }
}

impl std::ops::Sub for Wrench {
    type Output = Self;

    fn sub(self, rhs: Self) -> Self::Output {
        Self {
            force: self.force - rhs.force,
            torque: self.torque - rhs.torque,
        }
    }
}

impl std::ops::Mul<f64> for Wrench {
    type Output = Self;

    fn mul(self, rhs: f64) -> Self::Output {
        Self {
            force: self.force * rhs,
            torque: self.torque * rhs,
        }
    }
}

impl std::ops::Div<f64> for Wrench {
    type Output = Self;

    fn div(self, rhs: f64) -> Self::Output {
        Self {
            force: self.force / rhs,
            torque: self.torque / rhs,
        }
    }
}

pub type Accel = Acceleration;

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Odom {
    pub pose: Pose,
    pub twist: Twist,
}

impl Odom {
    pub fn is_set(&self) -> bool {
        self.pose.is_set() || self.twist.is_set()
    }

    pub fn to_mat(&self) -> mat::Vector<f64, 13> {
        mat::Vector::from([
            self.pose.point.x,
            self.pose.point.y,
            self.pose.point.z,
            self.pose.rotation.w,
            self.pose.rotation.x,
            self.pose.rotation.y,
            self.pose.rotation.z,
            self.twist.linear.vx,
            self.twist.linear.vy,
            self.twist.linear.vz,
            self.twist.angular.vx,
            self.twist.angular.vy,
            self.twist.angular.vz,
        ])
    }

    pub fn from_mat(v: mat::Vector<f64, 13>) -> Self {
        Self {
            pose: Pose {
                point: Point::new(v[0], v[1], v[2]),
                rotation: Quaternion::new(v[3], v[4], v[5], v[6]),
            },
            twist: Twist {
                linear: Velocity {
                    vx: v[7],
                    vy: v[8],
                    vz: v[9],
                },
                angular: Velocity {
                    vx: v[10],
                    vy: v[11],
                    vz: v[12],
                },
            },
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct Model {
    pub links: Vector<Link>,
    pub joints: Vector<Joint>,
    pub transmissions: Vector<Transmission>,
    pub props: Map<String, String>,
    pub root: u32,
    pub parent_of: Vector<u32>,
    pub joint_from_parent: Vector<u32>,
    pub children_of: Vector<Vector<u32>>,
}

impl Default for Model {
    fn default() -> Self {
        Self {
            links: Vector::default(),
            joints: Vector::default(),
            transmissions: Vector::default(),
            props: Map::default(),
            root: INVALID_ID,
            parent_of: Vector::default(),
            joint_from_parent: Vector::default(),
            children_of: Vector::default(),
        }
    }
}

impl Model {
    pub fn add_link(&mut self, link: Link) -> u32 {
        let id = self.links.len() as u32;
        self.links.push(link);
        self.parent_of.push(INVALID_ID);
        self.joint_from_parent.push(INVALID_ID);
        self.children_of.push(Vector::default());
        if self.root == INVALID_ID {
            self.root = id;
        }
        id
    }

    pub fn add_joint(&mut self, joint: Joint) -> u32 {
        let id = self.joints.len() as u32;
        self.joints.push(joint);
        id
    }

    pub fn connect(&mut self, parent: u32, child: u32, joint_id: u32) {
        self.joints[joint_id as usize].parent = parent;
        self.joints[joint_id as usize].child = child;
        self.parent_of[child as usize] = parent;
        self.joint_from_parent[child as usize] = joint_id;
        self.children_of[parent as usize].push(child);
    }

    pub fn num_links(&self) -> usize {
        self.links.len()
    }

    pub fn num_joints(&self) -> usize {
        self.joints.len()
    }

    pub fn is_valid_link(&self, id: u32) -> bool {
        (id as usize) < self.links.len()
    }

    pub fn is_valid_joint(&self, id: u32) -> bool {
        (id as usize) < self.joints.len()
    }

    pub fn get_parent(&self, link_id: u32) -> u32 {
        self.parent_of
            .get(link_id as usize)
            .copied()
            .unwrap_or(INVALID_ID)
    }

    pub fn get_parent_joint(&self, link_id: u32) -> u32 {
        self.joint_from_parent
            .get(link_id as usize)
            .copied()
            .unwrap_or(INVALID_ID)
    }

    pub fn get_children(&self, link_id: u32) -> &[u32] {
        self.children_of
            .get(link_id as usize)
            .map(|children| children.as_slice())
            .unwrap_or(&[])
    }

    pub fn is_leaf(&self, link_id: u32) -> bool {
        self.children_of
            .get(link_id as usize)
            .is_some_and(Vector::is_empty)
    }

    pub fn is_root(&self, link_id: u32) -> bool {
        link_id == self.root
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Robot {
    pub id: Identity,
    pub model: Model,
    pub props: Map<String, String>,
}

pub type PointMap<V> = Map<PointKey, V>;
pub type PointSet = Set<PointKey>;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub struct PointKey {
    pub x_bits: u64,
    pub y_bits: u64,
    pub z_bits: u64,
}

impl From<Point> for PointKey {
    fn from(value: Point) -> Self {
        Self {
            x_bits: value.x.to_bits(),
            y_bits: value.y.to_bits(),
            z_bits: value.z.to_bits(),
        }
    }
}

#[allow(dead_code)]
fn _keep_upstream_surface_visible(_geo: Geo, _aabb: Aabb, _obb: Obb) {}
