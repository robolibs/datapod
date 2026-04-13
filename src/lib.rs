//! Rust port of the `datapod` base library.
//!
//! The initial focus is the foundational type and geometry layers that
//! downstream crates such as `vectkit` depend on.

pub mod adapters;
pub mod associative;
pub mod lockfree;
pub mod matrix;
pub mod memory;
pub mod sequential;
pub mod spatial;
pub mod sugar;
pub mod temporal;
pub mod trees;
pub mod types;

pub use matrix::mat;
pub use sequential::Vector;
pub use spatial::{Aabb, Euler, Geo, Linestring, Obb, Point, Polygon, Quaternion, Segment, Size};
pub use types::{boolean, byte, f32, f64, i8, i16, i32, i64, isize, u8, u16, u32, u64, usize};
