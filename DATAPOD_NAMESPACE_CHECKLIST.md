# Datapod Namespace Utilities Checklist

This checklist tracks which POD datapod types need lowercase namespace utilities (e.g., `dp::quaternion::make()`).

## Pattern

```cpp
namespace datapod {
    struct TypeName { ... };
    
    namespace typename {  // lowercase
        inline TypeName make(...) { return TypeName{...}; }
        // Additional utilities as needed
    }
}
```

## Status Legend
- [ ] Not started
- [x] Completed
- [~] In progress
- [-] Not applicable / Skip

---

## Spatial Types (include/datapod/spatial/)

### Core Spatial
- [ ] **Quaternion** - `include/datapod/spatial/quaternion.hpp`
  - Functions: `make(w,x,y,z)`, `from_euler()`, `to_euler()`
- [ ] **Quaternionf** - `include/datapod/spatial/quaternion.hpp`
  - Functions: `make(w,x,y,z)`
- [ ] **Euler** - `include/datapod/spatial/euler.hpp`
  - Functions: `make(roll,pitch,yaw)`, `from_quaternion()`, `to_quaternion()`
- [ ] **Point** - `include/datapod/spatial/point.hpp`
  - Functions: `make(x,y)`, `make(x,y,z)`
- [ ] **Pose** - `include/datapod/spatial/pose.hpp`
  - Functions: `make(position, rotation)`
- [ ] **Transform** - `include/datapod/spatial/transform.hpp`
  - Functions: `make(...)`, `identity()`
- [ ] **State** - `include/datapod/spatial/state.hpp`
  - Functions: `make(position, velocity)`
- [ ] **Velocity** - `include/datapod/spatial/velocity.hpp`
  - Functions: `make(linear, angular)`
- [ ] **Acceleration** - `include/datapod/spatial/acceleration.hpp`
  - Functions: `make(linear, angular)`
- [ ] **Size** - `include/datapod/spatial/size.hpp`
  - Functions: `make(width, height)`, `make(width, height, depth)`

### Bounding Volumes
- [ ] **AABB** - `include/datapod/spatial/aabb.hpp`
  - Functions: `make(min, max)`, `from_points()`
- [ ] **OBB** - `include/datapod/spatial/obb.hpp`
  - Functions: `make(center, half_extents, rotation)`
- [ ] **BS** (Bounding Sphere) - `include/datapod/spatial/bs.hpp`
  - Functions: `make(center, radius)`
- [ ] **BoundingSphere** - `include/datapod/spatial/bounding_sphere.hpp`
  - Functions: `make(center, radius)`
- [ ] **Box** - `include/datapod/spatial/box.hpp`
  - Functions: `make(min, max)`

### Geometric Primitives (include/datapod/spatial/primitives/)
- [ ] **Circle** - `include/datapod/spatial/primitives/circle.hpp`
  - Functions: `make(center, radius)`
- [ ] **Line** - `include/datapod/spatial/primitives/line.hpp`
  - Functions: `make(point, direction)`
- [ ] **Segment** - `include/datapod/spatial/primitives/segment.hpp`
  - Functions: `make(start, end)`
- [ ] **Rectangle** - `include/datapod/spatial/primitives/rectangle.hpp`
  - Functions: `make(min, max)`
- [ ] **Square** - `include/datapod/spatial/primitives/square.hpp`
  - Functions: `make(center, side_length)`
- [ ] **Triangle** - `include/datapod/spatial/primitives/triangle.hpp`
  - Functions: `make(p1, p2, p3)`

### Complex Spatial (include/datapod/spatial/complex/)
- [ ] **Polygon** - `include/datapod/spatial/complex/polygon.hpp`
  - Functions: `make(points)`
- [ ] **Path** - `include/datapod/spatial/complex/path.hpp`
  - Functions: `make(waypoints)`
- [ ] **Trajectory** - `include/datapod/spatial/complex/trajectory.hpp`
  - Functions: `make(poses)`
- [-] **Grid** - `include/datapod/spatial/complex/grid.hpp` (template, skip)
- [-] **Layer** - `include/datapod/spatial/complex/layer.hpp` (template, skip)

### Geospatial
- [ ] **Geo** - `include/datapod/spatial/geo.hpp`
  - Functions: `make(lat, lon)`, `make(lat, lon, alt)`
- [ ] **Loc** - `include/datapod/spatial/loc.hpp`
  - Functions: `make(lat, lon)`
- [ ] **Linestring** - `include/datapod/spatial/linestring.hpp`
  - Functions: `make(points)`
- [ ] **Ring** - `include/datapod/spatial/ring.hpp`
  - Functions: `make(points)`

### Robotics (include/datapod/spatial/robot/)
- [ ] **Twist** - `include/datapod/spatial/robot/twist.hpp`
  - Functions: `make(linear, angular)`
- [ ] **Wrench** - `include/datapod/spatial/robot/wrench.hpp`
  - Functions: `make(force, torque)`
- [ ] **Accel** - `include/datapod/spatial/robot/accel.hpp`
  - Functions: `make(linear, angular)`
- [ ] **Odom** - `include/datapod/spatial/robot/odom.hpp`
  - Functions: `make(pose, twist)`
- [ ] **Inertia** - `include/datapod/spatial/robot/inertia.hpp`
  - Functions: `make(mass, center_of_mass, inertia_tensor)`

### Spatial Trees
- [-] **QuadTree** - `include/datapod/spatial/quadtree.hpp` (template container, skip)
- [-] **RTree** - `include/datapod/spatial/rtree.hpp` (template container, skip)

---

## Matrix Types (include/datapod/matrix/)

### Core Matrix
- [-] **Matrix** - `include/datapod/matrix/matrix.hpp` (template, skip)
- [-] **Vector** - `include/datapod/matrix/vector.hpp` (template, skip)
- [-] **Tensor** - `include/datapod/matrix/tensor.hpp` (template, skip)
- [-] **Scalar** - `include/datapod/matrix/scalar.hpp` (template wrapper, skip)

### Math Types (include/datapod/matrix/math/)
- [-] **Quaternion** - `include/datapod/matrix/math/quaternion.hpp` (template, wrapped by spatial::Quaternion)
- [-] **Complex** - `include/datapod/matrix/math/complex.hpp` (template, skip)
- [-] **Dual** - `include/datapod/matrix/math/dual.hpp` (template, skip)
- [-] **Phasor** - `include/datapod/matrix/math/phasor.hpp` (template, skip)
- [-] **Polynomial** - `include/datapod/matrix/math/polynomial.hpp` (template, skip)
- [-] **Fraction** - `include/datapod/matrix/math/fraction.hpp` (template, skip)
- [-] **Interval** - `include/datapod/matrix/math/interval.hpp` (template, skip)
- [-] **Modular** - `include/datapod/matrix/math/modular.hpp` (template, skip)
- [-] **BigInt** - `include/datapod/matrix/math/bigint.hpp` (template, skip)
- [-] **Hypercomplex** - `include/datapod/matrix/math/hypercomplex.hpp` (template, skip)

---

## Temporal Types (include/datapod/temporal/)

- [-] **Stamp** - `include/datapod/temporal/stamp.hpp` (template, skip)
- [ ] **LogEvent** - `include/datapod/temporal/event.hpp`
  - Functions: `make(timestamp, level, message)`
- [ ] **SystemEvent** - `include/datapod/temporal/event.hpp`
  - Functions: `make(timestamp, type, data)`
- [ ] **Tick** - `include/datapod/temporal/financial.hpp`
  - Functions: `make(timestamp, price, volume)`
- [ ] **OHLCV** - `include/datapod/temporal/financial.hpp`
  - Functions: `make(timestamp, open, high, low, close, volume)`
- [ ] **TimeWindow** - `include/datapod/temporal/window.hpp`
  - Functions: `make(start, end)`
- [-] **TimeSeries** - `include/datapod/temporal/time_series.hpp` (template container, skip)
- [-] **MultiSeries** - `include/datapod/temporal/multi_series.hpp` (container, skip)
- [-] **CircularBuffer** - `include/datapod/temporal/circular_buffer.hpp` (template container, skip)

---

## Summary

**Total POD types identified**: ~45
**Skipped (templates/containers)**: ~20
**To implement**: ~25

### Priority Order
1. **Phase 1** (Most common): Quaternion, Euler, Point, Pose, Velocity, Acceleration
2. **Phase 2** (Primitives): Circle, Rectangle, Line, Segment, Triangle, Square
3. **Phase 3** (Robotics): Twist, Wrench, Accel, Odom, Inertia
4. **Phase 4** (Bounding): AABB, OBB, BS, Box
5. **Phase 5** (Complex): Polygon, Path, Trajectory, Geo, Linestring
6. **Phase 6** (Temporal): LogEvent, SystemEvent, Tick, OHLCV, TimeWindow

---

## Notes

- Template types (Matrix, Vector, Tensor, Stamp, etc.) are skipped because they already have flexible constructors
- Container types (QuadTree, RTree, TimeSeries, etc.) are skipped - not POD utilities
- Focus on simple POD structs that benefit from explicit factory functions
- Each namespace should provide at minimum a `make()` function
- Conversion utilities (like `from_euler`, `to_euler`) added where appropriate
