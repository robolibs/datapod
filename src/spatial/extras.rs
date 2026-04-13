use crate::Vector;
use crate::associative::{Map, Set};
use crate::matrix::mat;

use super::{Aabb, Euler, Geo, Linestring, Obb, Point, Polygon, Quaternion, Segment, Size};

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Velocity {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Acceleration {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Pose {
    pub position: Point,
    pub orientation: Quaternion,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Transform {
    pub translation: Point,
    pub rotation: Quaternion,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct State {
    pub pose: Pose,
    pub velocity: Velocity,
    pub acceleration: Acceleration,
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
    pub point: Point,
    pub heading: Euler,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct BoundingSphere {
    pub center: Point,
    pub radius: f64,
}

pub type Bs = BoundingSphere;

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Box {
    pub min: Point,
    pub max: Point,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Circle {
    pub center: Point,
    pub radius: f64,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Line {
    pub point: Point,
    pub direction: Point,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Rectangle {
    pub center: Point,
    pub size: Size,
    pub rotation: Euler,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Square {
    pub center: Point,
    pub side: f64,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Triangle {
    pub a: Point,
    pub b: Point,
    pub c: Point,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Ring {
    pub points: Vector<Point>,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Grid<T = f64> {
    pub rows: usize,
    pub cols: usize,
    pub resolution: f64,
    pub values: Vector<T>,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Layer<T = f64> {
    pub name: String,
    pub values: Vector<T>,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Path {
    pub points: Vector<Point>,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Trajectory {
    pub poses: Vector<Pose>,
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

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Identity {
    pub id: u64,
}

#[derive(Debug, Clone, PartialEq)]
pub enum Geometry {
    Point(Point),
    Segment(Segment),
    Polygon(Polygon),
    Circle(Circle),
    Rectangle(Rectangle),
    Triangle(Triangle),
    Box(Box),
    Sphere(BoundingSphere),
    Path(Path),
}

impl Default for Geometry {
    fn default() -> Self {
        Self::Point(Point::default())
    }
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Visual {
    pub name: String,
    pub geometry: Geometry,
    pub pose: Pose,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Collision {
    pub name: String,
    pub geometry: Geometry,
    pub pose: Pose,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Inertial {
    pub mass: f64,
    pub inertia: mat::Matrix<f64, 3, 3>,
    pub pose: Pose,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Sensor {
    pub name: String,
    pub sensor_type: String,
    pub pose: Pose,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Joint {
    pub name: String,
    pub parent: String,
    pub child: String,
    pub transform: Transform,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Link {
    pub name: String,
    pub visuals: Vector<Visual>,
    pub collisions: Vector<Collision>,
    pub inertial: Option<Inertial>,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Transmission {
    pub name: String,
    pub joints: Vector<String>,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Twist {
    pub linear: Velocity,
    pub angular: Velocity,
}

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Wrench {
    pub force: Point,
    pub torque: Point,
}

pub type Accel = Acceleration;

#[derive(Debug, Clone, Copy, PartialEq, Default)]
pub struct Odom {
    pub pose: Pose,
    pub twist: Twist,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Model {
    pub name: String,
    pub links: Vector<Link>,
    pub joints: Vector<Joint>,
}

#[derive(Debug, Clone, PartialEq, Default)]
pub struct Robot {
    pub name: String,
    pub model: Model,
    pub sensors: Vector<Sensor>,
    pub transmissions: Vector<Transmission>,
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
