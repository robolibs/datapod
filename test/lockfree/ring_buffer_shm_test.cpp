#include <cassert>
#include <datapod/pods/lockfree/ring_buffer.hpp>
#include <datapod/pods/sequential/string.hpp>
#include <iostream>

using namespace datapod;

void test_shm_create_and_attach() {
    std::cout << "Test 1: SHM create and attach... ";

    String name("/test_shm_basic");
    shm_unlink(name.c_str());

    // Create SHM ring
    auto create_result = RingBuffer<SPSC, uint8_t>::create_shm(name, 4096);
    assert(create_result.is_ok());

    // Move from result (this triggers the move constructor)
    auto ring = std::move(create_result.value());

    // Try to attach from same process (should work now with the fix)
    auto attach_result = RingBuffer<SPSC, uint8_t>::attach_shm(name);
    if (!attach_result.is_ok()) {
        std::cerr << "FAILED: Attach failed: " << attach_result.error().message.c_str() << std::endl;
        shm_unlink(name.c_str());
        exit(1);
    }

    auto attached_ring = std::move(attach_result.value());

    // Test basic operations
    uint8_t test_val = 42;
    assert(ring.push(test_val).is_ok());

    auto pop_result = attached_ring.pop();
    assert(pop_result.is_ok());
    assert(pop_result.value() == test_val);

    // Cleanup
    shm_unlink(name.c_str());

    std::cout << "PASSED\n";
}

void test_shm_move_semantics() {
    std::cout << "Test 2: SHM move semantics... ";

    String name("/test_shm_move");
    shm_unlink(name.c_str());

    // Create SHM ring
    auto create_result = RingBuffer<SPSC, uint8_t>::create_shm(name, 1024);
    assert(create_result.is_ok());

    // Move to a new ring buffer (tests move constructor)
    auto ring1 = std::move(create_result.value());

    // Move again (tests move assignment)
    RingBuffer<SPSC, uint8_t> ring2(16);
    ring2 = std::move(ring1);

    // Verify the SHM still exists and can be attached
    auto attach_result = RingBuffer<SPSC, uint8_t>::attach_shm(name);
    assert(attach_result.is_ok());

    // Cleanup
    shm_unlink(name.c_str());

    std::cout << "PASSED\n";
}

void test_shm_ownership_transfer() {
    std::cout << "Test 3: SHM ownership transfer... ";

    String name("/test_shm_ownership");
    shm_unlink(name.c_str());

    // Create SHM ring and move it
    auto create_result = RingBuffer<SPSC, uint8_t>::create_shm(name, 2048);
    assert(create_result.is_ok());
    auto ring = std::move(create_result.value());

    // At this point, create_result.value() is moved-from and should NOT own the SHM
    // The moved-from object going out of scope should NOT unlink the SHM

    // Verify SHM still exists by attaching to it
    auto attach_result = RingBuffer<SPSC, uint8_t>::attach_shm(name);
    if (!attach_result.is_ok()) {
        std::cerr << "FAILED: Could not attach to SHM!" << std::endl;
        exit(1);
    }

    // Cleanup - manually unlink since ring still owns it
    shm_unlink(name.c_str());

    std::cout << "PASSED\n";
}

void test_shm_multiple_moves() {
    std::cout << "Test 4: SHM multiple moves... ";

    String name("/test_shm_multi_move");
    shm_unlink(name.c_str());

    // Create and move multiple times
    auto create_result = RingBuffer<SPSC, uint8_t>::create_shm(name, 512);
    assert(create_result.is_ok());

    auto ring1 = std::move(create_result.value());
    auto ring2 = std::move(ring1);
    auto ring3 = std::move(ring2);

    // Write data through ring3
    for (uint8_t i = 0; i < 10; i++) {
        assert(ring3.push(i).is_ok());
    }

    // Attach and verify data
    auto attach_result = RingBuffer<SPSC, uint8_t>::attach_shm(name);
    assert(attach_result.is_ok());
    auto attached = std::move(attach_result.value());

    for (uint8_t i = 0; i < 10; i++) {
        auto val = attached.pop();
        assert(val.is_ok());
        assert(val.value() == i);
    }

    // Cleanup
    shm_unlink(name.c_str());

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "Running RingBuffer SHM tests...\n\n";

    test_shm_create_and_attach();
    test_shm_move_semantics();
    test_shm_ownership_transfer();
    test_shm_multiple_moves();

    std::cout << "\nAll SHM tests PASSED!\n";
    std::cout << "The move semantics bug has been fixed!\n";

    return 0;
}
