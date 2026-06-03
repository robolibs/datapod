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
    m.add_function(wrap_pyfunction!(join_wire_message, m)?)?;
    m.add_function(wrap_pyfunction!(split_wire_message, m)?)?;
    m.add_function(wrap_pyfunction!(split_wire_message_v1, m)?)?;
    m.add_function(wrap_pyfunction!(validate_wire_message, m)?)?;
    m.add_function(wrap_pyfunction!(validate_wire_message_v1, m)?)?;
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
        if let Some(info) = crate::registry::find_type_info(type_hash) {
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
import typing as _datapod_typing


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
        if not isinstance(count, int) or count <= 0:
            raise TypeError("datapod.array count must be a positive int")
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
    return getattr(annotation, "__name__", None)


def _format_body(format):
    if format and format[0] in "@=<>!":
        return format[1:]
    return format


def _is_fixed_datapod_annotation(annotation):
    return (
        hasattr(annotation, "__datapod_header_format__")
        and getattr(annotation, "__datapod_payload_field__", None) is None
    )


def _resolve_header_annotation(annotation):
    origin = _datapod_typing.get_origin(annotation)
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
            item, count = body.rsplit(",", 1)
            count = int(count)
            if count <= 0:
                raise TypeError("datapod.array count must be a positive int")
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
        return _format_body(annotation.__datapod_header_format__)
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
        return annotation.__datapod_header_arity__
    return 1


def _construct_from_kwargs(cls, kwargs):
    try:
        return cls(**kwargs)
    except TypeError:
        obj = cls.__new__(cls)
        for name, value in kwargs.items():
            setattr(obj, name, value)
        return obj


def _pack_header_field(annotation, value, name, arity):
    annotation = _resolve_header_annotation(annotation)
    if _is_fixed_datapod_annotation(annotation):
        header = annotation.__datapod_pack_header__(value)
        return _datapod_struct.unpack(annotation.__datapod_header_format__, header)
    if isinstance(annotation, _DatapodArray) or _string_array_parts(annotation) is not None:
        items = tuple(value)
        if len(items) != arity:
            raise ValueError(
                f"field {name} expected {arity} items, got {len(items)}"
            )
        return items
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
            "canonical_name": annotation.__datapod_canonical_name__,
            "type_hash": annotation.TYPE_HASH,
            "header_format": annotation.__datapod_header_format__,
            "fields": annotation.__datapod_fields__,
        }
    name = _annotation_name(annotation)
    return {"kind": "python", "name": name or repr(annotation)}


def _class_schema(cls):
    type_hash = getattr(cls, "TYPE_HASH")
    fields = tuple(
        {
            "name": name,
            "arity": arity,
            "annotation": _annotation_schema(getattr(cls, "__annotations__", {}).get(name)),
        }
        for name, arity in zip(
            getattr(cls, "__datapod_fields__", ()),
            getattr(cls, "__datapod_field_arities__", ()),
        )
    )
    return {
        "canonical_name": getattr(cls, "__datapod_canonical_name__", None),
        "type_hash": type_hash,
        "canonical_type_hash": getattr(cls, "CANONICAL_TYPE_HASH", canonical_type_hash(type_hash)),
        "header_format": getattr(cls, "__datapod_header_format__", None),
        "header_size": header_size(type_hash),
        "payload_kind": payload_kind(type_hash),
        "payload_field": getattr(cls, "__datapod_payload_field__", None),
        "fields": fields,
        "wire_format": wire_format(type_hash),
        "emitted_hash_kind": emitted_hash_kind(type_hash),
    }


def describe_schema(target):
    """Return registry and declarative-field metadata for a datapod class/hash."""
    if isinstance(target, int):
        cls = __datapod_registry__.get(target)
        if cls is None:
            return {
                "type_hash": target,
                "canonical_type_hash": canonical_type_hash(target),
                "header_size": header_size(target),
                "payload_kind": payload_kind(target),
                "wire_format": wire_format(target),
                "emitted_hash_kind": emitted_hash_kind(target),
            }
        return _class_schema(cls)
    cls = target if isinstance(target, type) else target.__class__
    return _class_schema(cls)


def _is_payload_annotation(annotation):
    name = _annotation_name(annotation)
    return annotation in (bytes, bytearray, memoryview) or name in (
        "bytes",
        "bytearray",
        "memoryview",
    )


def is_valid_wire_message(type_hash, wire):
    try:
        validate_wire_message_v1(type_hash, wire)
        return True
    except ValueError:
        return False


def is_valid_wire_message_v1(type_hash, wire):
    try:
        validate_wire_message_v1(type_hash, wire)
        return True
    except ValueError:
        return False


class WireFrame:
    """Borrowed Python datapod frame: type_hash + header memoryview + payload memoryview."""

    __slots__ = ("type_hash", "header", "payload", "_owner")

    def __init__(self, type_hash, header, payload=b"", owner=None):
        self.type_hash = int(type_hash)
        self.header = memoryview(header)
        self.payload = memoryview(payload)
        self._owner = owner

    def joined_len(self):
        return len(self.header) + len(self.payload)

    def to_wire_message(self):
        return self.header.tobytes() + self.payload.tobytes()


def wire_frame(obj):
    return obj.to_wire_frame()


def view_wire_frame(type_or_hash, frame):
    if isinstance(type_or_hash, type):
        if hasattr(type_or_hash, "view_from_wire_frame"):
            return type_or_hash.view_from_wire_frame(frame)
        expected = getattr(type_or_hash, "TYPE_HASH")
    else:
        expected = int(type_or_hash)
    if frame.type_hash != expected:
        raise ValueError(
            f"wrong frame type hash: got {frame.type_hash}, expected {expected}"
        )
    validate_wire_frame(expected, frame.header, frame.payload)
    return frame.header, frame.payload


def validate_wire_frame(type_hash, header, payload=b""):
    header_view = memoryview(header)
    payload_view = memoryview(payload)
    expected = header_size_v1(type_hash)
    if len(header_view) != expected:
        raise ValueError(
            f"wrong frame header length: got {len(header_view)}, expected {expected}"
        )
    if payload_kind(type_hash) == "fixed" and len(payload_view) != 0:
        raise ValueError(
            f"fixed datapod frame payload must be empty, got {len(payload_view)}"
        )


def validate_wire_frame_v1(type_hash, header, payload=b""):
    validate_wire_frame(type_hash, header, payload)


def is_valid_wire_frame(type_hash, header, payload=b""):
    try:
        validate_wire_frame(type_hash, header, payload)
        return True
    except ValueError:
        return False


def is_valid_wire_frame_v1(type_hash, header, payload=b""):
    return is_valid_wire_frame(type_hash, header, payload)


def split_wire_message_view(type_hash, wire):
    view = memoryview(wire)
    validate_wire_message_v1(type_hash, view.tobytes())
    size = header_size_v1(type_hash)
    return bytes(view[:size]), view[size:]


def split_wire_message_view_v1(type_hash, wire):
    view = memoryview(wire)
    validate_wire_message_v1(type_hash, view.tobytes())
    size = header_size_v1(type_hash)
    return bytes(view[:size]), view[size:]


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
    if header_format is None:
        header_format = header
    elif header is not None and header != header_format:
        raise TypeError("datapod_type got conflicting header_format and header values")
    if header_format is None and byte_order not in ("@", "=", "<", ">", "!"):
        raise TypeError("datapod_type byte_order must be one of '@', '=', '<', '>', '!'")
    if payload_field is None:
        payload_field = payload
    elif payload is not None and payload != payload_field:
        raise TypeError("datapod_type got conflicting payload_field and payload values")
    if validate is not None and not callable(validate):
        raise TypeError("datapod_type validate must be callable")
    if dataclass not in (False, True):
        raise TypeError("datapod_type dataclass must be a bool")

    def decorate(cls):
        if dataclass and not _datapod_dataclasses.is_dataclass(cls):
            cls = _datapod_dataclasses.dataclass(cls)

        local_fields = fields
        local_payload_field = payload_field
        annotations = getattr(cls, "__annotations__", {})
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
            local_fields = getattr(cls, "__datapod_fields__", None)
        if local_fields is None:
            if header_format is None:
                local_fields = tuple(
                    name for name, annotation in annotations.items()
                    if name != local_payload_field and not _is_payload_annotation(annotation)
                )
            else:
                local_fields = tuple(name for name in annotations if name != local_payload_field)
        local_fields = tuple(local_fields)
        local_header_format = header_format
        if local_header_format is None:
            local_header_format = byte_order + "".join(
                _scalar_format(annotations[name]) for name in local_fields
            )
        local_field_arities = tuple(
            _field_arity(annotations[name]) if name in annotations else 1
            for name in local_fields
        )
        local_header_size = _datapod_struct.calcsize(local_header_format)
        if not local_fields and local_header_size:
            raise TypeError("datapod_type needs header fields for a non-empty header")

        kind = "bytes" if local_payload_field is not None else "fixed"
        type_id = register_type(cls, canonical_name, local_header_size, kind, type_hash)
        canonical_type_id = canonical_type_hash(type_id)

        cls.__datapod_canonical_name__ = canonical_name
        cls.CANONICAL_TYPE_HASH = canonical_type_id
        cls.__datapod_header_format__ = local_header_format
        cls.__datapod_fields__ = local_fields
        cls.__datapod_field_arities__ = local_field_arities
        cls.__datapod_header_arity__ = sum(local_field_arities)
        cls.__datapod_payload_field__ = local_payload_field
        cls.__datapod_validator__ = validate

        def run_custom_validator(obj):
            if validate is None:
                return
            if not validate(obj):
                raise ValueError("datapod custom validator rejected value")

        def pack_header(obj):
            values = []
            for name, arity in zip(local_fields, local_field_arities):
                annotation = annotations.get(name)
                value = getattr(obj, name)
                if annotation is None:
                    values.append(value)
                    continue
                values.extend(_pack_header_field(annotation, value, name, arity))
            return _datapod_struct.pack(local_header_format, *values)

        def unpack_kwargs(header):
            values = _datapod_struct.unpack(local_header_format, header)
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

        def schema_for_type(inner_cls):
            return describe_schema(inner_cls)

        cls.schema = classmethod(schema_for_type)
        cls.describe_schema = classmethod(schema_for_type)

        def to_wire_message(self):
            return to_wire_message_v1(self)

        def to_wire_message_v1(self):
            frame = to_wire_frame_v1(self)
            return frame.type_hash, frame.to_wire_message()

        def to_wire_frame(self):
            return to_wire_frame_v1(self)

        def to_wire_frame_v1(self):
            if not local_header_format.startswith("<"):
                raise ValueError(
                    "datapod-wire-v1/le requires an explicit little-endian '<' header format"
                )
            run_custom_validator(self)
            header = pack_header(self)
            payload = b"" if local_payload_field is None else memoryview(getattr(self, local_payload_field))
            return WireFrame(canonical_type_id, header, payload)

        @classmethod
        def from_wire_message(inner_cls, incoming_hash, wire):
            return from_wire_message_v1.__func__(inner_cls, incoming_hash, wire)

        @classmethod
        def from_wire_message_v1(inner_cls, incoming_hash, wire):
            if incoming_hash != canonical_type_id:
                raise ValueError(
                    f"wrong v1 type hash: got {incoming_hash}, expected canonical {canonical_type_id}"
                )
            header, payload = split_wire_message_v1(incoming_hash, wire)
            kwargs = unpack_kwargs(header)
            if local_payload_field is not None:
                kwargs[local_payload_field] = payload
            obj = _construct_from_kwargs(inner_cls, kwargs)
            run_custom_validator(obj)
            return obj

        @classmethod
        def from_wire_frame(inner_cls, frame):
            return from_wire_frame_v1.__func__(inner_cls, frame)

        @classmethod
        def from_wire_frame_v1(inner_cls, frame):
            if frame.type_hash != canonical_type_id:
                raise ValueError(
                    f"wrong v1 type hash: got {frame.type_hash}, expected canonical {canonical_type_id}"
                )
            validate_wire_frame_v1(frame.type_hash, frame.header, frame.payload)
            kwargs = unpack_kwargs(frame.header)
            if local_payload_field is not None:
                kwargs[local_payload_field] = frame.payload.tobytes()
            obj = _construct_from_kwargs(inner_cls, kwargs)
            run_custom_validator(obj)
            return obj

        @classmethod
        def validate_wire_message_for_type(inner_cls, incoming_hash, wire):
            validate_wire_message_v1_for_type.__func__(inner_cls, incoming_hash, wire)

        @classmethod
        def validate_wire_message_v1_for_type(inner_cls, incoming_hash, wire):
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
            if incoming_hash != canonical_type_id:
                raise ValueError(
                    f"wrong v1 type hash: got {incoming_hash}, expected canonical {canonical_type_id}"
                )
            header, payload = split_wire_message_view_v1(incoming_hash, wire)
            kwargs = unpack_kwargs(header)
            obj = inner_cls.__new__(inner_cls)
            for name, value in kwargs.items():
                setattr(obj, name, value)
            if local_payload_field is not None:
                setattr(obj, local_payload_field, payload)
            run_custom_validator(obj)
            return obj

        @classmethod
        def view_from_wire_frame(inner_cls, frame):
            return view_from_wire_frame_v1.__func__(inner_cls, frame)

        @classmethod
        def view_from_wire_frame_v1(inner_cls, frame):
            if frame.type_hash != canonical_type_id:
                raise ValueError(
                    f"wrong v1 type hash: got {frame.type_hash}, expected canonical {canonical_type_id}"
                )
            validate_wire_frame_v1(frame.type_hash, frame.header, frame.payload)
            kwargs = unpack_kwargs(frame.header)
            obj = inner_cls.__new__(inner_cls)
            for name, value in kwargs.items():
                setattr(obj, name, value)
            if local_payload_field is not None:
                setattr(obj, local_payload_field, frame.payload)
            run_custom_validator(obj)
            return obj

        cls.to_wire_message = to_wire_message
        cls.to_wire_message_v1 = to_wire_message_v1
        cls.to_wire_frame = to_wire_frame
        cls.to_wire_frame_v1 = to_wire_frame_v1
        cls.from_wire_message = from_wire_message
        cls.from_wire_message_v1 = from_wire_message_v1
        cls.from_wire_frame = from_wire_frame
        cls.from_wire_frame_v1 = from_wire_frame_v1
        cls.validate_wire_message = validate_wire_message_for_type
        cls.validate_wire_message_v1 = validate_wire_message_v1_for_type
        cls.view_from_wire = view_from_wire
        cls.view_from_wire_v1 = view_from_wire_v1
        cls.view_from_wire_frame = view_from_wire_frame
        cls.view_from_wire_frame_v1 = view_from_wire_frame_v1
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
    module.add("canonical_type_hash", m.getattr("canonical_type_hash")?)?;
    module.add("join_wire_message", m.getattr("join_wire_message")?)?;
    module.add("split_wire_message", m.getattr("split_wire_message")?)?;
    module.add("split_wire_message_v1", m.getattr("split_wire_message_v1")?)?;
    module.add("validate_wire_message", m.getattr("validate_wire_message")?)?;
    module.add(
        "validate_wire_message_v1",
        m.getattr("validate_wire_message_v1")?,
    )?;
    module.add("header_size", m.getattr("header_size")?)?;
    module.add("header_size_v1", m.getattr("header_size_v1")?)?;
    module.add("payload_kind", m.getattr("payload_kind")?)?;
    module.add("wire_format", m.getattr("wire_format")?)?;
    module.add("emitted_hash_kind", m.getattr("emitted_hash_kind")?)?;
    module.add("__datapod_registry__", m.getattr("__datapod_registry__")?)?;
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
    m.add("wire_frame", module.getattr("wire_frame")?)?;
    m.add("view_wire_frame", module.getattr("view_wire_frame")?)?;
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
    Ok(())
}

#[pyfunction]
fn to_wire_message(obj: &Bound<'_, PyAny>) -> PyResult<(u64, Vec<u8>)> {
    obj.call_method0("to_wire_message")?.extract()
}

#[pyfunction]
fn to_wire_message_v1(obj: &Bound<'_, PyAny>) -> PyResult<(u64, Vec<u8>)> {
    obj.call_method0("to_wire_message_v1")?.extract()
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
    let canonical_type_hash = crate::registry::find_type_info(type_hash)
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
    validate_wire_message_v1(type_hash, wire.clone())?;
    let header_size = header_size_v1(type_hash)?;
    if wire.len() < header_size {
        return Err(pyo3::exceptions::PyValueError::new_err(format!(
            "wire message too short: got {}, need at least {}",
            wire.len(),
            header_size
        )));
    }
    let payload = wire[header_size..].to_vec();
    let header = wire[..header_size].to_vec();
    Ok((header, payload))
}

#[pyfunction]
fn split_wire_message_v1(type_hash: u64, wire: Vec<u8>) -> PyResult<(Vec<u8>, Vec<u8>)> {
    validate_wire_message_v1(type_hash, wire.clone())?;
    let header_size = header_size_v1(type_hash)?;
    let payload = wire[header_size..].to_vec();
    let header = wire[..header_size].to_vec();
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

#[pymodule]
fn datapod(m: &Bound<'_, PyModule>) -> PyResult<()> {
    register_python_module(m)
}
