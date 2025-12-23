# Changelog

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

