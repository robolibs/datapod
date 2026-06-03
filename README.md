# datapod

Rust/C/Python datapod wire contract for robotics data types and POD-oriented
containers.

The crate exposes:

- Rust `DataPod` implementations and borrowed validation/access APIs.
- A C ABI with owned handles, generic wire helpers, borrowed `DatapodWireFrame`,
  and typed frame views.
- Python bindings with generic wire helpers and declarative schema decorators,
  including fixed-width annotation markers such as `datapod.u16`, plus
  `WireFrame`/`memoryview` payload access.

## Zero-copy model

Use `WireFrame` for the fast path:

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

camera.with_wire_frame(|frame| {
    let _view = CameraFrame::view_from_wire_frame(frame)?;
    publisher.publish_frame(frame)
})?;
```

For custom Rust types, `#[datapod(name = "...")]` sets the same canonical
schema name model used by C/Python runtime schemas. The macro emits
`CANONICAL_NAME`, `TYPE_HASH`, `register_schema`, `to_wire_message`,
`with_wire_frame`, `view_from_wire_frame`, and `from_wire_frame` helpers for
the type.

`WireFrame` borrows `header` and `payload` separately and does not allocate a
joined payload buffer. `WireMessage { bytes: Vec<u8> }` remains the owned
convenience path for files, tests, and transports that require contiguous bytes;
it is not the performance path.

C uses `DatapodWireFrame` plus `datapod_*_view_from_frame(...)`. Python uses
`obj.to_wire_frame()`, `datapod.view_wire_frame(...)`, and typed
`view_from_wire_frame(...)` helpers that expose payloads as `memoryview`.

## Wire format policy

See [`docs/WIRE_FORMAT.md`](docs/WIRE_FORMAT.md) for the current
`datapod-wire-v1/le` format, validation rules, and C/Python borrowed-view
lifetime rules.

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
