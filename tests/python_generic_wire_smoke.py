"""Smoke test for the Python generic datapod wire API.

Run against an installed/built wheel, for example:

    python tests/python_generic_wire_smoke.py

The Makefile's `make bind` builds the wheel; CI or local scripts can install it
into an isolated target directory before running this file.
"""

import ctypes
import dataclasses
import gc
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
    "acme.repeated_scalar_packet.v1",
    "<2H",
    ("left", "right"),
    payload_field="data",
)
class RepeatedScalarPacket:
    def __init__(self, left, right, data):
        self.left = left
        self.right = right
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


_side_effect_validator_calls = []


def _side_effect_validate(packet):
    _side_effect_validator_calls.append(packet.channel)
    return packet.channel > 0


@datapod.datapod_type(
    "acme.side_effect_validated_packet.v1",
    header="<H",
    payload="data",
    validate=_side_effect_validate,
)
class SideEffectValidatedPacket:
    channel: int
    data: bytes

    def __init__(self, channel, data):
        self.channel = channel
        self.data = bytes(data)


_low_hash_side_effect_validator_calls = []


def _low_hash_side_effect_validate(packet):
    _low_hash_side_effect_validator_calls.append(packet.channel)
    return packet.channel > 0


@datapod.datapod_type(
    "acme.low_hash_side_effect_packet.v1",
    header="<H",
    payload="data",
    validate=_low_hash_side_effect_validate,
)
class LowHashSideEffectPacket:
    channel: int
    data: bytes

    def __init__(self, channel, data):
        self.channel = channel
        self.data = bytes(data)


_strict_new_validator_calls = []


def _strict_new_validate(packet):
    _strict_new_validator_calls.append(packet.channel)
    return packet.channel > 0


@datapod.datapod_type(
    "acme.strict_new_validated_packet.v1",
    header="<H",
    payload="data",
    validate=_strict_new_validate,
)
class StrictNewValidatedPacket:
    channel: int
    data: bytes

    def __new__(cls, token):
        if token != "construct":
            raise TypeError("StrictNewValidatedPacket requires a construction token")
        return super().__new__(cls)


_frozen_validator_calls = []


def _frozen_validate(packet):
    _frozen_validator_calls.append((packet.channel, getattr(packet.data, "readonly", None)))
    return packet.channel > 0


@datapod.datapod_type(
    "acme.frozen_validated_packet.v1",
    header="<H",
    payload="data",
    validate=_frozen_validate,
)
@dataclasses.dataclass(frozen=True)
class FrozenValidatedPacket:
    channel: int
    data: bytes


@datapod.datapod_type(
    "acme.non_bool_validated_packet.v1",
    header="<H",
    payload="data",
    validate=lambda packet: "yes",
)
class NonBoolValidatedPacket:
    channel: int
    data: bytes

    def __init__(self, channel, data):
        self.channel = channel
        self.data = bytes(data)


_mutating_validator_payload_readonly = []


def _mutating_validate(packet):
    _mutating_validator_payload_readonly.append(packet.data.readonly)
    packet.data[0] = ord("X")
    return True


@datapod.datapod_type(
    "acme.mutating_validator_packet.v1",
    header="<H",
    payload="data",
    validate=_mutating_validate,
)
class MutatingValidatorPacket:
    channel: int
    data: bytes

    def __init__(self, channel, data):
        self.channel = channel
        self.data = bytes(data)


def _raising_validate(packet):
    raise RuntimeError("validator exploded")


@datapod.datapod_type(
    "acme.raising_validator_packet.v1",
    header="<H",
    payload="data",
    validate=_raising_validate,
)
class RaisingValidatorPacket:
    channel: int
    data: bytes

    def __init__(self, channel, data):
        self.channel = channel
        self.data = bytes(data)


@datapod.datapod_type(
    "acme.raising_constructor_packet.v1",
    header="<H",
    payload="data",
)
class RaisingConstructorPacket:
    channel: int
    data: bytes

    def __init__(self, channel, data):
        raise TypeError("constructor type errors must not be swallowed")


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


@dataclasses.dataclass
class InheritedHeaderBase:
    channel: datapod.u16


@datapod.datapod_type(
    "acme.inherited_dataclass_packet.v1",
    payload=True,
    dataclass=True,
)
class InheritedDataclassPacket(InheritedHeaderBase):
    flags: datapod.u8
    data: bytes


@dataclasses.dataclass
class InheritedPayloadBase:
    data: bytes


@datapod.datapod_type(
    "acme.inherited_payload_packet.v1",
    payload=True,
    dataclass=True,
)
class InheritedPayloadPacket(InheritedPayloadBase):
    flags: datapod.u8


@datapod.datapod_type(
    "acme.decorated_header_base.v1",
    dataclass=True,
)
class DecoratedHeaderBase:
    channel: datapod.u16


@datapod.datapod_type(
    "acme.decorated_header_child.v1",
    payload=True,
    dataclass=True,
)
class DecoratedHeaderChild(DecoratedHeaderBase):
    flags: datapod.u8
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
    class BoolReturningIndex:
        def __index__(self):
            return True

    class RaisingIndex:
        def __index__(self):
            raise RuntimeError("bad index")

    class HostileIndexLookup:
        def __getattribute__(self, name):
            if name == "__index__":
                raise RuntimeError("hostile __index__ lookup")
            return object.__getattribute__(self, name)

    try:
        datapod.type_hash_name("")
        raise AssertionError("type_hash_name should reject empty canonical names")
    except ValueError:
        pass
    try:
        datapod.register_schema("", 0, "fixed")
        raise AssertionError("register_schema should reject empty canonical names")
    except ValueError:
        pass
    for func, args in (
        (datapod.type_hash_name, ("acme.bad\x00name.v1",)),
        (datapod.register_schema, ("acme.bad\x00schema.v1", 0, "fixed")),
    ):
        try:
            func(*args)
            raise AssertionError(f"{func.__name__} should reject NUL in canonical names")
        except ValueError:
            pass
    for func, args in (
        (datapod.type_hash_name, ("Acme.bad-name.v1",)),
        (datapod.register_schema, ("acme.bad-name.v1", 0, "fixed")),
    ):
        try:
            func(*args)
            raise AssertionError(f"{func.__name__} should reject malformed canonical names")
        except ValueError:
            pass
    try:
        datapod.datapod_type("acme.bad\x00decorator.v1", "<H", ("channel",), payload="data")
        raise AssertionError("datapod_type should reject NUL in canonical names")
    except ValueError:
        pass
    try:
        datapod.datapod_type("acme.bad-name_decorator.v1", "<H", ("channel",), payload="data")
        raise AssertionError("datapod_type should reject malformed canonical names")
    except ValueError:
        pass
    try:
        datapod.register_type(object(), "acme.bad_register_type_target.v1", 0, "fixed")
        raise AssertionError("register_type should reject non-class targets")
    except TypeError:
        pass
    class BadNulNameDirectType:
        pass

    try:
        datapod.register_type(BadNulNameDirectType, "acme.bad\x00type.v1", 0, "fixed")
        raise AssertionError("register_type should reject NUL in canonical names")
    except ValueError:
        pass
    class BadMalformedNameDirectType:
        pass

    try:
        datapod.register_type(BadMalformedNameDirectType, "acme.bad-type.v1", 0, "fixed")
        raise AssertionError("register_type should reject malformed canonical names")
    except ValueError:
        pass
    for bad_header_size in (True, 1.5, BoolReturningIndex(), RaisingIndex()):
        try:
            datapod.register_schema(
                "acme.bad_schema_header_type.v1",
                bad_header_size,
                "fixed",
            )
            raise AssertionError("register_schema should reject non-integer header sizes")
        except TypeError:
            pass

        class BadDirectHeaderSize:
            pass

        try:
            datapod.register_type(
                BadDirectHeaderSize,
                "acme.bad_type_header_type.v1",
                bad_header_size,
                "fixed",
            )
            raise AssertionError("register_type should reject non-integer header sizes")
        except TypeError:
            pass
    for bad_header_size in (-1, 1 << (ctypes.sizeof(ctypes.c_size_t) * 8 - 1)):
        try:
            datapod.register_schema(
                "acme.bad_schema_header_size.v1",
                bad_header_size,
                "fixed",
            )
            raise AssertionError("register_schema should reject impossible header sizes")
        except ValueError:
            pass

        class BadDirectHeaderSizeValue:
            pass

        try:
            datapod.register_type(
                BadDirectHeaderSizeValue,
                "acme.bad_type_header_size.v1",
                bad_header_size,
                "fixed",
            )
            raise AssertionError("register_type should reject impossible header sizes")
        except ValueError:
            pass
    for bad_payload_kind in (True, 1.5):
        try:
            datapod.register_schema(
                "acme.bad_schema_payload_kind_type.v1",
                0,
                bad_payload_kind,
            )
            raise AssertionError("register_schema should reject non-string payload kinds")
        except TypeError:
            pass

        class BadDirectPayloadKindType:
            pass

        try:
            datapod.register_type(
                BadDirectPayloadKindType,
                "acme.bad_type_payload_kind_type.v1",
                0,
                bad_payload_kind,
            )
            raise AssertionError("register_type should reject non-string payload kinds")
        except TypeError:
            pass
    try:
        datapod.register_schema("acme.bad_schema_payload_kind_value.v1", 0, "blob")
        raise AssertionError("register_schema should reject unknown payload kind strings")
    except ValueError:
        pass

    class BadDirectPayloadKindValue:
        pass

    try:
        datapod.register_type(
            BadDirectPayloadKindValue,
            "acme.bad_type_payload_kind_value.v1",
            0,
            "blob",
        )
        raise AssertionError("register_type should reject unknown payload kind strings")
    except ValueError:
        pass
    try:
        datapod.datapod_type("", "<H", ("channel",), payload="data")
        raise AssertionError("datapod_type should reject empty canonical names")
    except ValueError:
        pass
    try:
        datapod.datapod_type(123, "<H", ("channel",), payload="data")
        raise AssertionError("datapod_type should reject non-string canonical names")
    except TypeError:
        pass
    try:
        datapod.datapod_type("acme.bad_payload_field_type.v1", "<H", ("channel",), payload=123)
        raise AssertionError("datapod_type should reject non-string payload field names")
    except TypeError:
        pass
    try:
        datapod.datapod_type("acme.bool_like_payload_field_type.v1", "<H", ("channel",), payload=1)
        raise AssertionError("datapod_type should reject bool-like non-string payload field names")
    except TypeError:
        pass
    try:
        datapod.datapod_type("acme.empty_payload_field_type.v1", "<H", ("channel",), payload="")
        raise AssertionError("datapod_type should reject empty payload field names")
    except TypeError:
        pass
    try:
        datapod.datapod_type(
            "acme.empty_payload_field_alias_type.v1",
            "<H",
            ("channel",),
            payload_field="",
        )
        raise AssertionError("datapod_type should reject empty payload_field names")
    except TypeError:
        pass
    try:
        datapod.datapod_type(
            "acme.conflicting_header_alias_rejected.v1",
            "<H",
            ("channel",),
            payload="data",
            header="<I",
        )
        raise AssertionError("datapod_type should reject conflicting header/header_format aliases")
    except TypeError:
        pass
    try:
        datapod.datapod_type(
            "acme.conflicting_payload_alias_rejected.v1",
            "<H",
            ("channel",),
            payload="data",
            payload_field="blob",
        )
        raise AssertionError("datapod_type should reject conflicting payload/payload_field aliases")
    except TypeError:
        pass
    try:
        datapod.datapod_type("acme.non_class_decorator_target_rejected.v1", "<H", ("channel",))(
            object()
        )
        raise AssertionError("datapod_type decorator should reject non-class targets")
    except TypeError:
        pass
    try:
        datapod.datapod_type(
            "acme.bool_like_dataclass_type.v1",
            "<H",
            ("channel",),
            payload="data",
            dataclass=1,
        )
        raise AssertionError("datapod_type should reject bool-like dataclass flags")
    except TypeError:
        pass
    for name, kwargs in [
        ("acme.bad_header_format_type.v1", {"header_format": 123}),
        ("acme.bad_header_alias_type.v1", {"header": 123}),
    ]:
        try:
            datapod.datapod_type(name, fields=("channel",), payload="data", **kwargs)
            raise AssertionError("datapod_type should reject non-string header formats")
        except TypeError:
            pass
    try:
        @datapod.datapod_type(
            "acme.bad_byte_order_type.v1",
            fields=("channel",),
            payload="data",
            byte_order=123,
        )
        class BadByteOrderType:
            channel: datapod.u16
            data: bytes

            def __init__(self, channel, data):
                self.channel = channel
                self.data = bytes(data)

        raise AssertionError("datapod_type should reject non-string byte_order")
    except TypeError:
        pass
    try:
        @datapod.datapod_type(
            "acme.unknown_byte_order_type.v1",
            fields=("channel",),
            payload="data",
            byte_order="little",
        )
        class UnknownByteOrderType:
            channel: datapod.u16
            data: bytes

            def __init__(self, channel, data):
                self.channel = channel
                self.data = bytes(data)

        raise AssertionError("datapod_type should reject unknown byte_order markers")
    except TypeError:
        pass
    try:
        @datapod.datapod_type(
            "acme.big_endian_byte_order_type.v1",
            fields=("channel",),
            payload="data",
            byte_order=">",
        )
        class BigEndianByteOrderType:
            channel: datapod.u16
            data: bytes

            def __init__(self, channel, data):
                self.channel = channel
                self.data = bytes(data)

        raise AssertionError("datapod-wire-v1/le should reject non-little-endian byte_order")
    except TypeError:
        pass
    try:
        @datapod.datapod_type(
            "acme.missing_inferred_annotation_rejected.v1",
            fields=("channel",),
            payload="data",
        )
        class MissingInferredAnnotationRejected:
            data: bytes

            def __init__(self, channel, data):
                self.channel = channel
                self.data = bytes(data)

        raise AssertionError("datapod_type should reject inferred header fields without annotations")
    except TypeError:
        pass
    for name, bad_fields in [
        ("acme.string_fields_rejected.v1", "channel"),
        ("acme.non_string_fields_rejected.v1", ("channel", 123)),
        ("acme.empty_fields_rejected.v1", ("channel", "")),
        ("acme.duplicate_fields_rejected.v1", ("channel", "channel")),
    ]:
        try:
            @datapod.datapod_type(name, "<H", bad_fields, payload="data")
            class BadFieldsRejected:
                channel: int
                data: bytes

                def __init__(self, channel, data):
                    self.channel = channel
                    self.data = bytes(data)

            raise AssertionError("datapod_type should reject malformed header field lists")
        except TypeError:
            pass
    class HostileAnnotationsMeta(type):
        def __getattribute__(cls, name):
            if name == "__annotations__":
                raise RuntimeError("hostile annotations lookup")
            return super().__getattribute__(name)

    class HostileAnnotationsBase(metaclass=HostileAnnotationsMeta):
        pass

    try:
        @datapod.datapod_type(
            "acme.hostile_annotations_base_rejected.v1",
            "<H",
            ("channel",),
            payload="data",
        )
        class HostileAnnotationsChild(HostileAnnotationsBase):
            def __init__(self, channel, data):
                self.channel = channel
                self.data = bytes(data)

        raise AssertionError("datapod_type should fail closed on hostile inherited annotations")
    except ValueError:
        pass
    class HostileAnnotationNameMeta(type):
        def __getattribute__(cls, name):
            if name == "__name__":
                raise RuntimeError("hostile annotation name lookup")
            return super().__getattribute__(name)

    class HostileAnnotationName(metaclass=HostileAnnotationNameMeta):
        pass

    try:
        @datapod.datapod_type(
            "acme.hostile_annotation_name_rejected.v1",
            fields=("channel",),
            payload="data",
        )
        class HostileAnnotationNameRejected:
            channel: HostileAnnotationName
            data: bytes

        raise AssertionError("datapod_type should fail closed on hostile annotation __name__ lookup")
    except ValueError:
        pass
    class HostileAnnotationInstance:
        def __getattribute__(self, name):
            if name in ("__name__", "__class__"):
                raise RuntimeError("hostile annotation label lookup")
            return object.__getattribute__(self, name)

    hostile_annotation_instance = HostileAnnotationInstance()

    try:
        @datapod.datapod_type(
            "acme.hostile_annotation_instance_rejected.v1",
            fields=("channel",),
            payload="data",
        )
        class HostileAnnotationInstanceRejected:
            channel: hostile_annotation_instance
            data: bytes

        raise AssertionError(
            "datapod_type should fail closed when annotation and fallback label metadata are hostile"
        )
    except ValueError:
        pass
    try:
        @datapod.datapod_type(
            "acme.payload_header_overlap_rejected.v1",
            "<H",
            ("channel", "data"),
            payload="data",
        )
        class PayloadHeaderOverlapRejected:
            channel: int
            data: bytes

            def __init__(self, channel, data):
                self.channel = channel
                self.data = bytes(data)

        raise AssertionError("datapod_type should reject payload/header field overlap")
    except TypeError:
        pass
    try:
        @datapod.datapod_type(
            "acme.payload_true_without_candidate_rejected.v1",
            payload=True,
            dataclass=True,
        )
        class PayloadTrueWithoutCandidateRejected:
            channel: datapod.u16
            flags: datapod.u8

        raise AssertionError("datapod_type payload=True should reject classes without a bytes-like payload field")
    except TypeError:
        pass
    try:
        @datapod.datapod_type(
            "acme.payload_true_with_multiple_candidates_rejected.v1",
            payload=True,
            dataclass=True,
        )
        class PayloadTrueWithMultipleCandidatesRejected:
            channel: datapod.u16
            data: bytes
            metadata: bytearray

        raise AssertionError("datapod_type payload=True should reject multiple bytes-like payload fields")
    except TypeError:
        pass
    for bad_count in (True, 1.5, BoolReturningIndex(), RaisingIndex(), 0, -1):
        try:
            datapod.array(datapod.u8, bad_count)
            raise AssertionError("datapod.array should reject non-positive/non-integer counts")
        except TypeError:
            pass
    class BadArrayCountIndex:
        def __index__(self):
            raise ValueError("bad array count")

    try:
        datapod.array(datapod.u8, BadArrayCountIndex())
        raise AssertionError("datapod.array should reject __index__ failures")
    except TypeError:
        pass
    try:
        datapod.array(datapod.u8, 1 << 100)
        raise AssertionError("datapod.array should reject impossible counts")
    except ValueError:
        pass

    try:
        @datapod.datapod_type(
            "acme.native_order_rejected.v1",
            "HH",
            ("channel", "flags"),
            payload="data",
        )
        class NativeOrderRejected:
            def __init__(self, channel, flags, data):
                self.channel = channel
                self.flags = flags
                self.data = bytes(data)

        raise AssertionError("datapod_type should reject non-v1 little-endian header formats")
    except TypeError:
        pass

    try:
        @datapod.datapod_type(
            "acme.too_many_header_values_rejected.v1",
            "<HHH",
            ("channel", "flags"),
            payload="data",
        )
        class TooManyHeaderValuesRejected:
            def __init__(self, channel, flags, data):
                self.channel = channel
                self.flags = flags
                self.data = bytes(data)

        raise AssertionError("datapod_type should reject header formats with extra values")
    except TypeError:
        pass

    try:
        @datapod.datapod_type(
            "acme.too_few_header_values_rejected.v1",
            "<H",
            ("channel", "flags"),
            payload="data",
        )
        class TooFewHeaderValuesRejected:
            def __init__(self, channel, flags, data):
                self.channel = channel
                self.flags = flags
                self.data = bytes(data)

        raise AssertionError("datapod_type should reject header formats with missing values")
    except TypeError:
        pass

    for name, header in [
        ("acme.string_header_rejected.v1", "<2s"),
        ("acme.pointer_header_rejected.v1", "<P"),
    ]:
        try:
            @datapod.datapod_type(name, header, ("field",), payload="data")
            class UnsupportedHeaderFormatRejected:
                def __init__(self, field, data):
                    self.field = field
                    self.data = bytes(data)

            raise AssertionError(
                f"datapod_type should reject unsupported struct header format {header}"
            )
        except TypeError:
            pass

    try:
        @datapod.datapod_type(
            "acme.bad_string_array_annotation_rejected.v1",
            payload=True,
            dataclass=True,
        )
        class BadStringArrayAnnotationRejected:
            rgb: "datapod.array(datapod.u8, nope)"
            data: bytes

        raise AssertionError("datapod_type should reject malformed string array annotations")
    except TypeError:
        pass

    try:
        @datapod.datapod_type(
            "acme.missing_count_string_array_annotation_rejected.v1",
            payload=True,
            dataclass=True,
        )
        class MissingCountStringArrayAnnotationRejected:
            rgb: "datapod.array(datapod.u8)"
            data: bytes

        raise AssertionError("datapod_type should reject string array annotations without a count")
    except TypeError:
        pass

    try:
        @datapod.datapod_type(
            "acme.empty_item_string_array_annotation_rejected.v1",
            payload=True,
            dataclass=True,
        )
        class EmptyItemStringArrayAnnotationRejected:
            rgb: "datapod.array(, 3)"
            data: bytes

        raise AssertionError("datapod_type should reject string array annotations without an item")
    except TypeError:
        pass

    try:
        @datapod.datapod_type(
            "acme.unknown_item_string_array_annotation_rejected.v1",
            payload=True,
            dataclass=True,
        )
        class UnknownItemStringArrayAnnotationRejected:
            rgb: "datapod.array(datapod.nope, 3)"
            data: bytes

        raise AssertionError("datapod_type should reject string array annotations with unknown items")
    except TypeError:
        pass

    for bad_hash in (-1, 2**64):
        try:
            @datapod.datapod_type(
                "acme.bad_type_hash_rejected.v1",
                "<H",
                ("channel",),
                type_hash=bad_hash,
                payload="data",
            )
            class BadTypeHashRejected:
                def __init__(self, channel, data):
                    self.channel = channel
                    self.data = bytes(data)

            raise AssertionError("datapod_type should reject type hashes outside u64")
        except ValueError:
            pass
        try:
            datapod.describe_schema(bad_hash)
            raise AssertionError("describe_schema should reject type hashes outside u64")
        except ValueError:
            pass
        try:
            datapod.from_wire_message(bad_hash, b"")
            raise AssertionError("from_wire_message should reject type hashes outside u64")
        except ValueError:
            pass
        try:
            datapod.decode_as(datapod.Point, bad_hash, b"")
            raise AssertionError("decode_as should reject type hashes outside u64")
        except ValueError:
            pass

    for bad_hash in (True, 1.5, BoolReturningIndex(), RaisingIndex()):
        try:
            @datapod.datapod_type(
                "acme.non_integer_type_hash_rejected.v1",
                "<H",
                ("channel",),
                type_hash=bad_hash,
                payload="data",
            )
            class NonIntegerTypeHashRejected:
                def __init__(self, channel, data):
                    self.channel = channel
                    self.data = bytes(data)

            raise AssertionError("datapod_type should reject non-integer type hashes")
        except TypeError:
            pass
        try:
            datapod.register_schema("acme.direct_bad_schema_hash.v1", 0, "fixed", bad_hash)
            raise AssertionError("register_schema should reject non-integer type hashes")
        except TypeError:
            pass
        try:
            datapod.describe_schema(bad_hash)
            raise AssertionError("describe_schema should reject non-integer type hashes")
        except TypeError:
            pass
        try:
            datapod.schema(bad_hash)
            raise AssertionError("schema should reject non-integer type hashes")
        except TypeError:
            pass

        class DirectBadTypeHashRejected:
            pass

        try:
            datapod.register_type(
                DirectBadTypeHashRejected,
                "acme.direct_bad_type_hash.v1",
                0,
                "fixed",
                bad_hash,
            )
            raise AssertionError("register_type should reject non-integer type hashes")
        except TypeError:
            pass

        for func, args in [
            (datapod.canonical_type_hash, (bad_hash,)),
            (datapod.emitted_type_hash, (bad_hash,)),
            (datapod.header_size, (bad_hash,)),
            (datapod.header_size_v1, (bad_hash,)),
            (datapod.payload_kind, (bad_hash,)),
            (datapod.format_version, (bad_hash,)),
            (datapod.wire_format, (bad_hash,)),
            (datapod.endian, (bad_hash,)),
            (datapod.alignment_policy, (bad_hash,)),
            (datapod.validator_kind, (bad_hash,)),
            (datapod.emitted_hash_kind, (bad_hash,)),
            (datapod.has_archive, (bad_hash,)),
            (datapod.has_view, (bad_hash,)),
            (datapod.has_owned_decode, (bad_hash,)),
            (datapod.archive_shape, (bad_hash,)),
            (datapod.describe_schema, (bad_hash,)),
            (datapod.join_wire_message, (bad_hash, b"")),
            (datapod.split_wire_message, (bad_hash, b"")),
            (datapod.split_wire_message_v1, (bad_hash, b"")),
            (datapod.validate_wire_message, (bad_hash, b"")),
            (datapod.validate_wire_message_v1, (bad_hash, b"")),
            (datapod.validate_wire_frame_parts_v1, (bad_hash, b"")),
            (datapod.from_wire_message, (bad_hash, b"")),
            (datapod.decode_as, (datapod.Point, bad_hash, b"")),
        ]:
            try:
                func(*args)
                raise AssertionError(f"{func.__name__} should reject non-integer type hashes")
            except TypeError:
                pass

    point = assert_round_trip(datapod.Point(1.0, 2.0, 3.0))
    assert (point.x, point.y, point.z) == (1.0, 2.0, 3.0)
    point_archive = point.archive()
    assert isinstance(point_archive, datapod.ArchiveFrame)
    assert point_archive.type_hash == datapod.Point.TYPE_HASH
    assert len(point_archive.payload) == 0
    point_view = datapod.Point.view_archive(point_archive)
    assert (point_view.x, point_view.y, point_view.z) == (1.0, 2.0, 3.0)
    point_from_archive = datapod.from_archive(datapod.Point, point_archive)
    assert (point_from_archive.x, point_from_archive.y, point_from_archive.z) == (1.0, 2.0, 3.0)
    assert datapod.view_archive(datapod.Point, point_archive).x == 1.0
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
    raw_archive = raw.archive()
    assert isinstance(raw_archive, datapod.ArchiveFrame)
    assert raw_archive.payload.tobytes() == bytes([9, 8, 7])
    raw_from_archive = datapod.from_archive(datapod.Bytes, raw_archive)
    assert raw_from_archive.payload_bytes() == bytes([9, 8, 7])
    raw_from_hash_archive = datapod.from_archive(datapod.Bytes.TYPE_HASH, raw_archive)
    assert raw_from_hash_archive.payload_bytes() == bytes([9, 8, 7])

    empty_raw = assert_round_trip(datapod.Bytes([]))
    assert empty_raw.payload_bytes() == b""
    empty_raw_archive = empty_raw.archive()
    assert isinstance(empty_raw_archive, datapod.ArchiveFrame)
    assert empty_raw_archive.type_hash == datapod.Bytes.TYPE_HASH
    assert empty_raw_archive.payload.tobytes() == b""
    assert empty_raw_archive.payload.readonly
    empty_raw_view = datapod.Bytes.view_archive(empty_raw_archive)
    assert empty_raw_view.payload.tobytes() == b""
    assert empty_raw_view.payload.readonly
    empty_raw_from_archive = datapod.from_archive(datapod.Bytes, empty_raw_archive)
    assert empty_raw_from_archive.payload_bytes() == b""

    empty_text = assert_round_trip(datapod.DpString([]))
    assert empty_text.payload_bytes() == b""
    empty_text_archive = empty_text.archive()
    assert isinstance(empty_text_archive, datapod.ArchiveFrame)
    assert empty_text_archive.type_hash == datapod.DpString.TYPE_HASH
    assert empty_text_archive.payload.tobytes() == b""
    assert empty_text_archive.payload.readonly
    empty_text_view = datapod.DpString.view_archive(empty_text_archive)
    assert empty_text_view.payload.tobytes() == b""
    assert empty_text_view.payload.readonly
    empty_text_from_archive = datapod.from_archive(datapod.DpString, empty_text_archive)
    assert empty_text_from_archive.payload_bytes() == b""

    empty_str = assert_round_trip(datapod.DpStr(""))
    assert empty_str.payload_bytes() == b""
    empty_str_archive = empty_str.archive()
    assert isinstance(empty_str_archive, datapod.ArchiveFrame)
    assert empty_str_archive.type_hash == datapod.DpStr.TYPE_HASH
    assert empty_str_archive.payload.tobytes() == b""
    assert empty_str_archive.payload.readonly
    empty_str_view = datapod.DpStr.view_archive(empty_str_archive)
    assert empty_str_view.payload.tobytes() == b""
    assert empty_str_view.payload.readonly
    empty_str_from_archive = datapod.from_archive(datapod.DpStr, empty_str_archive)
    assert empty_str_from_archive.payload_bytes() == b""

    for text_cls in (datapod.DpString, datapod.DpStr):
        invalid_text_archive = datapod.ArchiveFrame(text_cls.TYPE_HASH, b"", b"\xff")
        for func in (text_cls.view_archive, text_cls.from_archive):
            try:
                func(invalid_text_archive)
                raise AssertionError(
                    f"{func.__name__} should reject invalid UTF-8 archive payloads"
                )
            except ValueError as exc:
                assert "utf" in str(exc).lower()
        for func in (datapod.view_archive, datapod.from_archive):
            try:
                func(text_cls, invalid_text_archive)
                raise AssertionError(
                    f"{func.__name__} should reject invalid UTF-8 archive payloads"
                )
            except ValueError as exc:
                assert "utf" in str(exc).lower()

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
    try:
        datapod.Grid(2, 2, 11, False, 0.5, [0, 0, 0, 1, 0, 0, 0], bytes(range(15)))
        raise AssertionError("Grid constructor should reject invalid payload length")
    except ValueError:
        pass
    try:
        datapod.Grid.from_wire(py_grid.to_header_bytes(), bytes(range(15)))
        raise AssertionError("Grid.from_wire should reject invalid payload length")
    except ValueError:
        pass
    bad_grid_header = bytearray(py_grid.to_header_bytes())
    bad_grid_header[0:4] = (0).to_bytes(4, "little")
    bad_grid_frame = datapod.WireFrame(datapod.Grid.TYPE_HASH, bad_grid_header, bytes(range(16)))
    try:
        datapod.Grid.view_archive(bad_grid_frame)
        raise AssertionError("Grid.view_archive should reject invalid headers")
    except ValueError:
        pass

    py_matrix = datapod.Matrix(2, 3, 1, bytes([1, 2, 3, 4, 5, 6]))
    try:
        datapod.Matrix(2, 3, 1, bytes([1, 2, 3, 4, 5]))
        raise AssertionError("Matrix constructor should reject invalid payload length")
    except ValueError:
        pass
    try:
        datapod.Matrix.from_wire(py_matrix.to_header_bytes(), bytes([1, 2, 3, 4, 5]))
        raise AssertionError("Matrix.from_wire should reject invalid payload length")
    except ValueError:
        pass
    bad_matrix_header = bytearray(py_matrix.to_header_bytes())
    bad_matrix_header[12:16] = (1).to_bytes(4, "little")
    bad_matrix_frame = datapod.WireFrame(datapod.Matrix.TYPE_HASH, bad_matrix_header, bytes([1, 2, 3, 4, 5, 6]))
    try:
        datapod.Matrix.view_archive(bad_matrix_frame)
        raise AssertionError("Matrix.view_archive should reject reserved header bits")
    except ValueError:
        pass
    try:
        datapod.Vector(2, bytes([1, 2, 3]))
        raise AssertionError("Vector constructor should reject unaligned payload length")
    except ValueError:
        pass
    bad_encoding_header = bytearray(datapod.header_size(datapod.Encoding.TYPE_HASH))
    bad_encoding_header[0:4] = (99).to_bytes(4, "little")
    try:
        datapod.Encoding.from_wire(bad_encoding_header, b"")
        raise AssertionError("Encoding.from_wire should reject invalid discriminants")
    except ValueError:
        pass
    try:
        datapod.Encoding.view_archive(
            datapod.WireFrame(datapod.Encoding.TYPE_HASH, bad_encoding_header, b"")
        )
        raise AssertionError("Encoding.view_archive should reject invalid discriminants")
    except ValueError:
        pass
    bad_ip_header = bytearray(datapod.header_size(datapod.Ip.TYPE_HASH))
    bad_ip_header[0:4] = (5).to_bytes(4, "little")
    try:
        datapod.Ip.from_wire(bad_ip_header, b"")
        raise AssertionError("Ip.from_wire should reject invalid family tags")
    except ValueError:
        pass
    bad_mac_header = bytearray(datapod.header_size(datapod.MacAddr.TYPE_HASH))
    bad_mac_header[6] = 1
    try:
        datapod.MacAddr.from_wire(bad_mac_header, b"")
        raise AssertionError("MacAddr.from_wire should reject non-zero reserved padding")
    except ValueError:
        pass
    bad_link_header = bytearray(datapod.header_size(datapod.Link.TYPE_HASH))
    bad_link_header[12:16] = (1).to_bytes(4, "little")
    try:
        datapod.Link.from_wire(bad_link_header, b"")
        raise AssertionError("Link.from_wire should reject non-zero reserved padding")
    except ValueError:
        pass
    py_matrix_hash, py_matrix_wire = datapod.to_wire_message(py_matrix)
    py_matrix_v1_hash, py_matrix_v1_wire = datapod.to_wire_message_v1(py_matrix)
    assert py_matrix_v1_hash == datapod.canonical_type_hash(py_matrix_hash)
    datapod.validate_wire_message_v1(py_matrix_v1_hash, py_matrix_v1_wire)
    assert datapod.is_valid_wire_message_v1(py_matrix_v1_hash, py_matrix_v1_wire)
    assert not datapod.is_valid_wire_message_v1(datapod.Matrix.TYPE_HASH ^ 1, py_matrix_v1_wire)
    py_matrix_frame = py_matrix.to_wire_frame()
    assert isinstance(py_matrix_frame, datapod.WireFrame)
    assert datapod.ArchiveFrame is datapod.WireFrame
    py_matrix_archive = py_matrix.archive()
    assert isinstance(py_matrix_archive, datapod.ArchiveFrame)
    assert py_matrix_archive.type_hash == py_matrix_hash
    assert py_matrix_archive.payload.tobytes() == bytes([1, 2, 3, 4, 5, 6])
    assert datapod.archive(py_matrix).payload.tobytes() == bytes([1, 2, 3, 4, 5, 6])

    class HostileClassLookup:
        def __getattribute__(self, name):
            if name == "__class__":
                raise RuntimeError("hostile __class__ lookup")
            return object.__getattribute__(self, name)

    @datapod.datapod_type(
        "acme.hostile_header_lookup_packet.v1",
        "<H",
        ("channel",),
        payload="data",
    )
    class HostileHeaderLookupPacket:
        def __init__(self, channel, data):
            object.__setattr__(self, "channel", channel)
            object.__setattr__(self, "data", bytes(data))

        def __getattribute__(self, name):
            if name == "channel":
                raise RuntimeError("hostile header field lookup")
            return object.__getattribute__(self, name)

    @datapod.datapod_type(
        "acme.hostile_payload_lookup_packet.v1",
        "<H",
        ("channel",),
        payload="data",
    )
    class HostilePayloadLookupPacket:
        def __init__(self, channel, data):
            object.__setattr__(self, "channel", channel)
            object.__setattr__(self, "data", bytes(data))

        def __getattribute__(self, name):
            if name == "data":
                raise RuntimeError("hostile payload field lookup")
            return object.__getattribute__(self, name)

    try:
        datapod.wire_frame(object())
        raise AssertionError("wire_frame should reject unsupported objects")
    except ValueError:
        pass
    try:
        datapod.wire_frame(HostileClassLookup())
        raise AssertionError("wire_frame should fail closed when object __class__ is hostile")
    except ValueError:
        pass
    try:
        datapod.archive(object())
        raise AssertionError("archive should reject unsupported objects")
    except ValueError:
        pass
    try:
        datapod.archive(HostileClassLookup())
        raise AssertionError("archive should fail closed when object __class__ is hostile")
    except ValueError:
        pass
    try:
        datapod.to_wire_message(object())
        raise AssertionError("to_wire_message should reject unsupported objects")
    except ValueError:
        pass
    try:
        datapod.to_wire_message_v1(object())
        raise AssertionError("to_wire_message_v1 should reject unsupported objects")
    except ValueError:
        pass
    try:
        datapod.to_wire_message(HostileHeaderLookupPacket(1, b"x"))
        raise AssertionError("to_wire_message should fail closed on hostile header field lookup")
    except ValueError:
        pass
    try:
        datapod.to_wire_message(HostilePayloadLookupPacket(1, b"x"))
        raise AssertionError("to_wire_message should fail closed on hostile payload field lookup")
    except ValueError:
        pass
    class NonCallableToWireMessage:
        to_wire_message = object()

    try:
        datapod.to_wire_message(NonCallableToWireMessage())
        raise AssertionError("to_wire_message should reject non-callable methods")
    except ValueError:
        pass
    class NonCallableToWireMessageV1:
        to_wire_message_v1 = object()

    try:
        datapod.to_wire_message_v1(NonCallableToWireMessageV1())
        raise AssertionError("to_wire_message_v1 should reject non-callable methods")
    except ValueError:
        pass
    class BadToWireMessageReturn:
        def to_wire_message(self):
            return object()

    try:
        datapod.to_wire_message(BadToWireMessageReturn())
        raise AssertionError("to_wire_message should reject invalid return shapes")
    except ValueError:
        pass
    for bad_hash in (True, 1.5, -1, 2**64):
        class BadToWireMessageHashReturn:
            def to_wire_message(self):
                return bad_hash, b""

        try:
            datapod.to_wire_message(BadToWireMessageHashReturn())
            raise AssertionError("to_wire_message should reject invalid returned type hashes")
        except ValueError:
            pass
    unknown_return_hash = 1
    while True:
        try:
            datapod.header_size(unknown_return_hash)
        except ValueError:
            break
        unknown_return_hash += 1

    class UnknownToWireMessageHashReturn:
        def to_wire_message(self):
            return unknown_return_hash, b""

    try:
        datapod.to_wire_message(UnknownToWireMessageHashReturn())
        raise AssertionError("to_wire_message should reject unknown returned type hashes")
    except ValueError:
        pass

    class BadToWireMessageWireReturn:
        def to_wire_message(self):
            return datapod.Point.TYPE_HASH, b""

    try:
        datapod.to_wire_message(BadToWireMessageWireReturn())
        raise AssertionError("to_wire_message should reject invalid returned wire bytes")
    except ValueError:
        pass
    class ListToWireMessageWireReturn:
        def to_wire_message(self):
            return py_matrix_hash, list(py_matrix_wire)

    try:
        datapod.to_wire_message(ListToWireMessageWireReturn())
        raise AssertionError("to_wire_message should reject non-buffer returned wire bytes")
    except ValueError:
        pass
    class NonContiguousToWireMessageWireReturn:
        def to_wire_message(self):
            return py_matrix_hash, memoryview(bytearray(py_matrix_wire))[::2]

    try:
        datapod.to_wire_message(NonContiguousToWireMessageWireReturn())
        raise AssertionError("to_wire_message should reject non-contiguous returned wire buffers")
    except ValueError:
        pass
    class BytearrayToWireMessageWireReturn:
        def to_wire_message(self):
            return py_matrix_hash, bytearray(py_matrix_wire)

    bytearray_return_hash, bytearray_return_wire = datapod.to_wire_message(
        BytearrayToWireMessageWireReturn()
    )
    assert bytearray_return_hash == py_matrix_hash
    assert bytearray_return_wire == py_matrix_wire

    class MemoryviewToWireMessageWireReturn:
        def to_wire_message(self):
            return py_matrix_hash, memoryview(bytearray(py_matrix_wire))

    memoryview_return_hash, memoryview_return_wire = datapod.to_wire_message(
        MemoryviewToWireMessageWireReturn()
    )
    assert memoryview_return_hash == py_matrix_hash
    assert memoryview_return_wire == py_matrix_wire
    class BadToWireMessageV1Return:
        def to_wire_message_v1(self):
            return object()

    try:
        datapod.to_wire_message_v1(BadToWireMessageV1Return())
        raise AssertionError("to_wire_message_v1 should reject invalid return shapes")
    except ValueError:
        pass
    for bad_hash in (True, 1.5, -1, 2**64):
        class BadToWireMessageV1HashReturn:
            def to_wire_message_v1(self):
                return bad_hash, b""

        try:
            datapod.to_wire_message_v1(BadToWireMessageV1HashReturn())
            raise AssertionError("to_wire_message_v1 should reject invalid returned type hashes")
        except ValueError:
            pass
    class UnknownToWireMessageV1HashReturn:
        def to_wire_message_v1(self):
            return unknown_return_hash, b""

    try:
        datapod.to_wire_message_v1(UnknownToWireMessageV1HashReturn())
        raise AssertionError("to_wire_message_v1 should reject unknown returned type hashes")
    except ValueError:
        pass

    class BadToWireMessageV1WireReturn:
        def to_wire_message_v1(self):
            return datapod.Point.TYPE_HASH, b""

    try:
        datapod.to_wire_message_v1(BadToWireMessageV1WireReturn())
        raise AssertionError("to_wire_message_v1 should reject invalid returned wire bytes")
    except ValueError:
        pass
    class BytearrayToWireMessageV1WireReturn:
        def to_wire_message_v1(self):
            return py_matrix_v1_hash, bytearray(py_matrix_v1_wire)

    bytearray_v1_return_hash, bytearray_v1_return_wire = datapod.to_wire_message_v1(
        BytearrayToWireMessageV1WireReturn()
    )
    assert bytearray_v1_return_hash == py_matrix_v1_hash
    assert bytearray_v1_return_wire == py_matrix_v1_wire

    class MemoryviewToWireMessageV1WireReturn:
        def to_wire_message_v1(self):
            return py_matrix_v1_hash, memoryview(bytearray(py_matrix_v1_wire))

    memoryview_v1_return_hash, memoryview_v1_return_wire = datapod.to_wire_message_v1(
        MemoryviewToWireMessageV1WireReturn()
    )
    assert memoryview_v1_return_hash == py_matrix_v1_hash
    assert memoryview_v1_return_wire == py_matrix_v1_wire
    class NonCallableWireFrame:
        to_wire_frame = object()

    try:
        datapod.wire_frame(NonCallableWireFrame())
        raise AssertionError("wire_frame should reject non-callable to_wire_frame attributes")
    except ValueError:
        pass
    class NonCallableArchive:
        archive = object()

    try:
        datapod.archive(NonCallableArchive())
        raise AssertionError("archive should reject non-callable archive attributes")
    except ValueError:
        pass
    class BadWireFrameReturn:
        def to_wire_frame(self):
            return object()

    try:
        datapod.wire_frame(BadWireFrameReturn())
        raise AssertionError("wire_frame should reject non-WireFrame return values")
    except ValueError:
        pass
    class InvalidWireFrameReturn:
        def to_wire_frame(self):
            return datapod.WireFrame(datapod.Point.TYPE_HASH, b"", b"")

    try:
        datapod.wire_frame(InvalidWireFrameReturn())
        raise AssertionError("wire_frame should reject invalid returned WireFrame values")
    except ValueError:
        pass
    invalid_direct_frame = datapod.WireFrame(datapod.Point.TYPE_HASH, b"", b"")
    try:
        invalid_direct_frame.to_wire_message()
        raise AssertionError("WireFrame.to_wire_message should reject invalid frame contents")
    except ValueError:
        pass
    class BadArchiveReturn:
        def archive(self):
            return object()

    try:
        datapod.archive(BadArchiveReturn())
        raise AssertionError("archive should reject non-WireFrame return values")
    except ValueError:
        pass
    class InvalidArchiveReturn:
        def archive(self):
            return datapod.ArchiveFrame(datapod.Point.TYPE_HASH, b"", b"")

    try:
        datapod.archive(InvalidArchiveReturn())
        raise AssertionError("archive should reject invalid returned ArchiveFrame values")
    except ValueError:
        pass
    assert py_matrix_frame.type_hash == py_matrix_hash
    assert py_matrix_frame._owner is py_matrix
    assert py_matrix_frame.payload.tobytes() == bytes([1, 2, 3, 4, 5, 6])
    datapod.validate_wire_frame(py_matrix_frame.type_hash, py_matrix_frame.header, py_matrix_frame.payload)
    py_matrix_frame_view = datapod.Matrix.view_from_wire_frame(py_matrix_frame)
    py_matrix_archive_view = datapod.Matrix.view_archive(py_matrix_archive)
    assert py_matrix_archive_view.payload.tobytes() == bytes([1, 2, 3, 4, 5, 6])
    assert (py_matrix_frame_view.rows, py_matrix_frame_view.cols, py_matrix_frame_view.element_size) == (2, 3, 1)
    assert isinstance(py_matrix_frame_view.payload, memoryview)
    assert py_matrix_frame_view.payload.obj is py_matrix_frame.payload.obj
    assert py_matrix_frame_view.payload.tobytes() == bytes([1, 2, 3, 4, 5, 6])
    generic_matrix_frame_view = datapod.view_wire_frame(datapod.Matrix, py_matrix_frame)
    assert generic_matrix_frame_view.payload.tobytes() == bytes([1, 2, 3, 4, 5, 6])
    generic_matrix_archive_view = datapod.view_archive(datapod.Matrix, py_matrix_archive)
    assert generic_matrix_archive_view.payload.tobytes() == bytes([1, 2, 3, 4, 5, 6])
    try:
        datapod.view_archive(object, py_matrix_archive)
        raise AssertionError("generic view_archive should reject unsupported classes")
    except ValueError:
        pass
    try:
        datapod.from_archive(object, py_matrix_archive)
        raise AssertionError("generic from_archive should reject unsupported classes")
    except ValueError:
        pass
    try:
        datapod.view_wire_frame(datapod.Matrix, object())
        raise AssertionError("generic view_wire_frame should reject non-WireFrame arguments")
    except ValueError:
        pass
    try:
        datapod.view_archive(datapod.Matrix, object())
        raise AssertionError("generic view_archive should reject non-WireFrame arguments")
    except ValueError:
        pass
    try:
        datapod.from_archive(datapod.Matrix, object())
        raise AssertionError("generic from_archive should reject non-WireFrame arguments")
    except ValueError:
        pass
    class BoolTypeHashArchiveHooks:
        TYPE_HASH = True

        @classmethod
        def view_from_wire_frame(cls, frame):
            return cls()

        @classmethod
        def view_archive(cls, frame):
            return cls()

        @classmethod
        def from_archive(cls, frame):
            return cls()

    class FloatTypeHashArchiveHooks:
        TYPE_HASH = float(datapod.Matrix.TYPE_HASH)

        @classmethod
        def view_from_wire_frame(cls, frame):
            return cls()

        @classmethod
        def view_archive(cls, frame):
            return cls()

        @classmethod
        def from_archive(cls, frame):
            return cls()

    for bad_type in (BoolTypeHashArchiveHooks, FloatTypeHashArchiveHooks):
        for func, args in (
            (datapod.view_wire_frame, (bad_type, py_matrix_frame)),
            (datapod.view_archive, (bad_type, py_matrix_archive)),
            (datapod.from_archive, (bad_type, py_matrix_archive)),
        ):
            try:
                func(*args)
                raise AssertionError(
                    f"{func.__name__} should reject invalid class TYPE_HASH before hooks"
                )
            except TypeError:
                pass
    class HostileArchiveTypeHashDescriptor:
        def __get__(self, obj, cls=None):
            raise RuntimeError("hostile archive class TYPE_HASH lookup")

    class HostileTypeHashArchiveHooks:
        TYPE_HASH = HostileArchiveTypeHashDescriptor()

        @classmethod
        def view_from_wire_frame(cls, frame):
            return cls()

        @classmethod
        def view_archive(cls, frame):
            return cls()

        @classmethod
        def from_archive(cls, frame):
            return cls()

    for func, args in (
        (datapod.view_wire_frame, (HostileTypeHashArchiveHooks, py_matrix_frame)),
        (datapod.view_archive, (HostileTypeHashArchiveHooks, py_matrix_archive)),
        (datapod.from_archive, (HostileTypeHashArchiveHooks, py_matrix_archive)),
    ):
        try:
            func(*args)
            raise AssertionError(
                f"{func.__name__} should fail closed on hostile class TYPE_HASH metadata"
            )
        except ValueError:
            pass
    class NonCallableViewFromWireFrame:
        view_from_wire_frame = object()

    try:
        datapod.view_wire_frame(NonCallableViewFromWireFrame, py_matrix_frame)
        raise AssertionError("generic view_wire_frame should reject non-callable view hooks")
    except ValueError:
        pass
    class NonCallableViewArchive:
        view_archive = object()

    try:
        datapod.view_archive(NonCallableViewArchive, py_matrix_archive)
        raise AssertionError("generic view_archive should reject non-callable view hooks")
    except ValueError:
        pass
    class NonCallableFromArchive:
        from_archive = object()

    try:
        datapod.from_archive(NonCallableFromArchive, py_matrix_archive)
        raise AssertionError("generic from_archive should reject non-callable archive hooks")
    except ValueError:
        pass
    class NonCallableFromWireFrame:
        TYPE_HASH = datapod.Matrix.TYPE_HASH
        from_wire_frame = object()

    try:
        datapod.from_archive(NonCallableFromWireFrame, py_matrix_archive)
        raise AssertionError("generic from_archive should reject non-callable frame decoders")
    except ValueError:
        pass
    bad_registered_archive_hash = datapod.type_hash_name("acme.bad_registered_archive_decode.v1")
    previous_registered_archive = datapod.__datapod_registry__.get(bad_registered_archive_hash)
    class NonCallableRegisteredFromArchive:
        from_archive = object()

    datapod.__datapod_registry__[bad_registered_archive_hash] = NonCallableRegisteredFromArchive
    try:
        try:
            datapod.from_archive(bad_registered_archive_hash, py_matrix_archive)
            raise AssertionError("from_archive should reject non-callable registered archive decoders")
        except ValueError:
            pass
        class MissingRegisteredFromWireFrame:
            pass

        datapod.__datapod_registry__[bad_registered_archive_hash] = MissingRegisteredFromWireFrame
        try:
            datapod.from_archive(bad_registered_archive_hash, py_matrix_archive)
            raise AssertionError("from_archive should reject registered decoders without frame decode")
        except ValueError:
            pass
    finally:
        if previous_registered_archive is None:
            datapod.__datapod_registry__.pop(bad_registered_archive_hash, None)
        else:
            datapod.__datapod_registry__[bad_registered_archive_hash] = previous_registered_archive
    py_matrix_from_frame = datapod.Matrix.from_wire_frame(py_matrix_frame)
    py_matrix_from_archive = datapod.Matrix.from_archive(py_matrix_archive)
    assert (py_matrix_from_archive.rows, py_matrix_from_archive.cols, py_matrix_from_archive.element_size) == (2, 3, 1)
    assert (py_matrix_from_frame.rows, py_matrix_from_frame.cols, py_matrix_from_frame.element_size) == (2, 3, 1)
    matrix_decoded_as = datapod.decode_as(datapod.Matrix, py_matrix_hash, py_matrix_wire)
    assert (matrix_decoded_as.rows, matrix_decoded_as.cols, matrix_decoded_as.element_size) == (2, 3, 1)
    try:
        datapod.decode_as(object, py_matrix_hash, py_matrix_wire)
        raise AssertionError("decode_as should reject unsupported classes")
    except ValueError:
        pass
    try:
        datapod.decode_as(object(), py_matrix_hash, py_matrix_wire)
        raise AssertionError("decode_as should reject non-class targets")
    except TypeError:
        pass
    class NonCallableFromWireMessage:
        TYPE_HASH = py_matrix_hash
        from_wire_message = object()

    try:
        datapod.decode_as(NonCallableFromWireMessage, py_matrix_hash, py_matrix_wire)
        raise AssertionError("decode_as should reject non-callable from_wire_message")
    except ValueError:
        pass
    class WrongTypeHashDecodeAs:
        TYPE_HASH = datapod.Point.TYPE_HASH

        @classmethod
        def from_wire_message(cls, type_hash, wire):
            return cls()

    try:
        datapod.decode_as(WrongTypeHashDecodeAs, py_matrix_hash, py_matrix_wire)
        raise AssertionError("decode_as should reject class TYPE_HASH mismatches before hooks")
    except ValueError:
        pass
    class BoolTypeHashDecodeAs:
        TYPE_HASH = True

        @classmethod
        def from_wire_message(cls, type_hash, wire):
            return cls()

    class FloatTypeHashDecodeAs:
        TYPE_HASH = float(py_matrix_hash)

        @classmethod
        def from_wire_message(cls, type_hash, wire):
            return cls()

    for bad_cls in (BoolTypeHashDecodeAs, FloatTypeHashDecodeAs):
        try:
            datapod.decode_as(bad_cls, py_matrix_hash, py_matrix_wire)
            raise AssertionError("decode_as should reject invalid class TYPE_HASH values")
        except TypeError:
            pass
    class HostileDecodeAsMetadataDescriptor:
        def __get__(self, obj, cls=None):
            raise RuntimeError("hostile decode_as metadata lookup")

    class HostileTypeHashDecodeAs:
        TYPE_HASH = HostileDecodeAsMetadataDescriptor()

        @classmethod
        def from_wire_message(cls, type_hash, wire):
            return cls()

    try:
        datapod.decode_as(HostileTypeHashDecodeAs, py_matrix_hash, py_matrix_wire)
        raise AssertionError("decode_as should fail closed on hostile class TYPE_HASH metadata")
    except ValueError:
        pass
    class MaliciousDecodeAsIgnoresWire:
        TYPE_HASH = py_matrix_hash

        @classmethod
        def from_wire_message(cls, type_hash, wire):
            return cls()

    try:
        datapod.decode_as(MaliciousDecodeAsIgnoresWire, py_matrix_hash, b"")
        raise AssertionError("decode_as should validate wire before invoking custom decoders")
    except ValueError:
        pass
    bad_registered_decode_hash = datapod.type_hash_name("acme.bad_registered_decode.v1")
    previous_registered_decode = datapod.__datapod_registry__.get(bad_registered_decode_hash)
    class NonCallableRegisteredFromWireMessage:
        from_wire_message = object()

    datapod.__datapod_registry__[bad_registered_decode_hash] = NonCallableRegisteredFromWireMessage
    try:
        try:
            datapod.from_wire_message(bad_registered_decode_hash, b"")
            raise AssertionError("from_wire_message should reject non-callable registered decoders")
        except ValueError:
            pass
        class MissingRegisteredFromWireMessage:
            pass

        datapod.__datapod_registry__[bad_registered_decode_hash] = MissingRegisteredFromWireMessage
        try:
            datapod.from_wire_message(bad_registered_decode_hash, b"")
            raise AssertionError("from_wire_message should reject registered decoders without from_wire_message")
        except ValueError:
            pass
    finally:
        if previous_registered_decode is None:
            del datapod.__datapod_registry__[bad_registered_decode_hash]
        else:
            datapod.__datapod_registry__[bad_registered_decode_hash] = previous_registered_decode
    malicious_registered_hash = datapod.register_schema(
        "acme.malicious_registered_decode.v1",
        2,
        "bytes",
    )
    previous_malicious_registered = datapod.__datapod_registry__.get(malicious_registered_hash)

    class MaliciousRegisteredFromWireMessage:
        @classmethod
        def from_wire_message(cls, type_hash, wire):
            return cls()

    datapod.__datapod_registry__[malicious_registered_hash] = MaliciousRegisteredFromWireMessage
    try:
        datapod.from_wire_message(malicious_registered_hash, b"")
        raise AssertionError("from_wire_message should validate wire before registered decoders")
    except ValueError:
        pass
    finally:
        if previous_malicious_registered is None:
            datapod.__datapod_registry__.pop(malicious_registered_hash, None)
        else:
            datapod.__datapod_registry__[malicious_registered_hash] = previous_malicious_registered
    previous_matrix_registered = datapod.__datapod_registry__.get(py_matrix_hash)

    class WrongTypeHashRegisteredFromWireMessage:
        TYPE_HASH = datapod.Point.TYPE_HASH

        @classmethod
        def from_wire_message(cls, type_hash, wire):
            return cls()

    datapod.__datapod_registry__[py_matrix_hash] = WrongTypeHashRegisteredFromWireMessage
    try:
        try:
            datapod.from_wire_message(py_matrix_hash, py_matrix_wire)
            raise AssertionError("from_wire_message should reject registry classes with wrong TYPE_HASH")
        except ValueError:
            pass
        raw_from_wire_message = datapod.datapod_type.__globals__[
            "_datapod_raw_from_wire_message"
        ]
        try:
            raw_from_wire_message(py_matrix_hash, py_matrix_wire)
            raise AssertionError(
                "raw from_wire_message should reject registry classes with wrong TYPE_HASH"
            )
        except ValueError:
            pass
    finally:
        if previous_matrix_registered is None:
            datapod.__datapod_registry__.pop(py_matrix_hash, None)
        else:
            datapod.__datapod_registry__[py_matrix_hash] = previous_matrix_registered

    raw_decode_as = datapod.datapod_type.__globals__["_datapod_raw_decode_as"]
    try:
        raw_decode_as(WrongTypeHashRegisteredFromWireMessage, py_matrix_hash, py_matrix_wire)
        raise AssertionError("raw decode_as should reject classes with wrong TYPE_HASH")
    except ValueError:
        pass
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
    assert datapod.has_archive(custom_hash) is True
    assert datapod.has_view(custom_hash) is True
    assert datapod.has_owned_decode(custom_hash) is False
    assert datapod.archive_shape(custom_hash) == "runtime-schema"
    custom = CustomPacket(7, 3, b"payload")
    custom_wire_hash, custom_wire = datapod.to_wire_message(custom)
    assert custom_wire_hash == custom_hash
    custom_v1_hash, custom_v1_wire = datapod.to_wire_message_v1(custom)
    assert custom_v1_hash == CustomPacket.CANONICAL_TYPE_HASH
    assert custom_v1_hash == custom_hash
    assert custom_v1_wire == custom_wire
    custom_frame = custom.to_wire_frame()
    assert isinstance(custom_frame, datapod.WireFrame)
    custom_archive = custom.archive()
    assert isinstance(custom_archive, datapod.ArchiveFrame)
    assert custom_archive.payload.tobytes() == b"payload"
    assert custom_frame.type_hash == custom_hash
    assert custom_frame.header.tobytes() == custom_wire[:4]
    assert custom_frame.payload.tobytes() == b"payload"
    datapod.validate_wire_frame(custom_frame.type_hash, custom_frame.header, custom_frame.payload)
    assert datapod.is_valid_wire_frame(custom_frame.type_hash, custom_frame.header, custom_frame.payload)
    custom_from_frame = CustomPacket.from_wire_frame(custom_frame)
    custom_from_archive = CustomPacket.from_archive(custom_archive)
    assert (custom_from_archive.channel, custom_from_archive.flags, custom_from_archive.data) == (
        7,
        3,
        b"payload",
    )
    assert (custom_from_frame.channel, custom_from_frame.flags, custom_from_frame.data) == (
        7,
        3,
        b"payload",
    )
    custom_frame_view = CustomPacket.view_from_wire_frame(custom_frame)
    custom_archive_view = CustomPacket.view_archive(custom_archive)
    assert custom_archive_view.data.tobytes() == b"payload"
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
    custom_v1_header_view, custom_v1_payload_view = datapod.split_wire_message_view_v1(
        custom_v1_hash, custom_v1_wire
    )
    assert custom_v1_header_view.tobytes() == custom_wire[:4]
    assert custom_v1_payload_view.tobytes() == b"payload"
    custom_v1_view = CustomPacket.view_from_wire_v1(custom_v1_hash, custom_v1_wire)
    assert (custom_v1_view.channel, custom_v1_view.flags) == (7, 3)
    assert custom_v1_view.data.tobytes() == b"payload"
    custom_v1_out = CustomPacket.from_wire_message_v1(custom_v1_hash, custom_v1_wire)
    assert (custom_v1_out.channel, custom_v1_out.flags, custom_v1_out.data) == (7, 3, b"payload")
    datapod.validate_wire_message(custom_wire_hash, custom_wire)
    assert datapod.is_valid_wire_message(custom_wire_hash, custom_wire)
    assert not datapod.is_valid_wire_message(custom_wire_hash, custom_wire[:1])
    custom_header, custom_payload = datapod.split_wire_message_view(custom_wire_hash, custom_wire)
    assert custom_header.tobytes() == custom_wire[:4]
    assert isinstance(custom_payload, memoryview)
    assert custom_payload.tobytes() == b"payload"
    mutable_custom_wire = bytearray(custom_wire)
    mutable_header, mutable_payload = datapod.split_wire_message_view(
        custom_wire_hash, mutable_custom_wire
    )
    assert mutable_header.obj is mutable_custom_wire
    assert mutable_payload.obj is mutable_custom_wire
    mutable_custom_wire[4] = ord("P")
    assert mutable_payload.tobytes() == b"Payload"
    custom_view = CustomPacket.view_from_wire(custom_wire_hash, custom_wire)
    assert (custom_view.channel, custom_view.flags) == (7, 3)
    assert isinstance(custom_view.data, memoryview)
    assert custom_view.data.tobytes() == b"payload"
    CustomPacket.validate_wire_message(custom_wire_hash, custom_wire)
    custom_out = datapod.from_wire_message(custom_wire_hash, custom_wire)
    assert type(custom_out).__name__ == "CustomPacket"
    assert (custom_out.channel, custom_out.flags, custom_out.data) == (7, 3, b"payload")

    repeated = RepeatedScalarPacket(11, 12, b"repeat")
    repeated_hash, repeated_wire = datapod.to_wire_message(repeated)
    assert repeated_hash == RepeatedScalarPacket.TYPE_HASH
    assert repeated_wire[:4] == (11).to_bytes(2, "little") + (12).to_bytes(2, "little")
    repeated_out = RepeatedScalarPacket.from_wire_message(repeated_hash, repeated_wire)
    assert (repeated_out.left, repeated_out.right, repeated_out.data) == (11, 12, b"repeat")
    repeated_view = RepeatedScalarPacket.view_from_wire(repeated_hash, repeated_wire)
    assert (repeated_view.left, repeated_view.right) == (11, 12)
    assert repeated_view.data.tobytes() == b"repeat"

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
    try:
        datapod.join_wire_message(validated_hash, b"\x00\x00", b"bad")
        raise AssertionError("join_wire_message should run custom Python validators")
    except ValueError:
        pass
    invalid_validated_wire = b"\x00\x00bad"

    class InvalidValidatedTupleReturn:
        def to_wire_message(self):
            return validated_hash, invalid_validated_wire

        def to_wire_message_v1(self):
            return validated_hash, invalid_validated_wire

    try:
        datapod.to_wire_message(InvalidValidatedTupleReturn())
        raise AssertionError("generic to_wire_message should validate returned Python datapod tuples")
    except ValueError:
        pass
    try:
        datapod.to_wire_message_v1(InvalidValidatedTupleReturn())
        raise AssertionError("generic to_wire_message_v1 should validate returned Python datapod tuples")
    except ValueError:
        pass
    class MaliciousValidatedDecodeAsIgnoresWire:
        TYPE_HASH = validated_hash

        @classmethod
        def from_wire_message(cls, type_hash, wire):
            return cls()

    try:
        datapod.decode_as(
            MaliciousValidatedDecodeAsIgnoresWire,
            validated_hash,
            invalid_validated_wire,
        )
        raise AssertionError("decode_as should run Python validators before custom decoders")
    except ValueError:
        pass
    try:
        datapod.split_wire_message(validated_hash, invalid_validated_wire)
        raise AssertionError("split_wire_message should run custom Python validators")
    except ValueError:
        pass
    try:
        datapod.split_wire_message_v1(validated_hash, invalid_validated_wire)
        raise AssertionError("split_wire_message_v1 should run custom Python validators")
    except ValueError:
        pass
    try:
        datapod.validate_wire_message(validated_hash, invalid_validated_wire)
        raise AssertionError("generic validate_wire_message should run custom validators")
    except ValueError:
        pass
    try:
        datapod.validate_wire_message_v1(validated_hash, invalid_validated_wire)
        raise AssertionError("generic validate_wire_message_v1 should run custom validators")
    except ValueError:
        pass
    assert not datapod.is_valid_wire_message(validated_hash, invalid_validated_wire)
    assert not datapod.is_valid_wire_message_v1(validated_hash, invalid_validated_wire)
    invalid_validated_frame = datapod.WireFrame(validated_hash, b"\x00\x00", b"bad")
    try:
        datapod.validate_wire_frame(
            validated_hash,
            invalid_validated_frame.header,
            invalid_validated_frame.payload,
        )
        raise AssertionError("generic validate_wire_frame should run custom validators")
    except ValueError:
        pass
    try:
        datapod.validate_wire_frame_v1(
            validated_hash,
            invalid_validated_frame.header,
            invalid_validated_frame.payload,
        )
        raise AssertionError("generic validate_wire_frame_v1 should run custom validators")
    except ValueError:
        pass
    try:
        datapod.validate_wire_frame_parts_v1(
            validated_hash,
            invalid_validated_frame.header,
            invalid_validated_frame.payload,
        )
        raise AssertionError("generic validate_wire_frame_parts_v1 should run custom validators")
    except ValueError:
        pass
    assert not datapod.is_valid_wire_frame(
        validated_hash,
        invalid_validated_frame.header,
        invalid_validated_frame.payload,
    )
    assert not datapod.is_valid_wire_frame_v1(
        validated_hash,
        invalid_validated_frame.header,
        invalid_validated_frame.payload,
    )
    class MaliciousArchiveHooks:
        TYPE_HASH = validated_hash

        @classmethod
        def view_from_wire_frame(cls, frame):
            return cls()

        @classmethod
        def view_archive(cls, frame):
            return cls()

        @classmethod
        def from_archive(cls, frame):
            return cls()

    for func, args in (
        (datapod.view_wire_frame, (MaliciousArchiveHooks, invalid_validated_frame)),
        (datapod.view_archive, (MaliciousArchiveHooks, invalid_validated_frame)),
        (datapod.from_archive, (MaliciousArchiveHooks, invalid_validated_frame)),
    ):
        try:
            func(*args)
            raise AssertionError(
                f"{func.__name__} should validate archive frames before custom hooks"
            )
        except ValueError:
            pass
    try:
        ValidatedPacket.view_from_wire(validated_hash, invalid_validated_wire)
        raise AssertionError("custom Python validator should reject decoded channel=0")
    except ValueError:
        pass
    try:
        ValidatedPacket.validate_wire_message(validated_hash, invalid_validated_wire)
        raise AssertionError("typed validate_wire_message should run custom validators")
    except ValueError:
        pass
    try:
        ValidatedPacket.validate_wire_message_v1(validated_hash, invalid_validated_wire)
        raise AssertionError("typed validate_wire_message_v1 should run custom validators")
    except ValueError:
        pass
    assert (
        "__datapod_declarative_validator_registry__"
        not in datapod.datapod_type.__globals__
    )
    previous_validated_registry_entry = datapod.__datapod_registry__.get(validated_hash)

    class RegistryPoisonNoValidator:
        @classmethod
        def from_wire_message(cls, type_hash, wire):
            return cls()

        @classmethod
        def from_archive(cls, frame):
            return cls()

    datapod.__datapod_registry__[validated_hash] = RegistryPoisonNoValidator
    try:
        for func, args in (
            (datapod.validate_wire_message, (validated_hash, invalid_validated_wire)),
            (datapod.validate_wire_message_v1, (validated_hash, invalid_validated_wire)),
            (datapod.split_wire_message, (validated_hash, invalid_validated_wire)),
            (datapod.split_wire_message_view, (validated_hash, invalid_validated_wire)),
            (datapod.from_wire_message, (validated_hash, invalid_validated_wire)),
            (datapod.from_archive, (validated_hash, invalid_validated_frame)),
            (datapod.decode_as, (RegistryPoisonNoValidator, validated_hash, invalid_validated_wire)),
        ):
            try:
                func(*args)
                raise AssertionError(
                    f"{func.__name__} should not trust mutable public registry entries"
                )
            except ValueError:
                pass
        assert not datapod.is_valid_wire_message(validated_hash, invalid_validated_wire)
    finally:
        if previous_validated_registry_entry is None:
            datapod.__datapod_registry__.pop(validated_hash, None)
        else:
            datapod.__datapod_registry__[validated_hash] = previous_validated_registry_entry

    class MaliciousFrameProducer:
        def to_wire_frame(self):
            return invalid_validated_frame

    class MaliciousArchiveProducer:
        def archive(self):
            return invalid_validated_frame

    declarative_globals = datapod.datapod_type.__globals__
    poisoned_helper_names = (
        "_registered_declarative_validator_type",
        "_run_registered_declarative_frame_validator",
        "_run_registered_declarative_validator",
        "_require_wire_frame_result",
        "_source_validated_declarative_frame",
        "_validate_wire_frame_shape_v1",
        "validate_wire_message_v1",
        "validate_wire_frame",
    )
    previous_helpers = {
        name: declarative_globals[name]
        for name in poisoned_helper_names
    }
    try:
        for name in poisoned_helper_names:
            declarative_globals[name] = lambda *args, **kwargs: None
        for func, args in (
            (datapod.validate_wire_message, (validated_hash, invalid_validated_wire)),
            (datapod.validate_wire_message_v1, (validated_hash, invalid_validated_wire)),
            (
                datapod.validate_wire_frame_parts_v1,
                (
                    validated_hash,
                    invalid_validated_frame.header,
                    invalid_validated_frame.payload,
                ),
            ),
            (
                datapod.validate_wire_frame,
                (
                    validated_hash,
                    invalid_validated_frame.header,
                    invalid_validated_frame.payload,
                ),
            ),
            (
                datapod.validate_wire_frame_v1,
                (
                    validated_hash,
                    invalid_validated_frame.header,
                    invalid_validated_frame.payload,
                ),
            ),
            (
                datapod.join_wire_message,
                (
                    validated_hash,
                    invalid_validated_frame.header,
                    invalid_validated_frame.payload,
                ),
            ),
            (datapod.split_wire_message, (validated_hash, invalid_validated_wire)),
            (datapod.split_wire_message_v1, (validated_hash, invalid_validated_wire)),
            (datapod.split_wire_message_view, (validated_hash, invalid_validated_wire)),
            (datapod.split_wire_message_view_v1, (validated_hash, invalid_validated_wire)),
            (invalid_validated_frame.to_wire_message, ()),
            (datapod.view_wire_frame, (MaliciousArchiveHooks, invalid_validated_frame)),
            (datapod.view_archive, (MaliciousArchiveHooks, invalid_validated_frame)),
            (datapod.from_archive, (MaliciousArchiveHooks, invalid_validated_frame)),
            (datapod.wire_frame, (MaliciousFrameProducer(),)),
            (datapod.archive, (MaliciousArchiveProducer(),)),
        ):
            try:
                func(*args)
                raise AssertionError(
                    f"{func.__name__} should not trust monkey-patched helper globals"
                )
            except ValueError:
                pass
        assert not datapod.is_valid_wire_message(validated_hash, invalid_validated_wire)
        assert not datapod.is_valid_wire_message_v1(validated_hash, invalid_validated_wire)
        assert not datapod.is_valid_wire_frame(
            validated_hash,
            invalid_validated_frame.header,
            invalid_validated_frame.payload,
        )
        assert not datapod.is_valid_wire_frame_v1(
            validated_hash,
            invalid_validated_frame.header,
            invalid_validated_frame.payload,
        )
    finally:
        for name, value in previous_helpers.items():
            declarative_globals[name] = value

    side_effect_packet = SideEffectValidatedPacket(5, b"ok")
    _side_effect_validator_calls.clear()
    side_effect_hash, side_effect_wire = datapod.to_wire_message(side_effect_packet)
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    SideEffectValidatedPacket(5, b"ok").to_wire_message()
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    datapod.wire_frame(side_effect_packet)
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    datapod.archive(side_effect_packet)
    assert _side_effect_validator_calls == [5]
    original_side_effect_to_wire_frame = SideEffectValidatedPacket.to_wire_frame
    try:
        def spoofed_side_effect_to_wire_frame(self):
            return datapod.WireFrame(SideEffectValidatedPacket.TYPE_HASH, b"\x00\x00", b"bad")

        spoofed_side_effect_to_wire_frame.__datapod_generated_frame_encoder__ = True
        SideEffectValidatedPacket.to_wire_frame = spoofed_side_effect_to_wire_frame
        try:
            datapod.wire_frame(SideEffectValidatedPacket(0, b"bad"))
            raise AssertionError(
                "wire_frame should not trust fake generated-frame markers on monkey-patched methods"
            )
        except ValueError:
            pass
    finally:
        SideEffectValidatedPacket.to_wire_frame = original_side_effect_to_wire_frame
    original_side_effect_to_wire_message = SideEffectValidatedPacket.to_wire_message
    try:
        def spoofed_side_effect_to_wire_message(self):
            return SideEffectValidatedPacket.TYPE_HASH, b"\x00\x00bad"

        spoofed_side_effect_to_wire_message.__datapod_generated_message_encoder__ = True
        SideEffectValidatedPacket.to_wire_message = spoofed_side_effect_to_wire_message
        try:
            datapod.to_wire_message(SideEffectValidatedPacket(0, b"bad"))
            raise AssertionError(
                "to_wire_message should not trust fake generated-message markers on monkey-patched methods"
            )
        except ValueError:
            pass
    finally:
        SideEffectValidatedPacket.to_wire_message = original_side_effect_to_wire_message
    try:
        side_effect_packet.to_wire_message = lambda: (
            SideEffectValidatedPacket.TYPE_HASH,
            b"\x00\x00bad",
        )
        try:
            datapod.to_wire_message(side_effect_packet)
            raise AssertionError(
                "to_wire_message should not trust instance-level generated-message overrides"
            )
        except ValueError:
            pass
    finally:
        delattr(side_effect_packet, "to_wire_message")
    class HostileFuncAttribute:
        def __init__(self, result):
            self.result = result

        def __call__(self):
            return self.result

        def __getattribute__(self, name):
            if name == "__func__":
                raise RuntimeError("hostile __func__ lookup")
            return object.__getattribute__(self, name)

    try:
        side_effect_packet.to_wire_message = HostileFuncAttribute(
            (SideEffectValidatedPacket.TYPE_HASH, b"\x00\x00bad")
        )
        try:
            datapod.to_wire_message(side_effect_packet)
            raise AssertionError(
                "to_wire_message should fail closed on hostile instance method metadata"
            )
        except ValueError:
            pass
    finally:
        delattr(side_effect_packet, "to_wire_message")
    try:
        side_effect_packet.to_wire_message_v1 = lambda: (
            SideEffectValidatedPacket.TYPE_HASH,
            b"\x00\x00bad",
        )
        try:
            datapod.to_wire_message_v1(side_effect_packet)
            raise AssertionError(
                "to_wire_message_v1 should not trust instance-level generated-message overrides"
            )
        except ValueError:
            pass
    finally:
        delattr(side_effect_packet, "to_wire_message_v1")
    try:
        side_effect_packet.to_wire_frame = lambda: datapod.WireFrame(
            SideEffectValidatedPacket.TYPE_HASH,
            b"\x00\x00",
            b"bad",
        )
        try:
            datapod.wire_frame(side_effect_packet)
            raise AssertionError(
                "wire_frame should not trust instance-level generated-frame overrides"
            )
        except ValueError:
            pass
    finally:
        delattr(side_effect_packet, "to_wire_frame")
    original_side_effect_validate_object = SideEffectValidatedPacket.__dict__[
        "__datapod_validate_object__"
    ]
    class HostileValidatorDescriptor:
        def __get__(self, obj, cls=None):
            raise RuntimeError("hostile validator metadata lookup")

    try:
        SideEffectValidatedPacket.__datapod_validate_object__ = HostileValidatorDescriptor()
        _side_effect_validator_calls.clear()
        datapod.to_wire_message(side_effect_packet)
        assert _side_effect_validator_calls == [5, 5]
        _side_effect_validator_calls.clear()
        datapod.wire_frame(side_effect_packet)
        assert _side_effect_validator_calls == [5, 5]
        _side_effect_validator_calls.clear()
        datapod.archive(side_effect_packet)
        assert _side_effect_validator_calls == [5, 5]
    finally:
        SideEffectValidatedPacket.__datapod_validate_object__ = (
            original_side_effect_validate_object
        )
    hostile_runtime_validator_hash = datapod.register_schema(
        "acme.hostile_runtime_registry_validator.v1",
        0,
        "bytes",
    )
    previous_hostile_runtime_validator = datapod.__datapod_registry__.get(
        hostile_runtime_validator_hash
    )

    class HostileRuntimeValidatorDescriptor:
        def __get__(self, obj, cls=None):
            raise RuntimeError("hostile runtime registry validator metadata lookup")

    class HostileRuntimeValidatorClass:
        __datapod_validator__ = HostileRuntimeValidatorDescriptor()

    try:
        datapod.__datapod_registry__[hostile_runtime_validator_hash] = (
            HostileRuntimeValidatorClass
        )
        try:
            datapod.validate_wire_message(hostile_runtime_validator_hash, b"payload")
            raise AssertionError(
                "runtime-registry declarative validation should fail closed on hostile metadata"
            )
        except ValueError:
            pass
    finally:
        if previous_hostile_runtime_validator is None:
            datapod.__datapod_registry__.pop(hostile_runtime_validator_hash, None)
        else:
            datapod.__datapod_registry__[hostile_runtime_validator_hash] = (
                previous_hostile_runtime_validator
            )
    original_side_effect_from_wire_message = SideEffectValidatedPacket.__dict__["from_wire_message"]
    try:
        @classmethod
        def spoofed_side_effect_from_wire_message(cls, type_hash, wire):
            return cls.__new__(cls)

        spoofed_side_effect_from_wire_message.__func__.__datapod_generated_wire_decoder__ = True
        SideEffectValidatedPacket.from_wire_message = spoofed_side_effect_from_wire_message
        try:
            datapod.decode_as(SideEffectValidatedPacket, SideEffectValidatedPacket.TYPE_HASH, b"\x00\x00bad")
            raise AssertionError(
                "decode_as should not trust fake generated-decoder markers on monkey-patched methods"
            )
        except ValueError:
            pass
    finally:
        SideEffectValidatedPacket.from_wire_message = original_side_effect_from_wire_message
    _side_effect_validator_calls.clear()
    datapod.validate_wire_message(side_effect_hash, side_effect_wire)
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    datapod.split_wire_message(side_effect_hash, side_effect_wire)
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    datapod.split_wire_message_v1(side_effect_hash, side_effect_wire)
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    SideEffectValidatedPacket.from_wire_message(side_effect_hash, side_effect_wire)
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    SideEffectValidatedPacket.from_wire_message_v1(side_effect_hash, side_effect_wire)
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    datapod.from_wire_message(side_effect_hash, side_effect_wire)
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    datapod.decode_as(SideEffectValidatedPacket, side_effect_hash, side_effect_wire)
    assert _side_effect_validator_calls == [5]

    low_hash_packet = LowHashSideEffectPacket(7, b"ok")
    original_low_hash_type_hash = LowHashSideEffectPacket.TYPE_HASH
    assert original_low_hash_type_hash == datapod.type_hash_name(
        "acme.low_hash_side_effect_packet.v1"
    )
    _low_hash_side_effect_validator_calls.clear()
    LowHashSideEffectPacket.TYPE_HASH = float(original_low_hash_type_hash)
    try:
        datapod.to_wire_message(low_hash_packet)
        assert _low_hash_side_effect_validator_calls == [7, 7]
        _low_hash_side_effect_validator_calls.clear()
        datapod.wire_frame(low_hash_packet)
        assert _low_hash_side_effect_validator_calls == [7, 7]
        _low_hash_side_effect_validator_calls.clear()
        datapod.archive(low_hash_packet)
        assert _low_hash_side_effect_validator_calls == [7, 7]
        try:
            datapod.decode_as(LowHashSideEffectPacket, original_low_hash_type_hash, b"\x07\x00ok")
            raise AssertionError("decode_as should reject invalid class TYPE_HASH values")
        except (TypeError, ValueError):
            pass
    finally:
        LowHashSideEffectPacket.TYPE_HASH = original_low_hash_type_hash

    _low_hash_side_effect_validator_calls.clear()
    delattr(LowHashSideEffectPacket, "TYPE_HASH")
    try:
        datapod.to_wire_message(low_hash_packet)
        assert _low_hash_side_effect_validator_calls == [7, 7]
        _low_hash_side_effect_validator_calls.clear()
        datapod.wire_frame(low_hash_packet)
        assert _low_hash_side_effect_validator_calls == [7, 7]
        _low_hash_side_effect_validator_calls.clear()
        datapod.archive(low_hash_packet)
        assert _low_hash_side_effect_validator_calls == [7, 7]
    finally:
        LowHashSideEffectPacket.TYPE_HASH = original_low_hash_type_hash

    strict_new_packet = object.__new__(StrictNewValidatedPacket)
    strict_new_packet.channel = 9
    strict_new_packet.data = b"strict"
    _strict_new_validator_calls.clear()
    strict_new_hash, strict_new_wire = datapod.to_wire_message(strict_new_packet)
    assert strict_new_hash == StrictNewValidatedPacket.TYPE_HASH
    assert _strict_new_validator_calls == [9]
    _strict_new_validator_calls.clear()
    datapod.validate_wire_message(strict_new_hash, strict_new_wire)
    assert _strict_new_validator_calls == [9]
    _strict_new_validator_calls.clear()
    datapod.join_wire_message(strict_new_hash, b"\x09\x00", b"strict")
    assert _strict_new_validator_calls == [9]
    _strict_new_validator_calls.clear()
    strict_new_decoded = datapod.from_wire_message(strict_new_hash, strict_new_wire)
    assert (
        strict_new_decoded.channel,
        strict_new_decoded.data,
    ) == (9, b"strict")
    assert _strict_new_validator_calls == [9]
    _strict_new_validator_calls.clear()
    strict_new_typed_decoded = StrictNewValidatedPacket.from_wire_message(
        strict_new_hash,
        strict_new_wire,
    )
    assert (
        strict_new_typed_decoded.channel,
        strict_new_typed_decoded.data,
    ) == (9, b"strict")
    assert _strict_new_validator_calls == [9]
    _strict_new_validator_calls.clear()
    strict_new_view = StrictNewValidatedPacket.view_from_wire(
        strict_new_hash,
        strict_new_wire,
    )
    assert (
        strict_new_view.channel,
        strict_new_view.data.tobytes(),
    ) == (9, b"strict")
    assert strict_new_view.data.readonly
    assert _strict_new_validator_calls == [9]
    strict_new_frame = datapod.WireFrame(
        strict_new_hash,
        strict_new_wire[:2],
        strict_new_wire[2:],
    )
    _strict_new_validator_calls.clear()
    strict_new_frame_view = StrictNewValidatedPacket.view_from_wire_frame(strict_new_frame)
    assert (
        strict_new_frame_view.channel,
        strict_new_frame_view.data.tobytes(),
    ) == (9, b"strict")
    assert strict_new_frame_view.data.readonly
    assert _strict_new_validator_calls == [9]
    _strict_new_validator_calls.clear()
    strict_new_archive_view = StrictNewValidatedPacket.view_archive(strict_new_frame)
    assert (
        strict_new_archive_view.channel,
        strict_new_archive_view.data.tobytes(),
    ) == (9, b"strict")
    assert strict_new_archive_view.data.readonly
    assert _strict_new_validator_calls == [9]

    frozen_packet = FrozenValidatedPacket(10, b"frozen")
    _frozen_validator_calls.clear()
    frozen_hash, frozen_wire = datapod.to_wire_message(frozen_packet)
    assert frozen_hash == FrozenValidatedPacket.TYPE_HASH
    assert _frozen_validator_calls == [(10, None)]
    _frozen_validator_calls.clear()
    datapod.validate_wire_message(frozen_hash, frozen_wire)
    assert _frozen_validator_calls == [(10, True)]
    _frozen_validator_calls.clear()
    frozen_view = FrozenValidatedPacket.view_from_wire(frozen_hash, frozen_wire)
    assert (frozen_view.channel, frozen_view.data.tobytes()) == (10, b"frozen")
    assert frozen_view.data.readonly
    assert _frozen_validator_calls == [(10, True)]
    _frozen_validator_calls.clear()
    frozen_frame = datapod.WireFrame(frozen_hash, frozen_wire[:2], frozen_wire[2:])
    frozen_frame_view = FrozenValidatedPacket.view_from_wire_frame(frozen_frame)
    assert (frozen_frame_view.channel, frozen_frame_view.data.tobytes()) == (10, b"frozen")
    assert frozen_frame_view.data.readonly
    assert _frozen_validator_calls == [(10, True)]

    @datapod.datapod_type(
        "acme.hostile_setattr_label_packet.v1",
        "<H",
        ("channel",),
        payload="data",
    )
    class HostileSetattrLabelPacket:
        __slots__ = ()

        def __getattribute__(self, name):
            if name == "__class__":
                raise RuntimeError("hostile __class__ lookup")
            return object.__getattribute__(self, name)

    try:
        datapod.from_wire_message(
            HostileSetattrLabelPacket.TYPE_HASH,
            b"\x01\x00payload",
        )
        raise AssertionError(
            "declarative hydration should fail closed when field setting and __class__ lookup are hostile"
        )
    except ValueError as error:
        assert "cannot set declarative field" in str(error)

    try:
        NonBoolValidatedPacket(1, b"bad").to_wire_message()
        raise AssertionError("custom Python validators should return bool, not truthy objects")
    except TypeError:
        pass
    try:
        datapod.join_wire_message(
            NonBoolValidatedPacket.TYPE_HASH,
            b"\x01\x00",
            b"bad",
        )
        raise AssertionError("join_wire_message should reject non-bool custom validator results")
    except TypeError:
        pass
    non_bool_wire = b"\x01\x00bad"

    class NonBoolValidatedTupleReturn:
        def to_wire_message(self):
            return NonBoolValidatedPacket.TYPE_HASH, non_bool_wire

        def to_wire_message_v1(self):
            return NonBoolValidatedPacket.TYPE_HASH, non_bool_wire

    try:
        datapod.to_wire_message(NonBoolValidatedTupleReturn())
        raise AssertionError(
            "generic to_wire_message should reject non-bool validator results from returned tuples"
        )
    except TypeError:
        pass
    try:
        datapod.to_wire_message_v1(NonBoolValidatedTupleReturn())
        raise AssertionError(
            "generic to_wire_message_v1 should reject non-bool validator results from returned tuples"
        )
    except TypeError:
        pass
    try:
        datapod.validate_wire_message(NonBoolValidatedPacket.TYPE_HASH, non_bool_wire)
        raise AssertionError("generic validation should reject non-bool custom validator results")
    except TypeError:
        pass
    assert not datapod.is_valid_wire_message(NonBoolValidatedPacket.TYPE_HASH, non_bool_wire)
    raising_validator_wire = b"\x01\x00abc"
    try:
        datapod.validate_wire_message(RaisingValidatorPacket.TYPE_HASH, raising_validator_wire)
        raise AssertionError("validate_wire_message should propagate custom validator exceptions")
    except RuntimeError:
        pass
    assert not datapod.is_valid_wire_message(RaisingValidatorPacket.TYPE_HASH, raising_validator_wire)
    assert not datapod.is_valid_wire_message_v1(RaisingValidatorPacket.TYPE_HASH, raising_validator_wire)
    assert not datapod.is_valid_wire_frame(
        RaisingValidatorPacket.TYPE_HASH,
        raising_validator_wire[:2],
        raising_validator_wire[2:],
    )
    assert not datapod.is_valid_wire_frame_v1(
        RaisingValidatorPacket.TYPE_HASH,
        raising_validator_wire[:2],
        raising_validator_wire[2:],
    )
    mutating_wire = bytearray(b"\x01\x00abc")
    _mutating_validator_payload_readonly.clear()
    try:
        datapod.validate_wire_message(MutatingValidatorPacket.TYPE_HASH, mutating_wire)
        raise AssertionError("generic validators should receive read-only payload views")
    except TypeError:
        pass
    assert _mutating_validator_payload_readonly == [True]
    assert mutating_wire == bytearray(b"\x01\x00abc")
    _mutating_validator_payload_readonly.clear()
    assert not datapod.is_valid_wire_message(MutatingValidatorPacket.TYPE_HASH, mutating_wire)
    assert _mutating_validator_payload_readonly == [True]
    assert mutating_wire == bytearray(b"\x01\x00abc")
    mutating_header = bytearray(b"\x01\x00")
    mutating_payload = bytearray(b"abc")
    _mutating_validator_payload_readonly.clear()
    try:
        datapod.join_wire_message(
            MutatingValidatorPacket.TYPE_HASH,
            mutating_header,
            mutating_payload,
        )
        raise AssertionError("join_wire_message validators should receive read-only payload views")
    except TypeError:
        pass
    assert _mutating_validator_payload_readonly == [True]
    assert mutating_header == bytearray(b"\x01\x00")
    assert mutating_payload == bytearray(b"abc")
    _mutating_validator_payload_readonly.clear()
    try:
        datapod.split_wire_message_view(MutatingValidatorPacket.TYPE_HASH, mutating_wire)
        raise AssertionError(
            "split_wire_message_view validators should receive read-only payload views"
        )
    except TypeError:
        pass
    assert _mutating_validator_payload_readonly == [True]
    assert mutating_wire == bytearray(b"\x01\x00abc")
    _mutating_validator_payload_readonly.clear()
    try:
        MutatingValidatorPacket.view_from_wire(MutatingValidatorPacket.TYPE_HASH, mutating_wire)
        raise AssertionError("typed view_from_wire validators should see read-only payload views")
    except TypeError:
        pass
    assert _mutating_validator_payload_readonly == [True]
    assert mutating_wire == bytearray(b"\x01\x00abc")
    mutating_frame = datapod.WireFrame(
        MutatingValidatorPacket.TYPE_HASH,
        mutating_wire[:2],
        mutating_wire[2:],
    )
    _mutating_validator_payload_readonly.clear()
    try:
        MutatingValidatorPacket.view_from_wire_frame(mutating_frame)
        raise AssertionError(
            "typed view_from_wire_frame validators should see read-only payload views"
        )
    except TypeError:
        pass
    assert _mutating_validator_payload_readonly == [True]
    assert mutating_wire == bytearray(b"\x01\x00abc")
    _mutating_validator_payload_readonly.clear()
    try:
        MutatingValidatorPacket.view_archive(mutating_frame)
        raise AssertionError(
            "typed view_archive validators should see read-only payload views"
        )
    except TypeError:
        pass
    assert _mutating_validator_payload_readonly == [True]
    assert mutating_wire == bytearray(b"\x01\x00abc")
    _mutating_validator_payload_readonly.clear()
    try:
        datapod.view_archive(MutatingValidatorPacket, mutating_frame)
        raise AssertionError(
            "generic view_archive validators should see read-only payload views"
        )
    except TypeError:
        pass
    assert _mutating_validator_payload_readonly == [True]
    assert mutating_wire == bytearray(b"\x01\x00abc")
    _side_effect_validator_calls.clear()
    SideEffectValidatedPacket.validate_wire_message(side_effect_hash, side_effect_wire)
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    SideEffectValidatedPacket.validate_wire_message_v1(side_effect_hash, side_effect_wire)
    assert _side_effect_validator_calls == [5]
    side_effect_header, side_effect_payload = datapod.split_wire_message_view_v1(
        side_effect_hash,
        side_effect_wire,
    )
    _side_effect_validator_calls.clear()
    datapod.validate_wire_frame_parts_v1(
        side_effect_hash,
        side_effect_header,
        side_effect_payload,
    )
    assert _side_effect_validator_calls == [5]
    side_effect_frame = datapod.WireFrame(
        side_effect_hash,
        side_effect_header,
        side_effect_payload,
    )
    _side_effect_validator_calls.clear()
    SideEffectValidatedPacket.view_from_wire(side_effect_hash, side_effect_wire)
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    SideEffectValidatedPacket.view_from_wire_v1(side_effect_hash, side_effect_wire)
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    SideEffectValidatedPacket.view_from_wire_frame(side_effect_frame)
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    SideEffectValidatedPacket.from_wire_frame(side_effect_frame)
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    SideEffectValidatedPacket.view_archive(side_effect_frame)
    assert _side_effect_validator_calls == [5]
    _side_effect_validator_calls.clear()
    SideEffectValidatedPacket.from_archive(side_effect_frame)
    assert _side_effect_validator_calls == [5]
    raising_wire = datapod.join_wire_message(
        RaisingConstructorPacket.TYPE_HASH,
        b"\x01\x00",
        b"payload",
    )
    try:
        RaisingConstructorPacket.from_wire_message(
            RaisingConstructorPacket.TYPE_HASH,
            raising_wire,
        )
        raise AssertionError("declarative Python decode must not swallow constructor TypeError")
    except TypeError:
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
    assert isinstance(auto_out.data, bytes)
    mutable_auto_wire = bytearray(auto_wire)
    auto_owned = AutoDataclassPacket.from_wire_message(auto_hash, mutable_auto_wire)
    assert isinstance(auto_owned.data, bytes)
    assert auto_owned.data == b"auto"
    mutable_auto_wire[-1] = ord("X")
    assert auto_owned.data == b"auto"
    auto_view = AutoDataclassPacket.view_from_wire(auto_hash, auto_wire)
    assert (auto_view.channel, auto_view.flags) == (42, 7)
    assert isinstance(auto_view.data, memoryview)
    assert auto_view.data.readonly
    assert auto_view.data.tobytes() == b"auto"
    mutable_auto_view_wire = bytearray(auto_wire)
    auto_mutable_view = AutoDataclassPacket.view_from_wire(
        auto_hash,
        mutable_auto_view_wire,
    )
    assert auto_mutable_view.data.readonly
    try:
        auto_mutable_view.data[0] = ord("X")
        raise AssertionError("borrowed Python wire views should be read-only")
    except TypeError:
        pass
    assert mutable_auto_view_wire == bytearray(auto_wire)
    auto_payload = bytearray(b"zero-copy")
    auto_frame = AutoDataclassPacket(42, 7, auto_payload).to_wire_frame()
    assert auto_frame.payload.obj is auto_payload
    auto_frame_view = AutoDataclassPacket.view_from_wire_frame(auto_frame)
    assert auto_frame_view.data.obj is auto_payload
    assert auto_frame_view.data.readonly
    try:
        auto_frame_view.data[0] = ord("X")
        raise AssertionError("borrowed Python frame views should be read-only")
    except TypeError:
        pass
    auto_payload[0] = ord("Z")
    assert auto_frame_view.data.tobytes() == b"Zero-copy"
    for method_name in (
        "from_wire_frame",
        "from_wire_frame_v1",
        "from_archive",
        "from_archive_v1",
        "view_from_wire_frame",
        "view_from_wire_frame_v1",
        "view_archive",
        "view_archive_v1",
    ):
        try:
            getattr(AutoDataclassPacket, method_name)(object())
            raise AssertionError(f"{method_name} should reject non-WireFrame objects")
        except ValueError:
            pass
    for method_name in (
        "from_wire_message",
        "from_wire_message_v1",
        "validate_wire_message",
        "validate_wire_message_v1",
        "view_from_wire",
        "view_from_wire_v1",
    ):
        for bad_hash in (-1, 2**64):
            try:
                getattr(AutoDataclassPacket, method_name)(bad_hash, auto_wire)
                raise AssertionError(f"{method_name} should reject type hashes outside u64")
            except ValueError:
                pass
    noncontiguous_payload = memoryview(bytearray(b"abcd"))[::2]
    noncontiguous_header = memoryview(bytearray(b"\x2a\x00\x07X"))[::2]
    try:
        AutoDataclassPacket(42, 7, noncontiguous_payload).to_wire_frame()
        raise AssertionError("Python archive payloads should reject non-contiguous buffers")
    except ValueError:
        pass
    try:
        datapod.WireFrame(AutoDataclassPacket.TYPE_HASH, noncontiguous_header, b"payload")
        raise AssertionError("WireFrame should reject non-contiguous header buffers")
    except ValueError:
        pass
    try:
        datapod.WireFrame(AutoDataclassPacket.TYPE_HASH, b"\x00\x00\x00", noncontiguous_payload)
        raise AssertionError("WireFrame should reject non-contiguous payload buffers")
    except ValueError:
        pass
    try:
        datapod.validate_wire_frame(AutoDataclassPacket.TYPE_HASH, b"\x2a\x00\x07", noncontiguous_payload)
        raise AssertionError("validate_wire_frame should reject non-contiguous payload buffers")
    except ValueError:
        pass
    try:
        datapod.validate_wire_frame(AutoDataclassPacket.TYPE_HASH, noncontiguous_header, b"payload")
        raise AssertionError("validate_wire_frame should reject non-contiguous header buffers")
    except ValueError:
        pass
    try:
        datapod.split_wire_message_view(
            AutoDataclassPacket.TYPE_HASH,
            memoryview(bytearray(auto_wire))[::2],
        )
        raise AssertionError("split_wire_message_view should reject non-contiguous buffers")
    except ValueError:
        pass
    noncontiguous_wire = memoryview(bytearray(auto_wire))[::2]
    for func, args in (
        (datapod.validate_wire_message, (AutoDataclassPacket.TYPE_HASH, noncontiguous_wire)),
        (datapod.validate_wire_message_v1, (AutoDataclassPacket.TYPE_HASH, noncontiguous_wire)),
        (datapod.split_wire_message, (AutoDataclassPacket.TYPE_HASH, noncontiguous_wire)),
        (datapod.split_wire_message_v1, (AutoDataclassPacket.TYPE_HASH, noncontiguous_wire)),
        (datapod.from_wire_message, (AutoDataclassPacket.TYPE_HASH, noncontiguous_wire)),
        (datapod.decode_as, (AutoDataclassPacket, AutoDataclassPacket.TYPE_HASH, noncontiguous_wire)),
    ):
        try:
            func(*args)
            raise AssertionError(f"{func.__name__} should reject non-contiguous wire buffers")
        except ValueError:
            pass
    try:
        datapod.join_wire_message(AutoDataclassPacket.TYPE_HASH, b"\x2a\x00\x07", noncontiguous_payload)
        raise AssertionError("join_wire_message should reject non-contiguous payload buffers")
    except ValueError:
        pass
    try:
        datapod.join_wire_message(AutoDataclassPacket.TYPE_HASH, noncontiguous_header, b"payload")
        raise AssertionError("join_wire_message should reject non-contiguous header buffers")
    except ValueError:
        pass
    assert not datapod.is_valid_wire_frame(
        AutoDataclassPacket.TYPE_HASH,
        object(),
        b"payload",
    )
    assert not datapod.is_valid_wire_frame(
        AutoDataclassPacket.TYPE_HASH,
        b"\x2a\x00\x07",
        object(),
    )
    assert not datapod.is_valid_wire_message(AutoDataclassPacket.TYPE_HASH, object())
    assert not datapod.is_valid_wire_message(-1, auto_wire)
    assert not datapod.is_valid_wire_message(2**64, auto_wire)
    assert not datapod.is_valid_wire_frame(-1, b"\x2a\x00\x07", b"payload")
    assert not datapod.is_valid_wire_frame(2**64, b"\x2a\x00\x07", b"payload")
    for bad_hash in (-1, 2**64):
        try:
            datapod.WireFrame(bad_hash, b"\x2a\x00\x07", b"payload")
            raise AssertionError("WireFrame should reject type hashes outside u64")
        except ValueError:
            pass
        try:
            datapod.view_wire_frame(bad_hash, auto_frame)
            raise AssertionError("view_wire_frame should reject type hashes outside u64")
        except ValueError:
            pass
        try:
            datapod.split_wire_message_view(bad_hash, auto_wire)
            raise AssertionError("split_wire_message_view should reject type hashes outside u64")
        except ValueError:
            pass
        try:
            datapod.split_wire_message_view_v1(bad_hash, auto_wire)
            raise AssertionError("split_wire_message_view_v1 should reject type hashes outside u64")
        except ValueError:
            pass
        try:
            datapod.validate_wire_frame_v1(bad_hash, b"\x2a\x00\x07", b"payload")
            raise AssertionError("validate_wire_frame_v1 should reject type hashes outside u64")
        except ValueError:
            pass
        assert not datapod.is_valid_wire_frame_v1(bad_hash, b"\x2a\x00\x07", b"payload")
        try:
            datapod.from_archive(bad_hash, auto_frame)
            raise AssertionError("from_archive should reject type hashes outside u64")
        except ValueError:
            pass

    for bad_hash in (True, 1.5):
        try:
            datapod.WireFrame(bad_hash, b"\x2a\x00\x07", b"payload")
            raise AssertionError("WireFrame should reject non-integer type hashes")
        except TypeError:
            pass
        try:
            datapod.view_wire_frame(bad_hash, auto_frame)
            raise AssertionError("view_wire_frame should reject non-integer type hashes")
        except TypeError:
            pass
        try:
            datapod.split_wire_message_view(bad_hash, auto_wire)
            raise AssertionError("split_wire_message_view should reject non-integer type hashes")
        except TypeError:
            pass
        try:
            datapod.validate_wire_frame_v1(bad_hash, b"\x2a\x00\x07", b"payload")
            raise AssertionError("validate_wire_frame_v1 should reject non-integer type hashes")
        except TypeError:
            pass
        assert not datapod.is_valid_wire_frame_v1(bad_hash, b"\x2a\x00\x07", b"payload")

    assert dataclasses.is_dataclass(InheritedDataclassPacket)
    assert InheritedDataclassPacket.__datapod_header_format__ == "<HB"
    assert InheritedDataclassPacket.__datapod_fields__ == ("channel", "flags")
    assert InheritedDataclassPacket.__datapod_payload_field__ == "data"
    inherited_schema = InheritedDataclassPacket.schema()
    assert [field["name"] for field in inherited_schema["fields"]] == ["channel", "flags"]
    inherited_packet = InheritedDataclassPacket(515, 9, b"inherited")
    inherited_hash, inherited_wire = datapod.to_wire_message(inherited_packet)
    assert inherited_hash == InheritedDataclassPacket.TYPE_HASH
    assert inherited_wire[:3] == (515).to_bytes(2, "little") + bytes([9])
    inherited_out = datapod.from_wire_message(inherited_hash, inherited_wire)
    assert (
        inherited_out.channel,
        inherited_out.flags,
        inherited_out.data,
    ) == (515, 9, b"inherited")
    inherited_view = InheritedDataclassPacket.view_from_wire(
        inherited_hash,
        bytearray(inherited_wire),
    )
    assert (inherited_view.channel, inherited_view.flags) == (515, 9)
    assert inherited_view.data.readonly
    assert inherited_view.data.tobytes() == b"inherited"

    assert dataclasses.is_dataclass(InheritedPayloadPacket)
    assert InheritedPayloadPacket.__datapod_header_format__ == "<B"
    assert InheritedPayloadPacket.__datapod_fields__ == ("flags",)
    assert InheritedPayloadPacket.__datapod_payload_field__ == "data"
    inherited_payload_schema = InheritedPayloadPacket.schema()
    assert [field["name"] for field in inherited_payload_schema["fields"]] == ["flags"]
    inherited_payload_packet = InheritedPayloadPacket(b"base-payload", 11)
    inherited_payload_hash, inherited_payload_wire = datapod.to_wire_message(
        inherited_payload_packet,
    )
    assert inherited_payload_hash == InheritedPayloadPacket.TYPE_HASH
    assert inherited_payload_wire[:1] == bytes([11])
    assert inherited_payload_wire[1:] == b"base-payload"
    inherited_payload_out = datapod.from_wire_message(
        inherited_payload_hash,
        inherited_payload_wire,
    )
    assert (inherited_payload_out.flags, inherited_payload_out.data) == (
        11,
        b"base-payload",
    )
    inherited_payload_view = InheritedPayloadPacket.view_from_wire(
        inherited_payload_hash,
        bytearray(inherited_payload_wire),
    )
    assert inherited_payload_view.flags == 11
    assert inherited_payload_view.data.readonly
    assert inherited_payload_view.data.tobytes() == b"base-payload"

    assert DecoratedHeaderBase.__datapod_header_format__ == "<H"
    assert DecoratedHeaderChild.__datapod_header_format__ == "<HB"
    assert DecoratedHeaderChild.__datapod_fields__ == ("channel", "flags")
    decorated_child = DecoratedHeaderChild(516, 10, b"child")
    decorated_child_hash, decorated_child_wire = datapod.to_wire_message(decorated_child)
    assert decorated_child_hash == DecoratedHeaderChild.TYPE_HASH
    assert decorated_child_hash != DecoratedHeaderBase.TYPE_HASH
    assert decorated_child_wire[:3] == (516).to_bytes(2, "little") + bytes([10])
    decorated_child_out = datapod.from_wire_message(
        decorated_child_hash,
        decorated_child_wire,
    )
    assert (
        decorated_child_out.channel,
        decorated_child_out.flags,
        decorated_child_out.data,
    ) == (516, 10, b"child")

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
    class ExtraArrayItems:
        def __init__(self):
            self.read_count = 0

        def __iter__(self):
            for value in (1, 2, 3, 4):
                self.read_count += 1
                yield value
            raise AssertionError("array encoder should stop after the first extra item")

    extra_items = ExtraArrayItems()
    try:
        datapod.to_wire_message(ArrayPacket(extra_items, 0.5, b"bad"))
        raise AssertionError("fixed-length Python array should reject extra items")
    except ValueError:
        pass
    assert extra_items.read_count == 4
    class HostileArrayItems:
        def __iter__(self):
            raise RuntimeError("hostile array iterator")

    try:
        datapod.to_wire_message(ArrayPacket(HostileArrayItems(), 0.5, b"bad"))
        raise AssertionError("fixed-length Python array should fail closed on hostile iterators")
    except ValueError:
        pass
    try:
        datapod.to_wire_message(ArrayPacket(123, 0.5, b"bad"))
        raise AssertionError("fixed-length Python array should reject non-iterable values")
    except ValueError:
        pass
    try:
        datapod.to_wire_message(ArrayPacket((1, 2, 999), 0.5, b"bad"))
        raise AssertionError("fixed-width Python array values should reject out-of-range integers")
    except ValueError:
        pass
    try:
        datapod.to_wire_message(AnnotatedDataclassPacket(17, 999, -12, b"bad"))
        raise AssertionError("fixed-width Python scalar values should reject out-of-range integers")
    except ValueError:
        pass
    try:
        AnnotatedDataclassPacket.__datapod_from_header_bytes__(b"\x01")
        raise AssertionError("declarative Python header decode should reject truncated headers")
    except ValueError:
        pass
    missing_header = ArrayPacket.__new__(ArrayPacket)
    missing_header.gain = 0.5
    missing_header.data = b"bad"
    try:
        datapod.to_wire_message(missing_header)
        raise AssertionError("declarative Python encode should reject missing header fields")
    except ValueError:
        pass
    missing_payload = ArrayPacket.__new__(ArrayPacket)
    missing_payload.rgb = (1, 2, 3)
    missing_payload.gain = 0.5
    try:
        datapod.to_wire_message(missing_payload)
        raise AssertionError("declarative Python encode should reject missing payload fields")
    except ValueError:
        pass
    bad_payload = ArrayPacket.__new__(ArrayPacket)
    bad_payload.rgb = (1, 2, 3)
    bad_payload.gain = 0.5
    bad_payload.data = object()
    try:
        datapod.to_wire_message(bad_payload)
        raise AssertionError("declarative Python encode should reject non-buffer payload fields")
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
    assert nested_schema["has_archive"] is True
    assert nested_schema["has_view"] is True
    assert nested_schema["has_owned_decode"] is False
    assert nested_schema["archive_shape"] == "runtime-schema"
    assert nested_schema["payload_field"] == "data"
    assert nested_schema["header_format"] == "<IIB"
    assert nested_schema["fields"][0]["annotation"]["kind"] == "datapod"
    assert nested_schema["fields"][0]["annotation"]["canonical_name"] == "acme.python_stamp.v1"
    assert datapod.describe_schema(NestedPacket)["fields"][1]["annotation"]["name"] == "u8"
    assert datapod.schema(NestedPacket.TYPE_HASH)["canonical_name"] == "acme.nested_packet.v1"

    class BadNestedAnnotationTypeHash:
        TYPE_HASH = True
        __datapod_canonical_name__ = "acme.bad_nested_annotation_type_hash.v1"
        __datapod_header_format__ = "<H"
        __datapod_fields__ = ("value",)
        __datapod_header_arity__ = 1
        __datapod_payload_field__ = None

    @datapod.datapod_type(
        "acme.bad_nested_annotation_schema.v1",
        payload=True,
        dataclass=True,
    )
    class BadNestedAnnotationSchema:
        nested: BadNestedAnnotationTypeHash
        data: bytes

    try:
        BadNestedAnnotationSchema.schema()
        raise AssertionError("schema should reject nested datapod annotations with invalid TYPE_HASH")
    except TypeError:
        pass

    class HostileAnnotationMetadataDescriptor:
        def __get__(self, obj, cls=None):
            raise RuntimeError("hostile annotation metadata lookup")

    class HostileNestedAnnotationHeader:
        TYPE_HASH = datapod.Point.TYPE_HASH
        __datapod_canonical_name__ = "acme.hostile_nested_annotation_header.v1"
        __datapod_header_format__ = HostileAnnotationMetadataDescriptor()
        __datapod_fields__ = ("value",)
        __datapod_payload_field__ = None

    try:
        @datapod.datapod_type(
            "acme.hostile_nested_annotation_header_schema.v1",
            payload=True,
            dataclass=True,
        )
        class HostileNestedAnnotationHeaderSchema:
            nested: HostileNestedAnnotationHeader
            data: bytes

        raise AssertionError("datapod_type should fail closed on hostile nested annotation metadata")
    except ValueError:
        pass

    class HostileNestedAnnotationTypeHash:
        TYPE_HASH = HostileAnnotationMetadataDescriptor()
        __datapod_canonical_name__ = "acme.hostile_nested_annotation_type_hash.v1"
        __datapod_header_format__ = "<H"
        __datapod_fields__ = ("value",)
        __datapod_payload_field__ = None

    @datapod.datapod_type(
        "acme.hostile_nested_annotation_schema.v1",
        payload=True,
        dataclass=True,
    )
    class HostileNestedAnnotationSchema:
        nested: HostileNestedAnnotationTypeHash
        data: bytes

    try:
        HostileNestedAnnotationSchema.schema()
        raise AssertionError("schema should fail closed on hostile nested annotation TYPE_HASH")
    except ValueError:
        pass

    try:
        datapod.describe_schema(object)
        raise AssertionError("describe_schema should reject unsupported classes")
    except ValueError:
        pass
    try:
        datapod.describe_schema(HostileIndexLookup())
        raise AssertionError("describe_schema should fail closed on hostile __index__ lookup")
    except ValueError:
        pass
    try:
        datapod.describe_schema(HostileClassLookup())
        raise AssertionError("describe_schema should fail closed when object __class__ is hostile")
    except ValueError:
        pass
    class HostileMetadataDescriptor:
        def __get__(self, obj, cls=None):
            raise RuntimeError("hostile schema metadata lookup")

    class HostileMetadataLabel:
        def __str__(self):
            raise RuntimeError("hostile metadata label stringification")

    class HostileMetadataNameMeta(type):
        def __getattribute__(cls, name):
            if name == "__name__":
                return HostileMetadataLabel()
            return super().__getattribute__(name)

    class HostileTypeHashSchema:
        TYPE_HASH = HostileMetadataDescriptor()

    try:
        datapod.describe_schema(HostileTypeHashSchema)
        raise AssertionError("describe_schema should fail closed on hostile TYPE_HASH metadata")
    except ValueError:
        pass

    class HostileTypeHashAndNameSchema(metaclass=HostileMetadataNameMeta):
        TYPE_HASH = HostileMetadataDescriptor()

    try:
        datapod.describe_schema(HostileTypeHashAndNameSchema)
        raise AssertionError("describe_schema should fail closed on hostile metadata labels")
    except ValueError:
        pass

    class BadSchemaTypeHash:
        TYPE_HASH = -1

    try:
        datapod.describe_schema(BadSchemaTypeHash)
        raise AssertionError("describe_schema should reject class TYPE_HASH outside u64")
    except ValueError:
        pass
    class BadSchemaCanonicalHash:
        TYPE_HASH = datapod.Point.TYPE_HASH
        CANONICAL_TYPE_HASH = 2**64

    try:
        datapod.describe_schema(BadSchemaCanonicalHash)
        raise AssertionError("describe_schema should reject class CANONICAL_TYPE_HASH outside u64")
    except ValueError:
        pass
    class HostileCanonicalHashSchema:
        TYPE_HASH = datapod.Point.TYPE_HASH
        CANONICAL_TYPE_HASH = HostileMetadataDescriptor()

    try:
        datapod.describe_schema(HostileCanonicalHashSchema)
        raise AssertionError("describe_schema should fail closed on hostile CANONICAL_TYPE_HASH metadata")
    except ValueError:
        pass
    class HostileSchemaFields:
        TYPE_HASH = datapod.Point.TYPE_HASH
        __datapod_fields__ = HostileMetadataDescriptor()

    try:
        datapod.describe_schema(HostileSchemaFields)
        raise AssertionError("describe_schema should fail closed on hostile schema field metadata")
    except ValueError:
        pass
    class HostileSchemaHeaderFormat:
        TYPE_HASH = datapod.Point.TYPE_HASH
        __datapod_header_format__ = HostileMetadataDescriptor()

    try:
        datapod.describe_schema(HostileSchemaHeaderFormat)
        raise AssertionError("describe_schema should fail closed on hostile schema header metadata")
    except ValueError:
        pass
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
    try:
        datapod.register_schema(
            "acme.raw_huge_header.v1",
            1 << (ctypes.sizeof(ctypes.c_size_t) * 8 - 1),
            "bytes",
        )
        raise AssertionError("register_schema should reject impossible header sizes")
    except ValueError:
        pass

    raw_fixed_hash = datapod.register_schema("acme.raw_fixed_no_payload.v1", 2, "fixed")
    assert datapod.is_valid_wire_message(raw_fixed_hash, b"\x01\x02")
    assert not datapod.is_valid_wire_message(raw_fixed_hash, b"\x01\x02x")
    assert datapod.is_valid_wire_frame(raw_fixed_hash, b"\x01\x02", b"")
    assert not datapod.is_valid_wire_frame(raw_fixed_hash, b"\x01\x02", b"x")
    declarative_globals = datapod.datapod_type.__globals__
    previous_shape_helper = declarative_globals["_validate_wire_frame_shape_v1"]
    try:
        declarative_globals["_validate_wire_frame_shape_v1"] = lambda *args, **kwargs: None
        try:
            datapod.split_wire_message_view(raw_fixed_hash, b"\x01\x02x")
            raise AssertionError(
                "split_wire_message_view should not trust monkey-patched shape validation"
            )
        except ValueError:
            pass
        try:
            datapod.split_wire_message_view_v1(raw_fixed_hash, b"\x01\x02x")
            raise AssertionError(
                "split_wire_message_view_v1 should not trust monkey-patched shape validation"
            )
        except ValueError:
            pass
    finally:
        declarative_globals["_validate_wire_frame_shape_v1"] = previous_shape_helper
    try:
        datapod.join_wire_message(raw_fixed_hash, b"\x01\x02", b"x")
        raise AssertionError("join_wire_message should reject fixed-schema payload bytes")
    except ValueError:
        pass
    matrix_archive_for_join = datapod.Matrix(2, 2, 1, bytes([1, 2, 3, 4])).archive()
    assert datapod.is_valid_wire_frame(
        datapod.Matrix.TYPE_HASH,
        matrix_archive_for_join.header,
        matrix_archive_for_join.payload,
    )
    assert not datapod.is_valid_wire_frame(
        datapod.Matrix.TYPE_HASH,
        matrix_archive_for_join.header,
        b"\x01\x02\x03",
    )
    try:
        datapod.validate_wire_frame(
            datapod.Matrix.TYPE_HASH,
            matrix_archive_for_join.header,
            b"\x01\x02\x03",
        )
        raise AssertionError("validate_wire_frame should reject invalid Matrix payload length")
    except ValueError:
        pass
    try:
        datapod.join_wire_message(
            datapod.Matrix.TYPE_HASH,
            matrix_archive_for_join.header.tobytes(),
            b"\x01\x02\x03",
        )
        raise AssertionError("join_wire_message should reject invalid Matrix payload length")
    except ValueError:
        pass

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

    seen = set()
    for type_hash, cls in datapod.__datapod_registry__.items():
        if cls in seen:
            continue
        seen.add(cls)
        for method_name in (
            "archive",
            "archive_v1",
            "from_wire_frame",
            "from_wire_frame_v1",
            "from_archive",
            "from_archive_v1",
            "view_from_wire_frame",
            "view_from_wire_frame_v1",
            "view_archive",
            "view_archive_v1",
        ):
            method = getattr(cls, method_name, None)
            assert callable(method), (type_hash, cls, method_name)

    archive_samples = [
        datapod.Geo(1.0, 2.0, 3.0),
        datapod.Segment(datapod.Point(0.0, 0.0, 0.0), datapod.Point(1.0, 0.0, 0.0)),
        datapod.Polygon([(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0)]),
        datapod.Bytes([1, 2, 3]),
        datapod.DpStr("hi"),
        datapod.DpString([104, 105]),
        datapod.Vector(1, [1, 2, 3]),
        datapod.Tensor(1, 1, 3, 1, [1, 2, 3]),
        datapod.BitVec(8, [255]),
        datapod.Deque(1, 0, [1, 2]),
        datapod.Queue(1, 0, [1, 2]),
        datapod.Stack(1, [1, 2]),
        datapod.Map(),
        datapod.Set(),
    ]
    try:
        datapod.DpString([0xff, 0xfe])
        raise AssertionError("DpString should reject invalid UTF-8")
    except ValueError:
        pass
    py_map = datapod.Map()
    assert py_map.insert(b"k", b"v") is None
    assert py_map.insert(b"k", b"w") == b"v"
    assert py_map.len() == 1
    py_set = datapod.Set()
    assert py_set.insert(b"s") is True
    assert py_set.insert(b"s") is False
    assert py_set.len() == 1
    for sample in archive_samples:
        archive = sample.archive()
        assert archive.type_hash == type(sample).TYPE_HASH
        assert type(sample).view_archive(archive) is not None
        assert type(sample).view_archive_v1(archive) is not None
        assert type(sample).view_from_wire_frame(archive) is not None
        assert type(sample).view_from_wire_frame_v1(archive) is not None
        assert type(sample).from_archive(archive) is not None
        assert type(sample).from_archive_v1(archive) is not None
        assert type(sample).from_wire_frame(archive) is not None
        assert type(sample).from_wire_frame_v1(archive) is not None

    temp_bytes_view = datapod.Bytes.view_archive(datapod.Bytes([4, 5, 6]).archive())
    temp_matrix_view = datapod.Matrix(1, 3, 1, bytes([7, 8, 9])).archive()
    temp_matrix_view = datapod.Matrix.view_archive(temp_matrix_view)
    temp_grid_view = datapod.Grid(
        1, 2, 11, False, 1.0, [0, 0, 0, 1, 0, 0, 0], bytes([10, 11, 12, 13, 14, 15, 16, 17])
    ).archive()
    temp_grid_view = datapod.Grid.view_archive(temp_grid_view)
    temp_bytes_archive = datapod.Bytes([21, 22, 23]).archive()
    temp_matrix_archive = datapod.Matrix(1, 3, 1, bytes([24, 25, 26])).archive()
    temp_grid_archive = datapod.Grid(
        1, 2, 11, False, 1.0, [0, 0, 0, 1, 0, 0, 0], bytes([27, 28, 29, 30, 31, 32, 33, 34])
    ).archive()
    gc.collect()
    assert temp_bytes_view.payload.tobytes() == bytes([4, 5, 6])
    assert temp_matrix_view.payload.tobytes() == bytes([7, 8, 9])
    assert temp_grid_view.payload.tobytes() == bytes([10, 11, 12, 13, 14, 15, 16, 17])
    assert temp_bytes_archive.payload.tobytes() == bytes([21, 22, 23])
    assert temp_matrix_archive.payload.tobytes() == bytes([24, 25, 26])
    assert temp_grid_archive.payload.tobytes() == bytes([27, 28, 29, 30, 31, 32, 33, 34])
    assert temp_bytes_archive._owner is not None
    assert temp_matrix_archive._owner is not None
    assert temp_grid_archive._owner is not None

    for sample in archive_samples:
        archive = sample.archive()
        bad_archive = datapod.ArchiveFrame(
            archive.type_hash ^ 1,
            archive.header,
            archive.payload,
            sample,
        )

        class FakeFrame:
            def __init__(self, type_hash, header, payload):
                self.type_hash = type_hash
                self.header = header
                self.payload = payload

        bad_header_frame = FakeFrame(archive.type_hash, object(), archive.payload)
        bad_payload_frame = FakeFrame(archive.type_hash, archive.header, object())
        for method_name in (
            "view_archive",
            "view_archive_v1",
            "view_from_wire_frame",
            "view_from_wire_frame_v1",
            "from_archive",
            "from_archive_v1",
            "from_wire_frame",
            "from_wire_frame_v1",
        ):
            try:
                getattr(type(sample), method_name)(bad_archive)
                raise AssertionError(f"{method_name} should reject a wrong type hash")
            except ValueError:
                pass
            try:
                getattr(type(sample), method_name)(object())
                raise AssertionError(f"{method_name} should reject non-WireFrame inputs")
            except ValueError:
                pass
            try:
                getattr(type(sample), method_name)(bad_header_frame)
                raise AssertionError(f"{method_name} should reject frame-like objects with bad headers")
            except ValueError:
                pass
            except TypeError as exc:
                raise AssertionError(
                    f"{type(sample).__name__}.{method_name} leaked TypeError for bad headers"
                ) from exc
            try:
                getattr(type(sample), method_name)(bad_payload_frame)
                raise AssertionError(f"{method_name} should reject frame-like objects with bad payloads")
            except ValueError:
                pass
            except TypeError as exc:
                raise AssertionError(
                    f"{type(sample).__name__}.{method_name} leaked TypeError for bad payloads"
                ) from exc

    seen = set()
    for type_hash, cls in datapod.__datapod_registry__.items():
        if cls in seen:
            continue
        seen.add(cls)
        if datapod.payload_kind(cls.TYPE_HASH) != "fixed":
            assert hasattr(cls, "to_wire_frame"), (type_hash, cls)


if __name__ == "__main__":
    main()
