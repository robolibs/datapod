//! Fixed-Pod 2-D / 3-D shapes — used as geometric primitives that fit
//! entirely on the stack and ride the wire as their own header.

mod aabb;
mod bounding_sphere;
mod r#box;
mod circle;
mod gaussian;
mod line;
mod obb;
mod rectangle;
mod size;
mod square;
mod triangle;

pub use aabb::Aabb;
pub use bounding_sphere::BoundingSphere;
pub use r#box::Box;
pub use circle::Circle;
pub use gaussian::{GaussianBox, GaussianCircle, GaussianPoint, GaussianRectangle};
pub use line::Line;
pub use obb::Obb;
pub use rectangle::Rectangle;
pub use size::Size;
pub use square::Square;
pub use triangle::Triangle;
