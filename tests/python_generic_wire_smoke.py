"""Smoke test for the Python generic datapod wire API.

Run against an installed/built wheel, for example:

    python tests/python_generic_wire_smoke.py

The Makefile's `make bind` builds the wheel; CI or local scripts can install it
into an isolated target directory before running this file.
"""

import ctypes
import dataclasses
import pathlib
import platform
import subprocess

import datapod


ROOT = pathlib.Path(__file__).resolve().parents[1]


def assert_round_trip(value):
    type_hash, wire = datapod.to_wire_message(value)
    decoded = datapod.from_wire_message(type_hash, wire)
    assert type(decoded).__name__ == type(value).__name__
    return decoded


def run_fixture(*args):
    exe = ROOT / "target" / "debug" / "examples" / "wire_fixture"
    completed = subprocess.run(
        [str(exe), *map(str, args)],
        cwd=ROOT,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return completed.stdout.strip()


def rust_encoded(command):
    type_hash, wire_hex = run_fixture(command).split()
    return int(type_hash), bytes.fromhex(wire_hex)


class DatapodPoint(ctypes.Structure):
    _fields_ = [("x", ctypes.c_double), ("y", ctypes.c_double), ("z", ctypes.c_double)]


class DatapodOwnedBytes(ctypes.Structure):
    _fields_ = [
        ("ptr", ctypes.POINTER(ctypes.c_uint8)),
        ("len", ctypes.c_size_t),
        ("capacity", ctypes.c_size_t),
    ]


class DatapodWireMessage(ctypes.Structure):
    _fields_ = [
        ("type_hash", ctypes.c_uint64),
        ("data", ctypes.POINTER(ctypes.c_uint8)),
        ("len", ctypes.c_size_t),
    ]


class DatapodWireFrame(ctypes.Structure):
    _fields_ = [
        ("type_hash", ctypes.c_uint64),
        ("header", ctypes.POINTER(ctypes.c_uint8)),
        ("header_len", ctypes.c_size_t),
        ("payload", ctypes.POINTER(ctypes.c_uint8)),
        ("payload_len", ctypes.c_size_t),
    ]


class DatapodBytes(ctypes.Structure):
    _fields_ = [("ptr", ctypes.POINTER(ctypes.c_uint8)), ("len", ctypes.c_size_t)]


def load_c_abi():
    names = {
        "Linux": "libdatapod.so",
        "Darwin": "libdatapod.dylib",
        "Windows": "datapod.dll",
    }
    lib = ctypes.CDLL(str(ROOT / "target" / "debug" / names[platform.system()]))
    lib.datapod_point_to_wire.argtypes = [DatapodPoint, ctypes.POINTER(DatapodOwnedBytes)]
    lib.datapod_point_to_wire.restype = ctypes.c_bool
    lib.datapod_point_type_hash.restype = ctypes.c_uint64
    lib.datapod_owned_bytes_free.argtypes = [DatapodOwnedBytes]
    lib.datapod_wire_message_borrow.argtypes = [
        ctypes.c_uint64,
        ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t,
    ]
    lib.datapod_wire_message_borrow.restype = DatapodWireMessage
    lib.datapod_wire_message_is_valid.argtypes = [DatapodWireMessage]
    lib.datapod_wire_message_is_valid.restype = ctypes.c_bool
    lib.datapod_wire_message_validate.argtypes = [DatapodWireMessage]
    lib.datapod_wire_message_validate.restype = ctypes.c_bool
    lib.datapod_wire_message_validate_v1.argtypes = [DatapodWireMessage]
    lib.datapod_wire_message_validate_v1.restype = ctypes.c_bool
    lib.datapod_wire_message_is_valid_v1.argtypes = [DatapodWireMessage]
    lib.datapod_wire_message_is_valid_v1.restype = ctypes.c_bool
    lib.datapod_wire_message_header.argtypes = [DatapodWireMessage]
    lib.datapod_wire_message_header.restype = DatapodBytes
    lib.datapod_wire_message_payload.argtypes = [DatapodWireMessage]
    lib.datapod_wire_message_payload.restype = DatapodBytes
    lib.datapod_wire_message_header_v1.argtypes = [DatapodWireMessage]
    lib.datapod_wire_message_header_v1.restype = DatapodBytes
    lib.datapod_wire_message_payload_v1.argtypes = [DatapodWireMessage]
    lib.datapod_wire_message_payload_v1.restype = DatapodBytes
    lib.datapod_wire_frame_borrow.argtypes = [
        ctypes.c_uint64,
        ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t,
    ]
    lib.datapod_wire_frame_borrow.restype = DatapodWireFrame
    lib.datapod_wire_frame_from_message.argtypes = [
        DatapodWireMessage,
        ctypes.POINTER(DatapodWireFrame),
    ]
    lib.datapod_wire_frame_from_message.restype = ctypes.c_bool
    lib.datapod_wire_frame_validate.argtypes = [DatapodWireFrame]
    lib.datapod_wire_frame_validate.restype = ctypes.c_bool
    lib.datapod_header_size.argtypes = [ctypes.c_uint64]
    lib.datapod_header_size.restype = ctypes.c_size_t
    lib.datapod_header_size_v1.argtypes = [ctypes.c_uint64]
    lib.datapod_header_size_v1.restype = ctypes.c_size_t
    lib.datapod_type_hash_name.argtypes = [ctypes.c_char_p]
    lib.datapod_type_hash_name.restype = ctypes.c_uint64
    lib.datapod_canonical_type_hash.argtypes = [ctypes.c_uint64]
    lib.datapod_canonical_type_hash.restype = ctypes.c_uint64
    lib.datapod_emitted_type_hash.argtypes = [ctypes.c_uint64]
    lib.datapod_emitted_type_hash.restype = ctypes.c_uint64
    lib.datapod_emitted_hash_kind.argtypes = [ctypes.c_uint64]
    lib.datapod_emitted_hash_kind.restype = ctypes.c_uint32
    lib.datapod_hash_kind_canonical_name.restype = ctypes.c_uint32
    lib.datapod_current_wire_format_name.restype = ctypes.c_char_p
    lib.datapod_builtin_hash_policy.restype = ctypes.c_char_p
    lib.datapod_payload_kind_bytes.restype = ctypes.c_uint32
    lib.datapod_register_type_name.argtypes = [
        ctypes.c_char_p,
        ctypes.c_size_t,
        ctypes.c_uint32,
    ]
    lib.datapod_register_type_name.restype = ctypes.c_uint64
    lib.datapod_register_type.argtypes = [
        ctypes.c_uint64,
        ctypes.c_char_p,
        ctypes.c_size_t,
        ctypes.c_uint32,
    ]
    lib.datapod_register_type.restype = ctypes.c_bool
    lib.datapod_wire_message_join.argtypes = [
        ctypes.c_uint64,
        ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t,
        ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t,
        ctypes.POINTER(DatapodOwnedBytes),
    ]
    lib.datapod_wire_message_join.restype = ctypes.c_bool
    return lib


def owned_bytes_to_py(owned):
    return ctypes.string_at(owned.ptr, owned.len)


def borrowed_bytes_to_py(view):
    return ctypes.string_at(view.ptr, view.len)


@datapod.datapod_type(
    "acme.custom_packet.v1",
    "<HH",
    ("channel", "flags"),
    payload_field="data",
)
class CustomPacket:
    def __init__(self, channel, flags, data):
        self.channel = channel
        self.flags = flags
        self.data = bytes(data)


@datapod.datapod_type(
    "acme.validated_packet.v1",
    header="<H",
    payload="data",
    validate=lambda packet: packet.channel > 0,
)
class ValidatedPacket:
    channel: int
    data: bytes

    def __init__(self, channel, data):
        self.channel = channel
        self.data = bytes(data)


@datapod.datapod_type(
    "acme.auto_dataclass_packet.v1",
    header="<HB",
    payload=True,
    dataclass=True,
)
class AutoDataclassPacket:
    channel: int
    flags: int
    data: bytes


@datapod.datapod_type(
    "acme.annotated_dataclass_packet.v1",
    payload=True,
    dataclass=True,
)
class AnnotatedDataclassPacket:
    channel: datapod.u16
    flags: datapod.u8
    counter: datapod.i32
    data: bytes


@datapod.datapod_type(
    "acme.array_packet.v1",
    payload=True,
    dataclass=True,
)
class ArrayPacket:
    rgb: datapod.array(datapod.u8, 3)
    gain: datapod.f32
    data: bytes


@datapod.datapod_type(
    "acme.string_array_packet.v1",
    payload=True,
    dataclass=True,
)
class StringArrayPacket:
    rgb: "datapod.array(datapod.u8, 3)"
    data: bytes


@datapod.datapod_type(
    "acme.python_stamp.v1",
    dataclass=True,
)
class PythonStamp:
    sec: datapod.u32
    nsec: datapod.u32


@datapod.datapod_type(
    "acme.nested_packet.v1",
    payload=True,
    dataclass=True,
)
class NestedPacket:
    stamp: PythonStamp
    flags: datapod.u8
    data: bytes


def main():
    point = assert_round_trip(datapod.Point(1.0, 2.0, 3.0))
    assert (point.x, point.y, point.z) == (1.0, 2.0, 3.0)
    point_v1_hash, point_v1_wire = datapod.to_wire_message_v1(datapod.Point(1.0, 2.0, 3.0))
    assert point_v1_hash == datapod.canonical_type_hash(datapod.Point.TYPE_HASH)
    assert point_v1_hash == datapod.Point.TYPE_HASH
    assert datapod.header_size_v1(point_v1_hash) == len(datapod.Point(0, 0, 0).to_header_bytes())
    datapod.validate_wire_message_v1(point_v1_hash, point_v1_wire)

    grid = assert_round_trip(
        datapod.Grid(2, 2, 11, False, 1.0, [0, 0, 0, 1, 0, 0, 0], bytes(range(16)))
    )
    assert (grid.rows, grid.cols, grid.encoding_id) == (2, 2, 11)
    assert grid.payload_bytes() == bytes(range(16))

    matrix = assert_round_trip(datapod.Matrix(2, 3, 1, bytes([1, 2, 3, 4, 5, 6])))
    assert (matrix.rows, matrix.cols, matrix.element_size) == (2, 3, 1)
    assert matrix.payload_bytes() == bytes([1, 2, 3, 4, 5, 6])

    raw = assert_round_trip(datapod.Bytes([9, 8, 7]))
    assert raw.payload_bytes() == bytes([9, 8, 7])

    rust_grid_hash, rust_grid_wire = rust_encoded("encode-grid")
    rust_grid = datapod.from_wire_message(rust_grid_hash, rust_grid_wire)
    assert type(rust_grid).__name__ == "Grid"
    assert (rust_grid.rows, rust_grid.cols, rust_grid.encoding_id) == (2, 2, 11)
    assert rust_grid.payload_bytes() == bytes(range(16))

    rust_matrix_hash, rust_matrix_wire = rust_encoded("encode-matrix")
    rust_matrix = datapod.from_wire_message(rust_matrix_hash, rust_matrix_wire)
    assert type(rust_matrix).__name__ == "Matrix"
    assert (rust_matrix.rows, rust_matrix.cols, rust_matrix.element_size) == (2, 3, 1)
    assert rust_matrix.payload_bytes() == bytes([1, 2, 3, 4, 5, 6])

    py_grid = datapod.Grid(2, 2, 11, False, 0.5, [0, 0, 0, 1, 0, 0, 0], bytes(range(16)))
    py_grid_hash, py_grid_wire = datapod.to_wire_message(py_grid)
    py_grid_canonical_hash = datapod.canonical_type_hash(py_grid_hash)
    assert py_grid_canonical_hash == py_grid_hash
    assert datapod.header_size(py_grid_canonical_hash) == datapod.header_size(py_grid_hash)
    canonical_grid = datapod.from_wire_message(py_grid_canonical_hash, py_grid_wire)
    assert type(canonical_grid).__name__ == "Grid"
    assert canonical_grid.payload_bytes() == bytes(range(16))
    datapod.validate_wire_message(py_grid_hash, py_grid_wire)
    datapod.validate_wire_message(py_grid_canonical_hash, py_grid_wire)
    assert datapod.is_valid_wire_message(py_grid_hash, py_grid_wire)
    assert not datapod.is_valid_wire_message(py_grid_hash, py_grid_wire[:-1])
    assert run_fixture("decode-grid", py_grid_hash, py_grid_wire.hex()) == "ok grid"

    py_matrix = datapod.Matrix(2, 3, 1, bytes([1, 2, 3, 4, 5, 6]))
    py_matrix_hash, py_matrix_wire = datapod.to_wire_message(py_matrix)
    py_matrix_v1_hash, py_matrix_v1_wire = datapod.to_wire_message_v1(py_matrix)
    assert py_matrix_v1_hash == datapod.canonical_type_hash(py_matrix_hash)
    datapod.validate_wire_message_v1(py_matrix_v1_hash, py_matrix_v1_wire)
    assert datapod.is_valid_wire_message_v1(py_matrix_v1_hash, py_matrix_v1_wire)
    assert not datapod.is_valid_wire_message_v1(datapod.Matrix.TYPE_HASH ^ 1, py_matrix_v1_wire)
    py_matrix_frame = py_matrix.to_wire_frame()
    assert isinstance(py_matrix_frame, datapod.WireFrame)
    assert py_matrix_frame.type_hash == py_matrix_hash
    assert py_matrix_frame._owner is py_matrix
    assert py_matrix_frame.payload.tobytes() == bytes([1, 2, 3, 4, 5, 6])
    datapod.validate_wire_frame(py_matrix_frame.type_hash, py_matrix_frame.header, py_matrix_frame.payload)
    py_matrix_frame_view = datapod.Matrix.view_from_wire_frame(py_matrix_frame)
    assert (py_matrix_frame_view.rows, py_matrix_frame_view.cols, py_matrix_frame_view.element_size) == (2, 3, 1)
    assert isinstance(py_matrix_frame_view.payload, memoryview)
    assert py_matrix_frame_view.payload.obj is py_matrix_frame.payload.obj
    assert py_matrix_frame_view.payload.tobytes() == bytes([1, 2, 3, 4, 5, 6])
    generic_matrix_frame_view = datapod.view_wire_frame(datapod.Matrix, py_matrix_frame)
    assert generic_matrix_frame_view.payload.tobytes() == bytes([1, 2, 3, 4, 5, 6])
    py_matrix_from_frame = datapod.Matrix.from_wire_frame(py_matrix_frame)
    assert (py_matrix_from_frame.rows, py_matrix_from_frame.cols, py_matrix_from_frame.element_size) == (2, 3, 1)
    datapod.validate_wire_message(py_matrix_hash, py_matrix_wire)
    assert not datapod.is_valid_wire_message(py_matrix_hash, py_matrix_wire + b"\x00")
    assert run_fixture("decode-matrix", py_matrix_hash, py_matrix_wire.hex()) == "ok matrix"

    py_grid_frame = py_grid.to_wire_frame()
    assert py_grid_frame._owner is py_grid
    assert py_grid_frame.payload.tobytes() == bytes(range(16))
    datapod.validate_wire_frame(py_grid_frame.type_hash, py_grid_frame.header, py_grid_frame.payload)
    py_grid_frame_view = datapod.Grid.view_from_wire_frame(py_grid_frame)
    assert (py_grid_frame_view.rows, py_grid_frame_view.cols, py_grid_frame_view.encoding_id) == (2, 2, 11)
    assert isinstance(py_grid_frame_view.payload, memoryview)
    assert py_grid_frame_view.payload.obj is py_grid_frame.payload.obj
    assert py_grid_frame_view.payload.tobytes() == bytes(range(16))

    lib = load_c_abi()
    c_point_wire = DatapodOwnedBytes()
    assert lib.datapod_point_to_wire(DatapodPoint(1.0, 2.0, 3.0), ctypes.byref(c_point_wire))
    try:
        c_point_bytes = owned_bytes_to_py(c_point_wire)
        c_point_hash = lib.datapod_point_type_hash()
        c_point_wire_hash = lib.datapod_emitted_type_hash(c_point_hash)
        c_point_canonical_hash = lib.datapod_canonical_type_hash(c_point_hash)
        assert c_point_wire_hash == c_point_canonical_hash
        assert (
            lib.datapod_emitted_hash_kind(c_point_hash)
            == lib.datapod_hash_kind_canonical_name()
        )
        assert lib.datapod_current_wire_format_name() == b"datapod-wire-v1/le"
        assert b"canonical-name" in lib.datapod_builtin_hash_policy()
        c_point = datapod.from_wire_message(c_point_wire_hash, c_point_bytes)
        assert (c_point.x, c_point.y, c_point.z) == (1.0, 2.0, 3.0)
        assert run_fixture("decode-point", c_point_wire_hash, c_point_bytes.hex()) == "ok point"
    finally:
        lib.datapod_owned_bytes_free(c_point_wire)

    wire_buffer = ctypes.create_string_buffer(py_grid_wire)
    wire_ptr = ctypes.cast(wire_buffer, ctypes.POINTER(ctypes.c_uint8))
    message = lib.datapod_wire_message_borrow(py_grid_hash, wire_ptr, len(py_grid_wire))
    assert lib.datapod_wire_message_is_valid(message)
    assert lib.datapod_wire_message_validate(message)
    header = lib.datapod_wire_message_header(message)
    payload = lib.datapod_wire_message_payload(message)
    assert header.len == lib.datapod_header_size(py_grid_hash)
    assert borrowed_bytes_to_py(payload) == bytes(range(16))

    custom_hash = CustomPacket.TYPE_HASH
    assert custom_hash == datapod.type_hash_name("acme.custom_packet.v1")
    assert CustomPacket.__datapod_fields__ == ("channel", "flags")
    assert CustomPacket.__datapod_payload_field__ == "data"
    assert datapod.header_size(custom_hash) == 4
    assert datapod.payload_kind(custom_hash) == "bytes"
    assert datapod.format_version(custom_hash) == 1
    assert datapod.wire_format(custom_hash) == "datapod-wire-v1/le"
    assert datapod.current_wire_format() == "datapod-wire-v1/le"
    assert "canonical-name" in datapod.builtin_hash_policy()
    assert datapod.emitted_type_hash(custom_hash) == custom_hash
    assert datapod.emitted_hash_kind(custom_hash) == "canonical-name"
    assert datapod.endian(custom_hash) == "little"
    assert datapod.alignment_policy(custom_hash) == "unaligned-wire"
    assert datapod.validator_kind(custom_hash) == "runtime-schema"
    custom = CustomPacket(7, 3, b"payload")
    custom_wire_hash, custom_wire = datapod.to_wire_message(custom)
    assert custom_wire_hash == custom_hash
    custom_v1_hash, custom_v1_wire = datapod.to_wire_message_v1(custom)
    assert custom_v1_hash == CustomPacket.CANONICAL_TYPE_HASH
    assert custom_v1_hash == custom_hash
    assert custom_v1_wire == custom_wire
    custom_frame = custom.to_wire_frame()
    assert isinstance(custom_frame, datapod.WireFrame)
    assert custom_frame.type_hash == custom_hash
    assert custom_frame.header.tobytes() == custom_wire[:4]
    assert custom_frame.payload.tobytes() == b"payload"
    datapod.validate_wire_frame(custom_frame.type_hash, custom_frame.header, custom_frame.payload)
    assert datapod.is_valid_wire_frame(custom_frame.type_hash, custom_frame.header, custom_frame.payload)
    custom_from_frame = CustomPacket.from_wire_frame(custom_frame)
    assert (custom_from_frame.channel, custom_from_frame.flags, custom_from_frame.data) == (
        7,
        3,
        b"payload",
    )
    custom_frame_view = CustomPacket.view_from_wire_frame(custom_frame)
    assert (custom_frame_view.channel, custom_frame_view.flags) == (7, 3)
    assert isinstance(custom_frame_view.data, memoryview)
    assert custom_frame_view.data.tobytes() == b"payload"
    datapod.validate_wire_message_v1(custom_v1_hash, custom_v1_wire)
    assert datapod.is_valid_wire_message_v1(custom_v1_hash, custom_v1_wire)
    assert not datapod.is_valid_wire_message_v1(custom_v1_hash, custom_v1_wire[:1])
    custom_v1_header, custom_v1_payload = datapod.split_wire_message_v1(
        custom_v1_hash, custom_v1_wire
    )
    assert (custom_v1_header, custom_v1_payload) == (custom_wire[:4], b"payload")
    custom_v1_view = CustomPacket.view_from_wire_v1(custom_v1_hash, custom_v1_wire)
    assert (custom_v1_view.channel, custom_v1_view.flags) == (7, 3)
    assert custom_v1_view.data.tobytes() == b"payload"
    custom_v1_out = CustomPacket.from_wire_message_v1(custom_v1_hash, custom_v1_wire)
    assert (custom_v1_out.channel, custom_v1_out.flags, custom_v1_out.data) == (7, 3, b"payload")
    datapod.validate_wire_message(custom_wire_hash, custom_wire)
    assert datapod.is_valid_wire_message(custom_wire_hash, custom_wire)
    assert not datapod.is_valid_wire_message(custom_wire_hash, custom_wire[:1])
    custom_header, custom_payload = datapod.split_wire_message_view(custom_wire_hash, custom_wire)
    assert custom_header == custom_wire[:4]
    assert isinstance(custom_payload, memoryview)
    assert custom_payload.tobytes() == b"payload"
    custom_view = CustomPacket.view_from_wire(custom_wire_hash, custom_wire)
    assert (custom_view.channel, custom_view.flags) == (7, 3)
    assert isinstance(custom_view.data, memoryview)
    assert custom_view.data.tobytes() == b"payload"
    CustomPacket.validate_wire_message(custom_wire_hash, custom_wire)
    custom_out = datapod.from_wire_message(custom_wire_hash, custom_wire)
    assert type(custom_out).__name__ == "CustomPacket"
    assert (custom_out.channel, custom_out.flags, custom_out.data) == (7, 3, b"payload")

    validated = ValidatedPacket(9, b"ok")
    validated_hash, validated_wire = datapod.to_wire_message(validated)
    assert validated_hash == ValidatedPacket.TYPE_HASH
    validated_view = ValidatedPacket.view_from_wire(validated_hash, validated_wire)
    assert validated_view.channel == 9
    assert isinstance(validated_view.data, memoryview)
    try:
        datapod.to_wire_message(ValidatedPacket(0, b"bad"))
        raise AssertionError("custom Python validator should reject channel=0")
    except ValueError:
        pass
    invalid_validated_wire = datapod.join_wire_message(validated_hash, b"\x00\x00", b"bad")
    try:
        ValidatedPacket.view_from_wire(validated_hash, invalid_validated_wire)
        raise AssertionError("custom Python validator should reject decoded channel=0")
    except ValueError:
        pass

    assert dataclasses.is_dataclass(AutoDataclassPacket)
    assert AutoDataclassPacket.__datapod_fields__ == ("channel", "flags")
    assert AutoDataclassPacket.__datapod_payload_field__ == "data"
    auto_packet = AutoDataclassPacket(42, 7, b"auto")
    auto_hash, auto_wire = datapod.to_wire_message(auto_packet)
    assert auto_hash == AutoDataclassPacket.TYPE_HASH
    datapod.validate_wire_message(auto_hash, auto_wire)
    auto_out = datapod.from_wire_message(auto_hash, auto_wire)
    assert dataclasses.is_dataclass(auto_out)
    assert (auto_out.channel, auto_out.flags, auto_out.data) == (42, 7, b"auto")
    auto_view = AutoDataclassPacket.view_from_wire(auto_hash, auto_wire)
    assert (auto_view.channel, auto_view.flags) == (42, 7)
    assert isinstance(auto_view.data, memoryview)
    assert auto_view.data.tobytes() == b"auto"
    auto_payload = bytearray(b"zero-copy")
    auto_frame = AutoDataclassPacket(42, 7, auto_payload).to_wire_frame()
    assert auto_frame.payload.obj is auto_payload
    auto_frame_view = AutoDataclassPacket.view_from_wire_frame(auto_frame)
    assert auto_frame_view.data.obj is auto_payload
    auto_payload[0] = ord("Z")
    assert auto_frame_view.data.tobytes() == b"Zero-copy"

    assert dataclasses.is_dataclass(AnnotatedDataclassPacket)
    assert AnnotatedDataclassPacket.__datapod_header_format__ == "<HBi"
    assert AnnotatedDataclassPacket.__datapod_fields__ == ("channel", "flags", "counter")
    assert datapod.header_size(AnnotatedDataclassPacket.TYPE_HASH) == 7
    annotated_packet = AnnotatedDataclassPacket(17, 3, -12, b"typed")
    annotated_hash, annotated_wire = datapod.to_wire_message(annotated_packet)
    assert annotated_hash == AnnotatedDataclassPacket.TYPE_HASH
    annotated_v1_hash, annotated_v1_wire = datapod.to_wire_message_v1(annotated_packet)
    assert annotated_v1_hash == AnnotatedDataclassPacket.CANONICAL_TYPE_HASH
    assert annotated_v1_wire == annotated_wire
    datapod.validate_wire_message_v1(annotated_v1_hash, annotated_v1_wire)
    annotated_out = datapod.from_wire_message(annotated_hash, annotated_wire)
    assert (
        annotated_out.channel,
        annotated_out.flags,
        annotated_out.counter,
        annotated_out.data,
    ) == (17, 3, -12, b"typed")
    annotated_view = AnnotatedDataclassPacket.view_from_wire(annotated_hash, annotated_wire)
    assert (annotated_view.channel, annotated_view.flags, annotated_view.counter) == (17, 3, -12)
    assert annotated_view.data.tobytes() == b"typed"

    assert dataclasses.is_dataclass(ArrayPacket)
    assert ArrayPacket.__datapod_header_format__ == "<3Bf"
    assert ArrayPacket.__datapod_fields__ == ("rgb", "gain")
    assert datapod.header_size(ArrayPacket.TYPE_HASH) == 7
    array_packet = ArrayPacket((1, 2, 3), 0.5, b"pixels")
    array_hash, array_wire = datapod.to_wire_message(array_packet)
    assert array_hash == ArrayPacket.TYPE_HASH
    array_v1_hash, array_v1_wire = datapod.to_wire_message_v1(array_packet)
    assert array_v1_hash == ArrayPacket.CANONICAL_TYPE_HASH
    assert array_v1_wire == array_wire
    datapod.validate_wire_message_v1(array_v1_hash, array_v1_wire)
    array_out = ArrayPacket.from_wire_message(array_hash, array_wire)
    assert (array_out.rgb, array_out.gain, array_out.data) == ((1, 2, 3), 0.5, b"pixels")
    array_v1_out = ArrayPacket.from_wire_message_v1(array_v1_hash, array_v1_wire)
    assert (array_v1_out.rgb, array_v1_out.gain, array_v1_out.data) == (
        (1, 2, 3),
        0.5,
        b"pixels",
    )
    array_view = ArrayPacket.view_from_wire(array_hash, array_wire)
    assert (array_view.rgb, array_view.gain) == ((1, 2, 3), 0.5)
    assert array_view.data.tobytes() == b"pixels"
    array_v1_view = ArrayPacket.view_from_wire_v1(array_v1_hash, array_v1_wire)
    assert (array_v1_view.rgb, array_v1_view.gain) == ((1, 2, 3), 0.5)
    assert array_v1_view.data.tobytes() == b"pixels"
    try:
        datapod.to_wire_message(ArrayPacket((1, 2), 0.5, b"bad"))
        raise AssertionError("fixed-length Python array should reject wrong arity")
    except ValueError:
        pass

    assert StringArrayPacket.__datapod_header_format__ == "<3B"
    string_array_packet = StringArrayPacket((4, 5, 6), b"future-ish")
    string_array_hash, string_array_wire = datapod.to_wire_message(string_array_packet)
    string_array_out = StringArrayPacket.from_wire_message(string_array_hash, string_array_wire)
    assert (string_array_out.rgb, string_array_out.data) == ((4, 5, 6), b"future-ish")

    assert PythonStamp.__datapod_header_format__ == "<II"
    assert NestedPacket.__datapod_header_format__ == "<IIB"
    nested_schema = NestedPacket.schema()
    assert nested_schema["canonical_name"] == "acme.nested_packet.v1"
    assert nested_schema["payload_kind"] == "bytes"
    assert nested_schema["payload_field"] == "data"
    assert nested_schema["header_format"] == "<IIB"
    assert nested_schema["fields"][0]["annotation"]["kind"] == "datapod"
    assert nested_schema["fields"][0]["annotation"]["canonical_name"] == "acme.python_stamp.v1"
    assert datapod.describe_schema(NestedPacket)["fields"][1]["annotation"]["name"] == "u8"
    assert datapod.schema(NestedPacket.TYPE_HASH)["canonical_name"] == "acme.nested_packet.v1"
    nested_packet = NestedPacket(PythonStamp(10, 20), 3, b"nested")
    nested_hash, nested_wire = datapod.to_wire_message(nested_packet)
    nested_out = NestedPacket.from_wire_message(nested_hash, nested_wire)
    assert dataclasses.is_dataclass(nested_out.stamp)
    assert (nested_out.stamp.sec, nested_out.stamp.nsec, nested_out.flags, nested_out.data) == (
        10,
        20,
        3,
        b"nested",
    )
    nested_v1_hash, nested_v1_wire = datapod.to_wire_message_v1(nested_packet)
    assert nested_v1_hash == NestedPacket.CANONICAL_TYPE_HASH
    nested_v1_view = NestedPacket.view_from_wire_v1(nested_v1_hash, nested_v1_wire)
    assert dataclasses.is_dataclass(nested_v1_view.stamp)
    assert (nested_v1_view.stamp.sec, nested_v1_view.stamp.nsec, nested_v1_view.flags) == (
        10,
        20,
        3,
    )
    assert nested_v1_view.data.tobytes() == b"nested"

    raw_custom_hash = datapod.register_schema("acme.raw_packet.v1", 2, "bytes")
    raw_wire = datapod.join_wire_message(raw_custom_hash, b"\x01\x02", b"abc")
    datapod.validate_wire_message(raw_custom_hash, raw_wire)
    assert datapod.is_valid_wire_message(raw_custom_hash, raw_wire)
    assert not datapod.is_valid_wire_message(raw_custom_hash, b"\x01")
    assert datapod.split_wire_message(raw_custom_hash, raw_wire) == (b"\x01\x02", b"abc")

    c_custom_name = b"acme.c_custom_packet.v1"
    c_custom_hash = lib.datapod_register_type_name(
        c_custom_name,
        4,
        lib.datapod_payload_kind_bytes(),
    )
    assert c_custom_hash
    assert c_custom_hash == lib.datapod_type_hash_name(c_custom_name)
    assert lib.datapod_canonical_type_hash(c_custom_hash) == c_custom_hash
    c_header_buf = ctypes.create_string_buffer(b"\x01\x02\x03\x04")
    c_payload_buf = ctypes.create_string_buffer(b"abc")
    c_custom_joined = DatapodOwnedBytes()
    assert lib.datapod_wire_message_join(
        c_custom_hash,
        ctypes.cast(c_header_buf, ctypes.POINTER(ctypes.c_uint8)),
        4,
        ctypes.cast(c_payload_buf, ctypes.POINTER(ctypes.c_uint8)),
        3,
        ctypes.byref(c_custom_joined),
    )
    try:
        custom_message = lib.datapod_wire_message_borrow(
            c_custom_hash, c_custom_joined.ptr, c_custom_joined.len
        )
        assert lib.datapod_wire_message_is_valid(custom_message)
        custom_frame = DatapodWireFrame()
        assert lib.datapod_wire_frame_from_message(custom_message, ctypes.byref(custom_frame))
        assert lib.datapod_wire_frame_validate(custom_frame)
        assert custom_frame.header_len == 4
        assert custom_frame.payload_len == 3
        assert (
            borrowed_bytes_to_py(lib.datapod_wire_message_header(custom_message))
            == b"\x01\x02\x03\x04"
        )
        assert borrowed_bytes_to_py(lib.datapod_wire_message_payload(custom_message)) == b"abc"
    finally:
        lib.datapod_owned_bytes_free(c_custom_joined)

    for type_hash, cls in datapod.__datapod_registry__.items():
        assert hasattr(cls, "from_wire_message"), (type_hash, cls)


if __name__ == "__main__":
    main()
