# Concord ↔ Datapod Type Mapping

## Direct Name Mappings

| Concord Type | Datapod Type | Location | Notes |
|--------------|--------------|----------|-------|
| `Point` | `Point` | `spatial/point.hpp` | ✅ Same |
| `Line` | `Segment` | `spatial/primitives/segment.hpp` | ⚠️ Concord's "Line" = datapod's "Segment" |
| *(none)* | `Line` | `spatial/primitives/line.hpp` | ✨ NEW - infinite line |
| `Circle` | `Circle` | `spatial/primitives/circle.hpp` | ✅ Same |
| `Rectangle` | `Rectangle` | `spatial/primitives/rectangle.hpp` | ✅ Same |
| `Square` | `Square` | `spatial/primitives/square.hpp` | ✅ Same |
| `Triangle` | `Triangle` | `spatial/primitives/triangle.hpp` | ✅ Same |
| `Polygon` | `Polygon` | `spatial/complex/polygon.hpp` | ✅ Same |
| `AABB` | `AABB` | `spatial/aabb.hpp` | ✅ Same |
| `OBB` / `Bound` | `OBB` / `Box` | `spatial/obb.hpp` / `spatial/box.hpp` | ⚠️ Concord's "Bound" = datapod's "Box" |
| `BoundingSphere` | `BoundingSphere` | `spatial/bounding_sphere.hpp` | ✅ Same |
| `Euler` | `Euler` | `spatial/euler.hpp` | ✅ Same |
| `Quaternion` | `Quaternion` | `spatial/quaternion.hpp` | ✅ Same |
| `Pose` | `Pose` | `spatial/pose.hpp` | ✅ Same |
| `State` | `State` | `spatial/state.hpp` | ✅ Same |
| `Size` | `Size` | `spatial/size.hpp` | ✅ Same |

## New Types in Datapod (not in Concord)

| Datapod Type | Location | Purpose |
|--------------|----------|---------|
| `Line` | `spatial/primitives/line.hpp` | Infinite line (origin + direction) |
| `Linestring` | `spatial/linestring.hpp` | Open path (sequence of points) |
| `Ring` | `spatial/ring.hpp` | Closed polygon boundary |
| `MultiPoint` | `spatial/multi/multi_point.hpp` | Collection of points |
| `MultiLinestring` | `spatial/multi/multi_linestring.hpp` | Collection of linestrings |
| `MultiPolygon` | `spatial/multi/multi_polygon.hpp` | Collection of polygons |

## Types That Stay in Concord (too domain-specific)

| Concord Type | Reason to Keep in Concord |
|--------------|---------------------------|
| `Path` | Domain-specific: sequence of `Pose` for navigation |
| `Trajectory` | Domain-specific: sequence of `State` for motion planning |
| `Grid<T>` | Complex template with domain logic (spatial hashing, polygon overlap) |
| `WGS` / `Datum` / `ECEF` / `ENU` | Geographic/geodetic coordinate systems |

---

## Method Implementation Plan

### 1. Point (`spatial/point.hpp`)

**Add these methods:**
```cpp
inline double magnitude() const;
inline double distance_to(const Point& other) const;
inline double distance_to_2d(const Point& other) const;
inline bool is_set() const;  // Check if not at origin
```

---

### 2. Segment (`spatial/primitives/segment.hpp`)

**Concord has `Line::length()`**  
**Add these methods:**
```cpp
inline double length() const;
inline Point midpoint() const;
inline Point closest_point(const Point& p) const;
inline double distance_to(const Point& p) const;
```

---

### 3. Line (`spatial/primitives/line.hpp`)

**NEW type in datapod (infinite line)**  
**Add these methods:**
```cpp
inline Point closest_point(const Point& p) const;
inline double distance_to(const Point& p) const;
```

---

### 4. Circle (`spatial/primitives/circle.hpp`)

**Add these methods:**
```cpp
inline double area() const;
inline double perimeter() const;  // circumference
inline bool contains(const Point& p) const;
// OPTIONAL: Vector<Point> as_polygon(int segments = 32) const;
```

---

### 5. Rectangle (`spatial/primitives/rectangle.hpp`)

**Add these methods:**
```cpp
inline double area() const;
inline double perimeter() const;
inline bool contains(const Point& p) const;
inline Array<Point, 4> get_corners() const;
```

---

### 6. Square (`spatial/primitives/square.hpp`)

**Add these methods:**
```cpp
inline double area() const;
inline double perimeter() const;
inline double diagonal() const;
inline bool contains(const Point& p) const;
```

---

### 7. Triangle (`spatial/primitives/triangle.hpp`)

**Add these methods:**
```cpp
inline double area() const;
inline double perimeter() const;
inline bool contains(const Point& p) const;
// OPTIONAL: bool intersects_circle(const Point& center, double radius) const;
```

---

### 8. Polygon (`spatial/complex/polygon.hpp`)

**Add these methods:**
```cpp
inline double area() const;              // Shoelace formula
inline double perimeter() const;
inline bool contains(const Point& p) const;  // Ray casting algorithm
inline size_t num_vertices() const;
inline bool is_valid() const;            // has >= 3 vertices
inline AABB get_aabb() const;
inline Box get_obb() const;              // Oriented bounding box
```

---

### 9. AABB (`spatial/aabb.hpp`)

**Add these methods:**
```cpp
inline bool contains(const Point& p) const;
inline bool intersects(const AABB& other) const;
inline Point center() const;
inline Size size() const;
inline double volume() const;
inline double surface_area() const;
inline double area() const;              // 2D area (xy plane)
inline void expand(const Point& p);
inline AABB union_with(const AABB& other) const;
// OPTIONAL: 
// inline double distance_to(const Point& p) const;
// inline Array<Point, 8> corners() const;
// static AABB from_points(const Vector<Point>& points);
```

---

### 10. OBB / Box (`spatial/obb.hpp` / `spatial/box.hpp`)

**Datapod has both `OBB` and `Box` - clarify which to use**  
**Add these methods:**
```cpp
inline bool contains(const Point& p) const;
inline Array<Point, 8> corners() const;
```

---

### 11. BoundingSphere (`spatial/bounding_sphere.hpp`)

**Add these methods:**
```cpp
inline bool contains(const Point& p) const;
inline bool intersects(const BoundingSphere& other) const;
inline double volume() const;
inline double surface_area() const;
```

---

## Implementation Priority

### ✅ PHASE 1 (Start today - most critical):

1. **Point** - magnitude, distance_to, distance_to_2d
2. **Segment** - length, midpoint, closest_point
3. **Circle** - area, perimeter, contains
4. **Triangle** - area, perimeter, contains
5. **AABB** - contains, intersects, center, volume

### 🟡 PHASE 2 (Next session):

6. **Polygon** - area, perimeter, contains (ray casting!)
7. **Rectangle/Square** - area, perimeter, contains
8. **AABB extended** - expand, union_with, distance_to
9. **OBB/Box** - contains, corners
10. **BoundingSphere** - contains, intersects

### 🔵 PHASE 3 (Optional enhancements):

11. **Line** (infinite) - closest_point, distance_to
12. **Circle** - as_polygon
13. **Triangle** - intersects_circle
14. **Static factories** - AABB::from_points, etc.

---

## Key Clarifications

### Q: What's the difference between OBB and Box in datapod?

Need to check datapod code - they might be:
- **Box** = simple struct with pose + size
- **OBB** = oriented bounding box with transformation methods

### Q: Should methods be inline or in .cpp files?

**Answer:** All methods should be **inline in headers** to keep types POD-compatible and header-only.

### Q: What about operator overloads?

Most operators (==, !=, +, -, *, /) can be auto-generated via reflection, but geometric methods need explicit implementation.

---

## Testing Strategy

```
test/spatial/
├── point_test.cpp               # NEW tests for magnitude, distance_to
├── segment_test.cpp             # ✅ EXISTS - add length, closest_point tests
├── geometry_test.cpp            # ✅ EXISTS - add tests for new methods
├── primitives/
│   ├── circle_test.cpp          # NEW - area, perimeter, contains
│   ├── rectangle_test.cpp       # NEW - area, perimeter, contains
│   ├── triangle_test.cpp        # NEW - area, perimeter, contains
│   └── square_test.cpp          # NEW - area, perimeter, diagonal, contains
├── polygon_test.cpp             # NEW - area, perimeter, contains (ray casting!)
└── bounding_test.cpp            # NEW - AABB, OBB, BoundingSphere methods
```

