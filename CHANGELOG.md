# Changelog

## [0.0.19] - 2026-01-02

### <!-- 0 -->⛰️  Features

- Implement Vectra for small object optimization
- Implement Pool segregated free-list allocator
- Implement Arena linear allocator with bump-pointer semantics

## [0.0.18] - 2026-01-01

### <!-- 0 -->⛰️  Features

- Add serialization for C-style arrays

## [0.0.17] - 2026-01-01

### <!-- 1 -->🐛 Bug Fixes

- Properly handle SIMD and stack-allocated vectors

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Add architecture-specific SIMD flags
- Add support for pkg-config dependencies

## [0.0.16] - 2025-12-31

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Improve compiler selection and warning handling
- Add Zig as a supported build system
- Streamline build system and project configuration

### Build

- Update .envrc for build environment
- Refactor release command in Makefile
- Simplify release process and version management

## [0.0.15] - 2025-12-29

### <!-- 0 -->⛰️  Features

- Implement Trie (prefix tree) data structure
- Implement N-ary tree data structure
- Implement index-based BinaryTree
- Add IndexedHeap for priority queues with key updates
- Add OrderedSet container based on Red-Black Tree
- Add `OrderedMap` data structure
- Introduce Deque data structure
- Add heap/priority queue data structure
- Add `List` data structure
- Add ForwardList sequential container

## [0.0.14] - 2025-12-28

### <!-- 1 -->🐛 Bug Fixes

- Remove rerun_sdk support from build systems

### Build

- Optimize xmake compilation speed

## [0.0.13] - 2025-12-28

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Update build and configuration scripts

## [0.0.12] - 2025-12-27

### <!-- 0 -->⛰️  Features

- Introduce `mat::quaternion` and refactor `spatial::Quaternion`

## [0.0.11] - 2025-12-27

### <!-- 0 -->⛰️  Features

- Add `Loc` and `Utm` spatial data structures

## [0.0.10] - 2025-12-27

### <!-- 0 -->⛰️  Features

- Add partially dynamic tensor support
- Add Eigen-style dynamic matrix and tensor types
- Add complex spatial `Layer` type
- Move rigid body transform to spatial module

## [0.0.9] - 2025-12-26

### <!-- 0 -->⛰️  Features

- Introduce new mathematical numeric types and utilities

## [0.0.8] - 2025-12-24

### <!-- 0 -->⛰️  Features

- Implement heap allocation for vector, matrix, and tensor types
- Feat: Add extensive SIMD conversions for geometry types

### <!-- 4 -->⚡ Performance

- Introduce heap allocation for large matrix types

## [0.0.7] - 2025-12-24

### <!-- 0 -->⛰️  Features

- Refactor matrix and tensor for composition and SIMD

### <!-- 3 -->📚 Documentation

- Update README with architecture and installation instructions
- Add license to repo and update README

## [0.0.6] - 2025-12-24

### <!-- 0 -->⛰️  Features

- Add SIMD conversion for spatial data structures
- Add tensor and matrix types into `datapod::mat` namespace

### <!-- 2 -->🚜 Refactor

- Improve `to_mat` for grid-to-matrix conversion

## [0.0.5] - 2025-12-23

### <!-- 2 -->🚜 Refactor

- Refactor identity matrix setting and update acknowledgments

### Build

- Remove unnecessary object file

## [0.0.4] - 2025-12-23

### <!-- 0 -->⛰️  Features

- Feat: Add short namespace alias for datapod

### <!-- 2 -->🚜 Refactor

- Consolidate headers and rename test structs

### <!-- 3 -->📚 Documentation

- Outline project status and future development

## [0.0.3] - 2025-12-23

### <!-- 0 -->⛰️  Features

- Add basic sequential data structures
- Introduce Rust-inspired error handling with `Result` and `Error`
- Introduce temporal data structures and remove old docs
- Introduce geographic and odometry data structures
- Feat: Add spatial vector and inertia primitives
- Introduce 3D velocity and acceleration types
- Add tensor, matrix, and scalar data structures

### <!-- 3 -->📚 Documentation

- Introduce `Result` and `Error` types with examples
- Readme: Update project description

## [0.0.2] - 2025-12-23

### <!-- 0 -->⛰️  Features

- Add a generic QuadTree implementation and unit tests
- Introduce RTree and PointRTree spatial indexing
- Implement Polygon class with OBB, iterators, and tests
- Introduce a comprehensive and robust Grid data structure
- Add geometric methods to Polygon struct
- Add `const` overloads for Gaussian spatial `members()`
- Implement core spatial types and utilities
- Add rotations utility and test cases
- Add geometric property calculations to bounding shapes
- Add geometric properties to shape primitives
- Introduce fundamental geometric methods for Point spatial type
- Introduce new geometric primitives and their tests
- Implement high-frequency trading data structures
- Implement a circular time buffer
- Implement TimeSeries container for columnar data storage
- Introduce a generic timestamping mechanism
- Implement `UniquePtr` and `Variant` with examples and tests
- Implement a robust and feature-complete Cstring class
- Implement and test Bitvec container class
- Implement a comprehensive and robust array class
- Implement comprehensive `Bitset` functionality
- Enhance `Pair` and `Tuple` with C++23 monadic operations
- Implement `HashMap` and `HashSet` with improved `HashStorage`
- Implement comprehensive `String` and `Vector` utilities
- Implement extensive `BasicVector` functionality and tests
- Expose struct members for structured bindings
- Add serialization support for container classes
- Support `members()` for `to_tuple` reflection
- Introduce utilities for SFINAE-based member detection
- Introduce new POD structs for all geometry types
- Add new 2D and 3D geometry primitives
- Refactor `BasicVector` and add Rtree and Multimap tests
- Implement foundational data structures and their tests
- Introduce advanced dynamic memory structures
- Implement paged memory management system
- Introduce FwsMultimap container for multiple values
- Introduce new foundational container types
- Introduce serialization integrity checksums and generic utilities
- Add comprehensive `datagram` serialization integration tests
- Implement version tracking for serialization
- Implement `HashStorage` serialization and deserialization
- Add support for complex type (de)serialization
- Add zero-copy string_view to datagrams
- Add serialization and deserialization functionalities
- Add byte buffer manipulation and serialization utilities
- Add endian detection and byte swapping utilities
- Implement a robust and extensible type hashing system
- Implement custom `HashMap`, `HashSet`, `Tuple`, and `Variant`
- Introduce custom container types and hashing utilities
- Implement C++ reflection utilities for aggregate types
- Introduce generic memory-relocatable pointer system
- Implement cross-platform utilities for low-level programming
- Add core utilities for common data operations
- Init
- Init

### <!-- 1 -->🐛 Bug Fixes

- Update devbox.json and system path handling

### <!-- 2 -->🚜 Refactor

- Reorganize project into new module structure
- Refactor: Replace deprecated hash containers with standard ones
- Rename `is_datapod_container` to `is_container`
- Rename project from Bitcon to Datapod
- Rename project from Datagram to Bitcon

### <!-- 3 -->📚 Documentation

- Update acknowledgments and fix typo

### <!-- 6 -->🧪 Testing

- Implement FwsMultimap and MutableFwsMultimap with tests
- Reinforce struct reflection and serialization capabilities

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Set project version to 0.0.1

### Build

- Project structure

