"""Smoke test for the Python generic datapod wire API.

Run against an installed/built wheel, for example:

    python tests/python_generic_wire_smoke.py

The Makefile's `make bind` builds the wheel; CI or local scripts can install it
into an isolated target directory before running this file.
"""

import ctypes
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
    lib.datapod_wire_message_header.argtypes = [DatapodWireMessage]
    lib.datapod_wire_message_header.restype = DatapodBytes
    lib.datapod_wire_message_payload.argtypes = [DatapodWireMessage]
    lib.datapod_wire_message_payload.restype = DatapodBytes
    lib.datapod_header_size.argtypes = [ctypes.c_uint64]
    lib.datapod_header_size.restype = ctypes.c_size_t
    lib.datapod_type_hash_name.argtypes = [ctypes.c_char_p]
    lib.datapod_type_hash_name.restype = ctypes.c_uint64
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


def main():
    point = assert_round_trip(datapod.Point(1.0, 2.0, 3.0))
    assert (point.x, point.y, point.z) == (1.0, 2.0, 3.0)

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
    assert run_fixture("decode-grid", py_grid_hash, py_grid_wire.hex()) == "ok grid"

    py_matrix = datapod.Matrix(2, 3, 1, bytes([1, 2, 3, 4, 5, 6]))
    py_matrix_hash, py_matrix_wire = datapod.to_wire_message(py_matrix)
    assert run_fixture("decode-matrix", py_matrix_hash, py_matrix_wire.hex()) == "ok matrix"

    lib = load_c_abi()
    c_point_wire = DatapodOwnedBytes()
    assert lib.datapod_point_to_wire(DatapodPoint(1.0, 2.0, 3.0), ctypes.byref(c_point_wire))
    try:
        c_point_bytes = owned_bytes_to_py(c_point_wire)
        c_point_hash = lib.datapod_point_type_hash()
        c_point = datapod.from_wire_message(c_point_hash, c_point_bytes)
        assert (c_point.x, c_point.y, c_point.z) == (1.0, 2.0, 3.0)
        assert run_fixture("decode-point", c_point_hash, c_point_bytes.hex()) == "ok point"
    finally:
        lib.datapod_owned_bytes_free(c_point_wire)

    wire_buffer = ctypes.create_string_buffer(py_grid_wire)
    wire_ptr = ctypes.cast(wire_buffer, ctypes.POINTER(ctypes.c_uint8))
    message = lib.datapod_wire_message_borrow(py_grid_hash, wire_ptr, len(py_grid_wire))
    assert lib.datapod_wire_message_is_valid(message)
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
    custom = CustomPacket(7, 3, b"payload")
    custom_wire_hash, custom_wire = datapod.to_wire_message(custom)
    assert custom_wire_hash == custom_hash
    custom_out = datapod.from_wire_message(custom_wire_hash, custom_wire)
    assert type(custom_out).__name__ == "CustomPacket"
    assert (custom_out.channel, custom_out.flags, custom_out.data) == (7, 3, b"payload")

    raw_custom_hash = datapod.register_schema("acme.raw_packet.v1", 2, "bytes")
    raw_wire = datapod.join_wire_message(raw_custom_hash, b"\x01\x02", b"abc")
    assert datapod.split_wire_message(raw_custom_hash, raw_wire) == (b"\x01\x02", b"abc")

    c_custom_name = b"acme.c_custom_packet.v1"
    c_custom_hash = lib.datapod_register_type_name(
        c_custom_name,
        4,
        lib.datapod_payload_kind_bytes(),
    )
    assert c_custom_hash
    assert c_custom_hash == lib.datapod_type_hash_name(c_custom_name)
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
