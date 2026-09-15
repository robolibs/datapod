# datapod

`datapod` is the robolibs wire-contract crate: a small Rust data model plus
C ABI and Python bindings for robotics messages that need stable type identity,
plain-old-data headers, and zero-copy payload views.

It is not a transport. It is the shared schema, validation, and byte-layout
layer underneath transports such as shared memory or network pub/sub. A
transport can carry:

```text
type_hash + header bytes + payload bytes
```

and `datapod` provides the Rust, C, and Python APIs to validate, inspect, view,
and explicitly decode that data.

## What this crate contains

- Rust `DataPod` implementations for geometry, motion, raster, robot, sequence,
  associative, identifier, and geodetic types.
- `#[datapod(...)]` and `#[derive(DataPod)]` macros for application-specific
  Rust schemas.
- A built-in canonical-name registry with stable `type_hash` metadata.
- A `datapod-wire-v1/le` wire format using field-by-field little-endian header
  encoding.
- Archive/View/Owned APIs that separate the zero-copy fast path from the
  copying convenience path.
- A C ABI generated into `include/datapod.h`, including `DatapodArchiveFrame`,
  dynamic schema lookup, and typed owned-handle helpers.
- Python bindings built with `maturin` as package `robolibs-datapod` and module
  `datapod`, including declarative schemas, `obj.archive()`, `ArchiveFrame`,
  `PayloadView`, `memoryview` payload access, and dynamic built-in schema views.

## Repository layout

```text
.
├── Cargo.toml              # Rust workspace: datapod + datapod-macros
├── .make.lua               # canonical local build/test/binding commands
├── pyproject.toml          # maturin package metadata for robolibs-datapod
├── cbindgen.toml           # C header generation config
├── include/datapod.h       # checked-in generated C ABI header
├── crates/datapod-macros/  # proc macros: #[datapod] and derive support
├── src/
│   ├── wire.rs             # DataPod trait, wire format, Archive/View/Owned API
│   ├── registry.rs         # canonical type registry and runtime schema registry
│   ├── schema.rs           # built-in schema descriptors and schema hashing
│   ├── dynamic.rs          # zero-copy dynamic Rust field views
│   ├── ffi.rs              # C ABI implementation
│   ├── python/             # pyo3 bindings and Python declarative schemas
│   ├── geom/               # Point, Segment, Polygon, Ring, Path, shapes, ...
│   ├── motion/             # Euler, Quaternion, Pose, Transform, Velocity, ...
│   ├── world/              # Geo, Loc, Utm
│   ├── raster/             # Grid and Layer payload-backed rasters
│   ├── seq/                # Bytes, DpStr, Vector, Matrix, Tensor, queues, ...
│   ├── assoc/              # sorted byte-layout Map and Set
│   ├── id/                 # Uuid, Ip, MacAddr, DpString
│   └── robot/              # URDF-style robot/link/joint/sensor pods
├── examples/               # wire fixture and benchmark scaffolding
└── tests/                  # Rust, C ABI, Python, schema, and docs-surface tests
```

## Core model

Every transportable value implements `DataPod`:

```rust
pub trait DataPod {
    type Header: bytemuck::Pod + Copy;
    type Payload: ?Sized;

    fn header(&self) -> Self::Header;
    fn payload_bytes(&self) -> &[u8];
}
```

There are two common shapes.

### Fixed-POD values

Fixed values are their own wire header and have no payload. Examples include
`Point`, `Segment`, `Pose`, `Quaternion`, `Geo`, `Twist`, and many fixed robot
metadata structs.

```rust
use datapod::{Point, to_wire_message};

let point = Point::new(1.0, 2.0, 3.0);
let msg = to_wire_message(&point);

assert_eq!(msg.type_hash, datapod::bind::type_hash::<Point>());
assert!(!msg.bytes.is_empty());
```

### Payload-backed values

Heap-bearing values own their data in Rust but expose a compact fixed header
plus borrowed payload bytes on the wire. Examples include `Grid`, `Layer`,
`Matrix`, `Bytes`, `DpStr`, `Vector`, `Map`, `Set`, `Polygon`, `Ring`, and
`Trajectory`.

```rust
use datapod::{Encoding, Grid, Pose};

let grid = Grid::new(
    2,
    2,
    Encoding::Rgba8,
    0.5,
    false,
    Pose::default(),
    (0_u8..16).collect(),
);

assert_eq!(grid.data.len(), 16);
```

## Zero-copy model

The high-performance path is Archive/View/Owned:

| Step | API shape | Payload copy? | Use when |
| --- | --- | --- | --- |
| Archive | `ArchiveFrame` / `DatapodArchiveFrame` / Python `ArchiveFrame` | no | publishing, SHM slots, vectored writes |
| View | `T::View<'a>`, `PayloadView`, dynamic views | no | inspecting or consuming borrowed data |
| Owned | `T`, C owned handles, Python objects | yes when decoding from wire | you need ownership |
| Message | `OwnedWireMessage` / `WireMessage { bytes: Vec<u8> }` | yes | files, tests, simple transports |

`ArchiveFrame` is just:

```text
type_hash
borrowed header bytes
borrowed payload bytes
```

`OwnedWireMessage`/`WireMessage` is the convenience representation where
`bytes = header || payload`. It is intentionally not the performance path.

Rust:

```rust
use datapod::{Matrix, archive, view_archive, from_archive};

let matrix = Matrix::from_bytes::<u16>(
    2,
    3,
    bytemuck::cast_slice(&[1_u16, 2, 3, 4, 5, 6]).to_vec(),
);

archive(&matrix, |archive| {
    // View borrows the archive payload.
    let view = view_archive::<Matrix>(archive).expect("valid matrix view");
    assert_eq!(view.rows(), 2);

    // Owned decode is explicit and may copy.
    let owned = from_archive::<Matrix>(archive).expect("owned matrix decode");
    assert_eq!(owned.rows, 2);
}).expect("valid matrix archive");
```

C ABI:

```c
DatapodArchiveFrame archive = {0};
/* fill or split archive */

if (!datapod_archive_validate(archive)) {
    const char *error = datapod_last_error_message();
}

DatapodDynamicView view = {0};
DatapodWireMessage message = {type_hash, data, len}; /* data = header || payload */
if (datapod_dynamic_view_message(message, &view)) {
    DatapodBytes payload = datapod_dynamic_payload(view);
}
```

Python:

```python
import datapod

point = datapod.Point(1.0, 2.0, 3.0)
archive = point.archive()          # same as obj.archive()
view = datapod.view_archive(datapod.Point, archive)

msg_type, wire = point.to_wire_message()
dynamic = datapod.dynamic_view(msg_type, memoryview(wire))
assert dynamic["x"] == 1.0
```

## Wire identity and schema policy

Current format:

```text
datapod-wire-v1/le
```

Important rules:

- Built-in types emit canonical-name hashes, not Rust type-path hashes.
- Runtime C/Python schemas must register canonical names whose hashes match
  `datapod_type_hash_name(...)` / `datapod.type_hash_name(...)`.
- Headers are encoded field-by-field in little-endian order.
- Native Rust padding is not part of the v1 wire body.
- Incoming wire bytes may be unaligned.
- The normal packet stays lean: no per-packet schema blobs and no field names.
- Schema metadata is side metadata: built-ins come from the compiled registry;
  runtime schemas are registered once per process.

Rust schema lookup:

```rust
let type_hash = datapod::bind::type_hash::<datapod::Grid>();
let info = datapod::registry::find_type_info(type_hash).unwrap();
let schema = datapod::registry::find_schema(type_hash).unwrap();

assert_eq!(info.canonical_name, "datapod.grid.v1");
assert!(!schema.fields.is_empty());
```

Python schema lookup:

```python
import datapod

schema = datapod.schema_for(datapod.Grid)
print(schema["canonical_name"])
print([field["name"] for field in schema["fields"]])
```

## Dynamic views

Dynamic views are for generic receivers that know only:

```text
type_hash + wire bytes
```

They validate the registered type, borrow the header/payload, and read named
fields from schema offsets without decoding an owned object.

Rust:

```rust
use datapod::{Encoding, Grid, Pose, dynamic, to_wire_message};

let grid = Grid::new(
    2,
    2,
    Encoding::Rgba8,
    0.5,
    false,
    Pose::default(),
    (0_u8..16).collect(),
);
let msg = to_wire_message(&grid);
let view = dynamic::view_message(msg.type_hash, &msg.bytes).expect("dynamic grid view");
let rows = view.get_u32("rows").expect("rows field");
let payload = view.payload();
```

Python:

```python
view = datapod.dynamic_view(type_hash, memoryview(wire))
rows = view["rows"]
payload = view.payload
```

C:

```c
DatapodDynamicView view = {0};
uint32_t rows = 0;
datapod_dynamic_view_message(message, &view);
datapod_dynamic_field_u32(view, "rows", &rows);
```

## Custom Rust schemas

Use `#[datapod(name = "...")]` when application code needs a canonical schema
name shared across Rust, C, and Python.

```rust
#[datapod::datapod(name = "acme.camera_frame.v1")]
pub struct CameraFrame {
    pub width: u32,
    pub height: u32,
    pub encoding: u32,

    #[dp(bytes)]
    pub data: Vec<u8>,
}

let frame = CameraFrame {
    width: 640,
    height: 480,
    encoding: 11,
    data: vec![0; 640 * 480],
};

frame.archive(|archive| {
    let view = CameraFrame::view_archive(archive).expect("valid camera frame");
    assert_eq!(view.header.width, 640);
    transport_publish(archive.frame());
}).expect("valid camera archive");
```

The macro emits:

- `CANONICAL_NAME` and canonical-name `TYPE_HASH` helpers
- a generated `<Type>Header` for payload-backed structs
- `DataPod`, `DataPodDecode`, `DataPodValidate`, and `DataPodAccess`
- `archive`, `view_archive`, and `from_archive`
- compatibility aliases such as `to_wire_frame`, `view_from_wire_frame`, and
  `from_wire_frame`
- schema field metadata for dynamic views

## Python custom schemas

Python can register runtime schemas declaratively:

```python
import datapod

@datapod.datapod_type("acme.packet.v1", payload=True, dataclass=True)
class Packet:
    channel: datapod.u16
    flags: datapod.u8
    data: bytes

packet = Packet(channel=7, flags=1, data=b"hello")
archive = packet.archive()
view = Packet.view_archive(archive)
owned = Packet.from_archive(archive)
```

Repeated fixed-width header fields use `datapod.array(marker, n)`, and payloads
can be exposed as `bytes`, `bytearray`, or `memoryview`.

## Built-in type families

| Family | Examples | Notes |
| --- | --- | --- |
| `geom` | `Point`, `Segment`, `Polygon`, `Ring`, `Path`, `Aabb`, `Circle`, `Obb` | Fixed primitives plus payload-backed point collections |
| `motion` | `Euler`, `Quaternion`, `Pose`, `Transform`, `Velocity`, `Acceleration`, `State` | Fixed-POD transform and motion records |
| `world` | `Geo`, `Loc`, `Utm` | Geodetic/local/UTM helpers and validation |
| `raster` | `Grid`, `Layer` | Header metadata plus raw byte payloads |
| `seq` | `Bytes`, `DpStr`, `Vector`, `Matrix`, `Tensor`, `Queue`, `Stack`, `Heap` | Byte-backed generic `bytemuck::Pod` containers |
| `assoc` | `Map`, `Set`, aliases `OMap`, `OSet` | Sorted byte layouts, no in-memory hash table on wire |
| `id` | `Uuid`, `Ip`, `MacAddr`, `DpString` | Fixed and payload-backed identifiers |
| `robot` | `Robot`, `Model`, `Link`, `Joint`, `Sensor`, `Twist`, `Wrench`, `Odom` | URDF-style robotics metadata |

## Fallible owned container APIs

Owned byte-backed containers keep ergonomic methods such as `push`, `pop`,
`get`, `insert`, and `remove`, but the production path is the matching
fallible API:

- `try_push`
- `try_pop`
- `try_get`
- `try_set`
- `try_insert`
- `try_remove`
- `try_push_back`
- `try_push_front`
- `try_from_bytes`

The `try_*` methods validate the current owned value before mutation, use
checked wire-size arithmetic, and preserve no-mutate-on-error behavior for
malformed owned buffers.

## Build and test

Use the Makefile lanes from the repository root.

```sh
make build
make check
make test
make bind
make test-bindings
make bench
```

Inside the robolibs Nix workflow, run the same Makefile lanes through the dev
shell:

```sh
nix develop --command make build
nix develop --command make test
nix develop --command make bind
nix develop --command make test-bindings
```

What the main lanes do:

| Target | Meaning |
| --- | --- |
| `make build` | `cargo build --lib` |
| `make check` | `cargo check --all-targets` |
| `make test` | `cargo test --all-targets` |
| `make bind` | regenerate C header and build the Python wheel |
| `make test-py` | build/install the wheel into a temp target and run Python smoke tests |
| `make test-c-abi` | build the C ABI library and run the C runtime smoke test |
| `make test-bindings` | `make bind` plus Python and C ABI smoke tests |
| `make bench` | Rust, C, and Python owned-vs-borrowed wire benchmarks |

## Packaging notes

- Rust crate name: `datapod`
- Rust workspace member: `macros` / `datapod-macros`
- Rust crate type: `rlib` and `cdylib`
- Python distribution name: `robolibs-datapod`
- Python import name: `datapod`
- Python feature: `python`
- The Rust crate is currently marked `publish = false` in `Cargo.toml`.

Build the Python wheel locally:

```sh
make bind-py
```

The wheel is written under `target/wheels/`.

## Current direction

The active direction: every built-in datapod should be
transportable and inspectable from only:

```text
type_hash + wire bytes
```

The packet format stays lean, dynamic views stay borrowed, and schema metadata
stays out-of-band. Custom schema distribution is a later layer; built-in
dynamic schema descriptors and Rust/Python/C dynamic views are the immediate
foundation.
