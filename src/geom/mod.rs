//! Geometric types. Fixed-Pod (Point, Segment) and heap-bearing (Polygon,
//! Linestring, Ring, Path, Trajectory, MultiPoint) live side-by-side here.
//! 2-D / 3-D fixed-Pod shapes live in [`shapes`].

mod linestring;
mod multi_point;
mod path;
mod point;
mod polygon;
mod ring;
mod segment;
mod trajectory;

pub mod shapes;

pub use linestring::{Linestring, LinestringHeader};
pub use multi_point::{MultiPoint, MultiPointHeader};
pub use path::{Path, PathHeader};
pub use point::{Point, PointKey, PointMap, PointSet};
pub use polygon::{Polygon, PolygonHeader};
pub use ring::{Ring, RingHeader};
pub use segment::Segment;
pub use trajectory::{Trajectory, TrajectoryHeader};
