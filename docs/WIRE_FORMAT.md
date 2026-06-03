# datapod wire format policy

This document defines the datapod wire message contract across Rust, C ABI, and
Python bindings.

The current owned wire body is intentionally simple:

```text
type_hash + (header_bytes || payload_bytes)
```

The `type_hash` identifies the schema. The byte body is the concatenation of a
fixed-size POD header and an optional payload byte slice.

That joined byte body is the **owned convenience lane**. The performance lane is
the borrowed frame:

```text
WireFrame {
  type_hash,
  header: borrowed header bytes,
  payload: borrowed payload bytes,
}
```

`WireFrame` is the primary zero-copy shape for publishers, subscribers, C ABI
views, and Python `memoryview` users. `WireMessage { bytes: Vec<u8> }` is for
files, tests, simple callers, or transports that require one contiguous buffer.

## Terms

- **Canonical hash**: the stable hash of a registered canonical name,
  produced by `datapod::bind::type_hash::<T>()` for built-ins or
  `datapod::bind::type_hash_name("acme.type.v1")` for runtime schemas.
- **Wire body**: the bytes after the hash: `header_bytes || payload_bytes`.
- **Wire frame**: borrowed `type_hash + header + payload` slices; no joined
  allocation and no payload copy.
- **Wire message**: owned contiguous `header || payload` bytes; convenient but
  copying.
- **Header**: `T::Header`, a fixed-size POD struct.
- **Payload**: zero or more bytes, usually from a `#[dp(bytes)]` field.
- **Checked access**: validation followed by a borrowed view.
- **Owned decode**: allocation/reconstruction into the owning Rust/Python/C
  handle type.

## Current mode: `datapod-wire-v1/le`

`datapod-wire-v1/le` is the current emitted format.

Properties:

- Built-in Rust types emit canonical-name hashes.
- Runtime C/Python schemas emit canonical-name hashes.
- Built-in registry metadata includes both:
  - `type_hash`: emitted current hash
  - `canonical_type_hash`: alias of `type_hash` for introspection
- Headers are encoded field-by-field in little-endian order.
- Native struct padding is not copied into the current wire body.
- Incoming bytes may be unaligned.
- Safe Rust access decodes little-endian headers before borrowed payload access.
- C/Python borrowed payload views borrow caller-owned input buffers and do not
  store pointers globally.
- Payload bytes are raw bytes; semantic validators decide whether a payload is
  well-formed for a type.

Identity rule:

```text
emit and accept canonical-name hashes
```

Rust borrowed-frame helpers:

```rust
datapod::with_wire_frame(&value, |frame| {
    let view = datapod::access_wire_frame::<MyType>(frame)?;
    transport.publish_frame(frame)
})?;
```

`with_wire_frame_slices(frame, ...)` exposes scatter/gather slices in this
order:

```text
[type_hash_le, header, payload]
```

For multi-`Vec`/sectioned datapods,
`with_segmented_wire_frame(...)` and `with_wire_segmented_frame_slices(...)`
preserve the individual payload segments:

```text
prefix = [type_hash_le, header]
payloads = [payload_0, payload_1, ...]
```

Owned current-format helpers:

```rust
let msg = datapod::to_wire_message(&value);
let view = datapod::access_wire_bytes::<MyType>(msg.type_hash, &msg.bytes)?;
let owned = datapod::from_wire_message::<MyType>(&msg)?;
```

`to_wire_message` and `from_wire_message` are explicitly copying APIs. Prefer
`with_wire_frame` plus `access_wire_frame` when a borrowed view is sufficient.

`to_wire_message_v1` / `from_wire_message_v1` remain available as strict v1
aliases for callers that want the format in the function name.

C and Python defaults now also target v1/le. The `_v1` helpers remain as
explicit strict aliases:

- C: `datapod_header_size_v1`, `datapod_wire_message_join_v1`,
  `datapod_wire_message_validate_v1`, `datapod_wire_message_header_v1`, and
  `datapod_wire_message_payload_v1`.
- Python: `header_size_v1`, `validate_wire_message_v1`,
  `is_valid_wire_message_v1`, `split_wire_message_v1`,
  `split_wire_message_view_v1`, and decorator-generated
  `to_wire_message_v1` / `from_wire_message_v1` / `view_from_wire_v1` for
  custom declarative schemas.

## Hash policy

### Built-ins

For built-in Rust types:

```rust
let canonical = datapod::bind::type_hash::<T>();
```

Current behavior:

- `to_wire_message(&value)` emits `canonical` v1/le bytes.
- `from_wire_message::<T>(&message)` accepts canonical v1/le messages.
- `validate_wire::<T>(&message)` accepts canonical v1/le messages.
- `access_wire::<T>(&message)` accepts canonical v1/le messages.
- the registry resolves the canonical hash to `TypeInfo`.

### Runtime C/Python schemas

Runtime schemas are already canonical-name based:

```c
uint64_t h = datapod_type_hash_name("acme.packet.v1");
datapod_register_type(h, "acme.packet.v1", header_size, payload_kind);
```

For these schemas, `type_hash == canonical_type_hash`.

Python custom schemas can either pass an explicit `struct` format:

```python
@datapod.datapod_type("acme.packet.v1", "<HH", ("channel", "flags"), payload="data")
class Packet:
    ...
```

or use fixed-width annotation markers and let the decorator infer the header
layout:

```python
@datapod.datapod_type("acme.packet.v1", payload=True, dataclass=True)
class Packet:
    channel: datapod.u16
    flags: datapod.u8
    data: bytes
```

The marker-based form generates a fixed-width `struct` format using the
decorator's `byte_order` argument, which defaults to little-endian (`"<"`).
Repeated scalar header fields can be grouped with `datapod.array(marker, n)`:

```python
@datapod.datapod_type("acme.pixel.v1", payload=True, dataclass=True)
class Pixel:
    rgb: datapod.array(datapod.u8, 3)
    data: bytes
```

`Pixel(rgb=(1, 2, 3), data=b"...")` packs the `rgb` tuple as `"<3B"` and
owned/view decoders reconstruct it as a tuple. The decorator also accepts
stringized marker annotations such as `"datapod.array(datapod.u8, 3)"`.
Fixed declarative datapod classes can be nested as header fields:

```python
@datapod.datapod_type("acme.stamp.v1", dataclass=True)
class Stamp:
    sec: datapod.u32
    nsec: datapod.u32

@datapod.datapod_type("acme.stamped_packet.v1", payload=True, dataclass=True)
class StampedPacket:
    stamp: Stamp
    flags: datapod.u8
    data: bytes
```

`StampedPacket` flattens the fixed `Stamp` header into the parent header and
owned/view decoders reconstruct `stamp` as a nested `Stamp` instance.
Declarative schemas expose introspection for generated docs and binding checks:

```python
StampedPacket.schema()
datapod.describe_schema(StampedPacket)
datapod.schema(StampedPacket.TYPE_HASH)
```

The returned dictionary includes canonical name, hashes, header format/size,
payload kind/field, wire format, emitted hash kind, and per-field annotation
metadata.

Rust custom schemas use the same canonical-name model through the datapod
macro:

```rust
#[datapod::datapod(name = "robolibs.camera_frame.v1")]
pub struct CameraFrame {
    pub width: u32,
    pub height: u32,
    pub encoding: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}
```

The generated type exposes:

```rust
CameraFrame::CANONICAL_NAME
CameraFrame::TYPE_HASH
CameraFrame::register_schema()
camera.to_wire_message()
camera.with_wire_frame(...)
CameraFrame::view_from_wire_frame(...)
CameraFrame::from_wire_frame(...)
```

Unnamed Rust custom datapods still fall back to their Rust type-path hash. Use
`name = "namespace.type.vN"` for language-neutral Rust/C/Python ABI parity.

## Validation policy

Validation is the trust boundary for borrowed access.

Generic registry validation can only prove:

1. the type hash is known,
2. the byte body contains at least the registered header length.

Built-in semantic validation can additionally prove type invariants, such as:

- fixed types have no payload,
- `DpStr` payload is UTF-8,
- `Matrix`/`Tensor` dimensions match payload length,
- `Grid`/`Layer` dimensions and encoding match payload length,
- `Map`/`Set` offsets are in-bounds and sorted,
- `BitVec` trailing slack bits are zero,
- `Vecvec` offset tables are well-formed.

Checked borrowed access must always validate before exposing a view.

Unchecked access is only for trusted bytes and must document that callers are
responsible for all invariants.

## Multi-section payload policy

Future multi-payload datapods use a single payload blob with header sections:

```rust
#[repr(C)]
pub struct PayloadSection {
    pub offset: u32,
    pub len: u32,
}
```

Offsets are relative to the start of the payload blob. Writers should use
`PayloadLayoutBuilder` to append sections forward and copy the returned
`PayloadSection` values into the header.

Macro integration supports both explicit single-blob sections and direct
multi-`Vec` section fields. The single-blob form is:

```rust
#[datapod]
pub struct ImageWithMeta {
    pub width: u32,
    pub height: u32,
    pub pixels: PayloadSection,
    pub metadata: PayloadSection,
    #[dp(bytes)]
    pub payload_blob: Vec<u8>,
}
```

The generated validator recognizes `PayloadSection` header fields and validates
them against `payload_blob`.

For the higher-level direct form:

```rust
#[datapod]
pub struct ImageWithMeta {
    pub width: u32,
    pub height: u32,
    #[dp(bytes, section = "pixels")]
    pub pixels: Vec<u8>,
    #[dp(bytes, section = "metadata")]
    pub metadata: Vec<u8>,
}
```

the macro generates header fields named `pixels` and `metadata` of type
`PayloadSection`, writes the payload sections in field order, validates bounds
and per-section element width, and decodes the owned `Vec` fields from their
borrowed sections.

Validators for multi-section payloads must check:

- `offset + len` does not overflow,
- each section is inside the payload blob,
- sections are sorted when the schema requires sorted sections,
- sections do not overlap when the schema requires disjoint sections,
- any per-section element width or encoding invariant.

This keeps the wire C/Python friendly and avoids pointer-relative archived
layouts.

## Borrowed view lifetime policy

Rust views are lifetime-bound to the original byte slice.

C views contain borrowed pointers into caller-owned wire bytes:

```c
DatapodMatrixView view;
datapod_matrix_view_from_wire(bytes, len, &view);
```

For the zero-copy fast path, C callers can split or provide a borrowed frame and
then view it without joining or copying payload bytes:

```c
DatapodWireFrame frame = {
    .type_hash = matrix_hash,
    .header = header_ptr,
    .header_len = header_len,
    .payload = payload_ptr,
    .payload_len = payload_len,
};

DatapodMatrixView view;
datapod_matrix_view_from_frame(frame, &view);
```

The C ABI exposes typed borrowed views for the core raw payload families:
`Bytes`, `Matrix`, `Tensor`, `Vector`, `BitVec`, `Vecvec`, `Map`, `Set`, and
`Grid`. It also exposes borrowed views for UTF-8 strings, raster layers, and
the geometry collection families (`Polygon`, `Linestring`, `MultiPoint`,
`Ring`, `Path`, and `Trajectory`). These geometry views report element counts
and element byte sizes while still exposing payload memory as `DatapodBytes`,
because wire payload pointers are not guaranteed to satisfy C typed-pointer
alignment. The lower-level sequence families (`Stack`, `Queue`, `Deque`,
`Heap`, `IndexedHeap`, `List`, `ForwardList`, and `PagedVecvec`) expose the
same kind of validated borrowed byte views plus header-derived counts, offsets,
orders, or node sizes.

Rules:

- keep `bytes` alive while using `view`,
- do not free or mutate the buffer while the view is in use,
- datapod does not retain the pointer after the call,
- typed element APIs must be explicit about unaligned reads vs alignment checks.

Python `WireFrame`, `wire_frame(obj)`, `view_wire_frame(type_or_hash, frame)`,
Rust-backed `Matrix.view_from_wire_frame` / `Grid.view_from_wire_frame`, and
decorator-generated `view_from_wire_frame` expose payloads as `memoryview`
objects, preserving the input buffer lifetime through Python's buffer protocol.
`from_wire_frame` is the explicit owned/copying Python decode path.

## Benchmark policy

`make bench` runs Rust, C, and Python owned-vs-borrowed benchmark scaffolds for
64 B, 4 KiB, 1 MiB, and 64 MiB matrix payloads. The output includes
`copied_bytes_per_op`; borrowed frame/memoryview paths must report `0`, while
owned encode/decode paths report the header/payload bytes they copy.

## Registry metadata

The registry exposes current wire metadata:

- `type_hash`
- `canonical_type_hash`
- `canonical_name`
- `header_size`
- `payload_kind`
- `format_version`
- `wire_format`
- `endian`
- `alignment`
- `validator`
- `emitted_hash`
- `emitted_hash_kind`

Current built-ins report:

```text
format_version = 1
wire_format = DatapodWireV1Little
endian = Little
alignment = UnalignedWire
validator = BuiltIn
emitted_hash = type_hash
emitted_hash_kind = CanonicalName
```

Runtime C/Python schemas report:

```text
format_version = 1
wire_format = DatapodWireV1Little
endian = Little
alignment = UnalignedWire
validator = RuntimeSchema
emitted_hash = type_hash
emitted_hash_kind = CanonicalName
```

Rust exposes the current release-boundary policy through
`current_wire_format_name()` and `builtin_hash_policy()`. C and Python expose
matching helpers named
`datapod_current_wire_format_name` / `datapod.current_wire_format`,
`datapod_builtin_hash_policy` / `datapod.builtin_hash_policy`.

## Snapshot requirements

Tests must keep snapshots for:

- canonical built-in hashes,
- canonical names,
- header sizes,
- key C/Python agreement points.

Any intentional break must update the snapshots and the migration notes in this
document in the same change.
