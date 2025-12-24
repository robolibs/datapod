# Datapod Library Development Plan

## **CRITICAL REQUIREMENTS FOR ALL DATA TYPES**

### **Every data structure MUST have:**

1. **✅ `members()` function** - For reflection and serialization
   ```cpp
   auto members() noexcept { return std::tie(field1, field2, ...); }
   auto members() const noexcept { return std::tie(field1, field2, ...); }
   ```

2. **✅ Test file** - In appropriate `test/` subdirectory
   - Comprehensive test coverage
   - Construction, operations, edge cases
   - Serialization verification

3. **✅ Example file** (if non-trivial)
   - In `examples/` directory
   - Show real-world usage
   - Demonstrate key features

4. **✅ Structs**
   - Always use internal structs from this library instead of std::

---

## Project Structure

```
include/datapod/
├── adapters/        # Container adapters and wrappers
├── associative/     # Hash-based maps and sets
├── core/            # Low-level utilities
├── matrix/          # Linear algebra types
├── memory/          # Memory management
├── reflection/      # Reflection system
├── sequential/      # Linear containers
├── serialization/   # Serialization support
├── spatial/         # Geometry and spatial indexing
├── temporal/        # Time-series containers
└── type_hash/       # Type hashing utilities
```

---

## Status Legend

- ✅ **COMPLETE** - Has members(), test, and example (if needed)
- ⚠️ **NEEDS TEST** - Has members() but missing test
- ⚠️ **NEEDS EXAMPLE** - Has test but could use example
- 📋 **NEEDS MEMBERS** - Missing members() function
- ❌ **INCOMPLETE** - Missing multiple requirements

---

## 1. Sequential Containers (9 files)

Linear, ordered data structures for sequential access.

| File | members() | Test | Example | Status |
|------|-----------|------|---------|--------|
| `sequential/array.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `sequential/bitvec.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `sequential/cstring.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `sequential/string.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `sequential/vector.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `sequential/flat_matrix.hpp` | ❓ | ❌ | ❌ | ❌ NEEDS CHECK |
| `sequential/nvec.hpp` | ❓ | ❌ | ❌ | ❌ NEEDS CHECK |
| `sequential/paged_vecvec.hpp` | ❓ | ❌ | ❌ | ❌ NEEDS CHECK |
| `sequential/vecvec.hpp` | ❓ | ❌ | ❌ | ❌ NEEDS CHECK |

**Priority:** HIGH - flat_matrix, nvec, vecvec, paged_vecvec need review

---

## 2. Associative Containers (5 files)

Hash-based key-value and set structures.

| File | members() | Test | Example | Status |
|------|-----------|------|---------|--------|
| `associative/map.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `associative/set.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `associative/fws_multimap.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `associative/mutable_fws_multimap.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `associative/hash_storage.hpp` | ✅ | N/A | N/A | ✅ COMPLETE (internal) |

**Status:** 100% COMPLETE ✅

---

## 3. Adapters (6 files)

Container adapters and wrappers for type safety.

| File | members() | Test | Example | Status |
|------|-----------|------|---------|--------|
| `adapters/bitset.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `adapters/optional.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `adapters/pair.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `adapters/tuple.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `adapters/unique_ptr.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `adapters/variant.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |

**Status:** 100% COMPLETE ✅

---

## 4. Matrix Types (3 files)

Linear algebra and tensor operations.

| File | members() | Test | Example | Status |
|------|-----------|------|---------|--------|
| `matrix/matrix.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `matrix/scalar.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `matrix/tensor.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |

**Status:** 100% COMPLETE ✅

---

## 5. Spatial Types (49 files)

Geometry types and spatial indexing structures.

### 5.0 SIMD Conversion Support (to_mat/from_mat)

**Status:** Partial (14/40 types have conversions)

All spatial POD types should have `to_mat()` and `from_mat()` methods for SIMD-optimized operations via `datapod::mat::vector` types.

**Completed (14 types):**
- ✅ Point, Velocity, Acceleration, Euler, Size (3-component → `mat::vector<double, 3>`)
- ✅ Quaternion (4-component → `mat::vector<double, 4>`)
- ✅ Twist, Wrench, Accel (6-component → `mat::vector<double, 6>`)
- ✅ Pose (7-component → `mat::vector<double, 7>`)
- ✅ Inertia (10-component → `mat::vector<double, 10>`)
- ✅ State, Odom (13-component → `mat::vector<double, 13>`)
- ✅ Circle (primitives) (4-component → `mat::vector<double, 4>`)
- ✅ Grid (complex) (special: `to_mat<R,C>()` → `mat::matrix<T, R, C>`)

**TODO - Add to_mat/from_mat (26 types):**
- ⚠️ AABB (6: min_point + max_point → `mat::vector<double, 6>`)
- ⚠️ Box (10: pose + size → `mat::vector<double, 10>`)
- ⚠️ OBB (9: center + half_extents + orientation → `mat::vector<double, 9>`)
- ⚠️ BoundingSphere/BS (4: center + radius → `mat::vector<double, 4>`)
- ⚠️ Primitives: Rectangle (12: 4 corners), Square, Triangle, Line, Segment
- ⚠️ Gaussian types (4 files): point, circle, box, rectangle
- ⚠️ Complex: Path, Polygon, Trajectory
- ⚠️ Multi types (3 files): MultiPoint, MultiLinestring, MultiPolygon
- ⚠️ Geo, Linestring, Ring

**Priority:** MEDIUM - Add conversions to remaining simple POD types for SIMD optimization

### 5.1 Core Spatial (9 files)

| File | members() | Test | Example | Status |
|------|-----------|------|---------|--------|
| `spatial/point.hpp` | ✅ | ✅ | ❌ | ⚠️ NEEDS EXAMPLE |
| `spatial/size.hpp` | ✅ | ✅ | ❌ | ⚠️ NEEDS EXAMPLE |
| `spatial/euler.hpp` | ✅ | ✅ | ❌ | ⚠️ NEEDS EXAMPLE |
| `spatial/quaternion.hpp` | ✅ | ✅ | ❌ | ⚠️ NEEDS EXAMPLE |
| `spatial/pose.hpp` | ✅ | ❌ | ❌ | ⚠️ NEEDS TEST |
| `spatial/state.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/velocity.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/acceleration.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/geo.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |

### 5.2 Bounding Volumes (4 files)

| File | members() | Test | Example | Status |
|------|-----------|------|---------|--------|
| `spatial/aabb.hpp` | ✅ | ✅ | ❌ | ⚠️ NEEDS EXAMPLE |
| `spatial/obb.hpp` | ✅ | ✅ | ❌ | ⚠️ NEEDS EXAMPLE |
| `spatial/box.hpp` | ✅ | ✅ | ❌ | ⚠️ NEEDS EXAMPLE |
| `spatial/bounding_sphere.hpp` | ✅ | ❌ | ❌ | ⚠️ NEEDS TEST |
| `spatial/bs.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |

### 5.3 Primitives (6 files)

| File | members() | Test | Example | Status |
|------|-----------|------|---------|--------|
| `spatial/primitives/circle.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/primitives/rectangle.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/primitives/square.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/primitives/triangle.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/primitives/segment.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/primitives/line.hpp` | ✅ | ❌ | ❌ | ⚠️ NEEDS TEST |

### 5.4 Complex Geometry (4 files)

| File | members() | Test | Example | Status |
|------|-----------|------|---------|--------|
| `spatial/linestring.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/ring.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/complex/polygon.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/complex/grid.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/complex/path.hpp` | ✅ | ❌ | ❌ | ⚠️ NEEDS TEST |
| `spatial/complex/trajectory.hpp` | ✅ | ❌ | ❌ | ⚠️ NEEDS TEST |

### 5.5 Multi-Geometry (3 files)

| File | members() | Test | Example | Status |
|------|-----------|------|---------|--------|
| `spatial/multi/multi_point.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/multi/multi_linestring.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/multi/multi_polygon.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |

### 5.6 Gaussian Types (4 files)

| File | members() | Test | Example | Status |
|------|-----------|------|---------|--------|
| `spatial/gaussian/point.hpp` | ✅ | ❌ | ❌ | ⚠️ NEEDS TEST |
| `spatial/gaussian/circle.hpp` | ✅ | ❌ | ❌ | ⚠️ NEEDS TEST |
| `spatial/gaussian/box.hpp` | ✅ | ❌ | ❌ | ⚠️ NEEDS TEST |
| `spatial/gaussian/rectangle.hpp` | ✅ | ❌ | ❌ | ⚠️ NEEDS TEST |

### 5.7 Robot Types (5 files)

| File | members() | Test | Example | Status |
|------|-----------|------|---------|--------|
| `spatial/robot/twist.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/robot/accel.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/robot/wrench.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/robot/inertia.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/robot/odom.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |

### 5.8 Spatial Indexing (2 files)

| File | members() | Test | Example | Status |
|------|-----------|------|---------|--------|
| `spatial/rtree.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `spatial/quadtree.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |

**Spatial Summary:** 41/49 COMPLETE (84%)
- **Missing Tests:** 8 files (pose, bounding_sphere, line, path, trajectory, 4x gaussian)
- **Could Use Examples:** Many geometry types

---

## 6. Temporal Containers (7 files)

Time-series and timestamped data structures.

| File | members() | Test | Example | Status |
|------|-----------|------|---------|--------|
| `temporal/stamp.hpp` | ✅ | ✅ | ✅ | ✅ COMPLETE |
| `temporal/time_series.hpp` | ✅ | ✅ | ❌ | ⚠️ NEEDS EXAMPLE |
| `temporal/circular_buffer.hpp` | ✅ | ✅ | ❌ | ⚠️ NEEDS EXAMPLE |
| `temporal/financial.hpp` | ✅ | ✅ | ❌ | ⚠️ NEEDS EXAMPLE |
| `temporal/window.hpp` | ✅ | ❌ | ❌ | ⚠️ NEEDS TEST |
| `temporal/multi_series.hpp` | ✅ | ❌ | ❌ | ⚠️ NEEDS TEST |
| `temporal/event.hpp` | ✅ | ❌ | ❌ | ⚠️ NEEDS TEST |

**Status:** 4/7 COMPLETE (57%)
- **Missing Tests:** window, multi_series, event
- **Missing Examples:** time_series, circular_buffer, financial

---

## 7. Core Utilities (22 files)

Low-level infrastructure - mostly internal, don't need tests/examples.

| File | Type | Notes |
|------|------|-------|
| `core/aligned_alloc.hpp` | Utility | Memory alignment |
| `core/atomic.hpp` | Utility | Atomic operations |
| `core/bit_counting.hpp` | Utility | Bit manipulation |
| `core/buffer.hpp` | Container | Internal buffer |
| `core/char_traits.hpp` | Utility | String support |
| `core/chunk.hpp` | Utility | Chunk allocation |
| `core/const_iterator.hpp` | Utility | Iterator support |
| `core/decay.hpp` | Utility | Type traits |
| `core/endian.hpp` | Utility | Endian conversion |
| `core/equal_to.hpp` | Utility | Comparison |
| `core/exception.hpp` | Utility | Error handling |
| `core/hash.hpp` | Utility | Hashing |
| `core/indexed.hpp` | Utility | Indexing |
| `core/member_offset.hpp` | Utility | Reflection |
| `core/mmap.hpp` | Utility | Memory mapping |
| `core/mode.hpp` | Utility | Mode flags |
| `core/next_power_of_2.hpp` | Utility | Math utility |
| `core/offset_t.hpp` | Utility | Offset type |
| `core/strong.hpp` | Utility | Strong typing |
| `core/type_traits.hpp` | Utility | Type traits |
| `core/verify.hpp` | Utility | Assertions |

**Status:** Internal utilities - no action needed ✅

---

## 8. Memory Management (5 files)

Memory allocation and smart pointers.

| File | members() | Test | Example | Status |
|------|-----------|------|---------|--------|
| `memory/allocator.hpp` | N/A | ❌ | ❌ | Internal utility |
| `memory/mmap_vec.hpp` | ❓ | ❌ | ❌ | ❌ NEEDS CHECK |
| `memory/offset_ptr.hpp` | ✅ | ✅ | ❌ | ✅ COMPLETE |
| `memory/paged.hpp` | ❓ | ❌ | ❌ | ❌ NEEDS CHECK |
| `memory/ptr.hpp` | ❓ | ❌ | ❌ | ❌ NEEDS CHECK |

**Priority:** MEDIUM - Review mmap_vec, paged, ptr

---

## 9. Reflection System (5 files)

Compile-time reflection utilities.

| File | Type | Test | Notes |
|------|------|------|-------|
| `reflection/arity.hpp` | Utility | ✅ | Field counting |
| `reflection/comparable.hpp` | Utility | ✅ | Comparison generation |
| `reflection/for_each_field.hpp` | Utility | ✅ | Field iteration |
| `reflection/has_members.hpp` | Utility | ✅ | members() detection |
| `reflection/to_tuple.hpp` | Utility | ✅ | Tuple conversion |

**Status:** 100% COMPLETE ✅

---

## 10. Serialization System (3 files)

Serialization support infrastructure.

| File | Type | Test | Notes |
|------|------|------|-------|
| `serialization/buf.hpp` | Utility | ✅ | Buffer management |
| `serialization/serialize.hpp` | Utility | ✅ | Serialization |
| `serialization/serialized_size.hpp` | Utility | ✅ | Size calculation |

**Status:** 100% COMPLETE ✅

---

## 11. Type Hashing (2 files)

Type identification and hashing.

| File | Type | Test | Notes |
|------|------|------|-------|
| `type_hash/type_hash.hpp` | Utility | ✅ | Type hashing |
| `type_hash/type_name.hpp` | Utility | ✅ | Type names |

**Status:** 100% COMPLETE ✅

---

## 12. SIMD Conversion Support (to_mat/from_mat)

Zero-copy conversions between spatial types and SIMD-aligned `mat::vector<double, N>`.

### 12.1 Completed (23 types)

**Core Spatial (5 types):**
- ✅ `Point` → `mat::vector<double, 3>` (x, y, z)
- ✅ `Velocity` → `mat::vector<double, 3>` (vx, vy, vz)
- ✅ `Acceleration` → `mat::vector<double, 3>` (ax, ay, az)
- ✅ `Euler` → `mat::vector<double, 3>` (roll, pitch, yaw)
- ✅ `Size` → `mat::vector<double, 3>` (x, y, z)

**Rotation & Pose (2 types):**
- ✅ `Quaternion` → `mat::vector<double, 4>` (x, y, z, w)
- ✅ `Pose` → `mat::vector<double, 7>` (point(3), quat(4))

**State Representation (2 types):**
- ✅ `State` → `mat::vector<double, 13>` (pose(7), velocity(3), angular_velocity(3))
- ✅ `Odom` → `mat::vector<double, 13>` (pose(7), twist(6))

**Robot Dynamics (3 types):**
- ✅ `Twist` → `mat::vector<double, 6>` (linear(3), angular(3))
- ✅ `Wrench` → `mat::vector<double, 6>` (force(3), torque(3))
- ✅ `Accel` → `mat::vector<double, 6>` (linear(3), angular(3))
- ✅ `Inertia` → `mat::vector<double, 10>` (mass, com(3), inertia_matrix(6))

**Bounding Volumes (4 types):**
- ✅ `AABB` → `mat::vector<double, 6>` (min_point(3), max_point(3))
- ✅ `OBB` → `mat::vector<double, 9>` (center(3), half_extents(3), euler(3))
- ✅ `Box` → `mat::vector<double, 10>` (pose(7), size(3))
- ✅ `BS` (BoundingSphere) → `mat::vector<double, 4>` (center(3), radius)

**Primitives (5 types):**
- ✅ `Circle` → `mat::vector<double, 4>` (center(2), radius, z)
- ✅ `Rectangle` → `mat::vector<double, 12>` (4 points × 3)
- ✅ `Square` → `mat::vector<double, 4>` (center(3), side)
- ✅ `Triangle` → `mat::vector<double, 9>` (3 points × 3)
- ✅ `Line` → `mat::vector<double, 6>` (origin(3), direction(3))
- ✅ `Segment` → `mat::vector<double, 6>` (start(3), end(3))

**Special Cases (1 type):**
- ✅ `Grid<T>` → `mat::matrix<T, R, C>` (compile-time dimensions via `to_mat<R,C>()`)

### 12.2 Remaining (17 types - LOW PRIORITY)

**Gaussian Types (4 files):**
- ❌ `gaussian/point.hpp` - Variable size (mean + covariance)
- ❌ `gaussian/circle.hpp` - Variable size
- ❌ `gaussian/box.hpp` - Variable size
- ❌ `gaussian/rectangle.hpp` - Variable size

**Complex Types (3 files):**
- ❌ `complex/path.hpp` - Variable-length vector of Points
- ❌ `complex/polygon.hpp` - Variable-length rings
- ❌ `complex/trajectory.hpp` - Variable-length states

**Multi Types (3 files):**
- ❌ `multi/multi_point.hpp` - Variable-length
- ❌ `multi/multi_linestring.hpp` - Variable-length
- ❌ `multi/multi_polygon.hpp` - Variable-length

**Other (7 types):**
- ❌ `LineString` - Variable-length
- ❌ `Ring` - Variable-length
- ❌ `QuadTree` - Hierarchical structure
- ❌ `RTree` - Hierarchical structure
- ❌ `TimeSeries` - Variable-length temporal
- ❌ `CircularBuffer` - Variable-length temporal
- ❌ `Financial` - Variable-length temporal

**Status:** 23/40 types have SIMD conversions (57.5%)
- **Note:** Variable-length types (vectors, trees) don't benefit from fixed-size SIMD conversions
- **Focus:** Fixed-size spatial/robot types ✅ COMPLETE

---

## 13. Root Files (2 files)

| File | Type | Notes |
|------|------|-------|
| `datapod.hpp` | Header | Main include |
| `hashing.hpp` | Utility | Hash functions |

**Status:** Complete ✅

---

## Summary Statistics

### Overall Completion

| Category | Total Files | Complete | Needs Work | % Complete |
|----------|-------------|----------|------------|------------|
| Sequential | 9 | 5 | 4 | 56% |
| Associative | 5 | 5 | 0 | 100% |
| Adapters | 6 | 6 | 0 | 100% |
| Matrix | 3 | 3 | 0 | 100% |
| Spatial | 49 | 41 | 8 | 84% |
| Temporal | 7 | 4 | 3 | 57% |
| Memory | 5 | 1 | 4 | 20% |
| SIMD Conversions | 40 | 23 | 17 | 57.5% |
| **Total User-Facing** | **84** | **65** | **19** | **77%** |

### Test Coverage

- ✅ **78/79 tests passing (98.7%)**
- **Missing Tests:** ~15 files
- **Missing Examples:** ~25 files (many spatial/temporal types could benefit)

### SIMD Coverage

- ✅ **23/40 fixed-size types have to_mat/from_mat (57.5%)**
- **All fixed-size spatial/robot types COMPLETE**
- **Remaining:** Variable-length types (low priority)

---

## Next Priorities

### HIGH Priority (Core Functionality)

1. **Sequential containers** - Review flat_matrix, nvec, vecvec, paged_vecvec
2. **Temporal tests** - Add tests for window, multi_series, event
3. **Spatial tests** - Add tests for missing 8 files

### MEDIUM Priority (Polish)

4. **Memory types** - Review mmap_vec, paged, ptr
5. **Examples** - Add examples for temporal types
6. **Examples** - Add examples for key spatial types

### LOW Priority (Nice to Have)

7. **More examples** - Additional usage demonstrations
8. **Documentation** - Inline documentation improvements

---

## Development Workflow

For each incomplete file:

1. **Check members()** - Verify both const and non-const overloads
2. **Use datepod inernal structures instead of std::
3. **Create test** - In appropriate test/ subdirectory
   - Construction and basic operations
   - Edge cases and error conditions
   - Serialization round-trip
4. **Create example** (if non-trivial)
   - Real-world usage scenario
   - Key features demonstration
5. **Run tests** - `make test` should show 100% pass
6. **Update this plan** - Mark as complete ✅

---

## Success Criteria

✅ Every user-facing type has `members()` for serialization  
✅ Every user-facing type has comprehensive tests  
✅ Non-trivial types have usage examples  
✅ 100% test pass rate  
✅ All types are POD-compatible where possible  
✅ Consistent naming (CamelCase types, snake_case methods)
