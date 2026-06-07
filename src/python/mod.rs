//! Python bindings for datapod (pyo3).

mod fixed;
mod geometry;
mod heap;

use std::cell::RefCell;
use std::ffi::CString;

use pyo3::exceptions::PyValueError;
use pyo3::prelude::*;
use pyo3::types::{PyAny, PyBool, PyBytes, PyDict, PyModule, PyTuple};

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

thread_local! {
    static SOURCE_VALIDATED_DECLARATIVE_MESSAGE: RefCell<Option<Py<PyAny>>> =
        const { RefCell::new(None) };
    static RUN_REGISTERED_DECLARATIVE_VALIDATOR: RefCell<Option<Py<PyAny>>> =
        const { RefCell::new(None) };
}

pub fn register_python_module(m: &Bound<'_, PyModule>) -> PyResult<()> {
    geometry::register(m)?;
    fixed::register(m)?;
    heap::register(m)?;
    add_aliases(m)?;
    add_registry(m)?;
    m.add_function(wrap_pyfunction!(to_wire_message, m)?)?;
    m.add_function(wrap_pyfunction!(to_wire_message_v1, m)?)?;
    m.add_function(wrap_pyfunction!(decode_as, m)?)?;
    m.add_function(wrap_pyfunction!(from_wire_message, m)?)?;
    m.add_function(wrap_pyfunction!(type_hash_name, m)?)?;
    m.add_function(wrap_pyfunction!(canonical_type_hash, m)?)?;
    m.add_function(wrap_pyfunction!(emitted_type_hash, m)?)?;
    m.add_function(wrap_pyfunction!(register_schema, m)?)?;
    m.add_function(wrap_pyfunction!(register_type, m)?)?;
    m.add_function(wrap_pyfunction!(header_size, m)?)?;
    m.add_function(wrap_pyfunction!(header_size_v1, m)?)?;
    m.add_function(wrap_pyfunction!(payload_kind, m)?)?;
    m.add_function(wrap_pyfunction!(format_version, m)?)?;
    m.add_function(wrap_pyfunction!(wire_format, m)?)?;
    m.add_function(wrap_pyfunction!(current_wire_format, m)?)?;
    m.add_function(wrap_pyfunction!(builtin_hash_policy, m)?)?;
    m.add_function(wrap_pyfunction!(endian, m)?)?;
    m.add_function(wrap_pyfunction!(alignment_policy, m)?)?;
    m.add_function(wrap_pyfunction!(validator_kind, m)?)?;
    m.add_function(wrap_pyfunction!(emitted_hash_kind, m)?)?;
    m.add_function(wrap_pyfunction!(has_archive, m)?)?;
    m.add_function(wrap_pyfunction!(has_view, m)?)?;
    m.add_function(wrap_pyfunction!(has_owned_decode, m)?)?;
    m.add_function(wrap_pyfunction!(archive_shape, m)?)?;
    m.add_function(wrap_pyfunction!(join_wire_message, m)?)?;
    m.add_function(wrap_pyfunction!(split_wire_message, m)?)?;
    m.add_function(wrap_pyfunction!(split_wire_message_v1, m)?)?;
    m.add_function(wrap_pyfunction!(validate_wire_message, m)?)?;
    m.add_function(wrap_pyfunction!(validate_wire_message_v1, m)?)?;
    m.add_function(wrap_pyfunction!(validate_wire_frame_parts_v1, m)?)?;
    m.add_function(wrap_pyfunction!(schema_for_hash, m)?)?;
    add_declarative_layout(m)?;
    m.add("__version__", env!("CARGO_PKG_VERSION"))?;
    Ok(())
}

fn add_aliases(m: &Bound<'_, PyModule>) -> PyResult<()> {
    for (alias, target) in [
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
        registry.set_item(type_hash, &class)?;
        if let Some(info) = crate::registry::try_find_type_info(type_hash)
            .map_err(|error| pyo3::exceptions::PyValueError::new_err(error.to_string()))?
        {
            registry.set_item(info.canonical_type_hash, class)?;
        }
    }
    m.add("__datapod_registry__", registry)?;
    Ok(())
}

fn add_declarative_layout(m: &Bound<'_, PyModule>) -> PyResult<()> {
    let py = m.py();
    let code = CString::new(
        r#"
import struct as _datapod_struct
import dataclasses as _datapod_dataclasses
import inspect as _datapod_inspect
import operator as _datapod_operator
import sys as _datapod_sys
import typing as _datapod_typing

def _make_generated_hook_registry():
    registries = {
        "frame_encoder": set(),
        "message_encoder": set(),
        "wire_decoder": set(),
    }

    def register(kind, func):
        registries[kind].add(func)
        return func

    def contains(kind, func):
        return func in registries[kind]

    return register, contains


_register_generated_hook, _is_generated_hook = _make_generated_hook_registry()
_MISSING = object()

def _strict_index(value, label, description):
    if isinstance(value, bool):
        raise TypeError(f"{label} must be {description}, not bool")
    try:
        raw = value.__index__()
    except Exception as error:
        raise TypeError(f"{label} must be {description}") from error
    if isinstance(raw, bool):
        raise TypeError(f"{label} __index__ must not return bool")
    try:
        return _datapod_operator.index(raw)
    except Exception as error:
        raise TypeError(f"{label} must be {description}") from error


def _make_declarative_validator_registry():
    registry = {}

    def register(
        type_hash,
        canonical_type_hash,
        cls,
        unpack_kwargs,
        payload_field,
        validate_object,
    ):
        entry = (cls, unpack_kwargs, payload_field, validate_object)
        registry[type_hash] = entry
        registry[canonical_type_hash] = entry

    def get(type_hash):
        return registry.get(type_hash)

    return register, get


(
    _register_declarative_validator_type,
    _registered_declarative_validator_type,
) = _make_declarative_validator_registry()


class _DatapodScalar:
    __slots__ = ("name", "format")

    def __init__(self, name, format):
        self.name = name
        self.format = format

    def __repr__(self):
        return f"datapod.{self.name}"


class _DatapodArray:
    __slots__ = ("item", "count")

    def __init__(self, item, count):
        count = _strict_index(count, "datapod.array count", "a positive int")
        if count <= 0:
            raise TypeError("datapod.array count must be a positive int")
        if count > _datapod_sys.maxsize:
            raise ValueError("datapod.array count exceeds maximum supported length")
        self.item = item
        self.count = count

    def __repr__(self):
        return f"datapod.array({self.item!r}, {self.count})"


u8 = _DatapodScalar("u8", "B")
u16 = _DatapodScalar("u16", "H")
u32 = _DatapodScalar("u32", "I")
u64 = _DatapodScalar("u64", "Q")
i8 = _DatapodScalar("i8", "b")
i16 = _DatapodScalar("i16", "h")
i32 = _DatapodScalar("i32", "i")
i64 = _DatapodScalar("i64", "q")
f32 = _DatapodScalar("f32", "f")
f64 = _DatapodScalar("f64", "d")

_DATAPOD_SCALARS = {
    scalar.name: scalar
    for scalar in (u8, u16, u32, u64, i8, i16, i32, i64, f32, f64)
}


def array(item, count):
    """Fixed-length array annotation for declarative datapod header fields."""
    return _DatapodArray(item, count)


def _annotation_name(annotation):
    if isinstance(annotation, str):
        return annotation.strip("'\"").rsplit(".", 1)[-1]
    return _metadata_attr(annotation, "__name__", None)


def _format_body(format):
    if format and format[0] in "@=<>!":
        return format[1:]
    return format


def _struct_value_count(format):
    body = _format_body(format)
    total = 0
    i = 0
    value_codes = set("bB?hHiIlLqQefd")
    while i < len(body):
        if body[i].isspace():
            i += 1
            continue
        repeat = 0
        saw_repeat = False
        while i < len(body) and body[i].isdigit():
            saw_repeat = True
            repeat = repeat * 10 + int(body[i])
            i += 1
        if i >= len(body):
            raise TypeError(f"invalid struct header format {format!r}: missing format code")
        code = body[i]
        i += 1
        repeat = repeat if saw_repeat else 1
        if code == "x":
            continue
        if code in value_codes:
            total += repeat
            continue
        raise TypeError(
            f"unsupported struct header format code {code!r}; "
            "datapod declarative headers support fixed-size scalar fields only"
        )
    return total


def _is_fixed_datapod_annotation(annotation):
    header_format = _metadata_attr(annotation, "__datapod_header_format__")
    if header_format is _MISSING:
        return False
    return _metadata_attr(annotation, "__datapod_payload_field__", None) is None


def _resolve_header_annotation(annotation):
    try:
        origin = _datapod_typing.get_origin(annotation)
    except Exception as error:
        raise ValueError("datapod annotation metadata lookup failed") from error
    if origin is _datapod_typing.Annotated:
        base, *metadata = _datapod_typing.get_args(annotation)
        for item in metadata:
            try:
                _scalar_format(item)
                return item
            except TypeError:
                pass
        return _resolve_header_annotation(base)
    return annotation


def _string_array_parts(annotation):
    if not isinstance(annotation, str):
        return None
    text = annotation.strip("'\"").replace(" ", "")
    for prefix in ("datapod.array(", "array("):
        if text.startswith(prefix) and text.endswith(")"):
            body = text[len(prefix):-1]
            try:
                item, count = body.rsplit(",", 1)
            except ValueError as error:
                raise TypeError(
                    "datapod.array annotation must include item and count"
                ) from error
            if not item:
                raise TypeError("datapod.array item type must not be empty")
            try:
                count = int(count)
            except ValueError as error:
                raise TypeError("datapod.array count must be a positive int") from error
            if count <= 0:
                raise TypeError("datapod.array count must be a positive int")
            if count > _datapod_sys.maxsize:
                raise ValueError("datapod.array count exceeds maximum supported length")
            return item, count
    return None


def _scalar_format(annotation):
    annotation = _resolve_header_annotation(annotation)
    if isinstance(annotation, _DatapodArray):
        return f"{annotation.count}{_scalar_format(annotation.item)}"
    array_parts = _string_array_parts(annotation)
    if array_parts is not None:
        item, count = array_parts
        return f"{count}{_scalar_format(item)}"
    if isinstance(annotation, _DatapodScalar):
        return annotation.format
    if _is_fixed_datapod_annotation(annotation):
        return _format_body(_metadata_attr(annotation, "__datapod_header_format__"))
    name = _annotation_name(annotation)
    if name in _DATAPOD_SCALARS:
        return _DATAPOD_SCALARS[name].format
    if annotation is bool or name == "bool":
        return "?"
    raise TypeError(
        "datapod_type cannot infer a fixed-width header format for annotation "
        f"{annotation!r}; use datapod.u8/u16/u32/u64/i8/i16/i32/i64/f32/f64 "
        "or pass header_format/header explicitly"
    )


def _field_arity(annotation):
    annotation = _resolve_header_annotation(annotation)
    if isinstance(annotation, _DatapodArray):
        return annotation.count
    array_parts = _string_array_parts(annotation)
    if array_parts is not None:
        return array_parts[1]
    if _is_fixed_datapod_annotation(annotation):
        arity = _metadata_attr(annotation, "__datapod_header_arity__", None)
        if arity is None:
            return _struct_value_count(
                _metadata_attr(annotation, "__datapod_header_format__")
            )
        arity = _strict_index(arity, "datapod annotation header arity", "a positive integer")
        if arity <= 0:
            raise ValueError("datapod annotation header arity must be positive")
        return arity
    return 1


def _field_names_tuple(fields, label):
    if isinstance(fields, str):
        raise TypeError(f"datapod_type {label} must be an iterable of field-name strings, not str")
    try:
        names = tuple(fields)
    except TypeError as error:
        raise TypeError(f"datapod_type {label} must be an iterable of field-name strings") from error
    seen = set()
    for name in names:
        if not isinstance(name, str):
            raise TypeError(f"datapod_type {label} entries must be str")
        if name == "":
            raise TypeError(f"datapod_type {label} entries must not be empty")
        if name in seen:
            raise TypeError(f"datapod_type {label} entries must be unique")
        seen.add(name)
    return names


def _safe_type_name(obj):
    try:
        return type(obj).__name__
    except Exception:
        return "object"


def _safe_datapod_label(obj):
    try:
        is_type = isinstance(obj, type)
    except Exception:
        is_type = False
    if is_type:
        try:
            name = obj.__name__
        except Exception:
            return _safe_type_name(obj)
        return name if isinstance(name, str) else _safe_type_name(obj)
    return _safe_type_name(obj)


def _construct_from_kwargs(cls, kwargs):
    try:
        return cls(**kwargs)
    except TypeError as error:
        try:
            _datapod_inspect.signature(cls).bind(**kwargs)
        except TypeError:
            obj = _validation_shell(cls)
            for name, value in kwargs.items():
                _set_validation_field(obj, name, value)
            return obj
        except ValueError:
            raise error
        raise error


def _validation_shell(cls):
    try:
        return object.__new__(cls)
    except TypeError as error:
        raise ValueError(
            f"{_safe_type_name(cls)} cannot be allocated for declarative validation"
        ) from error


def _set_validation_field(obj, name, value):
    try:
        object.__setattr__(obj, name, value)
    except Exception as error:
        raise ValueError(
            f"{_safe_type_name(obj)} cannot set declarative field {name!r}"
        ) from error


def _pack_header_field(annotation, value, name, arity):
    annotation = _resolve_header_annotation(annotation)
    if _is_fixed_datapod_annotation(annotation):
        header = annotation.__datapod_pack_header__(value)
        return _datapod_struct.unpack(annotation.__datapod_header_format__, header)
    if isinstance(annotation, _DatapodArray) or _string_array_parts(annotation) is not None:
        try:
            iterator = iter(value)
        except Exception as error:
            raise ValueError(
                f"field {name} expected {arity} iterable items"
            ) from error
        items = []
        for index in range(arity):
            try:
                items.append(next(iterator))
            except StopIteration:
                raise ValueError(
                    f"field {name} expected {arity} items, got {len(items)}"
                ) from None
            except Exception as error:
                raise ValueError(
                    f"field {name} failed while reading array item {index}"
                ) from error
        try:
            next(iterator)
        except StopIteration:
            return tuple(items)
        except Exception as error:
            raise ValueError(
                f"field {name} failed while checking array length"
            ) from error
        raise ValueError(
            f"field {name} expected {arity} items, got more than {arity}"
        )
    return (value,)


def _unpack_header_field(annotation, values):
    annotation = _resolve_header_annotation(annotation)
    if _is_fixed_datapod_annotation(annotation):
        header = _datapod_struct.pack(annotation.__datapod_header_format__, *values)
        return annotation.__datapod_from_header_bytes__(header)
    if isinstance(annotation, _DatapodArray) or _string_array_parts(annotation) is not None:
        return tuple(values)
    return values[0]


def _annotation_schema(annotation):
    annotation = _resolve_header_annotation(annotation)
    if isinstance(annotation, _DatapodArray):
        return {
            "kind": "array",
            "count": annotation.count,
            "item": _annotation_schema(annotation.item),
            "repr": repr(annotation),
        }
    array_parts = _string_array_parts(annotation)
    if array_parts is not None:
        item, count = array_parts
        return {
            "kind": "array",
            "count": count,
            "item": _annotation_schema(item),
            "repr": annotation,
        }
    if isinstance(annotation, _DatapodScalar):
        return {"kind": "scalar", "name": annotation.name, "format": annotation.format}
    if _is_fixed_datapod_annotation(annotation):
        return {
            "kind": "datapod",
            "canonical_name": _metadata_attr(annotation, "__datapod_canonical_name__"),
            "type_hash": _u64_type_hash(
                _metadata_attr(annotation, "TYPE_HASH"),
                "annotation TYPE_HASH",
            ),
            "header_format": _metadata_attr(annotation, "__datapod_header_format__"),
            "fields": _metadata_attr(annotation, "__datapod_fields__"),
        }
    name = _annotation_name(annotation)
    return {"kind": "python", "name": name or repr(annotation)}


def _merged_annotations(cls):
    annotations = {}
    for base in reversed(_metadata_attr(cls, "__mro__", ())):
        annotations.update(_metadata_attr(base, "__annotations__", {}))
    return annotations


def _class_schema(cls):
    type_hash_value = _metadata_attr(cls, "TYPE_HASH")
    if type_hash_value is _MISSING:
        raise ValueError(f"{_safe_datapod_label(cls)} is not a registered datapod type")
    type_hash = _u64_type_hash(type_hash_value)
    canonical_hash = _metadata_attr(cls, "CANONICAL_TYPE_HASH", None)
    canonical_hash = (
        canonical_type_hash(type_hash)
        if canonical_hash is None
        else _u64_type_hash(canonical_hash, "CANONICAL_TYPE_HASH")
    )
    fields = tuple(
        {
            "name": name,
            "arity": arity,
            "annotation": _annotation_schema(_merged_annotations(cls).get(name)),
        }
        for name, arity in zip(
            _metadata_attr(cls, "__datapod_fields__", ()),
            _metadata_attr(cls, "__datapod_field_arities__", ()),
        )
    )
    return {
        "canonical_name": _metadata_attr(cls, "__datapod_canonical_name__", None),
        "type_hash": type_hash,
        "canonical_type_hash": canonical_hash,
        "header_format": _metadata_attr(cls, "__datapod_header_format__", None),
        "header_size": header_size(type_hash),
        "payload_kind": payload_kind(type_hash),
        "payload_field": _metadata_attr(cls, "__datapod_payload_field__", None),
        "fields": fields,
        "wire_format": wire_format(type_hash),
        "emitted_hash_kind": emitted_hash_kind(type_hash),
        "has_archive": has_archive(type_hash),
        "has_view": has_view(type_hash),
        "has_owned_decode": has_owned_decode(type_hash),
        "archive_shape": archive_shape(type_hash),
    }


def describe_schema(target):
    """Return registry and declarative-field metadata for a datapod class/hash."""
    try:
        is_numeric_target = isinstance(target, (int, bool, float))
    except Exception as error:
        raise ValueError("datapod schema target metadata lookup failed") from error
    if is_numeric_target or _safe_callable_attr(target, "__index__") is not None:
        target = _u64_type_hash(target)
        cls = __datapod_registry__.get(target)
        if cls is None:
            return {
                "type_hash": target,
                "canonical_type_hash": canonical_type_hash(target),
                "header_size": header_size(target),
                "payload_kind": payload_kind(target),
                "wire_format": wire_format(target),
                "emitted_hash_kind": emitted_hash_kind(target),
                "has_archive": has_archive(target),
                "has_view": has_view(target),
                "has_owned_decode": has_owned_decode(target),
                "archive_shape": archive_shape(target),
            }
        return _class_schema(cls)
    try:
        is_type_target = isinstance(target, type)
    except Exception as error:
        raise ValueError("datapod schema target metadata lookup failed") from error
    cls = target if is_type_target else type(target)
    return _class_schema(cls)


def schema_for(target):
    """Return full built-in/runtime schema metadata for a datapod class/hash."""
    if isinstance(target, type):
        type_hash = _metadata_attr(target, "TYPE_HASH")
        if type_hash is _MISSING:
            raise ValueError(f"{_safe_type_name(target)} is not a registered datapod type")
        return _datapod_raw_schema_for_hash(_u64_type_hash(type_hash))
    if not isinstance(target, (int, bool, float)) and _safe_callable_attr(target, "__index__") is None:
        type_hash = _metadata_attr(type(target), "TYPE_HASH")
        if type_hash is _MISSING:
            raise ValueError(f"{_safe_type_name(type(target))} is not a registered datapod type")
        return _datapod_raw_schema_for_hash(_u64_type_hash(type_hash))
    return _datapod_raw_schema_for_hash(_u64_type_hash(target))


class DynamicDatapod:
    """Borrowed dynamic view over a datapod wire message.

    The view stores memoryviews over the incoming wire/header/payload. It does
    not copy the payload; callers that pass a quicbit SHM memoryview must keep
    the owning sample alive.
    """

    __slots__ = ("schema", "type_hash", "wire", "header", "payload", "_fields")

    def __init__(self, schema, wire):
        self.schema = schema
        self.type_hash = _u64_type_hash(schema["type_hash"])
        self.wire = _byte_memoryview(wire, "dynamic wire message")
        header_size_value = _usize_header_size(schema["header_size"])
        if len(self.wire) < header_size_value:
            raise ValueError(
                f"dynamic wire message too short: got {len(self.wire)}, "
                f"need at least {header_size_value}"
            )
        self.header = self.wire[:header_size_value]
        self.payload = self.wire[header_size_value:]
        self._fields = {field["name"]: field for field in schema["fields"]}

    def __getitem__(self, name):
        field = self._fields[name]
        if field["role"] == "payload":
            return self.payload
        offset = _usize_header_size(field["offset"], "field offset")
        kind = field["kind"]
        if kind == "scalar":
            return _unpack_dynamic_scalar(field["scalar"], self.header, offset, field["name"])
        if kind == "array":
            size = _usize_header_size(field["wire_size"], "field wire_size")
            return _dynamic_header_slice(self.header, offset, size, field["name"])
        if kind == "nested_array":
            nested_schema = schema_for(field["nested_type_hash"])
            nested_size = _usize_header_size(nested_schema["header_size"], "nested header_size")
            count = _usize_header_size(field["len"], "nested array length")
            size = _checked_usize_mul(nested_size, count, f"dynamic field {field['name']!r}")
            return _dynamic_header_slice(self.header, offset, size, field["name"])
        if kind == "nested":
            nested_schema = schema_for(field["nested_type_hash"])
            size = _usize_header_size(nested_schema["header_size"], "nested header_size")
            return DynamicDatapod(
                nested_schema,
                _dynamic_header_slice(self.header, offset, size, field["name"]),
            )
        if kind == "payload_section":
            return _dynamic_header_slice(self.header, offset, 8, field["name"])
        if kind == "opaque":
            size = _usize_header_size(field["wire_size"], "field wire_size")
            return _dynamic_header_slice(self.header, offset, size, field["name"])
        if kind == "bytes":
            return self.payload
        raise ValueError(f"unsupported dynamic field kind {kind!r}")

    def fields(self):
        return tuple(self._fields)


def _checked_usize_mul(a, b, label):
    value = a * b
    _usize_header_size(value, f"{label} byte length")
    return value


def _dynamic_header_slice(header, offset, size, name):
    end = offset + size
    if offset > len(header) or size > len(header) - offset:
        raise ValueError(
            f"dynamic field {name!r} exceeds header bounds: "
            f"offset={offset}, size={size}, header_len={len(header)}"
        )
    return header[offset:end]


def _unpack_dynamic_scalar(scalar, header, offset, name):
    formats = {
        "u8": "B", "u16": "H", "u32": "I", "u64": "Q",
        "u128": None,
        "i8": "b", "i16": "h", "i32": "i", "i64": "q",
        "i128": None,
        "f32": "f", "f64": "d",
        "bool": "?",
    }
    fmt = formats.get(scalar)
    if scalar == "u128":
        return int.from_bytes(_dynamic_header_slice(header, offset, 16, name), "little")
    if scalar == "i128":
        return int.from_bytes(_dynamic_header_slice(header, offset, 16, name), "little", signed=True)
    if fmt is None:
        raise ValueError(f"unsupported dynamic scalar {scalar!r}")
    _dynamic_header_slice(header, offset, _datapod_struct.calcsize("<" + fmt), name)
    return _datapod_struct.unpack_from("<" + fmt, header, offset)[0]


def dynamic_view(type_hash, wire):
    type_hash = _u64_type_hash(type_hash)
    validate_wire_message_v1(type_hash, wire)
    return DynamicDatapod(schema_for(type_hash), wire)


def _is_payload_annotation(annotation):
    name = _annotation_name(annotation)
    return annotation in (bytes, bytearray, memoryview) or name in (
        "bytes",
        "bytearray",
        "memoryview",
    )


def _byte_memoryview(buffer, label):
    view = memoryview(buffer)
    if not view.c_contiguous:
        raise ValueError(f"{label} must be a C-contiguous byte buffer")
    try:
        return view.cast("B")
    except TypeError as error:
        raise TypeError(f"{label} cannot be viewed as raw bytes: {error}") from error


def _u64_type_hash(value, label="type_hash"):
    value = _strict_index(value, label, "an unsigned 64-bit integer")
    if value < 0 or value > 0xffffffffffffffff:
        raise ValueError(f"{label} must fit in an unsigned 64-bit integer")
    return value


def _usize_header_size(value, label="header_size"):
    value = _strict_index(value, label, "a non-negative platform-sized integer")
    if value < 0:
        raise ValueError(f"{label} must be a non-negative platform-sized integer")
    if value > _datapod_sys.maxsize:
        raise ValueError(f"{label} exceeds maximum supported slice length")
    return value


def canonical_type_hash(type_hash):
    return _datapod_raw_canonical_type_hash(_u64_type_hash(type_hash))


def emitted_type_hash(type_hash):
    return _datapod_raw_emitted_type_hash(_u64_type_hash(type_hash))


def register_schema(canonical_name, header_size, payload_kind, type_hash=None):
    _validate_canonical_name(canonical_name)
    local_type_hash = None if type_hash is None else _u64_type_hash(type_hash)
    return _datapod_raw_register_schema(
        canonical_name,
        _usize_header_size(header_size),
        payload_kind,
        local_type_hash,
    )


def register_type(cls, canonical_name, header_size, payload_kind, type_hash=None):
    if not isinstance(cls, type):
        raise TypeError("register_type cls must be a datapod class")
    _validate_canonical_name(canonical_name)
    local_type_hash = None if type_hash is None else _u64_type_hash(type_hash)
    return _datapod_raw_register_type(
        cls,
        canonical_name,
        _usize_header_size(header_size),
        payload_kind,
        local_type_hash,
    )


def header_size(type_hash):
    return _datapod_raw_header_size(_u64_type_hash(type_hash))


def header_size_v1(type_hash):
    return _datapod_raw_header_size_v1(_u64_type_hash(type_hash))


def payload_kind(type_hash):
    return _datapod_raw_payload_kind(_u64_type_hash(type_hash))


def format_version(type_hash):
    return _datapod_raw_format_version(_u64_type_hash(type_hash))


def wire_format(type_hash):
    return _datapod_raw_wire_format(_u64_type_hash(type_hash))


def endian(type_hash):
    return _datapod_raw_endian(_u64_type_hash(type_hash))


def alignment_policy(type_hash):
    return _datapod_raw_alignment_policy(_u64_type_hash(type_hash))


def validator_kind(type_hash):
    return _datapod_raw_validator_kind(_u64_type_hash(type_hash))


def emitted_hash_kind(type_hash):
    return _datapod_raw_emitted_hash_kind(_u64_type_hash(type_hash))


def has_archive(type_hash):
    return _datapod_raw_has_archive(_u64_type_hash(type_hash))


def has_view(type_hash):
    return _datapod_raw_has_view(_u64_type_hash(type_hash))


def has_owned_decode(type_hash):
    return _datapod_raw_has_owned_decode(_u64_type_hash(type_hash))


def archive_shape(type_hash):
    return _datapod_raw_archive_shape(_u64_type_hash(type_hash))


def _run_registered_declarative_frame_validator(
    type_hash,
    header,
    payload,
    _registered_lookup=_registered_declarative_validator_type,
):
    registered = _registered_lookup(type_hash)
    if registered is None:
        cls = __datapod_registry__.get(type_hash)
        if cls is None:
            return
        if _metadata_attr(cls, "__datapod_validator__", None) is None:
            return
        label = f"registered Python datapod class for type hash {type_hash}"
        unpack_kwargs = _required_callable_metadata_attr(
            cls,
            "__datapod_unpack_kwargs__",
            label,
        )
        validate_object = _required_callable_metadata_attr(
            cls,
            "__datapod_validate_object__",
            label,
        )
        payload_field = _metadata_attr(cls, "__datapod_payload_field__", None)
    else:
        cls, unpack_kwargs, payload_field, validate_object = registered
    obj = _validation_shell(cls)
    for name, value in unpack_kwargs(header).items():
        _set_validation_field(obj, name, value)
    if payload_field is not None:
        _set_validation_field(obj, payload_field, payload.toreadonly())
    validate_object(obj)


def _run_registered_declarative_validator(
    type_hash,
    wire,
    _validate_frame=_run_registered_declarative_frame_validator,
):
    view = _byte_memoryview(wire, "wire message")
    size = header_size_v1(type_hash)
    _validate_frame(type_hash, view[:size], view[size:])


def validate_wire_message_v1(
    type_hash,
    wire,
    _run_declarative=_run_registered_declarative_validator,
):
    type_hash = _u64_type_hash(type_hash)
    wire = _byte_memoryview(wire, "wire message")
    _datapod_raw_validate_wire_message_v1(type_hash, wire)
    _run_declarative(type_hash, wire)


def validate_wire_message(type_hash, wire, _validate=validate_wire_message_v1):
    _validate(type_hash, wire)


def is_valid_wire_message(type_hash, wire, _validate=validate_wire_message_v1):
    try:
        _validate(type_hash, wire)
        return True
    except Exception:
        return False


def is_valid_wire_message_v1(type_hash, wire, _validate=validate_wire_message_v1):
    try:
        _validate(type_hash, wire)
        return True
    except Exception:
        return False


def join_wire_message(
    type_hash,
    header,
    payload=None,
    _validate_frame=_run_registered_declarative_frame_validator,
):
    type_hash = _u64_type_hash(type_hash)
    header = _byte_memoryview(header, "wire message header")
    payload = _byte_memoryview(b"" if payload is None else payload, "wire message payload")
    wire = _datapod_raw_join_wire_message(type_hash, header, payload)
    _validate_frame(type_hash, header, payload)
    return wire


def split_wire_message(type_hash, wire, _validate=validate_wire_message_v1):
    type_hash = _u64_type_hash(type_hash)
    wire = _byte_memoryview(wire, "wire message")
    _validate(type_hash, wire)
    return _datapod_raw_split_wire_message(
        type_hash,
        wire,
    )


def split_wire_message_v1(type_hash, wire, _validate=validate_wire_message_v1):
    type_hash = _u64_type_hash(type_hash)
    wire = _byte_memoryview(wire, "wire message")
    _validate(type_hash, wire)
    return _datapod_raw_split_wire_message_v1(
        type_hash,
        wire,
    )


def validate_wire_frame_parts_v1(
    type_hash,
    header,
    payload=None,
    _validate_frame=_run_registered_declarative_frame_validator,
):
    type_hash = _u64_type_hash(type_hash)
    header = _byte_memoryview(header, "wire frame header")
    payload = _byte_memoryview(b"" if payload is None else payload, "wire frame payload")
    _validate_wire_frame_shape_v1(type_hash, header, payload)
    _validate_frame(type_hash, header, payload)


def decode_as(cls, type_hash, wire, _validate=validate_wire_message_v1):
    if not isinstance(cls, type):
        raise TypeError("decode_as cls must be a datapod class")
    type_hash = _u64_type_hash(type_hash)
    class_type_hash = _metadata_attr(cls, "TYPE_HASH")
    if class_type_hash is _MISSING:
        raise ValueError(f"{_safe_type_name(cls)} is not a registered datapod class")
    expected = _u64_type_hash(class_type_hash)
    if expected != type_hash:
        raise ValueError(
            f"wrong decode_as type hash: got {type_hash}, expected {expected}"
        )
    wire = _byte_memoryview(wire, "wire message")
    if not _class_validates_generated_wire_decode(cls, type_hash):
        _validate(type_hash, wire)
    return _datapod_raw_decode_as(
        cls,
        type_hash,
        wire,
    )


def from_wire_message(type_hash, wire, _validate=validate_wire_message_v1):
    type_hash = _u64_type_hash(type_hash)
    wire = _byte_memoryview(wire, "wire message")
    cls = __datapod_registry__.get(type_hash)
    if not _class_validates_generated_wire_decode(cls, type_hash):
        _validate(type_hash, wire)
    return _datapod_raw_from_wire_message(
        type_hash,
        wire,
    )


class WireFrame:
    """Borrowed Python datapod frame: type_hash + header memoryview + payload memoryview."""

    __slots__ = ("type_hash", "header", "payload", "_owner")

    def __init__(self, type_hash, header, payload=b"", owner=None):
        self.type_hash = _u64_type_hash(type_hash)
        self.header = _byte_memoryview(header, "wire frame header")
        self.payload = _byte_memoryview(payload, "wire frame payload")
        self._owner = owner

    def joined_len(self):
        return len(self.header) + len(self.payload)

    def to_wire_message(self):
        raise RuntimeError("datapod WireFrame.to_wire_message is not initialized")


ArchiveFrame = WireFrame


def _join_wire_frame_shape_v1(frame):
    _validate_wire_frame_shape_v1(frame.type_hash, frame.header, frame.payload)
    return frame.header.tobytes() + frame.payload.tobytes()


def _source_uses_generated_method(source, label, kind):
    if source is None:
        return False
    cls = source.__class__
    try:
        class_attr = getattr(cls, label, None)
    except Exception:
        return False
    func = _method_function(class_attr)
    if func is None:
        return False
    if not _is_generated_hook(kind, func):
        return False
    try:
        bound_attr = getattr(source, label, None)
    except Exception:
        return False
    return _method_function(bound_attr) is func or bound_attr is class_attr


def _source_uses_generated_frame_encoder(source, label):
    return _source_uses_generated_method(source, label, "frame_encoder")


def _class_uses_generated_method(cls, label, kind):
    try:
        class_attr = getattr(cls, label, None)
    except Exception:
        return False
    func = _method_function(class_attr)
    if func is None:
        return False
    return _is_generated_hook(kind, func)


def _method_function(value):
    try:
        return getattr(value, "__func__", value)
    except Exception:
        return None


def _safe_callable_attr(obj, name):
    try:
        value = getattr(obj, name, None)
    except Exception:
        return None
    return value if callable(value) else None


def _metadata_attr(obj, name, default=_MISSING):
    try:
        return getattr(obj, name)
    except AttributeError:
        return default
    except Exception as error:
        try:
            label = getattr(obj, "__name__")
        except Exception:
            label = _safe_type_name(obj)
        if not isinstance(label, str):
            label = _safe_type_name(obj)
        raise ValueError(f"{label} metadata {name} lookup failed") from error


def _required_callable_metadata_attr(obj, name, label):
    value = _metadata_attr(obj, name, None)
    if not callable(value):
        raise ValueError(f"{label} has non-callable {name}")
    return value


def _class_type_hash_matches(cls, type_hash):
    try:
        class_type_hash = getattr(cls, "TYPE_HASH")
    except Exception:
        return False
    try:
        return _u64_type_hash(class_type_hash) == type_hash
    except (TypeError, ValueError):
        return False


def _source_validated_declarative_message(source, type_hash, label):
    if source is None:
        return False
    cls = source.__class__
    if _safe_callable_attr(cls, "__datapod_validate_object__") is None:
        return False
    if not _class_type_hash_matches(cls, type_hash):
        return False
    return _source_uses_generated_method(
        source,
        label,
        "message_encoder",
    )


def _class_validates_generated_wire_decode(cls, type_hash):
    if cls is None:
        return False
    if not _class_type_hash_matches(cls, type_hash):
        return False
    return _class_uses_generated_method(
        cls,
        "from_wire_message",
        "wire_decoder",
    )


def _source_validated_declarative_frame(source, frame, label):
    if source is None:
        return False
    cls = source.__class__
    if _safe_callable_attr(cls, "__datapod_validate_object__") is None:
        return False
    if not _source_uses_generated_frame_encoder(source, label):
        return False
    return _class_type_hash_matches(cls, frame.type_hash)


def _require_wire_frame_result(value, label, source=None):
    if not isinstance(value, WireFrame):
        raise ValueError(f"{label} must return datapod.WireFrame")
    if _source_validated_declarative_frame(source, value, label):
        _validate_wire_frame_shape_v1(value.type_hash, value.header, value.payload)
    else:
        validate_wire_frame_v1(value.type_hash, value.header, value.payload)
    return value


def _require_wire_frame_arg(value, label):
    if not isinstance(value, WireFrame):
        raise ValueError(f"{label} must be datapod.WireFrame")
    return value


def wire_frame(obj):
    to_wire_frame = getattr(obj, "to_wire_frame", None)
    if not callable(to_wire_frame):
        raise ValueError(f"{_safe_datapod_label(obj)} is not a datapod object")
    return _require_wire_frame_result(to_wire_frame(), "to_wire_frame", obj)


def archive(obj):
    archive_method = getattr(obj, "archive", None)
    if archive_method is not None:
        if not callable(archive_method):
            raise ValueError(f"{_safe_datapod_label(obj)}.archive is not callable")
        return _require_wire_frame_result(archive_method(), "archive", obj)
    to_wire_frame = getattr(obj, "to_wire_frame", None)
    if not callable(to_wire_frame):
        raise ValueError(f"{_safe_datapod_label(obj)} is not a datapod object")
    return _require_wire_frame_result(to_wire_frame(), "to_wire_frame", obj)


def view_wire_frame(type_or_hash, frame):
    frame = _require_wire_frame_arg(frame, "wire frame")
    if isinstance(type_or_hash, type):
        type_hash_value = _metadata_attr(type_or_hash, "TYPE_HASH")
        if type_hash_value is _MISSING:
            raise ValueError(
                f"{_safe_datapod_label(type_or_hash)} is not a registered datapod type"
            )
        expected = _u64_type_hash(type_hash_value)
    else:
        expected = _u64_type_hash(type_or_hash)
    if frame.type_hash != expected:
        raise ValueError(
            f"wrong frame type hash: got {frame.type_hash}, expected {expected}"
        )
    validate_wire_frame(expected, frame.header, frame.payload)
    if isinstance(type_or_hash, type):
        view_from_wire_frame = getattr(type_or_hash, "view_from_wire_frame", None)
        if view_from_wire_frame is not None:
            if not callable(view_from_wire_frame):
                raise ValueError(
                    f"{_safe_datapod_label(type_or_hash)}.view_from_wire_frame is not callable"
                )
            return view_from_wire_frame(frame)
    return frame.header, frame.payload


def view_archive(type_or_hash, archive_frame):
    archive_frame = _require_wire_frame_arg(archive_frame, "archive frame")
    if isinstance(type_or_hash, type):
        type_hash_value = _metadata_attr(type_or_hash, "TYPE_HASH")
        if type_hash_value is _MISSING:
            raise ValueError(
                f"{_safe_datapod_label(type_or_hash)} is not a registered datapod type"
            )
        expected = _u64_type_hash(type_hash_value)
        if archive_frame.type_hash != expected:
            raise ValueError(
                f"wrong archive frame type hash: got {archive_frame.type_hash}, expected {expected}"
            )
        validate_wire_frame(expected, archive_frame.header, archive_frame.payload)
        view_archive_method = getattr(type_or_hash, "view_archive", None)
        if view_archive_method is not None:
            if not callable(view_archive_method):
                raise ValueError(f"{_safe_datapod_label(type_or_hash)}.view_archive is not callable")
            return view_archive_method(archive_frame)
    return view_wire_frame(type_or_hash, archive_frame)


def from_archive(type_or_hash, archive_frame):
    archive_frame = _require_wire_frame_arg(archive_frame, "archive frame")
    if isinstance(type_or_hash, type):
        type_hash_value = _metadata_attr(type_or_hash, "TYPE_HASH")
        if type_hash_value is _MISSING:
            raise ValueError(
                f"{_safe_datapod_label(type_or_hash)} is not a registered datapod type"
            )
        expected = _u64_type_hash(type_hash_value)
        if archive_frame.type_hash != expected:
            raise ValueError(
                f"wrong archive frame type hash: got {archive_frame.type_hash}, expected {expected}"
            )
        validate_wire_frame(expected, archive_frame.header, archive_frame.payload)
        from_archive_method = getattr(type_or_hash, "from_archive", None)
        if from_archive_method is not None:
            if not callable(from_archive_method):
                raise ValueError(f"{_safe_datapod_label(type_or_hash)}.from_archive is not callable")
            return from_archive_method(archive_frame)
        from_wire_frame = getattr(type_or_hash, "from_wire_frame", None)
        if from_wire_frame is not None:
            if not callable(from_wire_frame):
                raise ValueError(f"{_safe_datapod_label(type_or_hash)}.from_wire_frame is not callable")
            return from_wire_frame(archive_frame)
        cls = type_or_hash
    else:
        expected = _u64_type_hash(type_or_hash)
        if archive_frame.type_hash != expected:
            raise ValueError(
                f"wrong archive frame type hash: got {archive_frame.type_hash}, expected {expected}"
            )
        validate_wire_frame(expected, archive_frame.header, archive_frame.payload)
        cls = __datapod_registry__.get(expected)
        if cls is None:
            raise ValueError(f"unknown datapod type hash: {type_or_hash}")
    from_archive_method = getattr(cls, "from_archive", None)
    if from_archive_method is not None:
        if not callable(from_archive_method):
            raise ValueError(f"{_safe_datapod_label(cls)}.from_archive is not callable")
        return from_archive_method(archive_frame)
    from_wire_frame = getattr(cls, "from_wire_frame", None)
    if not callable(from_wire_frame):
        raise ValueError(f"{_safe_datapod_label(cls)}.from_wire_frame is not callable")
    return from_wire_frame(archive_frame)


def _validate_wire_frame_shape_v1(type_hash, header, payload):
    _validate_wire_frame_v1(type_hash, header, payload)


def _join_wire_frame_shape_v1_hardened(
    frame,
    _validate_shape=_validate_wire_frame_shape_v1,
):
    _validate_shape(frame.type_hash, frame.header, frame.payload)
    return frame.header.tobytes() + frame.payload.tobytes()


_join_wire_frame_shape_v1 = _join_wire_frame_shape_v1_hardened


def validate_wire_frame(
    type_hash,
    header,
    payload=b"",
    _validate_shape=_validate_wire_frame_shape_v1,
    _validate_frame=_run_registered_declarative_frame_validator,
):
    type_hash = _u64_type_hash(type_hash)
    header = _byte_memoryview(header, "wire frame header")
    payload = _byte_memoryview(payload, "wire frame payload")
    _validate_shape(type_hash, header, payload)
    _validate_frame(type_hash, header, payload)


def validate_wire_frame_v1(type_hash, header, payload=b"", _validate=validate_wire_frame):
    _validate(type_hash, header, payload)


def _require_wire_frame_result_hardened(
    value,
    label,
    source=None,
    _source_validated=_source_validated_declarative_frame,
    _validate_shape=_validate_wire_frame_shape_v1,
    _validate=validate_wire_frame_v1,
):
    if not isinstance(value, WireFrame):
        raise ValueError(f"{label} must return datapod.WireFrame")
    if _source_validated(source, value, label):
        _validate_shape(value.type_hash, value.header, value.payload)
    else:
        _validate(value.type_hash, value.header, value.payload)
    return value


def _wire_frame_hardened(obj, _require_result=_require_wire_frame_result_hardened):
    to_wire_frame = getattr(obj, "to_wire_frame", None)
    if not callable(to_wire_frame):
        raise ValueError(f"{_safe_datapod_label(obj)} is not a datapod object")
    return _require_result(to_wire_frame(), "to_wire_frame", obj)


def _archive_hardened(obj, _require_result=_require_wire_frame_result_hardened):
    archive_method = getattr(obj, "archive", None)
    if archive_method is not None:
        if not callable(archive_method):
            raise ValueError(f"{_safe_datapod_label(obj)}.archive is not callable")
        return _require_result(archive_method(), "archive", obj)
    to_wire_frame = getattr(obj, "to_wire_frame", None)
    if not callable(to_wire_frame):
        raise ValueError(f"{_safe_datapod_label(obj)} is not a datapod object")
    return _require_result(to_wire_frame(), "to_wire_frame", obj)


wire_frame = _wire_frame_hardened
archive = _archive_hardened


def _wire_frame_to_wire_message(self, _validate=validate_wire_frame_v1):
    _validate(self.type_hash, self.header, self.payload)
    return self.header.tobytes() + self.payload.tobytes()


WireFrame.to_wire_message = _wire_frame_to_wire_message


def _view_wire_frame_hardened(
    type_or_hash,
    frame,
    _require_arg=_require_wire_frame_arg,
    _validate=validate_wire_frame,
):
    frame = _require_arg(frame, "wire frame")
    if isinstance(type_or_hash, type):
        type_hash_value = _metadata_attr(type_or_hash, "TYPE_HASH")
        if type_hash_value is _MISSING:
            raise ValueError(
                f"{_safe_type_name(type_or_hash)} is not a registered datapod type"
            )
        expected = _u64_type_hash(type_hash_value)
    else:
        expected = _u64_type_hash(type_or_hash)
    if frame.type_hash != expected:
        raise ValueError(
            f"wrong frame type hash: got {frame.type_hash}, expected {expected}"
        )
    _validate(expected, frame.header, frame.payload)
    if isinstance(type_or_hash, type):
        view_from_wire_frame = getattr(type_or_hash, "view_from_wire_frame", None)
        if view_from_wire_frame is not None:
            if not callable(view_from_wire_frame):
                raise ValueError(
                    f"{_safe_datapod_label(type_or_hash)}.view_from_wire_frame is not callable"
                )
            return view_from_wire_frame(frame)
    return frame.header, frame.payload


def _view_archive_hardened(
    type_or_hash,
    archive_frame,
    _require_arg=_require_wire_frame_arg,
    _validate=validate_wire_frame,
    _view_wire_frame=_view_wire_frame_hardened,
):
    archive_frame = _require_arg(archive_frame, "archive frame")
    if isinstance(type_or_hash, type):
        type_hash_value = _metadata_attr(type_or_hash, "TYPE_HASH")
        if type_hash_value is _MISSING:
            raise ValueError(
                f"{_safe_type_name(type_or_hash)} is not a registered datapod type"
            )
        expected = _u64_type_hash(type_hash_value)
        if archive_frame.type_hash != expected:
            raise ValueError(
                f"wrong archive frame type hash: got {archive_frame.type_hash}, expected {expected}"
            )
        _validate(expected, archive_frame.header, archive_frame.payload)
        view_archive_method = getattr(type_or_hash, "view_archive", None)
        if view_archive_method is not None:
            if not callable(view_archive_method):
                raise ValueError(f"{_safe_datapod_label(type_or_hash)}.view_archive is not callable")
            return view_archive_method(archive_frame)
    return _view_wire_frame(type_or_hash, archive_frame)


def _from_archive_hardened(
    type_or_hash,
    archive_frame,
    _require_arg=_require_wire_frame_arg,
    _validate=validate_wire_frame,
    _registry=None,
):
    if _registry is None:
        _registry = __datapod_registry__
    archive_frame = _require_arg(archive_frame, "archive frame")
    if isinstance(type_or_hash, type):
        type_hash_value = _metadata_attr(type_or_hash, "TYPE_HASH")
        if type_hash_value is _MISSING:
            raise ValueError(
                f"{_safe_type_name(type_or_hash)} is not a registered datapod type"
            )
        expected = _u64_type_hash(type_hash_value)
        if archive_frame.type_hash != expected:
            raise ValueError(
                f"wrong archive frame type hash: got {archive_frame.type_hash}, expected {expected}"
            )
        _validate(expected, archive_frame.header, archive_frame.payload)
        from_archive_method = getattr(type_or_hash, "from_archive", None)
        if from_archive_method is not None:
            if not callable(from_archive_method):
                raise ValueError(f"{_safe_datapod_label(type_or_hash)}.from_archive is not callable")
            return from_archive_method(archive_frame)
        from_wire_frame = getattr(type_or_hash, "from_wire_frame", None)
        if from_wire_frame is not None:
            if not callable(from_wire_frame):
                raise ValueError(f"{_safe_datapod_label(type_or_hash)}.from_wire_frame is not callable")
            return from_wire_frame(archive_frame)
        cls = type_or_hash
    else:
        expected = _u64_type_hash(type_or_hash)
        if archive_frame.type_hash != expected:
            raise ValueError(
                f"wrong archive frame type hash: got {archive_frame.type_hash}, expected {expected}"
            )
        _validate(expected, archive_frame.header, archive_frame.payload)
        cls = _registry.get(expected)
        if cls is None:
            raise ValueError(f"unknown datapod type hash: {type_or_hash}")
    from_archive_method = getattr(cls, "from_archive", None)
    if from_archive_method is not None:
        if not callable(from_archive_method):
            raise ValueError(f"{_safe_datapod_label(cls)}.from_archive is not callable")
        return from_archive_method(archive_frame)
    from_wire_frame = getattr(cls, "from_wire_frame", None)
    if not callable(from_wire_frame):
        raise ValueError(f"{_safe_datapod_label(cls)}.from_wire_frame is not callable")
    return from_wire_frame(archive_frame)


view_wire_frame = _view_wire_frame_hardened
view_archive = _view_archive_hardened
from_archive = _from_archive_hardened


def is_valid_wire_frame(type_hash, header, payload=b"", _validate=validate_wire_frame):
    try:
        _validate(type_hash, header, payload)
        return True
    except Exception:
        return False


def is_valid_wire_frame_v1(
    type_hash,
    header,
    payload=b"",
    _validate=validate_wire_frame_v1,
):
    try:
        _validate(type_hash, header, payload)
        return True
    except Exception:
        return False


def _split_wire_message_view_shape_v1(
    type_hash,
    wire,
    _to_u64=_u64_type_hash,
    _bytes=_byte_memoryview,
    _header_size=header_size_v1,
    _validate_shape=_validate_wire_frame_shape_v1,
):
    type_hash = _to_u64(type_hash)
    view = _bytes(wire, "wire message")
    size = _header_size(type_hash)
    if len(view) < size:
        raise ValueError(
            f"wire message too short: got {len(view)}, need at least {size}"
        )
    header = view[:size]
    payload = view[size:]
    _validate_shape(type_hash, header, payload)
    return header, payload


def split_wire_message_view(
    type_hash,
    wire,
    _split_shape=_split_wire_message_view_shape_v1,
    _validate_frame=_run_registered_declarative_frame_validator,
):
    type_hash = _u64_type_hash(type_hash)
    header, payload = _split_shape(type_hash, wire)
    _validate_frame(type_hash, header, payload)
    return header, payload


def split_wire_message_view_v1(type_hash, wire, _split=split_wire_message_view):
    return _split(type_hash, wire)


def _validate_canonical_name(canonical_name):
    if not isinstance(canonical_name, str):
        raise TypeError("datapod canonical type name must be a str")
    if canonical_name == "":
        raise ValueError("datapod canonical type name is empty")
    if "\x00" in canonical_name:
        raise ValueError("datapod canonical type name must not contain NUL bytes")
    previous_was_dot = True
    for ch in canonical_name:
        if ch == ".":
            if previous_was_dot:
                raise ValueError(
                    "datapod canonical type name must not contain empty dot-separated segments"
                )
            previous_was_dot = True
        elif "a" <= ch <= "z" or "0" <= ch <= "9" or ch == "_":
            previous_was_dot = False
        else:
            raise ValueError(
                "datapod canonical type name must contain only ASCII lowercase letters, "
                "digits, underscores, and dots"
            )
    if previous_was_dot:
        raise ValueError(
            "datapod canonical type name must not contain empty dot-separated segments"
        )


def datapod_type(
    canonical_name,
    header_format=None,
    fields=None,
    *,
    header=None,
    payload_field=None,
    payload=None,
    type_hash=None,
    validate=None,
    dataclass=False,
    byte_order="<",
):
    """Declare a Python datapod schema and auto-generate wire methods.

    Example:

        @datapod.datapod_type("acme.packet.v1", "<HH", ("channel", "flags"), payload_field="data")
        class Packet:
            ...
    """
    if not isinstance(canonical_name, str):
        raise TypeError("datapod_type canonical_name must be a str")
    _validate_canonical_name(canonical_name)
    if header_format is None:
        header_format = header
    elif header is not None and header != header_format:
        raise TypeError("datapod_type got conflicting header_format and header values")
    if header_format is not None and not isinstance(header_format, str):
        raise TypeError("datapod_type header_format/header must be a str")
    if header_format is None:
        if not isinstance(byte_order, str):
            raise TypeError("datapod_type byte_order must be a str")
        if byte_order not in ("@", "=", "<", ">", "!"):
            raise TypeError("datapod_type byte_order must be one of '@', '=', '<', '>', '!'")
    if payload_field is None:
        payload_field = payload
    elif payload is not None and payload != payload_field:
        raise TypeError("datapod_type got conflicting payload_field and payload values")
    if payload_field is not None and payload_field is not True and not isinstance(payload_field, str):
        raise TypeError("datapod_type payload_field/payload must be a str")
    if isinstance(payload_field, str) and payload_field == "":
        raise TypeError("datapod_type payload_field/payload must not be empty")
    if validate is not None and not callable(validate):
        raise TypeError("datapod_type validate must be callable")
    if not isinstance(dataclass, bool):
        raise TypeError("datapod_type dataclass must be a bool")

    def decorate(cls):
        if not isinstance(cls, type):
            raise TypeError("datapod_type decorator expects a class")
        if dataclass and "__dataclass_fields__" not in cls.__dict__:
            cls = _datapod_dataclasses.dataclass(cls)

        local_fields = fields
        local_payload_field = payload_field
        annotations = _merged_annotations(cls)
        if local_payload_field is True:
            payload_candidates = tuple(
                name for name, annotation in annotations.items()
                if _is_payload_annotation(annotation)
            )
            if len(payload_candidates) != 1:
                raise TypeError(
                    "datapod_type payload=True requires exactly one bytes-like annotated field"
                )
            local_payload_field = payload_candidates[0]
        if local_fields is None:
            local_fields = cls.__dict__.get("__datapod_fields__", None)
        if local_fields is None:
            if header_format is None:
                local_fields = tuple(
                    name for name, annotation in annotations.items()
                    if name != local_payload_field and not _is_payload_annotation(annotation)
                )
            else:
                local_fields = tuple(name for name in annotations if name != local_payload_field)
        local_fields = _field_names_tuple(local_fields, "fields")
        if local_payload_field is not None and local_payload_field in local_fields:
            raise TypeError("datapod_type payload field must not also be a header field")
        local_header_format = header_format
        if local_header_format is None:
            missing_annotations = tuple(name for name in local_fields if name not in annotations)
            if missing_annotations:
                raise TypeError(
                    "datapod_type needs annotations for inferred header fields: "
                    f"{missing_annotations!r}"
                )
            local_header_format = byte_order + "".join(
                _scalar_format(annotations[name]) for name in local_fields
            )
        if not local_header_format.startswith("<"):
            raise TypeError(
                "datapod-wire-v1/le requires header_format/header to start with '<' "
                "for explicit little-endian layout"
            )
        local_field_arities = tuple(
            _field_arity(annotations[name]) if name in annotations else 1
            for name in local_fields
        )
        local_header_arity = sum(local_field_arities)
        local_header_value_count = _struct_value_count(local_header_format)
        if local_header_value_count != local_header_arity:
            raise TypeError(
                "datapod_type header format/field mismatch: "
                f"format expects {local_header_value_count} values but fields describe "
                f"{local_header_arity}"
            )
        local_header_size = _datapod_struct.calcsize(local_header_format)
        if not local_fields and local_header_size:
            raise TypeError("datapod_type needs header fields for a non-empty header")

        kind = "bytes" if local_payload_field is not None else "fixed"
        local_type_hash = None if type_hash is None else _u64_type_hash(type_hash)
        type_id = register_type(cls, canonical_name, local_header_size, kind, local_type_hash)
        canonical_type_id = canonical_type_hash(type_id)

        cls.__datapod_canonical_name__ = canonical_name
        cls.CANONICAL_TYPE_HASH = canonical_type_id
        cls.__datapod_header_format__ = local_header_format
        cls.__datapod_fields__ = local_fields
        cls.__datapod_field_arities__ = local_field_arities
        cls.__datapod_header_arity__ = local_header_arity
        cls.__datapod_payload_field__ = local_payload_field
        cls.__datapod_validator__ = validate

        def run_custom_validator(obj):
            if validate is None:
                return
            valid = validate(obj)
            if not isinstance(valid, bool):
                raise TypeError("datapod custom validator must return bool")
            if not valid:
                raise ValueError("datapod custom validator rejected value")

        def pack_header(obj):
            values = []
            for name, arity in zip(local_fields, local_field_arities):
                annotation = annotations.get(name)
                try:
                    value = getattr(obj, name)
                except AttributeError as error:
                    raise ValueError(
                        f"{canonical_name} object is missing header field {name!r}"
                    ) from error
                except Exception as error:
                    raise ValueError(
                        f"{canonical_name} object cannot read header field {name!r}"
                    ) from error
                if annotation is None:
                    values.append(value)
                    continue
                values.extend(_pack_header_field(annotation, value, name, arity))
            try:
                return _datapod_struct.pack(local_header_format, *values)
            except _datapod_struct.error as error:
                raise ValueError(
                    f"{canonical_name} header values do not fit "
                    f"{local_header_format}: {error}"
                ) from error

        def unpack_kwargs(header):
            try:
                values = _datapod_struct.unpack(local_header_format, header)
            except _datapod_struct.error as error:
                raise ValueError(
                    f"{canonical_name} header bytes do not match "
                    f"{local_header_format}: {error}"
                ) from error
            kwargs = {}
            cursor = 0
            for name, arity in zip(local_fields, local_field_arities):
                annotation = annotations.get(name)
                if annotation is None:
                    kwargs[name] = values[cursor]
                else:
                    kwargs[name] = _unpack_header_field(
                        annotation,
                        values[cursor:cursor + arity],
                    )
                cursor += arity
            return kwargs

        def from_header_bytes(header):
            kwargs = unpack_kwargs(header)
            obj = _construct_from_kwargs(cls, kwargs)
            run_custom_validator(obj)
            return obj

        cls.__datapod_pack_header__ = pack_header
        cls.__datapod_unpack_kwargs__ = unpack_kwargs
        cls.__datapod_from_header_bytes__ = from_header_bytes
        cls.__datapod_validate_object__ = run_custom_validator
        _register_declarative_validator_type(
            type_id,
            canonical_type_id,
            cls,
            unpack_kwargs,
            local_payload_field,
            run_custom_validator,
        )

        def schema_for_type(inner_cls):
            return describe_schema(inner_cls)

        cls.schema = classmethod(schema_for_type)
        cls.describe_schema = classmethod(schema_for_type)

        def to_wire_message(self):
            return to_wire_message_v1(self)

        def to_wire_message_v1(self):
            frame = to_wire_frame_v1(self)
            return frame.type_hash, _join_wire_frame_shape_v1(frame)

        def to_wire_frame(self):
            return to_wire_frame_v1(self)

        def to_wire_frame_v1(self):
            if not local_header_format.startswith("<"):
                raise ValueError(
                    "datapod-wire-v1/le requires an explicit little-endian '<' header format"
                )
            run_custom_validator(self)
            header = pack_header(self)
            if local_payload_field is None:
                payload = b""
            else:
                try:
                    payload_value = getattr(self, local_payload_field)
                except AttributeError as error:
                    raise ValueError(
                        f"{canonical_name} object is missing payload field "
                        f"{local_payload_field!r}"
                    ) from error
                except Exception as error:
                    raise ValueError(
                        f"{canonical_name} object cannot read payload field "
                        f"{local_payload_field!r}"
                    ) from error
                try:
                    payload = memoryview(payload_value)
                except TypeError as error:
                    raise ValueError(
                        f"{canonical_name} payload field "
                        f"{local_payload_field!r} must be bytes-like"
                    ) from error
            return WireFrame(canonical_type_id, header, payload)

        def archive(self):
            return to_wire_frame_v1(self)

        def archive_v1(self):
            return to_wire_frame_v1(self)

        _register_generated_hook("frame_encoder", to_wire_frame)
        _register_generated_hook("frame_encoder", to_wire_frame_v1)
        _register_generated_hook("frame_encoder", archive)
        _register_generated_hook("frame_encoder", archive_v1)
        _register_generated_hook("message_encoder", to_wire_message)
        _register_generated_hook("message_encoder", to_wire_message_v1)

        @classmethod
        def from_wire_message(inner_cls, incoming_hash, wire):
            return from_wire_message_v1.__func__(inner_cls, incoming_hash, wire)

        @classmethod
        def from_wire_message_v1(inner_cls, incoming_hash, wire):
            incoming_hash = _u64_type_hash(incoming_hash)
            if incoming_hash != canonical_type_id:
                raise ValueError(
                    f"wrong v1 type hash: got {incoming_hash}, expected canonical {canonical_type_id}"
                )
            header, payload = _split_wire_message_view_shape_v1(incoming_hash, wire)
            kwargs = unpack_kwargs(header)
            if local_payload_field is not None:
                kwargs[local_payload_field] = payload.tobytes() if hasattr(payload, "tobytes") else bytes(payload)
            obj = _construct_from_kwargs(inner_cls, kwargs)
            run_custom_validator(obj)
            return obj

        @classmethod
        def from_wire_frame(inner_cls, frame):
            return from_wire_frame_v1.__func__(inner_cls, frame)

        @classmethod
        def from_wire_frame_v1(inner_cls, frame):
            frame = _require_wire_frame_arg(frame, "wire frame")
            if frame.type_hash != canonical_type_id:
                raise ValueError(
                    f"wrong v1 type hash: got {frame.type_hash}, expected canonical {canonical_type_id}"
                )
            _validate_wire_frame_shape_v1(frame.type_hash, frame.header, frame.payload)
            kwargs = unpack_kwargs(frame.header)
            if local_payload_field is not None:
                kwargs[local_payload_field] = frame.payload.tobytes()
            obj = _construct_from_kwargs(inner_cls, kwargs)
            run_custom_validator(obj)
            return obj

        @classmethod
        def from_archive(inner_cls, frame):
            frame = _require_wire_frame_arg(frame, "archive frame")
            return from_wire_frame_v1.__func__(inner_cls, frame)

        @classmethod
        def from_archive_v1(inner_cls, frame):
            frame = _require_wire_frame_arg(frame, "archive frame")
            return from_wire_frame_v1.__func__(inner_cls, frame)

        _register_generated_hook("wire_decoder", from_wire_message.__func__)
        _register_generated_hook("wire_decoder", from_wire_message_v1.__func__)

        @classmethod
        def validate_wire_message_for_type(inner_cls, incoming_hash, wire):
            validate_wire_message_v1_for_type.__func__(inner_cls, incoming_hash, wire)

        @classmethod
        def validate_wire_message_v1_for_type(inner_cls, incoming_hash, wire):
            incoming_hash = _u64_type_hash(incoming_hash)
            if incoming_hash != canonical_type_id:
                raise ValueError(
                    f"wrong v1 type hash: got {incoming_hash}, expected canonical {canonical_type_id}"
                )
            validate_wire_message_v1(incoming_hash, wire)

        @classmethod
        def view_from_wire(inner_cls, incoming_hash, wire):
            return view_from_wire_v1.__func__(inner_cls, incoming_hash, wire)

        @classmethod
        def view_from_wire_v1(inner_cls, incoming_hash, wire):
            incoming_hash = _u64_type_hash(incoming_hash)
            if incoming_hash != canonical_type_id:
                raise ValueError(
                    f"wrong v1 type hash: got {incoming_hash}, expected canonical {canonical_type_id}"
            )
            header, payload = _split_wire_message_view_shape_v1(incoming_hash, wire)
            kwargs = unpack_kwargs(header)
            obj = _validation_shell(inner_cls)
            for name, value in kwargs.items():
                _set_validation_field(obj, name, value)
            if local_payload_field is not None:
                _set_validation_field(obj, local_payload_field, payload.toreadonly())
            run_custom_validator(obj)
            return obj

        @classmethod
        def view_from_wire_frame(inner_cls, frame):
            return view_from_wire_frame_v1.__func__(inner_cls, frame)

        @classmethod
        def view_from_wire_frame_v1(inner_cls, frame):
            frame = _require_wire_frame_arg(frame, "wire frame")
            if frame.type_hash != canonical_type_id:
                raise ValueError(
                    f"wrong v1 type hash: got {frame.type_hash}, expected canonical {canonical_type_id}"
            )
            _validate_wire_frame_shape_v1(frame.type_hash, frame.header, frame.payload)
            kwargs = unpack_kwargs(frame.header)
            obj = _validation_shell(inner_cls)
            for name, value in kwargs.items():
                _set_validation_field(obj, name, value)
            if local_payload_field is not None:
                _set_validation_field(obj, local_payload_field, frame.payload.toreadonly())
            run_custom_validator(obj)
            return obj

        @classmethod
        def view_archive(inner_cls, frame):
            frame = _require_wire_frame_arg(frame, "archive frame")
            return view_from_wire_frame_v1.__func__(inner_cls, frame)

        @classmethod
        def view_archive_v1(inner_cls, frame):
            frame = _require_wire_frame_arg(frame, "archive frame")
            return view_from_wire_frame_v1.__func__(inner_cls, frame)

        cls.to_wire_message = to_wire_message
        cls.to_wire_message_v1 = to_wire_message_v1
        cls.to_wire_frame = to_wire_frame
        cls.to_wire_frame_v1 = to_wire_frame_v1
        cls.archive = archive
        cls.archive_v1 = archive_v1
        cls.from_wire_message = from_wire_message
        cls.from_wire_message_v1 = from_wire_message_v1
        cls.from_wire_frame = from_wire_frame
        cls.from_wire_frame_v1 = from_wire_frame_v1
        cls.from_archive = from_archive
        cls.from_archive_v1 = from_archive_v1
        cls.validate_wire_message = validate_wire_message_for_type
        cls.validate_wire_message_v1 = validate_wire_message_v1_for_type
        cls.view_from_wire = view_from_wire
        cls.view_from_wire_v1 = view_from_wire_v1
        cls.view_from_wire_frame = view_from_wire_frame
        cls.view_from_wire_frame_v1 = view_from_wire_frame_v1
        cls.view_archive = view_archive
        cls.view_archive_v1 = view_archive_v1
        return cls

    return decorate


def _install_archive_aliases(datapod_module):
    """Fill Archive/View/Owned aliases for Rust-backed classes that only expose lower-level helpers."""

    def make_archive(cls):
        def archive(self):
            to_wire_frame = getattr(self, "to_wire_frame", None)
            if to_wire_frame is not None:
                if not callable(to_wire_frame):
                    raise ValueError(f"{_safe_datapod_label(cls)}.to_wire_frame is not callable")
                return to_wire_frame()
            type_hash = _u64_type_hash(_metadata_attr(cls, "TYPE_HASH"), "TYPE_HASH")
            if payload_kind(type_hash) != "fixed":
                raise TypeError(
                    f"{_safe_datapod_label(cls)} lacks direct zero-copy to_wire_frame(); "
                    "refusing to synthesize a heap archive from payload_bytes()"
                )
            return datapod_module.ArchiveFrame(
                type_hash,
                self.to_header_bytes(),
                b"",
                self,
            )
        return archive

    def make_archive_v1(cls):
        def archive_v1(self):
            to_wire_frame_v1 = getattr(self, "to_wire_frame_v1", None)
            if to_wire_frame_v1 is not None:
                if not callable(to_wire_frame_v1):
                    raise ValueError(f"{_safe_datapod_label(cls)}.to_wire_frame_v1 is not callable")
                return to_wire_frame_v1()
            return make_archive(cls)(self)
        return archive_v1

    def frame_part_bytes(frame, attr, label):
        try:
            part = getattr(frame, attr)
            return part.tobytes()
        except ValueError:
            raise
        except Exception as error:
            raise ValueError(f"{label} must expose {attr}.tobytes()") from error

    def make_from_wire_frame(cls):
        @classmethod
        def from_wire_frame(inner_cls, frame):
            frame = _require_wire_frame_arg(frame, "wire frame")
            type_hash_value = _metadata_attr(inner_cls, "TYPE_HASH")
            if type_hash_value is _MISSING:
                raise ValueError(
                    f"{_safe_datapod_label(inner_cls)} is not a registered datapod type"
                )
            type_hash = _u64_type_hash(type_hash_value, "TYPE_HASH")
            if type_hash != frame.type_hash:
                raise ValueError(
                    f"wrong frame type hash: got {frame.type_hash}, expected {type_hash}"
                )
            from_wire = getattr(inner_cls, "from_wire", None)
            if not callable(from_wire):
                raise ValueError(f"{_safe_datapod_label(inner_cls)}.from_wire is not callable")
            return from_wire(
                frame_part_bytes(frame, "header", _safe_datapod_label(inner_cls)),
                frame_part_bytes(frame, "payload", _safe_datapod_label(inner_cls)),
            )
        return from_wire_frame

    def make_from_archive(cls):
        @classmethod
        def from_archive(inner_cls, frame):
            frame = _require_wire_frame_arg(frame, "archive frame")
            from_wire_frame = getattr(inner_cls, "from_wire_frame", None)
            if from_wire_frame is not None:
                if not callable(from_wire_frame):
                    raise ValueError(f"{_safe_datapod_label(inner_cls)}.from_wire_frame is not callable")
                return from_wire_frame(frame)
            return make_from_wire_frame(cls).__func__(inner_cls, frame)
        return from_archive

    def make_view_from_wire_frame(cls):
        @classmethod
        def view_from_wire_frame(inner_cls, frame):
            frame = _require_wire_frame_arg(frame, "wire frame")
            type_hash_value = _metadata_attr(inner_cls, "TYPE_HASH")
            if type_hash_value is _MISSING:
                raise ValueError(
                    f"{_safe_datapod_label(inner_cls)} is not a registered datapod type"
                )
            type_hash = _u64_type_hash(type_hash_value, "TYPE_HASH")
            if type_hash != frame.type_hash:
                raise ValueError(
                    f"wrong frame type hash: got {frame.type_hash}, expected {type_hash}"
                )
            try:
                validate_wire_frame_v1(frame.type_hash, frame.header, frame.payload)
            except ValueError:
                raise
            except Exception as error:
                raise ValueError(
                    f"{_safe_datapod_label(inner_cls)} frame validation failed"
                ) from error
            from_wire = getattr(inner_cls, "from_wire", None)
            if payload_kind(type_hash) == "fixed" and from_wire is not None:
                if not callable(from_wire):
                    raise ValueError(f"{_safe_datapod_label(inner_cls)}.from_wire is not callable")
                return from_wire(
                    frame_part_bytes(frame, "header", _safe_datapod_label(inner_cls)),
                    b"",
                )
            return datapod_module.PayloadView(frame.header, frame.payload, frame)
        return view_from_wire_frame

    def make_view_archive(cls):
        @classmethod
        def view_archive(inner_cls, frame):
            frame = _require_wire_frame_arg(frame, "archive frame")
            view_from_wire_frame = getattr(inner_cls, "view_from_wire_frame", None)
            if view_from_wire_frame is not None:
                if not callable(view_from_wire_frame):
                    raise ValueError(f"{_safe_datapod_label(inner_cls)}.view_from_wire_frame is not callable")
                return view_from_wire_frame(frame)
            return make_view_from_wire_frame(cls).__func__(inner_cls, frame)
        return view_archive

    seen = set()
    for cls in getattr(datapod_module, "__datapod_registry__", {}).values():
        if cls in seen:
            continue
        seen.add(cls)
        if not hasattr(cls, "archive"):
            cls.archive = make_archive(cls)
        if not hasattr(cls, "archive_v1"):
            cls.archive_v1 = make_archive_v1(cls)
        if not hasattr(cls, "from_wire_frame"):
            cls.from_wire_frame = make_from_wire_frame(cls)
        if not hasattr(cls, "from_wire_frame_v1"):
            cls.from_wire_frame_v1 = make_from_wire_frame(cls)
        if not hasattr(cls, "from_archive"):
            cls.from_archive = make_from_archive(cls)
        if not hasattr(cls, "from_archive_v1"):
            cls.from_archive_v1 = make_from_archive(cls)
        if not hasattr(cls, "view_from_wire_frame"):
            cls.view_from_wire_frame = make_view_from_wire_frame(cls)
        if not hasattr(cls, "view_from_wire_frame_v1"):
            cls.view_from_wire_frame_v1 = make_view_from_wire_frame(cls)
        if not hasattr(cls, "view_archive"):
            cls.view_archive = make_view_archive(cls)
        if not hasattr(cls, "view_archive_v1"):
            cls.view_archive_v1 = make_view_archive(cls)
"#,
    )
    .map_err(|err| PyValueError::new_err(format!("invalid declarative datapod source: {err}")))?;
    let file = CString::new("<datapod declarative layout>").map_err(|err| {
        PyValueError::new_err(format!("invalid declarative datapod file name: {err}"))
    })?;
    let name = CString::new("_datapod_declarative").map_err(|err| {
        PyValueError::new_err(format!("invalid declarative datapod module name: {err}"))
    })?;
    let module = PyModule::from_code(py, code.as_c_str(), file.as_c_str(), name.as_c_str())?;
    SOURCE_VALIDATED_DECLARATIVE_MESSAGE.with(|slot| {
        *slot.borrow_mut() = Some(
            module
                .getattr("_source_validated_declarative_message")?
                .unbind(),
        );
        Ok::<_, pyo3::PyErr>(())
    })?;
    RUN_REGISTERED_DECLARATIVE_VALIDATOR.with(|slot| {
        *slot.borrow_mut() = Some(
            module
                .getattr("_run_registered_declarative_validator")?
                .unbind(),
        );
        Ok::<_, pyo3::PyErr>(())
    })?;
    module.add(
        "_datapod_raw_canonical_type_hash",
        m.getattr("canonical_type_hash")?,
    )?;
    module.add(
        "_datapod_raw_emitted_type_hash",
        m.getattr("emitted_type_hash")?,
    )?;
    module.add(
        "_datapod_raw_register_schema",
        m.getattr("register_schema")?,
    )?;
    module.add("_datapod_raw_register_type", m.getattr("register_type")?)?;
    module.add("_datapod_raw_header_size", m.getattr("header_size")?)?;
    module.add("_datapod_raw_header_size_v1", m.getattr("header_size_v1")?)?;
    module.add("_datapod_raw_payload_kind", m.getattr("payload_kind")?)?;
    module.add("_datapod_raw_format_version", m.getattr("format_version")?)?;
    module.add("_datapod_raw_wire_format", m.getattr("wire_format")?)?;
    module.add("_datapod_raw_endian", m.getattr("endian")?)?;
    module.add(
        "_datapod_raw_alignment_policy",
        m.getattr("alignment_policy")?,
    )?;
    module.add("_datapod_raw_validator_kind", m.getattr("validator_kind")?)?;
    module.add(
        "_datapod_raw_emitted_hash_kind",
        m.getattr("emitted_hash_kind")?,
    )?;
    module.add("_datapod_raw_has_archive", m.getattr("has_archive")?)?;
    module.add("_datapod_raw_has_view", m.getattr("has_view")?)?;
    module.add(
        "_datapod_raw_has_owned_decode",
        m.getattr("has_owned_decode")?,
    )?;
    module.add("_datapod_raw_archive_shape", m.getattr("archive_shape")?)?;
    module.add(
        "_datapod_raw_join_wire_message",
        m.getattr("join_wire_message")?,
    )?;
    module.add(
        "_datapod_raw_split_wire_message",
        m.getattr("split_wire_message")?,
    )?;
    module.add(
        "_datapod_raw_split_wire_message_v1",
        m.getattr("split_wire_message_v1")?,
    )?;
    module.add(
        "_datapod_raw_validate_wire_message",
        m.getattr("validate_wire_message")?,
    )?;
    module.add(
        "_datapod_raw_validate_wire_message_v1",
        m.getattr("validate_wire_message_v1")?,
    )?;
    module.add(
        "_datapod_raw_validate_wire_frame_parts_v1",
        m.getattr("validate_wire_frame_parts_v1")?,
    )?;
    module.add(
        "_datapod_raw_schema_for_hash",
        m.getattr("schema_for_hash")?,
    )?;
    module.add("_datapod_raw_decode_as", m.getattr("decode_as")?)?;
    module.add(
        "_datapod_raw_from_wire_message",
        m.getattr("from_wire_message")?,
    )?;
    module.add(
        "_validate_wire_frame_v1",
        m.getattr("validate_wire_frame_parts_v1")?,
    )?;
    module.add("__datapod_registry__", m.getattr("__datapod_registry__")?)?;
    for name in [
        "canonical_type_hash",
        "emitted_type_hash",
        "register_schema",
        "register_type",
        "header_size",
        "header_size_v1",
        "payload_kind",
        "format_version",
        "wire_format",
        "endian",
        "alignment_policy",
        "validator_kind",
        "emitted_hash_kind",
        "has_archive",
        "has_view",
        "has_owned_decode",
        "archive_shape",
        "join_wire_message",
        "split_wire_message",
        "split_wire_message_v1",
        "validate_wire_message",
        "validate_wire_message_v1",
        "validate_wire_frame_parts_v1",
        "decode_as",
        "from_wire_message",
    ] {
        m.add(name, module.getattr(name)?)?;
    }
    m.add("datapod_type", module.getattr("datapod_type")?)?;
    for name in [
        "u8", "u16", "u32", "u64", "i8", "i16", "i32", "i64", "f32", "f64",
    ] {
        m.add(name, module.getattr(name)?)?;
    }
    m.add("array", module.getattr("array")?)?;
    m.add(
        "is_valid_wire_message",
        module.getattr("is_valid_wire_message")?,
    )?;
    m.add(
        "is_valid_wire_message_v1",
        module.getattr("is_valid_wire_message_v1")?,
    )?;
    m.add(
        "split_wire_message_view",
        module.getattr("split_wire_message_view")?,
    )?;
    m.add(
        "split_wire_message_view_v1",
        module.getattr("split_wire_message_view_v1")?,
    )?;
    m.add("WireFrame", module.getattr("WireFrame")?)?;
    m.add("ArchiveFrame", module.getattr("ArchiveFrame")?)?;
    m.add("wire_frame", module.getattr("wire_frame")?)?;
    m.add("archive", module.getattr("archive")?)?;
    m.add("view_wire_frame", module.getattr("view_wire_frame")?)?;
    m.add("view_archive", module.getattr("view_archive")?)?;
    m.add("from_archive", module.getattr("from_archive")?)?;
    m.add(
        "validate_wire_frame",
        module.getattr("validate_wire_frame")?,
    )?;
    m.add(
        "validate_wire_frame_v1",
        module.getattr("validate_wire_frame_v1")?,
    )?;
    m.add(
        "is_valid_wire_frame",
        module.getattr("is_valid_wire_frame")?,
    )?;
    m.add(
        "is_valid_wire_frame_v1",
        module.getattr("is_valid_wire_frame_v1")?,
    )?;
    m.add("describe_schema", module.getattr("describe_schema")?)?;
    m.add("schema", module.getattr("describe_schema")?)?;
    m.add("schema_for", module.getattr("schema_for")?)?;
    m.add("dynamic_view", module.getattr("dynamic_view")?)?;
    m.add("DynamicDatapod", module.getattr("DynamicDatapod")?)?;
    module.getattr("_install_archive_aliases")?.call1((m,))?;
    Ok(())
}

fn extract_wire_message_return(
    value: Bound<'_, PyAny>,
    label: &str,
    source: &Bound<'_, PyAny>,
) -> PyResult<(u64, Vec<u8>)> {
    let tuple = value.downcast::<PyTuple>().map_err(|error| {
        PyValueError::new_err(format!("{label} must return (type_hash, bytes): {error}"))
    })?;
    if tuple.len() != 2 {
        return Err(PyValueError::new_err(format!(
            "{label} must return exactly two values: (type_hash, bytes)"
        )));
    }
    let type_hash = tuple.get_item(0)?;
    if type_hash.is_instance_of::<PyBool>() {
        return Err(PyValueError::new_err(format!(
            "{label} type_hash must be an unsigned 64-bit integer, not bool"
        )));
    }
    let type_hash = type_hash.extract::<u64>().map_err(|error| {
        PyValueError::new_err(format!(
            "{label} type_hash must fit in an unsigned 64-bit integer: {error}"
        ))
    })?;
    let wire = extract_contiguous_bytes_like(tuple.get_item(1)?, label)?;
    crate::validate_registered_wire_v1(type_hash, &wire).map_err(|error| {
        PyValueError::new_err(format!("{label} returned invalid wire message: {error}"))
    })?;
    let py = value.py();
    let already_validated = SOURCE_VALIDATED_DECLARATIVE_MESSAGE.with(|slot| {
        let helper = slot.borrow();
        let helper = helper.as_ref().ok_or_else(|| {
            PyValueError::new_err("datapod declarative validation helper is not initialized")
        })?;
        helper
            .bind(py)
            .call1((source, type_hash, label))?
            .extract::<bool>()
    })?;
    if !already_validated {
        let wire_bytes = PyBytes::new(value.py(), &wire);
        RUN_REGISTERED_DECLARATIVE_VALIDATOR.with(|slot| {
            let helper = slot.borrow();
            let helper = helper.as_ref().ok_or_else(|| {
                PyValueError::new_err("datapod declarative validator helper is not initialized")
            })?;
            helper.bind(py).call1((type_hash, wire_bytes))?;
            Ok::<_, pyo3::PyErr>(())
        })?;
    }
    Ok((type_hash, wire))
}

fn extract_contiguous_bytes_like(value: Bound<'_, PyAny>, label: &str) -> PyResult<Vec<u8>> {
    let builtins = PyModule::import(value.py(), "builtins")?;
    let view = builtins
        .getattr("memoryview")?
        .call1((value,))
        .map_err(|error| {
            PyValueError::new_err(format!("{label} bytes must be bytes-like: {error}"))
        })?;
    if !view.getattr("c_contiguous")?.extract::<bool>()? {
        return Err(PyValueError::new_err(format!(
            "{label} bytes must be a C-contiguous byte buffer"
        )));
    }
    view.call_method0("tobytes")?
        .extract::<Vec<u8>>()
        .map_err(|error| {
            PyValueError::new_err(format!("{label} bytes must be bytes-like: {error}"))
        })
}

fn validate_python_class_type_hash(
    cls: &Bound<'_, PyAny>,
    type_hash: u64,
    label: &str,
) -> PyResult<()> {
    if !cls.hasattr("TYPE_HASH")? {
        return Err(PyValueError::new_err(format!(
            "{label} class is missing TYPE_HASH"
        )));
    }
    let class_type_hash = cls.getattr("TYPE_HASH")?;
    if class_type_hash.is_instance_of::<PyBool>() {
        return Err(PyValueError::new_err(format!(
            "{label} class TYPE_HASH must be an unsigned 64-bit integer, not bool"
        )));
    }
    let class_type_hash = class_type_hash.extract::<u64>().map_err(|error| {
        PyValueError::new_err(format!(
            "{label} class TYPE_HASH must fit in an unsigned 64-bit integer: {error}"
        ))
    })?;
    if class_type_hash != type_hash {
        return Err(PyValueError::new_err(format!(
            "{label} class TYPE_HASH mismatch: got {class_type_hash}, expected {type_hash}"
        )));
    }
    Ok(())
}

#[pyfunction]
fn to_wire_message(obj: &Bound<'_, PyAny>) -> PyResult<(u64, Vec<u8>)> {
    if !obj.hasattr("to_wire_message")? {
        return Err(PyValueError::new_err(format!(
            "{} is not a datapod object",
            obj.get_type().name()?
        )));
    }
    let method = obj.getattr("to_wire_message")?;
    if !method.is_callable() {
        return Err(PyValueError::new_err(format!(
            "{}.to_wire_message is not callable",
            obj.get_type().name()?
        )));
    }
    extract_wire_message_return(method.call0()?, "to_wire_message", obj)
}

#[pyfunction]
fn to_wire_message_v1(obj: &Bound<'_, PyAny>) -> PyResult<(u64, Vec<u8>)> {
    if !obj.hasattr("to_wire_message_v1")? {
        return Err(PyValueError::new_err(format!(
            "{} is not a datapod object",
            obj.get_type().name()?
        )));
    }
    let method = obj.getattr("to_wire_message_v1")?;
    if !method.is_callable() {
        return Err(PyValueError::new_err(format!(
            "{}.to_wire_message_v1 is not callable",
            obj.get_type().name()?
        )));
    }
    extract_wire_message_return(method.call0()?, "to_wire_message_v1", obj)
}

#[pyfunction]
fn decode_as(cls: &Bound<'_, PyAny>, type_hash: u64, wire: Vec<u8>) -> PyResult<PyObject> {
    crate::validate_registered_wire_v1(type_hash, &wire)
        .map_err(|error| PyValueError::new_err(error.to_string()))?;
    validate_python_class_type_hash(cls, type_hash, "decode_as")?;
    if !cls.hasattr("from_wire_message")? {
        return Err(PyValueError::new_err(format!(
            "{} is not a datapod class",
            cls.get_type().name()?
        )));
    }
    let method = cls.getattr("from_wire_message")?;
    if !method.is_callable() {
        return Err(PyValueError::new_err(format!(
            "{}.from_wire_message is not callable",
            cls.get_type().name()?
        )));
    }
    Ok(method.call1((type_hash, wire))?.unbind())
}

#[pyfunction]
fn from_wire_message(py: Python<'_>, type_hash: u64, wire: Vec<u8>) -> PyResult<PyObject> {
    crate::validate_registered_wire_v1(type_hash, &wire)
        .map_err(|error| PyValueError::new_err(error.to_string()))?;
    let module = PyModule::import(py, "datapod")?;
    let registry = module
        .getattr("__datapod_registry__")?
        .downcast_into::<PyDict>()?;
    let class = registry.get_item(type_hash)?.ok_or_else(|| {
        pyo3::exceptions::PyValueError::new_err(format!("unknown datapod type hash: {type_hash}"))
    })?;
    validate_python_class_type_hash(&class, type_hash, "from_wire_message")?;
    if !class.hasattr("from_wire_message")? {
        return Err(PyValueError::new_err(format!(
            "registered Python datapod class for type hash {type_hash} lacks from_wire_message"
        )));
    }
    let method = class.getattr("from_wire_message")?;
    if !method.is_callable() {
        return Err(PyValueError::new_err(format!(
            "registered Python datapod class for type hash {type_hash} has non-callable from_wire_message"
        )));
    }
    Ok(method.call1((type_hash, wire))?.unbind())
}

#[pyfunction]
fn type_hash_name(canonical_name: &str) -> PyResult<u64> {
    validate_canonical_name(canonical_name)?;
    Ok(crate::registry::type_hash_name(canonical_name))
}

fn validate_canonical_name(canonical_name: &str) -> PyResult<()> {
    crate::registry::validate_canonical_name(canonical_name)
        .map_err(|error| pyo3::exceptions::PyValueError::new_err(error.to_string()))
}

#[pyfunction]
fn canonical_type_hash(type_hash: u64) -> PyResult<u64> {
    Ok(type_info(type_hash)?.canonical_type_hash)
}

#[pyfunction]
fn emitted_type_hash(type_hash: u64) -> PyResult<u64> {
    Ok(type_info(type_hash)?.emitted_hash)
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
    crate::registry::try_find_type_info(type_hash)
        .map_err(|error| pyo3::exceptions::PyValueError::new_err(error.to_string()))?
        .ok_or_else(|| {
            pyo3::exceptions::PyValueError::new_err(format!(
                "unknown datapod type hash: {type_hash}"
            ))
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
    validate_canonical_name(canonical_name)?;
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
    let canonical_type_hash = crate::registry::try_find_type_info(type_hash)
        .map_err(|error| pyo3::exceptions::PyValueError::new_err(error.to_string()))?
        .map(|info| info.canonical_type_hash)
        .unwrap_or_else(|| crate::registry::type_hash_name(canonical_name));
    cls.setattr("CANONICAL_TYPE_HASH", canonical_type_hash)?;
    cls.setattr("__datapod_canonical_name__", canonical_name)?;
    cls.setattr("__datapod_header_size__", header_size)?;
    cls.setattr("__datapod_payload_kind__", payload_kind_name(payload_kind))?;

    let module = PyModule::import(py, "datapod")?;
    let registry = module
        .getattr("__datapod_registry__")?
        .downcast_into::<PyDict>()?;
    registry.set_item(type_hash, cls)?;
    registry.set_item(canonical_type_hash, cls)?;
    Ok(type_hash)
}

#[pyfunction]
fn header_size(type_hash: u64) -> PyResult<usize> {
    Ok(type_info(type_hash)?.header_size)
}

#[pyfunction]
fn header_size_v1(type_hash: u64) -> PyResult<usize> {
    crate::registry::v1_header_size(type_hash).ok_or_else(|| {
        pyo3::exceptions::PyValueError::new_err(format!(
            "unknown datapod type hash or missing v1 header metadata: {type_hash}"
        ))
    })
}

#[pyfunction]
fn payload_kind(type_hash: u64) -> PyResult<&'static str> {
    Ok(payload_kind_name(type_info(type_hash)?.payload_kind))
}

#[pyfunction]
fn format_version(type_hash: u64) -> PyResult<u32> {
    Ok(type_info(type_hash)?.format_version)
}

#[pyfunction]
fn wire_format(type_hash: u64) -> PyResult<&'static str> {
    Ok(type_info(type_hash)?.wire_format.name())
}

#[pyfunction]
fn current_wire_format() -> &'static str {
    crate::registry::current_wire_format_name()
}

#[pyfunction]
fn builtin_hash_policy() -> &'static str {
    crate::registry::builtin_hash_policy()
}

#[pyfunction]
fn endian(type_hash: u64) -> PyResult<&'static str> {
    Ok(match type_info(type_hash)?.endian {
        crate::registry::Endian::Little => "little",
    })
}

#[pyfunction]
fn alignment_policy(type_hash: u64) -> PyResult<&'static str> {
    Ok(match type_info(type_hash)?.alignment {
        crate::registry::AlignmentPolicy::UnalignedWire => "unaligned-wire",
        crate::registry::AlignmentPolicy::AlignedPayload { .. } => "aligned-payload",
    })
}

#[pyfunction]
fn validator_kind(type_hash: u64) -> PyResult<&'static str> {
    Ok(match type_info(type_hash)?.validator {
        crate::registry::ValidatorKind::RegistryOnly => "registry-only",
        crate::registry::ValidatorKind::BuiltIn => "built-in",
        crate::registry::ValidatorKind::RuntimeSchema => "runtime-schema",
    })
}

#[pyfunction]
fn emitted_hash_kind(type_hash: u64) -> PyResult<&'static str> {
    Ok(match type_info(type_hash)?.emitted_hash_kind {
        crate::registry::HashKind::CanonicalName => "canonical-name",
    })
}

#[pyfunction]
fn has_archive(type_hash: u64) -> PyResult<bool> {
    Ok(type_info(type_hash)?.has_archive)
}

#[pyfunction]
fn has_view(type_hash: u64) -> PyResult<bool> {
    Ok(type_info(type_hash)?.has_view)
}

#[pyfunction]
fn has_owned_decode(type_hash: u64) -> PyResult<bool> {
    Ok(type_info(type_hash)?.has_owned_decode)
}

#[pyfunction]
fn archive_shape(type_hash: u64) -> PyResult<&'static str> {
    Ok(type_info(type_hash)?.archive_shape.name())
}

#[pyfunction]
fn schema_for_hash(py: Python<'_>, type_hash: u64) -> PyResult<Py<PyDict>> {
    let schema = crate::registry::try_find_schema(type_hash)
        .map_err(|error| pyo3::exceptions::PyValueError::new_err(error.to_string()))?
        .ok_or_else(|| {
            pyo3::exceptions::PyValueError::new_err(format!(
                "unknown datapod type hash: {type_hash}"
            ))
        })?;
    let dict = PyDict::new(py);
    dict.set_item("canonical_name", schema.canonical_name)?;
    dict.set_item("type_hash", schema.type_hash)?;
    dict.set_item("schema_hash", schema.schema_hash)?;
    dict.set_item("header_size", schema.header_size)?;
    dict.set_item("payload_kind", payload_kind_name(schema.payload_kind))?;
    let field_items = schema
        .fields
        .iter()
        .map(|field| {
            let item = PyDict::new(py);
            item.set_item("name", field.name)?;
            item.set_item(
                "role",
                match field.role {
                    crate::schema::FieldRole::Header => "header",
                    crate::schema::FieldRole::Payload => "payload",
                },
            )?;
            item.set_item("offset", field.offset)?;
            match field.ty {
                crate::schema::FieldType::Scalar(scalar) => {
                    item.set_item("kind", "scalar")?;
                    item.set_item("scalar", scalar.name())?;
                    item.set_item("wire_size", scalar.wire_size())?;
                }
                crate::schema::FieldType::Array { element, len } => {
                    item.set_item("kind", "array")?;
                    item.set_item("scalar", element.name())?;
                    item.set_item("len", len)?;
                    item.set_item("wire_size", element.wire_size().saturating_mul(len))?;
                }
                crate::schema::FieldType::NestedArray { type_hash, len } => {
                    item.set_item("kind", "nested_array")?;
                    item.set_item("nested_type_hash", type_hash)?;
                    item.set_item("len", len)?;
                }
                crate::schema::FieldType::Nested { type_hash } => {
                    item.set_item("kind", "nested")?;
                    item.set_item("nested_type_hash", type_hash)?;
                }
                crate::schema::FieldType::Opaque { wire_size } => {
                    item.set_item("kind", "opaque")?;
                    item.set_item("wire_size", wire_size)?;
                }
                crate::schema::FieldType::PayloadSection => {
                    item.set_item("kind", "payload_section")?;
                    item.set_item("wire_size", 8usize)?;
                }
                crate::schema::FieldType::Bytes => {
                    item.set_item("kind", "bytes")?;
                }
            }
            Ok::<_, pyo3::PyErr>(item.into_any().unbind())
        })
        .collect::<PyResult<Vec<_>>>()?;
    let fields = PyTuple::new(py, field_items)?;
    dict.set_item("fields", fields)?;
    Ok(dict.into())
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
    if info.payload_kind == crate::registry::PayloadKind::Fixed && !payload.is_empty() {
        return Err(pyo3::exceptions::PyValueError::new_err(format!(
            "fixed-size datapod wire message cannot carry payload bytes: got {}",
            payload.len()
        )));
    }
    let total_len = header.len().checked_add(payload.len()).ok_or_else(|| {
        pyo3::exceptions::PyValueError::new_err(
            "wire header + payload length overflows addressable memory",
        )
    })?;
    let mut wire = Vec::new();
    wire.try_reserve_exact(total_len).map_err(|error| {
        pyo3::exceptions::PyMemoryError::new_err(format!(
            "failed to reserve {total_len} wire message bytes: {error}"
        ))
    })?;
    wire.extend_from_slice(&header);
    wire.extend_from_slice(&payload);
    crate::validate_registered_wire_v1(type_hash, &wire)
        .map_err(|error| pyo3::exceptions::PyValueError::new_err(error.to_string()))?;
    Ok(wire)
}

fn copy_bytes_for_python(label: &str, bytes: &[u8]) -> PyResult<Vec<u8>> {
    let mut out = Vec::new();
    out.try_reserve_exact(bytes.len()).map_err(|error| {
        pyo3::exceptions::PyMemoryError::new_err(format!(
            "failed to reserve {} {label} bytes: {error}",
            bytes.len()
        ))
    })?;
    out.extend_from_slice(bytes);
    Ok(out)
}

#[pyfunction]
fn split_wire_message(type_hash: u64, wire: Vec<u8>) -> PyResult<(Vec<u8>, Vec<u8>)> {
    crate::validate_registered_wire_v1(type_hash, &wire)
        .map_err(|error| pyo3::exceptions::PyValueError::new_err(error.to_string()))?;
    let header_size = header_size_v1(type_hash)?;
    if wire.len() < header_size {
        return Err(pyo3::exceptions::PyValueError::new_err(format!(
            "wire message too short: got {}, need at least {}",
            wire.len(),
            header_size
        )));
    }
    let header_range = wire.get(..header_size).ok_or_else(|| {
        pyo3::exceptions::PyValueError::new_err("wire header range is out of bounds")
    })?;
    let payload_range = wire.get(header_size..).ok_or_else(|| {
        pyo3::exceptions::PyValueError::new_err("wire payload range is out of bounds")
    })?;
    let header = copy_bytes_for_python("wire header", header_range)?;
    let payload = copy_bytes_for_python("wire payload", payload_range)?;
    Ok((header, payload))
}

#[pyfunction]
fn split_wire_message_v1(type_hash: u64, wire: Vec<u8>) -> PyResult<(Vec<u8>, Vec<u8>)> {
    crate::validate_registered_wire_v1(type_hash, &wire)
        .map_err(|error| pyo3::exceptions::PyValueError::new_err(error.to_string()))?;
    let header_size = header_size_v1(type_hash)?;
    let header_range = wire.get(..header_size).ok_or_else(|| {
        pyo3::exceptions::PyValueError::new_err("wire header range is out of bounds")
    })?;
    let payload_range = wire.get(header_size..).ok_or_else(|| {
        pyo3::exceptions::PyValueError::new_err("wire payload range is out of bounds")
    })?;
    let header = copy_bytes_for_python("wire header", header_range)?;
    let payload = copy_bytes_for_python("wire payload", payload_range)?;
    Ok((header, payload))
}

#[pyfunction]
fn validate_wire_message(type_hash: u64, wire: Vec<u8>) -> PyResult<()> {
    crate::validate_registered_wire_v1(type_hash, &wire)
        .map_err(|error| pyo3::exceptions::PyValueError::new_err(error.to_string()))
}

#[pyfunction]
fn validate_wire_message_v1(type_hash: u64, wire: Vec<u8>) -> PyResult<()> {
    crate::validate_registered_wire_v1(type_hash, &wire)
        .map_err(|error| pyo3::exceptions::PyValueError::new_err(error.to_string()))
}

#[pyfunction]
#[pyo3(signature = (type_hash, header, payload=None))]
fn validate_wire_frame_parts_v1(
    type_hash: u64,
    header: Vec<u8>,
    payload: Option<Vec<u8>>,
) -> PyResult<()> {
    let payload = payload.unwrap_or_default();
    crate::validate_registered_wire_frame_v1(crate::WireFrame {
        type_hash,
        header: &header,
        payload: &payload,
    })
    .map_err(|error| pyo3::exceptions::PyValueError::new_err(error.to_string()))
}

#[pymodule]
fn datapod(m: &Bound<'_, PyModule>) -> PyResult<()> {
    register_python_module(m)
}
