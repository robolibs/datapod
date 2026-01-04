<img align="right" width="26%" src="./misc/logo.png">

# Datapod

High-performance robotics data structures with zero-cost serialization, SIMD-optimized spatial types, and real-time guarantees

## Development Status

See [TODO.md](./TODO.md) for the complete development plan and current progress.

## Overview

Datapod is a modern C++20 library providing high-performance, Plain Old Data (POD) compatible containers specifically designed for **robotics applications**. Built for real-time control loops, SLAM pipelines, and navigation stacks where deterministic performance and zero-copy data interchange are critical.

**Why Datapod for Robotics:**
- **Real-Time Safe**: No dynamic allocations in hot paths, deterministic performance for control loops running at 1kHz+
- **ROS2 Ready**: POD types work seamlessly with shared memory transport, zero-copy between nodes
- **SLAM-Optimized**: Spatial indexing (R-trees, quadtrees) for efficient point cloud processing and map queries
- **Sensor Fusion**: SIMD-optimized matrix operations for Kalman filters, pose estimation, and IMU processing
- **Navigation Stack**: Built-in geometric primitives (AABB, OBB, poses) and path representations

**Key Design Principles:**
- **POD-First**: All types are trivially copyable - safe for inter-process shared memory
- **Zero-Cost Abstraction**: No virtual functions, no heap overhead - perfect for embedded systems
- **SIMD-Ready**: 32-byte aligned matrices for vectorized odometry and transformation pipelines
- **Reflection Built-in**: Automatic serialization for ROS2 messages, logging, and network transport
- **Deterministic**: Bounded execution time for safety-critical robot control

Perfect for mobile robots, manipulators, drones, autonomous vehicles, and any robotics system requiring predictable, high-frequency data processing.

### Architecture

Datapod is organized into specialized modules for different robotics domains:

```
┌────────────────────────────────────────────────────────────────────────────┐
│                              DATAPOD LIBRARY                               │
├──────────┬─────────────┬──────────────┬──────────────┬──────────┬──────────┤
│  Types   │ Sequential  │ Associative  │   Matrix     │  Spatial │ Temporal │
│ (Rust)   │ Containers  │  Containers  │   (SIMD)     │ Geometry │  Series  │
│          │             │              │              │          │          │
│ ┌──────┐ │ ┌─────────┐ │ ┌──────────┐ │ ┌──────────┐ │ ┌──────┐ │ ┌──────┐ │
│ │ dp:: │ │ │ Vector  │ │ │   Map    │ │ │  vector  │ │ │ Point│ │ │ Time │ │
│ │ u8   │ │ │ Array   │ │ │   Set    │ │ │  matrix  │ │ │ Pose │ │ │Series│ │
│ │ i32  │ │ │ String  │ │ │ Multimap │ │ │  tensor  │ │ │ Twist│ │ │ Stamp│ │
│ │ f64  │ │ └─────────┘ │ └──────────┘ │ └──────────┘ │ │ Odom │ │ └──────┘ │
│ └──────┘ │             │              │              │ │ RTree│ │          │
│          │             │              │              │ └──────┘ │          │
└──────────┴─────────────┴──────────────┴──────────────┴──────────┴──────────┘
     │            │               │              │           │          │
     └────────────┴───────────────┴──────────────┴───────────┴──────────┘
                                      │
                           ┌──────────▼──────────┐
                           │  Reflection System  │
                           │  (Serialization)    │
                           └──────────┬──────────┘
                                      │
                        ┌─────────────┼─────────────┐
                        │             │             │
                  ┌─────▼─────┐ ┌─────▼────┐ ┌──────▼────┐
                  │ ROS2 Msgs │ │ Logging  │ │  Network  │
                  └───────────┘ └──────────┘ └───────────┘
```

## Installation

### Quick Start (CMake FetchContent)

```cmake
include(FetchContent)
FetchContent_Declare(
  datapod
  GIT_REPOSITORY https://github.com/robolibs/datapod.git
  GIT_TAG main
)
FetchContent_MakeAvailable(datapod)

target_link_libraries(your_robot_node PRIVATE datapod)
```

### Recommended: XMake

[XMake](https://xmake.io/) is a modern, fast, and cross-platform build system.

**Install XMake:**
```bash
curl -fsSL https://xmake.io/shget.text | bash
```

**Add to your xmake.lua:**
```lua
add_requires("datapod")

target("your_robot")
    set_kind("binary")
    add_packages("datapod")
    add_files("src/*.cpp")
```

**Build:**
```bash
xmake
xmake run
```

### Complete Development Environment (Nix + Direnv + Devbox)

For the ultimate reproducible development environment:

**1. Install Nix (package manager from NixOS):**
```bash
curl --proto '=https' --tlsv1.2 -sSf -L https://install.determinate.systems/nix | sh -s -- install
```
[Nix](https://nixos.org/) - Reproducible, declarative package management

**2. Install direnv (automatic environment switching):**
```bash
sudo apt install direnv

# Add to your shell (~/.bashrc or ~/.zshrc):
eval "$(direnv hook bash)"  # or zsh
```
[direnv](https://direnv.net/) - Load environment variables based on directory

**3. Install Devbox (Nix-powered development environments):**
```bash
curl -fsSL https://get.jetpack.io/devbox | bash
```
[Devbox](https://www.jetpack.io/devbox/) - Portable, isolated dev environments

**4. Use the environment:**
```bash
cd datapod
direnv allow  # Allow .envrc (one-time)
# Environment automatically loaded! All dependencies available.

make build   # or xmake
make test
```

## Usage

### Basic Usage

```cpp
#include <datapod/datapod.hpp>
#include <datapod/types/types.hpp>  // Rust-inspired types

using namespace datapod;

// Use Rust-style types with dp:: namespace
dp::u32 robot_id = 42;
dp::f64 battery_voltage = 12.6;
dp::usize sensor_count = 16;

// Robot pose tracking (6-DOF localization)
Point robot_position{1.0, 2.0, 0.0};
Quaternion robot_orientation{1.0, 0.0, 0.0, 0.0};
Pose robot_pose{robot_position, robot_orientation};

// Velocity commands for mobile base
Velocity linear_vel{0.5, 0.0, 0.0};  // 0.5 m/s forward
Velocity angular_vel{0.0, 0.0, 0.1};  // 0.1 rad/s rotation
Twist cmd_vel{linear_vel, angular_vel};

// Zero-copy serialization for ROS2 shared memory transport
auto buffer = serialize(robot_pose);
auto restored = deserialize<Mode::NONE, Pose>(buffer);
```

### Advanced Usage

```cpp
#include <datapod/datapod.hpp>
#include <datapod/spatial.hpp>
#include <datapod/matrix.hpp>

using namespace datapod;
using namespace datapod::mat;

// SLAM: Point cloud spatial indexing for nearest neighbor search
RTree<Point> point_cloud;
for (const auto& laser_point : scan) {
    point_cloud.insert(Point{laser_point.x, laser_point.y, laser_point.z});
}

AABB robot_vicinity{Point{-5.0, -5.0, 0.0}, Point{5.0, 5.0, 2.0}};
auto nearby_points = point_cloud.query(robot_vicinity);

// Navigation: Landmark-based localization
Map<String, Pose> landmark_poses;
landmark_poses.insert({"charging_station", Pose{Point{0.0, 0.0, 0.0}, Quaternion{1,0,0,0}}});
landmark_poses.insert({"docking_bay", Pose{Point{10.0, 5.0, 0.0}, Quaternion{0.707,0,0,0.707}}});

// Odometry fusion: SIMD-optimized state estimation
Odom wheel_odom, visual_odom, fused_odom;
wheel_odom.pose = Pose{Point{1.0, 2.0, 0.0}, Quaternion{1.0, 0.0, 0.0, 0.0}};
wheel_odom.twist = Twist{Velocity{0.5, 0.0, 0.0}, Velocity{0.0, 0.0, 0.1}};

// Convert to SIMD vectors for Kalman filter update
auto wheel_vec = wheel_odom.to_mat();    // mat::vector<double, 13>
auto visual_vec = visual_odom.to_mat();  // SIMD-friendly fusion
// ... perform vectorized Kalman filter operations ...
fused_odom = Odom::from_mat(fused_vec);

// Sensor data buffering: IMU time-series for motion prediction
TimeSeries<Acceleration> imu_buffer;
imu_buffer.push_back({Stamp::now(), Acceleration{0.1, 0.0, 9.81}});
imu_buffer.push_back({Stamp::now(), Acceleration{0.2, 0.0, 9.80}});

// Manipulation: Force-torque sensor monitoring
Wrench gripper_force;
gripper_force.force = Point{0.0, 0.0, -5.0};   // 5N downward
gripper_force.torque = Point{0.1, 0.0, 0.0};   // 0.1 Nm torque
```

## Features

### Primitive Types (Rust-Inspired)
Datapod provides a complete type system with **zero `std::` dependencies**, using compiler built-ins:

```cpp
#include <datapod/types/types.hpp>

// Integer types (signed)
dp::i8   tiny = -128;      // 8-bit signed
dp::i16  small = -32000;   // 16-bit signed
dp::i32  normal = -2000;   // 32-bit signed
dp::i64  big = -9000000;   // 64-bit signed

// Integer types (unsigned)
dp::u8   byte = 255;       // 8-bit unsigned
dp::u16  word = 65535;     // 16-bit unsigned
dp::u32  dword = 4000000;  // 32-bit unsigned
dp::u64  qword = 18000000; // 64-bit unsigned

// Floating point
dp::f32  single = 3.14f;   // 32-bit float
dp::f64  double = 2.718;   // 64-bit double

// Platform types
dp::usize size = 1024;     // Platform-dependent size
dp::isize diff = -42;      // Platform-dependent signed size

// Other types
dp::boolean flag = true;   // Boolean
dp::byte raw = 0xFF;       // Raw byte
```

**Why use `dp::` types?**
- ✅ **No `std::` dependencies** - Pure compiler built-ins
- ✅ **Rust-style naming** - Familiar to modern developers
- ✅ **Explicit sizes** - `i32` is always 32 bits, no surprises
- ✅ **Shorter syntax** - `dp::u32` vs `std::uint32_t`
- ✅ **Consistent** - Same naming across all platforms

Available in both `datapod::` and `dp::` namespaces for convenience.

### Sequential Containers
- **`Vector<T>`** - Dynamic array with custom allocators
  ```cpp
  Vector<int> v{1, 2, 3};
  v.push_back(4);
  ```
- **`Array<T, N>`** - Fixed-size array with bounds checking
- **`String`** - Small string optimization (SSO) with POD layout
- **`BitVec`** - Compact bit vector for boolean arrays

### Associative Containers
- **`Map<K, V>`** - Robin Hood hash map with open addressing
  ```cpp
  Map<String, int> ages;
  ages.insert({"Alice", 30});
  ```
- **`Set<T>`** - Hash set for unique elements
- **`FWSMultimap<K, V>`** - Frozen multi-map (build once, query many)

### Matrix Types (SIMD-Optimized)
All types in `datapod::mat` namespace are 32-byte aligned for SIMD operations:
- **`scalar<T>`** - Rank-0 (0D) tagged scalar value
- **`vector<T, N>`** - Rank-1 (1D) fixed-size vector
- **`matrix<T, R, C>`** - Rank-2 (2D) column-major matrix
- **`tensor<T, Dims...>`** - Rank-N (3D+) N-dimensional tensor
  ```cpp
  mat::vector<double, 3> v{1.0, 2.0, 3.0};
  mat::matrix<double, 3, 3> rotation;
  rotation.set_identity();
  ```

### Spatial Types
**Core Types:**
- **`Point`**, **`Velocity`**, **`Acceleration`** - 3D motion
- **`Euler`**, **`Quaternion`** - Rotation representations
- **`Pose`**, **`State`** - 6-DOF pose and state

**Robot Types:**
- **`Twist`** - Linear + angular velocity (6-DOF)
- **`Wrench`** - Force + torque (6-DOF)
- **`Odom`** - Odometry (pose + twist)
- **`Inertia`** - Rigid body inertial properties

**Bounding Volumes:**
- **`AABB`** - Axis-aligned bounding box
- **`OBB`** - Oriented bounding box
- **`BoundingSphere`** - Spherical bounds

**SIMD Conversion:**
All spatial types support `to_mat()` / `from_mat()` for SIMD operations:
```cpp
Point p{1.0, 2.0, 3.0};
auto v = p.to_mat();  // mat::vector<double, 3>
// ... SIMD operations ...
Point p2 = Point::from_mat(v);
```

### Spatial Indexing
- **`RTree<T>`** - R-tree for spatial queries
  ```cpp
  RTree<Point> tree;
  tree.insert(Point{1.0, 2.0, 3.0});
  auto nearby = tree.query(aabb);
  ```
- **`Quadtree<T>`** - 2D spatial partitioning

### Temporal Containers
- **`TimeSeries<T>`** - Timestamped sequential data
- **`CircularBuffer<T, N>`** - Fixed-size ring buffer
- **`Stamp`** - Nanosecond-precision timestamps

### Serialization
Zero-copy, reflection-based serialization for all types:
```cpp
// Any POD type with members() is serializable
auto buffer = serialize(my_data);
auto copy = deserialize<Mode::NONE, MyType>(buffer);

// Supports hash verification, versioning, etc.
auto checked = deserialize<Mode::HASH, MyType>(buffer);
```

### Reflection System
Compile-time reflection via `members()`:
```cpp
struct MyData {
    int x;
    double y;
    String name;

    auto members() noexcept { return std::tie(x, y, name); }
    auto members() const noexcept { return std::tie(x, y, name); }
};

// Automatic serialization, comparison, iteration
for_each_field(data, [](auto& field) { /* ... */ });
```

### Type Safety
- **`Optional<T>`** - Type-safe optional values
- **`Result<T, E>`** - Railway-oriented error handling
- **`Variant<Ts...>`** - Type-safe discriminated union
- **`Strong<T, Tag>`** - Strongly-typed wrappers

## Performance

Designed for real-time robotics where every microsecond counts:

- **Zero-cost abstractions** - No virtual calls, fully inlined - suitable for 1kHz+ control loops
- **Cache-optimized** - Contiguous memory layout for sequential sensor data processing
- **SIMD-ready** - 32-byte aligned matrices for vectorized odometry and Kalman filters
- **Minimal allocations** - Deterministic memory usage, no surprise GC pauses
- **POD-compatible** - `memcpy`-safe for ROS2 shared memory transport and zero-copy IPC
- **Real-time safe** - Bounded execution time, no locks in read paths
- **Embedded-friendly** - Works on ARM Cortex-M, suitable for microcontroller-based robots

## Requirements

- **C++20** compiler (GCC 10+, Clang 12+)
- **CMake 3.15+** or **XMake 2.5+**

## License

MIT License - see [LICENSE](./LICENSE) for details.

## Acknowledgments

Made possible thanks to [these amazing projects](./ACKNOWLEDGMENTS.md).
