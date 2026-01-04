# Datapod Namespace Utilities Checklist

This checklist tracks ALL datapod types that need lowercase namespace utilities.

## Updated Requirements (2026-01-04)

**ALL pods get a namespace** - even templates and containers:
- If `make()` makes sense → implement it
- If `make()` doesn't make sense → add `unimplemented()` placeholder function

## Pattern

```cpp
namespace datapod {
    struct TypeName { ... };
    
    namespace typename {  // lowercase
        // Option 1: Has useful make() function
        inline TypeName make(...) { return TypeName{...}; }
        
        // Option 2: No useful make() - add placeholder
        inline void unimplemented() {}
    }
}
```

## Status Legend
- [ ] Not started
- [x] Completed
- [~] In progress

---

## Spatial Types (include/datapod/pods/spatial/)

### Core Spatial
- [x] **Quaternion** - `include/datapod/pods/spatial/quaternion.hpp`
  - Namespace: `quaternion` - Functions: `make(w,x,y,z)` for double and float
- [x] **Euler** - `include/datapod/pods/spatial/euler.hpp`
  - Namespace: `euler` - Functions: `make(roll,pitch,yaw)`
- [x] **Point** - `include/datapod/pods/spatial/point.hpp`
  - Namespace: `point` - Functions: `make(x,y)`, `make(x,y,z)`, `origin()`
- [x] **Pose** - `include/datapod/pods/spatial/pose.hpp`
  - Namespace: `pose` - Functions: `make(position, rotation)`, `identity()`
- [x] **Transform** - `include/datapod/pods/spatial/transform.hpp`
  - Namespace: `transform` - Functions: `make(...)`, `identity()`
- [x] **State** - `include/datapod/pods/spatial/state.hpp`
  - Namespace: `state` - Functions: `make(pose, velocities)`, `at_rest()`
- [x] **Velocity** - `include/datapod/pods/spatial/velocity.hpp`
  - Namespace: `velocity` - Functions: `make(vx, vy, vz)`
- [x] **Acceleration** - `include/datapod/pods/spatial/acceleration.hpp`
  - Namespace: `acceleration` - Functions: `make(ax, ay, az)`
- [x] **Size** - `include/datapod/pods/spatial/size.hpp`
  - Namespace: `size` - Functions: `make(width, height)`, `make(width, height, depth)`, `uniform()`, `zero()`

### Bounding Volumes
- [x] **AABB** - `include/datapod/pods/spatial/aabb.hpp`
  - Namespace: `aabb` - Functions: `make(min, max)`, `from_center()`, `unit()`
- [x] **OBB** - `include/datapod/pods/spatial/obb.hpp`
  - Namespace: `obb` - Functions: `make(center, half_extents, rotation)`, `unit()`
- [x] **BS** (Bounding Sphere) - `include/datapod/pods/spatial/bs.hpp`
  - Namespace: `bs` - Functions: `make(center, radius)`, `unit()`
- [x] **BoundingSphere** - `include/datapod/pods/spatial/bounding_sphere.hpp`
  - Namespace: `bounding_sphere` - Functions: `make(center, radius)`, `unit()`
- [x] **Box** - `include/datapod/pods/spatial/box.hpp`
  - Namespace: `box` - Functions: `make(pose, size)`, `unit()`

### Geometric Primitives (include/datapod/pods/spatial/primitives/)
- [x] **Circle** - `include/datapod/pods/spatial/primitives/circle.hpp`
  - Namespace: `circle` - Functions: `make(center, radius)`, `unit()`
- [x] **Line** - `include/datapod/pods/spatial/primitives/line.hpp`
  - Namespace: `line` - Functions: `make(origin, direction)`, `from_points(p1, p2)`
- [x] **Segment** - `include/datapod/pods/spatial/primitives/segment.hpp`
  - Namespace: `segment` - Functions: `make(start, end)`
- [x] **Rectangle** - `include/datapod/pods/spatial/primitives/rectangle.hpp`
  - Namespace: `rectangle` - Functions: `make(corners)`, `make(min, max)`, `make(center, width, height)`
- [x] **Square** - `include/datapod/pods/spatial/primitives/square.hpp`
  - Namespace: `square` - Functions: `make(center, side)`, `unit()`
- [x] **Triangle** - `include/datapod/pods/spatial/primitives/triangle.hpp`
  - Namespace: `triangle` - Functions: `make(p1, p2, p3)`

### Complex Spatial (include/datapod/pods/spatial/complex/)
- [ ] **Polygon** - `include/datapod/pods/spatial/complex/polygon.hpp`
  - Namespace: `polygon` - Functions: `unimplemented()` (container-like)
- [ ] **Path** - `include/datapod/pods/spatial/complex/path.hpp`
  - Namespace: `path` - Functions: `unimplemented()` (container-like)
- [ ] **Trajectory** - `include/datapod/pods/spatial/complex/trajectory.hpp`
  - Namespace: `trajectory` - Functions: `unimplemented()` (container-like)
- [ ] **Grid** - `include/datapod/pods/spatial/complex/grid.hpp`
  - Namespace: `grid` - Functions: `unimplemented()` (template)
- [ ] **Layer** - `include/datapod/pods/spatial/complex/layer.hpp`
  - Namespace: `layer` - Functions: `unimplemented()` (template)

### Geospatial
- [x] **Geo** - `include/datapod/pods/spatial/geo.hpp`
  - Namespace: `geo` - Functions: `make(lat, lon)`, `make(lat, lon, alt)`, `origin()`, `without_altitude()`
- [x] **Loc** - `include/datapod/pods/spatial/loc.hpp`
  - Namespace: `loc` - Functions: `make(local, origin)`, `at_origin()`
- [x] **UTM** - `include/datapod/pods/spatial/utm.hpp`
  - Namespace: `utm` - Functions: `make(zone, band, easting, northing, altitude)`, `without_altitude()`
- [x] **Linestring** - `include/datapod/pods/spatial/linestring.hpp`
  - Namespace: `linestring` - Functions: `unimplemented()` (container-like)
- [x] **Ring** - `include/datapod/pods/spatial/ring.hpp`
  - Namespace: `ring` - Functions: `unimplemented()` (container-like)

### Multi Geometries (include/datapod/pods/spatial/multi/)
- [x] **MultiPoint** - `include/datapod/pods/spatial/multi/multi_point.hpp`
  - Namespace: `multi_point` - Functions: `unimplemented()` (container)
- [x] **MultiLinestring** - `include/datapod/pods/spatial/multi/multi_linestring.hpp`
  - Namespace: `multi_linestring` - Functions: `unimplemented()` (container)
- [x] **MultiPolygon** - `include/datapod/pods/spatial/multi/multi_polygon.hpp`
  - Namespace: `multi_polygon` - Functions: `unimplemented()` (container)

### Gaussian Types (include/datapod/pods/spatial/gaussian/)
- [ ] **GaussianPoint** - `include/datapod/pods/spatial/gaussian/point.hpp`
  - Namespace: `gaussian_point` - Functions: `make(mean, covariance)`
- [ ] **GaussianBox** - `include/datapod/pods/spatial/gaussian/box.hpp`
  - Namespace: `gaussian_box` - Functions: `make(...)`
- [ ] **GaussianCircle** - `include/datapod/pods/spatial/gaussian/circle.hpp`
  - Namespace: `gaussian_circle` - Functions: `make(...)`
- [ ] **GaussianRectangle** - `include/datapod/pods/spatial/gaussian/rectangle.hpp`
  - Namespace: `gaussian_rectangle` - Functions: `make(...)`

### Robotics (include/datapod/pods/spatial/robot/)
- [x] **Twist** - `include/datapod/pods/spatial/robot/twist.hpp`
  - Namespace: `twist` - Functions: `make(linear, angular)`, `linear()`, `angular()`, `zero()`
- [x] **Wrench** - `include/datapod/pods/spatial/robot/wrench.hpp`
  - Namespace: `wrench` - Functions: `make(force, torque)`, `force()`, `torque()`, `zero()`
- [x] **Accel** - `include/datapod/pods/spatial/robot/accel.hpp`
  - Namespace: `accel` - Functions: `make(linear, angular)`, `linear()`, `angular()`, `zero()`
- [x] **Odom** - `include/datapod/pods/spatial/robot/odom.hpp`
  - Namespace: `odom` - Functions: `make(pose, twist)`, `at_rest()`
- [x] **Inertia** - `include/datapod/pods/spatial/robot/inertia.hpp`
  - Namespace: `inertia` - Functions: `make()`, `diagonal()`, `point_mass()`, `sphere()`, `box()`, `cylinder()`

### Spatial Trees
- [ ] **QuadTree** - `include/datapod/pods/spatial/quadtree.hpp`
  - Namespace: `quadtree` - Functions: `unimplemented()` (template container)
- [ ] **RTree** - `include/datapod/pods/spatial/rtree.hpp`
  - Namespace: `rtree` - Functions: `unimplemented()` (template container)

---

## Matrix Types (include/datapod/pods/matrix/)

### Core Matrix
- [ ] **Matrix** - `include/datapod/pods/matrix/matrix.hpp`
  - Namespace: `matrix` - Functions: `unimplemented()` (template)
- [ ] **Vector** - `include/datapod/pods/matrix/vector.hpp`
  - Namespace: `vector` - Functions: `unimplemented()` (template)
- [ ] **Tensor** - `include/datapod/pods/matrix/tensor.hpp`
  - Namespace: `tensor` - Functions: `unimplemented()` (template)
- [ ] **Scalar** - `include/datapod/pods/matrix/scalar.hpp`
  - Namespace: `scalar` - Functions: `unimplemented()` (template wrapper)
- [ ] **Dynamic** - `include/datapod/pods/matrix/dynamic.hpp`
  - Namespace: `dynamic` - Functions: `unimplemented()` (dynamic matrix)

### Math Types (include/datapod/pods/matrix/math/)
- [ ] **mat::Quaternion** - `include/datapod/pods/matrix/math/quaternion.hpp`
  - Namespace: `mat_quaternion` - Functions: `unimplemented()` (template, wrapped by spatial::Quaternion)
- [ ] **Complex** - `include/datapod/pods/matrix/math/complex.hpp`
  - Namespace: `complex` - Functions: `unimplemented()` (template)
- [ ] **Dual** - `include/datapod/pods/matrix/math/dual.hpp`
  - Namespace: `dual` - Functions: `unimplemented()` (template)
- [ ] **Phasor** - `include/datapod/pods/matrix/math/phasor.hpp`
  - Namespace: `phasor` - Functions: `unimplemented()` (template)
- [ ] **Polynomial** - `include/datapod/pods/matrix/math/polynomial.hpp`
  - Namespace: `polynomial` - Functions: `unimplemented()` (template)
- [ ] **Fraction** - `include/datapod/pods/matrix/math/fraction.hpp`
  - Namespace: `fraction` - Functions: `unimplemented()` (template)
- [ ] **Interval** - `include/datapod/pods/matrix/math/interval.hpp`
  - Namespace: `interval` - Functions: `unimplemented()` (template)
- [ ] **Modular** - `include/datapod/pods/matrix/math/modular.hpp`
  - Namespace: `modular` - Functions: `unimplemented()` (template)
- [ ] **BigInt** - `include/datapod/pods/matrix/math/bigint.hpp`
  - Namespace: `bigint` - Functions: `unimplemented()` (template)
- [ ] **Hypercomplex** - `include/datapod/pods/matrix/math/hypercomplex.hpp`
  - Namespace: `hypercomplex` - Functions: `unimplemented()` (template)

---

## Sequential Types (include/datapod/pods/sequential/)

- [ ] **Array** - `include/datapod/pods/sequential/array.hpp`
  - Namespace: `array` - Functions: `unimplemented()` (template container)
- [ ] **Vector** - `include/datapod/pods/sequential/vector.hpp`
  - Namespace: `seq_vector` - Functions: `unimplemented()` (template container)
- [ ] **Deque** - `include/datapod/pods/sequential/deque.hpp`
  - Namespace: `deque` - Functions: `unimplemented()` (template container)
- [ ] **List** - `include/datapod/pods/sequential/list.hpp`
  - Namespace: `list` - Functions: `unimplemented()` (template container)
- [ ] **ForwardList** - `include/datapod/pods/sequential/forward_list.hpp`
  - Namespace: `forward_list` - Functions: `unimplemented()` (template container)
- [ ] **String** - `include/datapod/pods/sequential/string.hpp`
  - Namespace: `string` - Functions: `unimplemented()` (template container)
- [ ] **CString** - `include/datapod/pods/sequential/cstring.hpp`
  - Namespace: `cstring` - Functions: `unimplemented()` (fixed string)
- [ ] **Stack** - `include/datapod/pods/sequential/stack.hpp`
  - Namespace: `stack` - Functions: `unimplemented()` (template adapter)
- [ ] **Queue** - `include/datapod/pods/sequential/queue.hpp`
  - Namespace: `queue` - Functions: `unimplemented()` (template adapter)
- [ ] **FixedQueue** - `include/datapod/pods/sequential/fixed_queue.hpp`
  - Namespace: `fixed_queue` - Functions: `unimplemented()` (template)
- [ ] **Heap** - `include/datapod/pods/sequential/heap.hpp`
  - Namespace: `heap` - Functions: `unimplemented()` (template)
- [ ] **IndexedHeap** - `include/datapod/pods/sequential/indexed_heap.hpp`
  - Namespace: `indexed_heap` - Functions: `unimplemented()` (template)
- [ ] **BitVec** - `include/datapod/pods/sequential/bitvec.hpp`
  - Namespace: `bitvec` - Functions: `unimplemented()` (bit vector)
- [ ] **Vectra** - `include/datapod/pods/sequential/vectra.hpp`
  - Namespace: `vectra` - Functions: `unimplemented()` (template)
- [ ] **VecVec** - `include/datapod/pods/sequential/vecvec.hpp`
  - Namespace: `vecvec` - Functions: `unimplemented()` (template)
- [ ] **PagedVecVec** - `include/datapod/pods/sequential/paged_vecvec.hpp`
  - Namespace: `paged_vecvec` - Functions: `unimplemented()` (template)
- [ ] **NVec** - `include/datapod/pods/sequential/nvec.hpp`
  - Namespace: `nvec` - Functions: `unimplemented()` (template)
- [ ] **FlatMatrix** - `include/datapod/pods/sequential/flat_matrix.hpp`
  - Namespace: `flat_matrix` - Functions: `unimplemented()` (template)

---

## Associative Types (include/datapod/pods/associative/)

- [ ] **Map** - `include/datapod/pods/associative/map.hpp`
  - Namespace: `map` - Functions: `unimplemented()` (template container)
- [ ] **Set** - `include/datapod/pods/associative/set.hpp`
  - Namespace: `set` - Functions: `unimplemented()` (template container)
- [ ] **FWSMultimap** - `include/datapod/pods/associative/fws_multimap.hpp`
  - Namespace: `fws_multimap` - Functions: `unimplemented()` (template)
- [ ] **MutableFWSMultimap** - `include/datapod/pods/associative/mutable_fws_multimap.hpp`
  - Namespace: `mutable_fws_multimap` - Functions: `unimplemented()` (template)
- [ ] **HashStorage** - `include/datapod/pods/associative/hash_storage.hpp`
  - Namespace: `hash_storage` - Functions: `unimplemented()` (template)

---

## Tree Types (include/datapod/pods/trees/)

- [ ] **BinaryTree** - `include/datapod/pods/trees/binary_tree.hpp`
  - Namespace: `binary_tree` - Functions: `unimplemented()` (template)
- [ ] **NaryTree** - `include/datapod/pods/trees/nary_tree.hpp`
  - Namespace: `nary_tree` - Functions: `unimplemented()` (template)
- [ ] **OrderedMap** - `include/datapod/pods/trees/ordered_map.hpp`
  - Namespace: `ordered_map` - Functions: `unimplemented()` (template)
- [ ] **OrderedSet** - `include/datapod/pods/trees/ordered_set.hpp`
  - Namespace: `ordered_set` - Functions: `unimplemented()` (template)
- [ ] **Trie** - `include/datapod/pods/trees/trie.hpp`
  - Namespace: `trie` - Functions: `unimplemented()` (template)

---

## Adapter Types (include/datapod/pods/adapters/)

- [ ] **Optional** - `include/datapod/pods/adapters/optional.hpp`
  - Namespace: `optional` - Functions: `unimplemented()` (template)
- [ ] **Result** - `include/datapod/pods/adapters/result.hpp`
  - Namespace: `result` - Functions: `unimplemented()` (template)
- [ ] **Variant** - `include/datapod/pods/adapters/variant.hpp`
  - Namespace: `variant` - Functions: `unimplemented()` (template)
- [ ] **Either** - `include/datapod/pods/adapters/either.hpp`
  - Namespace: `either` - Functions: `unimplemented()` (template)
- [ ] **Pair** - `include/datapod/pods/adapters/pair.hpp`
  - Namespace: `pair` - Functions: `unimplemented()` (template)
- [ ] **Tuple** - `include/datapod/pods/adapters/tuple.hpp`
  - Namespace: `tuple` - Functions: `unimplemented()` (template)
- [ ] **UniquePtr** - `include/datapod/pods/adapters/unique_ptr.hpp`
  - Namespace: `unique_ptr` - Functions: `unimplemented()` (template)
- [ ] **SharedPtr** - `include/datapod/pods/adapters/shared_ptr.hpp`
  - Namespace: `shared_ptr` - Functions: `unimplemented()` (template)
- [ ] **NonNull** - `include/datapod/pods/adapters/non_null.hpp`
  - Namespace: `non_null` - Functions: `unimplemented()` (template)
- [ ] **Pin** - `include/datapod/pods/adapters/pin.hpp`
  - Namespace: `pin` - Functions: `unimplemented()` (template)
- [ ] **RefCell** - `include/datapod/pods/adapters/ref_cell.hpp`
  - Namespace: `ref_cell` - Functions: `unimplemented()` (template)
- [ ] **OnceCell** - `include/datapod/pods/adapters/once_cell.hpp`
  - Namespace: `once_cell` - Functions: `unimplemented()` (template)
- [ ] **Lazy** - `include/datapod/pods/adapters/lazy.hpp`
  - Namespace: `lazy` - Functions: `unimplemented()` (template)
- [ ] **MaybeUninit** - `include/datapod/pods/adapters/maybe_uninit.hpp`
  - Namespace: `maybe_uninit` - Functions: `unimplemented()` (template)
- [ ] **Bitset** - `include/datapod/pods/adapters/bitset.hpp`
  - Namespace: `bitset` - Functions: `unimplemented()` (template)
- [ ] **Error** - `include/datapod/pods/adapters/error.hpp`
  - Namespace: `error` - Functions: `unimplemented()` (error type)
- [ ] **COW** - `include/datapod/pods/adapters/cow.hpp`
  - Namespace: `cow` - Functions: `unimplemented()` (copy-on-write)
- [ ] **Conversions** - `include/datapod/pods/adapters/conversions.hpp`
  - Namespace: `conversions` - Functions: `unimplemented()` (utilities)

---

## Temporal Types (include/datapod/pods/temporal/)

- [ ] **Stamp** - `include/datapod/pods/temporal/stamp.hpp`
  - Namespace: `stamp` - Functions: `unimplemented()` (template)
- [ ] **Event** - `include/datapod/pods/temporal/event.hpp`
  - Namespace: `event` - Functions: `make(...)` (if concrete types exist)
- [ ] **Financial** - `include/datapod/pods/temporal/financial.hpp`
  - Namespace: `financial` - Functions: `make(...)` for Tick, OHLCV
- [ ] **Window** - `include/datapod/pods/temporal/window.hpp`
  - Namespace: `window` - Functions: `make(start, end)`
- [ ] **TimeSeries** - `include/datapod/pods/temporal/time_series.hpp`
  - Namespace: `time_series` - Functions: `unimplemented()` (template container)
- [ ] **MultiSeries** - `include/datapod/pods/temporal/multi_series.hpp`
  - Namespace: `multi_series` - Functions: `unimplemented()` (container)
- [ ] **CircularBuffer** - `include/datapod/pods/temporal/circular_buffer.hpp`
  - Namespace: `circular_buffer` - Functions: `unimplemented()` (template container)

---

## Memory Types (include/datapod/pods/memory/)

- [ ] **Arena** - `include/datapod/pods/memory/arena.hpp`
  - Namespace: `arena` - Functions: `unimplemented()` (allocator)
- [ ] **Pool** - `include/datapod/pods/memory/pool.hpp`
  - Namespace: `pool` - Functions: `unimplemented()` (allocator)
- [ ] **Allocator** - `include/datapod/pods/memory/allocator.hpp`
  - Namespace: `allocator` - Functions: `unimplemented()` (allocator)
- [ ] **Ptr** - `include/datapod/pods/memory/ptr.hpp`
  - Namespace: `ptr` - Functions: `unimplemented()` (pointer type)
- [ ] **OffsetPtr** - `include/datapod/pods/memory/offset_ptr.hpp`
  - Namespace: `offset_ptr` - Functions: `unimplemented()` (offset pointer)
- [ ] **MmapVec** - `include/datapod/pods/memory/mmap_vec.hpp`
  - Namespace: `mmap_vec` - Functions: `unimplemented()` (mmap vector)
- [ ] **Paged** - `include/datapod/pods/memory/paged.hpp`
  - Namespace: `paged` - Functions: `unimplemented()` (paged allocator)

---

## Lockfree Types (include/datapod/pods/lockfree/)

- [ ] **RingBuffer** - `include/datapod/pods/lockfree/ring_buffer.hpp`
  - Namespace: `ring_buffer` - Functions: `unimplemented()` (template)

---

## Summary

**Total types identified**: ~120+
**With useful make()**: ~35 (spatial PODs, primitives, robotics)
**With unimplemented()**: ~85+ (templates, containers, adapters)

### Priority Order
1. **Phase 1** (Core Spatial - useful make()): Quaternion ✓, Euler, Point, Pose, Velocity, Acceleration, Transform, State, Size
2. **Phase 2** (Primitives - useful make()): Circle, Rectangle, Line, Segment, Triangle, Square
3. **Phase 3** (Robotics - useful make()): Twist, Wrench, Accel, Odom, Inertia
4. **Phase 4** (Bounding - useful make()): AABB, OBB, BS, BoundingSphere, Box
5. **Phase 5** (Geospatial - useful make()): Geo, Loc, UTM
6. **Phase 6** (Gaussian - useful make()): GaussianPoint, GaussianBox, GaussianCircle, GaussianRectangle
7. **Phase 7** (Temporal - useful make()): Event, Financial, Window
8. **Phase 8** (All remaining - unimplemented()): Templates, containers, adapters, memory types

---

## Notes

- **ALL types get a namespace** - no exceptions
- Types with useful constructors get `make()` functions
- Templates/containers/adapters get `unimplemented()` placeholder
- This provides consistent API surface across entire library
- Namespaces enable future extension without breaking changes
