//! Gridded raster data (heap-bearing): 2-D [`Grid`] and 3-D voxel [`Layer`].

mod grid;
mod layer;

pub use grid::{Grid, GridHeader, GridView};
pub use layer::{Layer, LayerHeader, LayerView};
