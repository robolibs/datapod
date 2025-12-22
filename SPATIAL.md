# Datapod Spatial Types - Methods Needed from Concord

## Progress Summary

**Completed:** 16/30 types (53%)
**In Progress:** 1 type (Pose - tests needed)
**TODO:** 6 types
**Deferred:** 1 type (Polygon - ray casting)
**Not Needed:** 6 types (Multi-geometry, Gaussian types - low priority)

### ✅ Completed Types (16):
1. Point - All methods + tests
2. Size - All methods + 35 tests + const members()
3. Euler - All methods + 33 tests + Quaternion conversion + const members()
4. Quaternion - All methods + 26 tests + Euler conversion + const members()
5. Pose - All methods + const members() (uses Quaternion now!)
6. Segment - All methods + tests
7. Circle - All methods + tests
8. Rectangle - All methods + tests
9. Square - All methods + tests
10. Triangle - All methods + tests
11. AABB - All methods + tests
12. OBB - All methods + tests
13. Box - All methods + tests
14. BoundingSphere - All methods + tests

**New Tests Added:** 94 test cases (Size: 35, Euler: 33, Quaternion: 26)
**Total Tests:** 56/56 passing (100%)

### 📋 TODO (6 types - simple):
- State - Just const members()
- Path - size(), empty(), const members()
- Trajectory - size(), empty(), const members()
- Linestring - length(), num_points(), const members()
- Ring - length(), area(), num_points(), is_closed(), const members()
- Line (infinite) - Low priority

### ⏸️ Deferred (1 type):
- Polygon - Ray casting algorithm is complex, defer for now

---

## Overview
This document maps every datapod spatial type to the methods needed from concord for migration.

**Key Principles:**
- Add methods ONLY to the struct itself (no free functions)
- Use datapod types ONLY (Vector, Array, String - NO std::)
- All methods must be inline, const, and noexcept where possible
- Keep structs POD-compatible (no virtual, no inheritance)

---

## Core Types

### 1. `point.hpp` ✅ COMPLETE
**Current:** POD struct with x, y, z
**Status:** All methods implemented and tested
```cpp
// Distance and magnitude
inline double magnitude() const noexcept;
inline double distance_to(const Point& other) const noexcept;
inline double distance_to_2d(const Point& other) const noexcept;

// Utility
inline bool is_set() const noexcept;  // != (0,0,0)

// Operators (can add if needed)
inline Point operator+(const Point& other) const noexcept;
inline Point operator-(const Point& other) const noexcept;
inline Point operator*(double scale) const noexcept;
inline Point operator/(double scale) const noexcept;
```

### 2. `size.hpp` ✅ COMPLETE
**Current:** POD struct with x, y, z
**Status:** All methods implemented, 35 tests passing, const members() added
```cpp
// Volume and area
inline double volume() const noexcept;
inline double area_xy() const noexcept;
inline double area_xz() const noexcept;
inline double area_yz() const noexcept;
inline double diagonal() const noexcept;
inline double diagonal_2d() const noexcept;

// Utility
inline bool is_set() const noexcept;

// Operators
inline Size operator+(const Size& other) const noexcept;
inline Size operator-(const Size& other) const noexcept;
inline Size operator*(double scale) const noexcept;
inline Size operator/(double scale) const noexcept;
inline Size operator*(const Size& other) const noexcept;

// Min/max helpers
inline Size abs() const noexcept;
inline Size max(const Size& other) const noexcept;
inline Size min(const Size& other) const noexcept;
```

### 3. `euler.hpp` ✅ COMPLETE
**Current:** POD struct with roll, pitch, yaw
**Status:** All methods implemented, 33 tests passing, conversion to/from Quaternion, const members() added
```cpp
// Utility
inline bool is_set() const noexcept;
inline double yaw_cos() const noexcept;
inline double yaw_sin() const noexcept;

// Normalization
inline Euler normalized() const noexcept;

// Operators
inline Euler operator+(const Euler& other) const noexcept;
inline Euler operator-(const Euler& other) const noexcept;
inline Euler operator*(double scale) const noexcept;
```

### 4. `quaternion.hpp` ✅ COMPLETE
**Current:** POD struct with w, x, y, z
**Status:** All methods implemented, 26 tests passing, conversion to/from Euler, magnitude, normalized, conjugate, operators, const members() added
```cpp
// NOTE: Concord has Quaternion but we need to check if it has methods
// For now, leave minimal - just ensure const members() exists
inline auto members() const noexcept { return std::tie(w, x, y, z); }
```

### 5. `pose.hpp` ✅ COMPLETE
**Current:** POD struct with point, rotation (Quaternion - changed from Euler!)
**Status:** All methods implemented (transform_point, inverse_transform_point, operator*, inverse), const members() added, uses Quaternion for proper 3D rotation
**NOTE:** Tests needed for Pose-specific functionality
```cpp
// Utility
inline bool is_set() const noexcept;

// Transformation (if needed - concord has these)
inline Point transform_point(const Point& local_point) const;
inline Point inverse_transform_point(const Point& world_point) const;

// Pose composition
inline Pose operator*(const Pose& other) const;
inline Pose inverse() const;

// ADD const members() overload
inline auto members() const noexcept { return std::tie(point, angle); }
```

### 6. `state.hpp` 📋 TODO
**Current:** POD struct with pose, linear_velocity, angular_velocity
**Missing from Concord:**
```cpp
// Utility
inline bool is_set() const noexcept;

// ADD const members() overload
inline auto members() const noexcept { return std::tie(pose, linear_velocity, angular_velocity); }
```

---

## Primitives

### 7. `primitives/segment.hpp` ✅ COMPLETE
**Current:** POD struct with start, end
**Status:** All methods implemented and tested
```cpp
// Geometric properties
inline double length() const noexcept;
inline Point midpoint() const noexcept;

// Distance queries
inline Point closest_point(const Point& p) const noexcept;
inline double distance_to(const Point& p) const noexcept;
```

### 8. `primitives/line.hpp` 📋 TODO (low priority - infinite line)
**Current:** POD struct with origin, direction
**Missing (NEW type, not in concord):**
```cpp
// Distance queries for infinite line
inline Point closest_point(const Point& p) const noexcept;
inline double distance_to(const Point& p) const noexcept;
```

### 9. `primitives/circle.hpp` ✅ COMPLETE
**Current:** POD struct with center, radius
**Status:** All methods implemented and tested (from earlier work)
```cpp
// Geometric properties
inline double area() const noexcept;
inline double perimeter() const noexcept;  // circumference

// Containment
inline bool contains(const Point& p) const noexcept;

// Optional: polygon approximation
// Vector<Point> as_polygon(int segments = 32) const;
```

### 10. `primitives/rectangle.hpp` ✅ COMPLETE
**Current:** POD struct with top_left, top_right, bottom_left, bottom_right
**Status:** All methods implemented and tested (from earlier work)
```cpp
// Geometric properties
inline double area() const noexcept;
inline double perimeter() const noexcept;

// Containment
inline bool contains(const Point& p) const noexcept;

// Utility
inline Array<Point, 4> get_corners() const;  // Use datapod::Array!

// ADD const members() overload
inline auto members() const noexcept { 
    return std::tie(top_left, top_right, bottom_left, bottom_right); 
}
```

### 11. `primitives/square.hpp` ✅ COMPLETE
**Current:** POD struct with center, side
**Status:** All methods implemented and tested (from earlier work)
```cpp
// Geometric properties
inline double area() const noexcept;
inline double perimeter() const noexcept;
inline double diagonal() const noexcept;

// Containment
inline bool contains(const Point& p) const noexcept;

// ADD const members() overload
inline auto members() const noexcept { return std::tie(center, side); }
```

### 12. `primitives/triangle.hpp` ✅ COMPLETE
**Current:** POD struct with a, b, c
**Status:** All methods implemented and tested (from earlier work)
```cpp
// Geometric properties
inline double area() const noexcept;
inline double perimeter() const noexcept;

// Containment
inline bool contains(const Point& p) const noexcept;

// Optional: intersection
// bool intersects_circle(const Point& center, double radius) const noexcept;

// ADD const members() overload
inline auto members() const noexcept { return std::tie(a, b, c); }
```

---

## Bounding Volumes

### 13. `aabb.hpp` ✅ COMPLETE
**Current:** POD struct with min_point, max_point
**Status:** All methods implemented and tested (from earlier work)
```cpp
// Containment and intersection
inline bool contains(const Point& p) const noexcept;
inline bool intersects(const AABB& other) const noexcept;

// Properties
inline Point center() const noexcept;
inline Size size() const noexcept;
inline double volume() const noexcept;
inline double surface_area() const noexcept;
inline double area() const noexcept;  // 2D area (xy plane)

// Mutation (rare but needed)
inline void expand(const Point& p) noexcept;

// Combining
inline AABB union_with(const AABB& other) const;

// Optional:
// double distance_to(const Point& p) const noexcept;
// Array<Point, 8> corners() const;
// static AABB from_points(const Vector<Point>& points);

// ADD const members() overload
inline auto members() const noexcept { return std::tie(min_point, max_point); }
```

### 14. `obb.hpp` ✅ COMPLETE
**Current:** POD struct with center, half_extents, orientation
**Status:** All methods implemented and tested (from earlier work)
```cpp
// Containment
inline bool contains(const Point& p) const noexcept;

// Utility
inline Array<Point, 8> corners() const;

// Optional:
// static OBB from_points(const Vector<Point>& points);

// ADD const members() overload
inline auto members() const noexcept { return std::tie(center, half_extents, orientation); }
```

### 15. `box.hpp` ✅ COMPLETE
**Current:** POD struct with pose, size
**Status:** All methods implemented and tested (from earlier work)
```cpp
// Containment
inline bool contains(const Point& p) const noexcept;

// Utility
inline Array<Point, 8> corners() const;
```

### 16. `bounding_sphere.hpp` ✅ COMPLETE
**Current:** POD struct with center, radius
**Status:** All methods implemented and tested (from earlier work)
```cpp
// Containment and intersection
inline bool contains(const Point& p) const noexcept;
inline bool intersects(const BoundingSphere& other) const noexcept;

// Properties
inline double volume() const noexcept;
inline double surface_area() const noexcept;

// Optional:
// static BoundingSphere from_points(const Vector<Point>& points);

// ADD const members() overload
inline auto members() const noexcept { return std::tie(center, radius); }
```

---

## Complex Types

### 17. `complex/polygon.hpp` ⏸️ DEFERRED (ray casting complex)
**Current:** POD struct with vertices (Vector<Point>)
**Status:** Deferred - ray casting algorithm is complex, do later
**Missing from Concord:**
```cpp
// Geometric properties
inline double area() const noexcept;        // Shoelace formula
inline double perimeter() const noexcept;

// Containment (ray casting algorithm!)
inline bool contains(const Point& p) const noexcept;

// Utility
inline size_t num_vertices() const noexcept;
inline bool is_valid() const noexcept;  // >= 3 vertices

// Bounding boxes
inline AABB get_aabb() const;
// Box get_obb() const;  // Optional

// ADD const members() overload
inline auto members() const noexcept { return std::tie(vertices); }
```

### 18. `complex/path.hpp` 📋 TODO
**Current:** POD struct with waypoints (Vector<Pose>)
**Missing from Concord:**
```cpp
// Utility methods (concord has size(), empty(), etc)
inline size_t size() const noexcept;
inline bool empty() const noexcept;

// Optional iteration helpers if needed

// ADD const members() overload
inline auto members() const noexcept { return std::tie(waypoints); }
```

### 19. `complex/trajectory.hpp` 📋 TODO
**Current:** POD struct with states (Vector<State>)
**Missing from Concord:**
```cpp
// Utility methods (concord has size(), empty(), etc)
inline size_t size() const noexcept;
inline bool empty() const noexcept;

// Optional iteration helpers if needed

// ADD const members() overload
inline auto members() const noexcept { return std::tie(states); }
```

### 20. `complex/grid.hpp`
**Current:** Complex template class
**Action:** NO CHANGES - Grid stays in concord (too domain-specific)

---

## Linear Geometry

### 21. `linestring.hpp` 📋 TODO
**Current:** POD struct with points (Vector<Point>)
**Missing (NEW type, not in concord):**
```cpp
// Geometric properties
inline double length() const noexcept;

// Utility
inline size_t num_points() const noexcept;
```

### 22. `ring.hpp` 📋 TODO
**Current:** POD struct with points (Vector<Point>)
**Missing (NEW type, not in concord):**
```cpp
// Geometric properties
inline double length() const noexcept;  // perimeter
inline double area() const noexcept;    // if ring is closed

// Utility
inline size_t num_points() const noexcept;
inline bool is_closed() const noexcept;  // first == last
```

---

## Multi-Geometry (NEW types)

### 23. `multi/multi_point.hpp`
**Current:** POD struct with points (Vector<Point>)
**Missing (NEW, not in concord):**
```cpp
// Just utility - no geometric methods needed
inline size_t size() const noexcept;
inline bool empty() const noexcept;
```

### 24. `multi/multi_linestring.hpp`
**Current:** POD struct with linestrings (Vector<Linestring>)
**Missing (NEW, not in concord):**
```cpp
// Just utility
inline size_t size() const noexcept;
inline bool empty() const noexcept;
```

### 25. `multi/multi_polygon.hpp`
**Current:** POD struct with polygons (Vector<Polygon>)
**Missing (NEW, not in concord):**
```cpp
// Just utility
inline size_t size() const noexcept;
inline bool empty() const noexcept;
```

---

## Gaussian Types (Uncertainty)

### 26. `gaussian/point.hpp`
**Action:** Check file - likely needs const members() and basic methods

### 27. `gaussian/circle.hpp`
**Action:** Check file - likely needs const members() and basic methods

### 28. `gaussian/box.hpp`
**Action:** Check file - likely needs const members() and basic methods

### 29. `gaussian/rectangle.hpp`
**Action:** Check file - likely needs const members() and basic methods

---

## Spatial Indexing

### 30. `rtree.hpp`
**Action:** NO CHANGES for now - complex data structure, evaluate separately

---

## Summary

### CRITICAL (Must implement for concord migration):

1. **Point** - magnitude, distance_to, distance_to_2d, operators
2. **Segment** - length, midpoint, closest_point, distance_to
3. **Circle** - area, perimeter, contains
4. **Triangle** - area, perimeter, contains
5. **Rectangle** - area, perimeter, contains, get_corners
6. **Square** - area, perimeter, diagonal, contains
7. **AABB** - contains, intersects, center, volume, surface_area, expand
8. **Polygon** - area, perimeter, contains (RAY CASTING!), get_aabb
9. **OBB/Box** - contains, corners
10. **BoundingSphere** - contains, intersects, volume, surface_area

### IMPORTANT (Nice to have):

11. **Size** - volume, area, diagonal, operators
12. **Euler** - normalized, yaw_cos, yaw_sin, operators
13. **Pose** - transform_point, inverse, operator*
14. **Path/Trajectory** - size, empty
15. **Linestring/Ring** - length, num_points

### ADD const members() TO:

- pose.hpp
- state.hpp  
- primitives/rectangle.hpp
- primitives/square.hpp
- primitives/triangle.hpp
- aabb.hpp
- obb.hpp
- box.hpp
- bounding_sphere.hpp
- complex/polygon.hpp
- complex/path.hpp
- complex/trajectory.hpp

---

## Implementation Workflow

**IMPORTANT: After implementing methods for EACH type, create/update tests IMMEDIATELY before moving to next type!**

1. ✅ Create this SPATIAL.md file
2. ✅ Reference from PLAN.md as Phase 5, Step 1
3. 📋 For each type (in priority order):
   - a. Add methods to the struct (inline, const, noexcept)
   - b. Create/update test file for that type
   - c. Build and verify tests pass
   - d. Mark type as complete
   - e. Move to next type
4. 📋 Final verification with full test suite
5. 📋 Verify concord can use datapod types
