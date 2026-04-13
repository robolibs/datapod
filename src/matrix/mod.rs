mod dynamic;
pub mod mat;
mod matrix;
mod scalar;
mod tensor;
mod vector;

pub mod math;

pub use dynamic::Dynamic;
pub use matrix::Matrix;
pub use scalar::Scalar;
pub use tensor::Tensor;
pub use vector::{
    Vector, Vector1, Vector2, Vector3, Vector3d, Vector3f, Vector4, Vector4d, Vector4f, Vector6,
    Vector6d, Vector6f,
};
