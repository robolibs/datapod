#include <datapod/pods/lockfree/ring_buffer.hpp>
#include <cassert>
#include <iostream>

using namespace datapod;

void test_basic() {
    std::cout << "Test 1: Basic push/pop... ";
    
    RingBuffer<SPSC, int> ring(16);
    assert(ring.empty());
    assert(ring.capacity() == 16);
    
    auto r1 = ring.push(42);
    assert(r1.is_ok());
    assert(!ring.empty());
    assert(ring.size() == 1);
    
    auto r2 = ring.pop();
    assert(r2.is_ok());
    assert(r2.value() == 42);
    assert(ring.empty());
    
    std::cout << "PASSED\n";
}

void test_full() {
    std::cout << "Test 2: Full detection... ";
    
    RingBuffer<SPSC, int> ring(4);
    
    for (int i = 0; i < 4; i++) {
        assert(ring.push(i).is_ok());
    }
    assert(ring.full());
    assert(ring.size() == 4);
    
    auto r = ring.push(99);
    assert(!r.is_ok());
    
    std::cout << "PASSED\n";
}

void test_wrapping() {
    std::cout << "Test 3: Wrapping... ";
    
    RingBuffer<SPSC, int> ring(4);
    
    for (int round = 0; round < 10; round++) {
        for (int i = 0; i < 4; i++) {
            ring.push(i);
        }
        for (int i = 0; i < 4; i++) {
            auto val = ring.pop();
            assert(val.value() == i);
        }
    }
    
    std::cout << "PASSED\n";
}

void test_peek() {
    std::cout << "Test 4: Peek... ";
    
    RingBuffer<SPSC, int> ring(8);
    
    ring.push(100);
    ring.push(200);
    
    auto p1 = ring.peek();
    assert(p1.is_ok());
    assert(*p1.value() == 100);
    assert(ring.size() == 2);
    
    auto v1 = ring.pop();
    assert(v1.value() == 100);
    
    auto p2 = ring.peek();
    assert(p2.is_ok());
    assert(*p2.value() == 200);
    
    std::cout << "PASSED\n";
}

void test_emplace() {
    std::cout << "Test 5: Emplace... ";
    
    struct Point {
        int x, y;
        Point(int x_, int y_) : x(x_), y(y_) {}
    };
    
    RingBuffer<SPSC, Point> ring(8);
    
    auto r = ring.emplace(10, 20);
    assert(r.is_ok());
    
    auto p = ring.pop();
    assert(p.is_ok());
    assert(p.value().x == 10);
    assert(p.value().y == 20);
    
    std::cout << "PASSED\n";
}

void test_snapshot() {
    std::cout << "Test 6: Snapshot... ";
    
    RingBuffer<SPSC, int> ring(16);
    
    for (int i = 0; i < 5; i++) {
        ring.push(i);
    }
    
    auto snap = ring.snapshot();
    assert(snap.magic == 0x53505343);
    assert(snap.capacity == 16);
    assert(snap.write_pos - snap.read_pos == 5);
    
    std::cout << "PASSED\n";
}

void test_snapshot_with_data() {
    std::cout << "Test 7: Snapshot with data... ";
    
    RingBuffer<SPSC, int> ring(16);
    
    for (int i = 0; i < 5; i++) {
        ring.push(i * 10);
    }
    
    auto snap = ring.snapshot_with_data();
    assert(snap.data.size() == 5);
    assert(snap.data[0] == 0);
    assert(snap.data[1] == 10);
    assert(snap.data[2] == 20);
    assert(snap.data[3] == 30);
    assert(snap.data[4] == 40);
    
    std::cout << "PASSED\n";
}

void test_from_snapshot() {
    std::cout << "Test 8: From snapshot... ";
    
    RingBuffer<SPSC, int> ring1(16);
    
    for (int i = 0; i < 5; i++) {
        ring1.push(i * 100);
    }
    
    auto snap = ring1.snapshot_with_data();
    auto result = RingBuffer<SPSC, int>::from_snapshot(snap);
    
    assert(result.is_ok());
    auto& ring2 = result.value();
    
    assert(ring2.size() == 5);
    assert(ring2.pop().value() == 0);
    assert(ring2.pop().value() == 100);
    assert(ring2.pop().value() == 200);
    
    std::cout << "PASSED\n";
}

void test_drain() {
    std::cout << "Test 9: Drain... ";
    
    RingBuffer<SPSC, int> ring(16);
    
    for (int i = 0; i < 10; i++) {
        ring.push(i);
    }
    
    auto drained = ring.drain();
    assert(drained.size() == 10);
    assert(ring.empty());
    
    for (size_t i = 0; i < drained.size(); i++) {
        assert(drained[i] == static_cast<int>(i));
    }
    
    std::cout << "PASSED\n";
}

int main() {
    std::cout << "Running RingBuffer tests...\n\n";
    
    test_basic();
    test_full();
    test_wrapping();
    test_peek();
    test_emplace();
    test_snapshot();
    test_snapshot_with_data();
    test_from_snapshot();
    test_drain();
    
    std::cout << "\nAll tests PASSED!\n";
    
    return 0;
}
