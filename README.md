# datapod

Rust/C/Python datapod wire contract for robotics data types and POD-oriented
containers.

The crate exposes:

- Rust `DataPod` implementations and borrowed Archive/View/Owned APIs.
- A C ABI with owned handles, generic wire helpers, borrowed
  `DatapodArchiveFrame`, and typed archive views.
- Python bindings with generic wire helpers and declarative schema decorators,
  including fixed-width annotation markers such as `datapod.u16`, plus
  `ArchiveFrame`/`memoryview` payload access.

## Zero-copy model

Use Archive/View for the fast path:

```rust
#[datapod::datapod(name = "robolibs.camera_frame.v1")]
pub struct CameraFrame {
    pub width: u32,
    pub height: u32,
    pub encoding: u32,
    #[dp(bytes)]
    pub data: Vec<u8>,
}

let camera = CameraFrame {
    width: 640,
    height: 480,
    encoding: 11,
    data: frame_bytes,
};

camera.archive(|archive| {
    let _view = CameraFrame::view_archive(archive)?;
    publisher.publish_archive(archive)
})?;

let msg = camera.to_wire_message(); // owned/copying convenience path
```

For custom Rust types, `#[datapod(name = "...")]` sets the same canonical
schema name model used by C/Python runtime schemas. The macro emits
`CANONICAL_NAME`, `TYPE_HASH`, `register_schema`, `to_wire_message`,
`archive`, `segmented_archive`, `view_archive`, and `from_archive` helpers for
the type. The older `with_wire_frame`, `view_from_wire_frame`, and
`from_wire_frame` names remain as compatibility aliases.

`ArchiveFrame`/`WireFrame` borrows `header` and `payload` separately and does
not allocate a joined payload buffer. `OwnedWireMessage`/`WireMessage { bytes:
Vec<u8> }` remains the owned convenience path for files, tests, and transports
that require contiguous bytes; it is not the performance path.

C uses `DatapodArchiveFrame`, generic `datapod_archive_split(...)` /
`datapod_archive_to_message(...)`, and typed archive helpers for opaque owned
handles such as `datapod_matrix_archive(...)`,
`datapod_matrix_view_from_archive(...)`, and `datapod_matrix_from_archive(...)`.
Python uses `obj.archive()`, `datapod.view_archive(...)`, and typed
`view_archive(...)` helpers that expose payloads as `memoryview`.

## Wire format policy

See [`docs/WIRE_FORMAT.md`](docs/WIRE_FORMAT.md) for the current
`datapod-wire-v1/le` format, validation rules, and C/Python borrowed-view
lifetime rules.

## Fallible owned container APIs

Rust byte-backed containers keep the historical ergonomic methods (`push`,
`pop`, `get`, `insert`, etc.) as panic-on-invalid convenience wrappers, but the
production path is the matching `try_*` API. Examples include
`try_push`, `try_pop`, `try_get`, `try_set`, `try_insert`, `try_push_back`,
`try_push_front`, and `try_from_bytes`. These fallible methods validate the
current owned value before mutation, use checked wire-size arithmetic, and
preserve no-mutate-on-error behavior for malformed owned buffers.

## Build and test

Use the Makefile lanes:

```sh
nix develop --command make test
nix develop --command make bench
nix develop --command make bind
nix develop --command make test-bindings
```

`make bench` runs Rust, C, and Python owned-vs-borrowed payload benchmarks and
prints copied bytes per operation.
