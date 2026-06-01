use pyo3::exceptions::PyIndexError;
use pyo3::prelude::*;
use pyo3::types::PyModule;

use crate::{Geo, Point, Polygon, Segment};

#[pyclass(name = "Point")]
#[derive(Clone, Copy)]
pub struct PyPoint {
    #[pyo3(get, set)]
    pub x: f64,
    #[pyo3(get, set)]
    pub y: f64,
    #[pyo3(get, set)]
    pub z: f64,
}

impl From<Point> for PyPoint {
    fn from(value: Point) -> Self {
        Self {
            x: value.x,
            y: value.y,
            z: value.z,
        }
    }
}

impl From<PyPoint> for Point {
    fn from(value: PyPoint) -> Self {
        Self::new(value.x, value.y, value.z)
    }
}

#[pymethods]
impl PyPoint {
    #[new]
    #[pyo3(signature = (x=0.0, y=0.0, z=0.0))]
    fn new(x: f64, y: f64, z: f64) -> Self {
        Self { x, y, z }
    }

    fn magnitude(&self) -> f64 {
        Point::from(*self).magnitude()
    }

    fn distance_to(&self, other: PyRef<'_, PyPoint>) -> f64 {
        Point::from(*self).distance_to(Point::from(*other))
    }

    fn distance_to_2d(&self, other: PyRef<'_, PyPoint>) -> f64 {
        Point::from(*self).distance_to_2d(Point::from(*other))
    }

    fn is_set(&self) -> bool {
        Point::from(*self).is_set()
    }

    fn __repr__(&self) -> String {
        format!("Point(x={}, y={}, z={})", self.x, self.y, self.z)
    }
}

#[pyclass(name = "Geo")]
#[derive(Clone, Copy)]
pub struct PyGeo {
    #[pyo3(get, set)]
    pub latitude: f64,
    #[pyo3(get, set)]
    pub longitude: f64,
    #[pyo3(get, set)]
    pub altitude: f64,
}

impl From<Geo> for PyGeo {
    fn from(value: Geo) -> Self {
        Self {
            latitude: value.latitude,
            longitude: value.longitude,
            altitude: value.altitude,
        }
    }
}

impl From<PyGeo> for Geo {
    fn from(value: PyGeo) -> Self {
        Self::new(value.latitude, value.longitude, value.altitude)
    }
}

#[pymethods]
impl PyGeo {
    #[new]
    #[pyo3(signature = (latitude=0.0, longitude=0.0, altitude=0.0))]
    fn new(latitude: f64, longitude: f64, altitude: f64) -> Self {
        Self {
            latitude,
            longitude,
            altitude,
        }
    }

    fn is_valid(&self) -> bool {
        Geo::from(*self).is_valid()
    }

    fn is_set(&self) -> bool {
        Geo::from(*self).is_set()
    }

    fn has_altitude(&self) -> bool {
        Geo::from(*self).has_altitude()
    }

    fn distance_to(&self, other: PyRef<'_, PyGeo>) -> f64 {
        Geo::from(*self).distance_to(Geo::from(*other))
    }

    fn bearing_to(&self, other: PyRef<'_, PyGeo>) -> f64 {
        Geo::from(*self).bearing_to(Geo::from(*other))
    }

    fn __repr__(&self) -> String {
        format!(
            "Geo(latitude={}, longitude={}, altitude={})",
            self.latitude, self.longitude, self.altitude
        )
    }
}

#[pyclass(name = "Segment")]
#[derive(Clone, Copy)]
pub struct PySegment {
    inner: Segment,
}

#[pymethods]
impl PySegment {
    #[new]
    fn new(start: PyRef<'_, PyPoint>, end: PyRef<'_, PyPoint>) -> Self {
        Self {
            inner: Segment::new(Point::from(*start), Point::from(*end)),
        }
    }

    #[getter]
    fn start(&self) -> PyPoint {
        self.inner.start.into()
    }

    #[getter]
    fn end(&self) -> PyPoint {
        self.inner.end.into()
    }

    fn length(&self) -> f64 {
        self.inner.length()
    }

    fn midpoint(&self) -> PyPoint {
        self.inner.midpoint().into()
    }

    fn closest_point(&self, point: PyRef<'_, PyPoint>) -> PyPoint {
        self.inner.closest_point(Point::from(*point)).into()
    }

    fn distance_to(&self, point: PyRef<'_, PyPoint>) -> f64 {
        self.inner.distance_to(Point::from(*point))
    }

    fn __repr__(&self) -> String {
        format!(
            "Segment(start=Point(x={}, y={}, z={}), end=Point(x={}, y={}, z={}))",
            self.inner.start.x,
            self.inner.start.y,
            self.inner.start.z,
            self.inner.end.x,
            self.inner.end.y,
            self.inner.end.z
        )
    }
}

#[pyclass(name = "Polygon")]
pub struct PyPolygon {
    inner: Polygon,
}

#[pymethods]
impl PyPolygon {
    #[new]
    fn new(vertices: Vec<(f64, f64, f64)>) -> Self {
        Self {
            inner: Polygon::new(
                vertices
                    .into_iter()
                    .map(|(x, y, z)| Point::new(x, y, z))
                    .collect(),
            ),
        }
    }

    #[staticmethod]
    fn from_xy(vertices: Vec<(f64, f64)>) -> Self {
        Self {
            inner: Polygon::new(
                vertices
                    .into_iter()
                    .map(|(x, y)| Point::new(x, y, 0.0))
                    .collect(),
            ),
        }
    }

    fn len(&self) -> usize {
        self.inner.num_vertices()
    }

    fn is_empty(&self) -> bool {
        self.inner.empty()
    }

    fn is_valid(&self) -> bool {
        self.inner.is_valid()
    }

    fn area(&self) -> f64 {
        self.inner.area()
    }

    fn perimeter(&self) -> f64 {
        self.inner.perimeter()
    }

    fn contains(&self, point: PyRef<'_, PyPoint>) -> bool {
        self.inner.contains(Point::from(*point))
    }

    fn vertex(&self, index: usize) -> PyResult<PyPoint> {
        self.inner
            .vertices
            .get(index)
            .copied()
            .map(PyPoint::from)
            .ok_or_else(|| PyIndexError::new_err("polygon vertex index out of range"))
    }

    fn vertices(&self) -> Vec<PyPoint> {
        self.inner
            .vertices
            .iter()
            .copied()
            .map(PyPoint::from)
            .collect()
    }

    fn __len__(&self) -> usize {
        self.inner.num_vertices()
    }
}

pub(super) fn register(m: &Bound<'_, PyModule>) -> PyResult<()> {
    m.add_class::<PyPoint>()?;
    m.add_class::<PyGeo>()?;
    m.add_class::<PySegment>()?;
    m.add_class::<PyPolygon>()?;
    Ok(())
}
