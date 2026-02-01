# Changelog

## [0.0.50] - 2026-02-01

### <!-- 0 -->⛰️  Features

- Add new robot model concepts and types

## [0.0.49] - 2026-01-31

### <!-- 0 -->⛰️  Features

- Add robot identity and sugar types

## [0.0.48] - 2026-01-31

### <!-- 0 -->⛰️  Features

- Add Sensor POD and integrate into Link

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Remove URDF_PROPS.md documentation

## [0.0.47] - 2026-01-31

### <!-- 0 -->⛰️  Features

- Add URDF property extension mechanism

## [0.0.46] - 2026-01-30

### <!-- 0 -->⛰️  Features

- Add robot transmission PODs

## [0.0.45] - 2026-01-30

### <!-- 0 -->⛰️  Features

- Refactor Inertia to Inertial for URDF compatibility

## [0.0.44] - 2026-01-30

### <!-- 0 -->⛰️  Features

- Add Joint::Dynamics struct for URDF dynamics
- Migrate robot PODs to `datapod::robot` namespace

## [0.0.43] - 2026-01-30

### <!-- 0 -->⛰️  Features

- Add initial robot model PODs

## [0.0.42] - 2026-01-25

### <!-- 1 -->🐛 Bug Fixes

- Rename `size` friend function to resolve conflict

## [0.0.41] - 2026-01-18

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Skip re-fetching already populated dependencies

## [0.0.40] - 2026-01-18

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Refactor CMake file for cleaner output

## [0.0.39] - 2026-01-18

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Consolidate project metadata into single file

## [0.0.38] - 2026-01-18

### <!-- 1 -->🐛 Bug Fixes

- Resolve issues with optional, string, and offset_ptr

## [0.0.37] - 2026-01-17

### <!-- 0 -->⛰️  Features

- Enhance BasicString to be on par with std::string and add modern conveniences

### <!-- 1 -->🐛 Bug Fixes

- Improve reflection and serialization of PODs

## [0.0.36] - 2026-01-12

### <!-- 0 -->⛰️  Features

- Add type-safe dispatch mechanism for serialized data
- Introduce stable primitive type IDs for type hashing

## [0.0.35] - 2026-01-12

### <!-- 0 -->⛰️  Features

- Add Bytes type for raw binary data handling

## [0.0.34] - 2026-01-08

### <!-- 0 -->⛰️  Features

- Add Result monadic ternary operations
- Add MPMC and SPMC lock-free ring buffer variants

## [0.0.33] - 2026-01-08

### <!-- 0 -->⛰️  Features

- Implement additional monadic Result methods

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Add internet connectivity check to Makefile

## [0.0.32] - 2026-01-06

### <!-- 1 -->🐛 Bug Fixes

- Prevent buffer overflow in string operations

## [0.0.31] - 2026-01-05

### <!-- 0 -->⛰️  Features

- Introduce `result::ok/err` and `result::Ok/Err` factories

## [0.0.30] - 2026-01-05

### <!-- 0 -->⛰️  Features

- Add Result<void, E> specialization and Unit type

## [0.0.29] - 2026-01-04

### <!-- 1 -->🐛 Bug Fixes

- Refactor short namespace alias handling

## [0.0.28] - 2026-01-04

### <!-- 1 -->🐛 Bug Fixes

- Refactor short namespace alias handling

## [0.0.27] - 2026-01-04

### <!-- 0 -->⛰️  Features

- Replace 591 std:: type uses with datapod:: types
- Add Rust-inspired type system with zero std:: dependencies
- Add namespace utilities for Phase 8 - ALL remaining types
- Add namespace utilities for Phase 6-7 complex spatial and gaussian types
- Add namespace utilities for Phase 5 geospatial types
- Add namespace utilities for Phase 3 robotics types
- Add namespace utilities for Phase 4 bounding volumes
- Add namespace utilities for Phase 2 geometric primitives
- Add namespace utilities for Phase 1 core spatial types

### <!-- 1 -->🐛 Bug Fixes

- Remove duplicate DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN defines

### <!-- 2 -->🚜 Refactor

- Reorganize include directory to separate PODs from tools

## [0.0.26] - 2026-01-04

### <!-- 0 -->⛰️  Features

- Extend automatic reflection from 10 to 64 fields

## [0.0.25] - 2026-01-04

### <!-- 1 -->🐛 Bug Fixes

- Correct RingBuffer move semantics to prevent premature SHM cleanup

## [0.0.24] - 2026-01-04

### <!-- 0 -->⛰️  Features

- Add Pin<T> to prevent moving of values
- Add RefCell<T>, Ref<T>, and RefMut<T> for interior mutability
- Add MaybeUninit<T> for uninitialized memory
- Add SharedPtr<T> and WeakPtr<T>
- Add Cow<T> for copy-on-write
- Add NonNull<T> for null safety
- Add Either<L,R> for binary choice
- Add Lazy<T> for deferred computation
- Add OnceCell<T> for one-time initialization
- Add advanced Bitset operations
- Add Tuple utility functions
- Add UniquePtr array support and comparisons

## [0.0.23] - 2026-01-04

### <!-- 0 -->⛰️  Features

- Add advanced Variant operations
- Add iterator support to Optional and Result
- Add conversion methods between Optional and Result
- Add monadic operations to Result<T,E>
- Add monadic operations to Optional<T>

## [0.0.22] - 2026-01-03

### <!-- 0 -->⛰️  Features

- Rename mat types to PascalCase

## [0.0.21] - 2026-01-03

### <!-- 3 -->📚 Documentation

- Update Datapod architecture diagram

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

