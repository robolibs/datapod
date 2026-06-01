//! Python bindings for datapod (pyo3).

mod geometry;

use pyo3::prelude::*;
use pyo3::types::PyModule;

pub fn register_python_module(m: &Bound<'_, PyModule>) -> PyResult<()> {
    geometry::register(m)?;
    m.add("__version__", env!("CARGO_PKG_VERSION"))?;
    Ok(())
}

#[pymodule]
fn datapod(m: &Bound<'_, PyModule>) -> PyResult<()> {
    register_python_module(m)
}
