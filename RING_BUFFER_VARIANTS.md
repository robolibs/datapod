# Ring Buffer Variants

This document describes the three lock-free ring buffer variants implemented in datapod.

## Overview

The ring buffer implementation provides three policy-based variants optimized for different concurrency patterns:

1. **SPSC** - Single Producer Single Consumer
2. **SPMC** - Single Producer Multiple Consumer  
3. **MPMC** - Multiple Producer Multiple Consumer

All variants are:
- Lock-free (using atomic operations)
- Cache-line aligned (64-byte alignment to prevent false sharing)
- Support shared memory for inter-process communication
- Trivially copyable types only

## Variant Details

### SPSC (Single Producer Single Consumer)

**Use Case:** One thread writes, one thread reads

**Performance:** Fastest - no CAS (Compare-And-Swap) operations needed

**Memory Ordering:**
- Producer: relaxed load on write_pos, acquire load on read_pos, release store on write_pos
- Consumer: relaxed load on read_pos, acquire load on write_pos, release store on read_pos

**Example:**
```cpp
RingBuffer<SPSC, int> ring(256);

// Producer thread
std::thread producer([&ring]() {
    for (int i = 0; i < 100; i++) {
        ring.push(i);
    }
});

// Consumer thread
std::thread consumer([&ring]() {
    for (int i = 0; i < 100; i++) {
        auto val = ring.pop();
        // process val
    }
});
```

### SPMC (Single Producer Multiple Consumer)

**Use Case:** One thread writes, multiple threads read

**Performance:** Fast writes (no CAS), CAS on reads for consumer coordination

**Memory Ordering:**
- Producer: Same as SPSC (no contention)
- Consumers: CAS on read_pos to claim slots atomically

**Example:**
```cpp
RingBuffer<SPMC, int> ring(256);

// Single producer
std::thread producer([&ring]() {
    for (int i = 0; i < 1000; i++) {
        ring.push(i);
    }
});

// Multiple consumers
std::vector<std::thread> consumers;
for (int c = 0; c < 4; c++) {
    consumers.emplace_back([&ring]() {
        while (true) {
            auto val = ring.pop();
            if (val.is_ok()) {
                // process val.value()
            }
        }
    });
}
```

**Use Cases:**
- Event dispatching to multiple workers
- Work distribution from single source
- Fan-out patterns

### MPMC (Multiple Producer Multiple Consumer)

**Use Case:** Multiple threads write, multiple threads read

**Performance:** CAS on both reads and writes (most flexible, slightly slower)

**Memory Ordering:**
- Producers: CAS on write_pos to claim slots atomically
- Consumers: CAS on read_pos to claim slots atomically

**Example:**
```cpp
RingBuffer<MPMC, int> ring(256);

// Multiple producers
std::vector<std::thread> producers;
for (int p = 0; p < 4; p++) {
    producers.emplace_back([&ring, p]() {
        for (int i = 0; i < 100; i++) {
            ring.push(p * 100 + i);
        }
    });
}

// Multiple consumers
std::vector<std::thread> consumers;
for (int c = 0; c < 4; c++) {
    consumers.emplace_back([&ring]() {
        while (true) {
            auto val = ring.pop();
            if (val.is_ok()) {
                // process val.value()
            }
        }
    });
}
```

**Use Cases:**
- Thread pools
- General message passing
- Multi-threaded work queues

## API

All variants share the same API:

### Construction
```cpp
// In-memory
RingBuffer<Policy, T> ring(capacity);

// Shared memory (for IPC)
auto result = RingBuffer<Policy, T>::create_shm("/name", capacity);
auto result = RingBuffer<Policy, T>::attach_shm("/name");
```

### Operations
```cpp
// Push operations
Result<bool, Error> push(const T& item);
Result<bool, Error> push(T&& item);
Result<bool, Error> emplace(Args&&... args);

// Pop operations
Result<T, Error> pop();
Result<const T*, Error> peek() const;

// Status
bool empty() const noexcept;
bool full() const noexcept;
size_t size() const noexcept;
size_t capacity() const noexcept;

// Snapshot/restore
Snapshot snapshot() const noexcept;
SnapshotWithData snapshot_with_data() const;
static Result<RingBuffer, Error> from_snapshot(const SnapshotWithData& snap);

// Drain
Vector<T> drain();
```

## Performance Characteristics

| Variant | Producer Overhead | Consumer Overhead | Best For |
|---------|------------------|-------------------|----------|
| SPSC    | Minimal (no CAS) | Minimal (no CAS)  | 1:1 communication |
| SPMC    | Minimal (no CAS) | CAS per pop       | 1:N work distribution |
| MPMC    | CAS per push     | CAS per pop       | N:M general queuing |

## Implementation Details

### Cache Line Alignment

The header structure is carefully aligned to prevent false sharing:

```cpp
struct alignas(64) Header {
    std::atomic<uint64_t> write_pos;
    uint8_t padding1[64 - sizeof(std::atomic<uint64_t>)];
    
    std::atomic<uint64_t> read_pos;
    uint8_t padding2[64 - sizeof(std::atomic<uint64_t>)];
    
    uint64_t capacity;
    uint32_t magic;
    uint32_t version;
};
```

### Magic Numbers

Each variant has a unique magic number for validation:
- SPSC: `0x53505343` ("SPSC")
- SPMC: `0x53504D43` ("SPMC")
- MPMC: `0x4D504D43` ("MPMC")

### Shared Memory Support

All variants support shared memory for inter-process communication:

```cpp
// Process A: Create
auto ring = RingBuffer<MPMC, int>::create_shm("/my_ring", 256);

// Process B: Attach
auto ring = RingBuffer<MPMC, int>::attach_shm("/my_ring");
```

## Testing

Comprehensive tests are provided for each variant:

- `test/lockfree/ring_buffer_test.cpp` - SPSC tests
- `test/lockfree/ring_buffer_mpmc_simple_test.cpp` - MPMC tests
- `test/lockfree/ring_buffer_spmc_simple_test.cpp` - SPMC tests
- `test/lockfree/ring_buffer_shm_test.cpp` - Shared memory tests

## Examples

- `examples/ring_buffer_usage.cpp` - SPSC examples
- `examples/ring_buffer_variants_usage.cpp` - All variants comparison

## Choosing the Right Variant

1. **Use SPSC when:**
   - You have exactly one producer and one consumer
   - You need maximum performance
   - Example: Pipeline stages, single-threaded event loops

2. **Use SPMC when:**
   - You have one data source and multiple workers
   - Producers don't need coordination
   - Example: Event dispatcher, work distribution, fan-out

3. **Use MPMC when:**
   - You have multiple producers and/or consumers
   - You need maximum flexibility
   - Example: Thread pools, general message queues, work stealing

## Thread Safety

- **SPSC**: Safe with one producer thread and one consumer thread
- **SPMC**: Safe with one producer thread and multiple consumer threads
- **MPMC**: Safe with multiple producer and consumer threads

All variants use appropriate memory ordering to ensure correctness without locks.
