//! Python bindings for datapod (pyo3).

mod fixed;
mod geometry;
mod heap;

use std::ffi::CString;

use pyo3::prelude::*;
use pyo3::types::{PyAny, PyDict, PyModule};

const REGISTERED_TYPE_NAMES: &[&str] = &[
    "Envelope",
    "Encoding",
    "Point",
    "Geo",
    "Segment",
    "Polygon",
    "Euler",
    "Quaternion",
    "Velocity",
    "Acceleration",
    "Pose",
    "Transform",
    "State",
    "Loc",
    "Utm",
    "Line",
    "Rectangle",
    "Aabb",
    "BoundingSphere",
    "Circle",
    "Triangle",
    "Twist",
    "Wrench",
    "Odom",
    "JointLimits",
    "Inertial",
    "Uuid",
    "Ip",
    "MacAddr",
    "Bytes",
    "DpStr",
    "DpString",
    "Linestring",
    "MultiPoint",
    "Ring",
    "Path",
    "Trajectory",
    "Grid",
    "Layer",
    "Map",
    "Set",
    "Vector",
    "Matrix",
    "Tensor",
    "BitVec",
    "Deque",
    "Queue",
    "Stack",
    "List",
    "ForwardList",
    "Heap",
    "IndexedHeap",
    "Vecvec",
    "PagedVecvec",
    "PointKey",
    "Size",
    "Square",
    "Obb",
    "Box",
    "GaussianPoint",
    "GaussianCircle",
    "GaussianRectangle",
    "GaussianBox",
    "Accel",
    "JointDynamics",
    "JointMimic",
    "JointSafetyController",
    "JointCalibration",
    "KV",
    "BoxShape",
    "SphereShape",
    "CylinderShape",
    "MeshShape",
    "GeometryKind",
    "Geometry",
    "Identity",
    "Model",
    "Material",
    "Visual",
    "Collision",
    "JointType",
    "Joint",
    "Link",
    "Sensor",
    "Robot",
    "Actuator",
    "TransmissionJoint",
    "Transmission",
];

pub fn register_python_module(m: &Bound<'_, PyModule>) -> PyResult<()> {
    geometry::register(m)?;
    fixed::register(m)?;
    heap::register(m)?;
    add_aliases(m)?;
    add_registry(m)?;
    m.add_function(wrap_pyfunction!(to_wire_message, m)?)?;
    m.add_function(wrap_pyfunction!(decode_as, m)?)?;
    m.add_function(wrap_pyfunction!(from_wire_message, m)?)?;
    m.add_function(wrap_pyfunction!(type_hash_name, m)?)?;
    m.add_function(wrap_pyfunction!(register_schema, m)?)?;
    m.add_function(wrap_pyfunction!(register_type, m)?)?;
    m.add_function(wrap_pyfunction!(header_size, m)?)?;
    m.add_function(wrap_pyfunction!(payload_kind, m)?)?;
    m.add_function(wrap_pyfunction!(join_wire_message, m)?)?;
    m.add_function(wrap_pyfunction!(split_wire_message, m)?)?;
    add_declarative_layout(m)?;
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

fn add_registry(m: &Bound<'_, PyModule>) -> PyResult<()> {
    let registry = PyDict::new(m.py());
    for name in REGISTERED_TYPE_NAMES {
        let Ok(class) = m.getattr(*name) else {
            continue;
        };
        let Ok(type_hash) = class.getattr("TYPE_HASH").and_then(|h| h.extract::<u64>()) else {
            continue;
        };
        registry.set_item(type_hash, class)?;
    }
    m.add("__datapod_registry__", registry)?;
    Ok(())
}

fn add_declarative_layout(m: &Bound<'_, PyModule>) -> PyResult<()> {
    let py = m.py();
    let code = CString::new(
        r#"
import struct as _datapod_struct


def datapod_type(canonical_name, header_format, fields=None, *, payload_field=None, type_hash=None):
    """Declare a Python datapod schema and auto-generate wire methods.

    Example:

        @datapod.datapod_type("acme.packet.v1", "<HH", ("channel", "flags"), payload_field="data")
        class Packet:
            ...
    """
    header_size = _datapod_struct.calcsize(header_format)

    def decorate(cls):
        local_fields = fields
        if local_fields is None:
            local_fields = getattr(cls, "__datapod_fields__", None)
        if local_fields is None:
            annotations = getattr(cls, "__annotations__", {})
            local_fields = tuple(name for name in annotations if name != payload_field)
        local_fields = tuple(local_fields)
        if not local_fields and header_size:
            raise TypeError("datapod_type needs header fields for a non-empty header")

        kind = "bytes" if payload_field is not None else "fixed"
        type_id = register_type(cls, canonical_name, header_size, kind, type_hash)

        cls.__datapod_canonical_name__ = canonical_name
        cls.__datapod_header_format__ = header_format
        cls.__datapod_fields__ = local_fields
        cls.__datapod_payload_field__ = payload_field

        def to_wire_message(self):
            values = [getattr(self, name) for name in local_fields]
            header = _datapod_struct.pack(header_format, *values)
            payload = b"" if payload_field is None else bytes(getattr(self, payload_field))
            return type_id, join_wire_message(type_id, header, payload)

        @classmethod
        def from_wire_message(inner_cls, incoming_hash, wire):
            if incoming_hash != type_id:
                raise ValueError(f"wrong type hash: got {incoming_hash}, expected {type_id}")
            header, payload = split_wire_message(incoming_hash, wire)
            values = _datapod_struct.unpack(header_format, header)
            kwargs = dict(zip(local_fields, values))
            if payload_field is not None:
                kwargs[payload_field] = payload
            try:
                return inner_cls(**kwargs)
            except TypeError:
                obj = inner_cls.__new__(inner_cls)
                for name, value in kwargs.items():
                    setattr(obj, name, value)
                return obj

        cls.to_wire_message = to_wire_message
        cls.from_wire_message = from_wire_message
        return cls

    return decorate
"#,
    )
    .expect("declarative datapod Python source contains no NULs");
    let file = CString::new("<datapod declarative layout>")
        .expect("declarative datapod file name contains no NULs");
    let name = CString::new("_datapod_declarative").expect("module name contains no NULs");
    let module = PyModule::from_code(py, code.as_c_str(), file.as_c_str(), name.as_c_str())?;
    module.add("register_type", m.getattr("register_type")?)?;
    module.add("join_wire_message", m.getattr("join_wire_message")?)?;
    module.add("split_wire_message", m.getattr("split_wire_message")?)?;
    m.add("datapod_type", module.getattr("datapod_type")?)?;
    Ok(())
}

#[pyfunction]
fn to_wire_message(obj: &Bound<'_, PyAny>) -> PyResult<(u64, Vec<u8>)> {
    obj.call_method0("to_wire_message")?.extract()
}

#[pyfunction]
fn decode_as(cls: &Bound<'_, PyAny>, type_hash: u64, wire: Vec<u8>) -> PyResult<PyObject> {
    Ok(cls
        .call_method1("from_wire_message", (type_hash, wire))?
        .unbind())
}

#[pyfunction]
fn from_wire_message(py: Python<'_>, type_hash: u64, wire: Vec<u8>) -> PyResult<PyObject> {
    let module = PyModule::import(py, "datapod")?;
    let registry = module
        .getattr("__datapod_registry__")?
        .downcast_into::<PyDict>()?;
    let class = registry.get_item(type_hash)?.ok_or_else(|| {
        pyo3::exceptions::PyValueError::new_err(format!("unknown datapod type hash: {type_hash}"))
    })?;
    Ok(class
        .call_method1("from_wire_message", (type_hash, wire))?
        .unbind())
}

#[pyfunction]
fn type_hash_name(canonical_name: &str) -> u64 {
    crate::registry::type_hash_name(canonical_name)
}

fn parse_payload_kind(payload_kind: &str) -> PyResult<crate::registry::PayloadKind> {
    match payload_kind {
        "fixed" | "Fixed" | "FIXED" => Ok(crate::registry::PayloadKind::Fixed),
        "bytes" | "Bytes" | "BYTES" => Ok(crate::registry::PayloadKind::Bytes),
        other => Err(pyo3::exceptions::PyValueError::new_err(format!(
            "invalid datapod payload kind {other:?}; expected 'fixed' or 'bytes'"
        ))),
    }
}

fn payload_kind_name(payload_kind: crate::registry::PayloadKind) -> &'static str {
    match payload_kind {
        crate::registry::PayloadKind::Fixed => "fixed",
        crate::registry::PayloadKind::Bytes => "bytes",
    }
}

fn type_info(type_hash: u64) -> PyResult<crate::registry::TypeInfo> {
    crate::registry::find_type_info(type_hash).ok_or_else(|| {
        pyo3::exceptions::PyValueError::new_err(format!("unknown datapod type hash: {type_hash}"))
    })
}

#[pyfunction]
#[pyo3(signature = (canonical_name, header_size, payload_kind, type_hash=None))]
fn register_schema(
    canonical_name: &str,
    header_size: usize,
    payload_kind: &str,
    type_hash: Option<u64>,
) -> PyResult<u64> {
    let payload_kind = parse_payload_kind(payload_kind)?;
    let type_hash = type_hash.unwrap_or_else(|| crate::registry::type_hash_name(canonical_name));
    crate::registry::register_type(type_hash, canonical_name, header_size, payload_kind)
        .map_err(|error| pyo3::exceptions::PyValueError::new_err(error.to_string()))?;
    Ok(type_hash)
}

#[pyfunction]
#[pyo3(signature = (cls, canonical_name, header_size, payload_kind, type_hash=None))]
fn register_type(
    py: Python<'_>,
    cls: &Bound<'_, PyAny>,
    canonical_name: &str,
    header_size: usize,
    payload_kind: &str,
    type_hash: Option<u64>,
) -> PyResult<u64> {
    let payload_kind = parse_payload_kind(payload_kind)?;
    let type_hash = register_schema(
        canonical_name,
        header_size,
        payload_kind_name(payload_kind),
        type_hash,
    )?;

    cls.setattr("TYPE_HASH", type_hash)?;
    cls.setattr("__datapod_canonical_name__", canonical_name)?;
    cls.setattr("__datapod_header_size__", header_size)?;
    cls.setattr("__datapod_payload_kind__", payload_kind_name(payload_kind))?;

    let module = PyModule::import(py, "datapod")?;
    let registry = module
        .getattr("__datapod_registry__")?
        .downcast_into::<PyDict>()?;
    registry.set_item(type_hash, cls)?;
    Ok(type_hash)
}

#[pyfunction]
fn header_size(type_hash: u64) -> PyResult<usize> {
    Ok(type_info(type_hash)?.header_size)
}

#[pyfunction]
fn payload_kind(type_hash: u64) -> PyResult<&'static str> {
    Ok(payload_kind_name(type_info(type_hash)?.payload_kind))
}

#[pyfunction]
#[pyo3(signature = (type_hash, header, payload=None))]
fn join_wire_message(
    type_hash: u64,
    header: Vec<u8>,
    payload: Option<Vec<u8>>,
) -> PyResult<Vec<u8>> {
    let info = type_info(type_hash)?;
    if header.len() != info.header_size {
        return Err(pyo3::exceptions::PyValueError::new_err(format!(
            "wrong header length: got {}, expected {}",
            header.len(),
            info.header_size
        )));
    }
    let payload = payload.unwrap_or_default();
    let mut wire = Vec::with_capacity(header.len() + payload.len());
    wire.extend_from_slice(&header);
    wire.extend_from_slice(&payload);
    Ok(wire)
}

#[pyfunction]
fn split_wire_message(type_hash: u64, wire: Vec<u8>) -> PyResult<(Vec<u8>, Vec<u8>)> {
    let info = type_info(type_hash)?;
    if wire.len() < info.header_size {
        return Err(pyo3::exceptions::PyValueError::new_err(format!(
            "wire message too short: got {}, need at least {}",
            wire.len(),
            info.header_size
        )));
    }
    let payload = wire[info.header_size..].to_vec();
    let header = wire[..info.header_size].to_vec();
    Ok((header, payload))
}

#[pymodule]
fn datapod(m: &Bound<'_, PyModule>) -> PyResult<()> {
    register_python_module(m)
}
