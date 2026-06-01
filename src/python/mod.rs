//! Python bindings for datapod (pyo3).

mod fixed;
mod geometry;
mod heap;

use pyo3::prelude::*;
use pyo3::types::PyModule;

pub fn register_python_module(m: &Bound<'_, PyModule>) -> PyResult<()> {
    geometry::register(m)?;
    fixed::register(m)?;
    heap::register(m)?;
    add_aliases(m)?;
    m.add("__version__", env!("CARGO_PKG_VERSION"))?;
    Ok(())
}

fn add_aliases(m: &Bound<'_, PyModule>) -> PyResult<()> {
    for (alias, target) in [
        ("Bs", "BoundingSphere"),
        ("IP", "Ip"),
        ("UUID", "Uuid"),
        ("OMap", "Map"),
        ("OSet", "Set"),
        ("MaxHeap", "Heap"),
        ("PriorityQueue", "Heap"),
        ("Fifo", "Queue"),
    ] {
        let value = m.getattr(target)?;
        m.add(alias, value)?;
    }
    Ok(())
}

#[pymodule]
fn datapod(m: &Bound<'_, PyModule>) -> PyResult<()> {
    register_python_module(m)
}
