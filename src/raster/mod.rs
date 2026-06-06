//! Gridded raster data (heap-bearing): 2-D [`Grid`] and 3-D voxel [`Layer`].

mod grid;
mod layer;

pub use grid::{Grid, GridHeader, GridView};
pub use layer::{Layer, LayerHeader, LayerView};

use crate::WireError;

pub(super) fn checked_world_axis<T: 'static>(
    local: f64,
    resolution: f64,
    max_index: u32,
    label: &'static str,
) -> Result<usize, WireError> {
    let value = ((local / resolution) - 0.5).round();
    if !value.is_finite() {
        return Err(crate::wire::invalid_payload::<T>(format!(
            "{label} coordinate is not finite"
        )));
    }
    let index = rounded_axis_index_u32::<T>(value, max_index, label)?;
    usize::try_from(index).map_err(|_| {
        crate::wire::invalid_header::<T>(format!("{label} index does not fit in usize"))
    })
}

fn rounded_axis_index_u32<T: 'static>(
    value: f64,
    max_index: u32,
    label: &'static str,
) -> Result<u32, WireError> {
    if value <= 0.0 {
        return Ok(0);
    }
    if value >= f64::from(max_index) {
        return Ok(max_index);
    }

    let mut low = 0u32;
    let mut high = max_index;
    while low < high {
        let mid = low + ((high - low) / 2);
        if f64::from(mid) < value {
            low = mid.checked_add(1).ok_or_else(|| {
                crate::wire::invalid_payload::<T>(format!(
                    "{label} index overflowed during conversion"
                ))
            })?;
        } else {
            high = mid;
        }
    }
    Ok(low)
}
