#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "datagram/containers/allocator.hpp"
#include "datagram/containers/offset_ptr.hpp"
#include "datagram/containers/ptr.hpp"

// Test basic offset_ptr construction
void test_offset_ptr_construction() {
    std::cout << "Testing offset_ptr construction... ";

    // Default construction
    datagram::OffsetPtr<int> p1;
    assert(p1.get() == nullptr);
    assert(!p1);
    assert(p1 == nullptr);

    // nullptr construction
    datagram::OffsetPtr<int> p2(nullptr);
    assert(p2 == nullptr);

    // Construction from raw pointer
    int value = 42;
    datagram::OffsetPtr<int> p3(&value);
    assert(p3.get() == &value);
    assert(p3);
    assert(p3 != nullptr);
    assert(*p3 == 42);

    // Copy construction
    datagram::OffsetPtr<int> p4(p3);
    assert(p4.get() == &value);
    assert(*p4 == 42);

    // Move construction
    datagram::OffsetPtr<int> p5(std::move(p3));
    assert(p5.get() == &value);

    std::cout << "PASSED\n";
}

// Test offset_ptr assignment
void test_offset_ptr_assignment() {
    std::cout << "Testing offset_ptr assignment... ";

    int value1 = 10;
    int value2 = 20;

    datagram::OffsetPtr<int> p1(&value1);
    datagram::OffsetPtr<int> p2;

    // Copy assignment
    p2 = p1;
    assert(p2.get() == &value1);
    assert(*p2 == 10);

    // Assignment from raw pointer
    p1 = &value2;
    assert(p1.get() == &value2);
    assert(*p1 == 20);

    // nullptr assignment
    p1 = nullptr;
    assert(p1 == nullptr);

    std::cout << "PASSED\n";
}

// Test offset_ptr dereferencing and member access
void test_offset_ptr_dereferencing() {
    std::cout << "Testing offset_ptr dereferencing... ";

    struct Data {
        int x;
        int y;
        int sum() const { return x + y; }
    };

    Data data{10, 20};
    datagram::OffsetPtr<Data> p(&data);

    // Operator*
    assert((*p).x == 10);
    assert((*p).y == 20);

    // Operator->
    assert(p->x == 10);
    assert(p->y == 20);
    assert(p->sum() == 30);

    // Modify through pointer
    p->x = 100;
    assert(data.x == 100);

    std::cout << "PASSED\n";
}

// Test offset_ptr comparison
void test_offset_ptr_comparison() {
    std::cout << "Testing offset_ptr comparison... ";

    int values[3] = {1, 2, 3};

    datagram::OffsetPtr<int> p1(&values[0]);
    datagram::OffsetPtr<int> p2(&values[0]);
    datagram::OffsetPtr<int> p3(&values[1]);
    datagram::OffsetPtr<int> p4(nullptr);

    // Equality
    assert(p1 == p2);
    assert(!(p1 != p2));
    assert(p1 != p3);
    assert(p4 == nullptr);

    // Comparison with raw pointer
    assert(p1 == &values[0]);
    assert(p1 != &values[1]);

    // Ordering
    assert(p1 < p3);
    assert(p1 <= p3);
    assert(p3 > p1);
    assert(p3 >= p1);
    assert(p1 <= p2);
    assert(p1 >= p2);

    std::cout << "PASSED\n";
}

// Test offset_ptr arithmetic
void test_offset_ptr_arithmetic() {
    std::cout << "Testing offset_ptr arithmetic... ";

    int values[5] = {10, 20, 30, 40, 50};
    datagram::OffsetPtr<int> p(&values[0]);

    // Increment
    ++p;
    assert(p.get() == &values[1]);
    assert(*p == 20);

    p++;
    assert(p.get() == &values[2]);
    assert(*p == 30);

    // Decrement
    --p;
    assert(p.get() == &values[1]);
    assert(*p == 20);

    p--;
    assert(p.get() == &values[0]);
    assert(*p == 10);

    // Addition
    auto p2 = p + 2;
    assert(p2.get() == &values[2]);
    assert(*p2 == 30);

    // Subtraction
    auto p3 = p2 - 1;
    assert(p3.get() == &values[1]);
    assert(*p3 == 20);

    // Compound assignment
    p += 3;
    assert(p.get() == &values[3]);
    assert(*p == 40);

    p -= 2;
    assert(p.get() == &values[1]);
    assert(*p == 20);

    // Pointer difference
    datagram::OffsetPtr<int> p4(&values[0]);
    datagram::OffsetPtr<int> p5(&values[4]);
    assert(p5 - p4 == 4);
    assert(p4 - p5 == -4);

    // Array subscript
    datagram::OffsetPtr<int> p6(&values[0]);
    assert(p6[0] == 10);
    assert(p6[1] == 20);
    assert(p6[2] == 30);
    assert(p6[4] == 50);

    std::cout << "PASSED\n";
}

// Test offset_ptr relocation (the key feature!)
void test_offset_ptr_relocation() {
    std::cout << "Testing offset_ptr relocation... ";

    struct Block {
        int value;
        datagram::OffsetPtr<int> ptr;
    };

    // Create a block with a pointer to its own value
    Block block1;
    block1.value = 42;
    block1.ptr = &block1.value;

    // Verify pointer works
    assert(*block1.ptr == 42);

    // Copy the block to a new memory location
    Block block2;
    std::memcpy(&block2, &block1, sizeof(Block));

    // The offset_ptr should still work in the new location!
    assert(*block2.ptr == 42);
    assert(block2.ptr.get() == &block2.value);

    // Modify through the relocated pointer
    *block2.ptr = 100;
    assert(block2.value == 100);

    // Allocate array and relocate
    constexpr size_t SIZE = 3;
    struct ArrayBlock {
        int values[SIZE];
        datagram::OffsetPtr<int> ptrs[SIZE];
    };

    ArrayBlock arr1;
    for (size_t i = 0; i < SIZE; ++i) {
        arr1.values[i] = static_cast<int>(i * 10);
        arr1.ptrs[i] = &arr1.values[i];
    }

    // Verify all pointers work
    for (size_t i = 0; i < SIZE; ++i) {
        assert(*arr1.ptrs[i] == static_cast<int>(i * 10));
    }

    // Relocate the entire array
    ArrayBlock arr2;
    std::memcpy(&arr2, &arr1, sizeof(ArrayBlock));

    // All pointers should still work!
    for (size_t i = 0; i < SIZE; ++i) {
        assert(*arr2.ptrs[i] == static_cast<int>(i * 10));
        assert(arr2.ptrs[i].get() == &arr2.values[i]);
    }

    std::cout << "PASSED\n";
}

// Test offset_ptr with const
void test_offset_ptr_const() {
    std::cout << "Testing offset_ptr with const... ";

    int value = 42;
    const int const_value = 100;

    datagram::OffsetPtr<int> p1(&value);
    datagram::OffsetPtr<const int> p2(&const_value);
    datagram::OffsetPtr<const int> p3(&value); // non-const to const conversion

    assert(*p1 == 42);
    assert(*p2 == 100);
    assert(*p3 == 42);

    *p1 = 50;
    assert(*p1 == 50);

    // p2 = &value; // Should not compile (const correctness)

    std::cout << "PASSED\n";
}

// Test ptr.hpp mode selection
void test_ptr_mode_selection() {
    std::cout << "Testing ptr mode selection... ";

    int value = 42;

    // Raw mode
    datagram::raw::ptr<int> raw_ptr = &value;
    assert(*raw_ptr == 42);

    // Offset mode
    datagram::offset::ptr<int> offset_ptr(&value);
    assert(*offset_ptr == 42);

    // Generic ptr template
    datagram::ptr<int, datagram::RawMode> p1 = &value;
    datagram::ptr<int, datagram::OffsetMode> p2(&value);

    assert(*p1 == 42);
    assert(*p2 == 42);

    // Type traits
    static_assert(datagram::is_raw_ptr_v<int *>);
    static_assert(!datagram::is_raw_ptr_v<datagram::OffsetPtr<int>>);

    static_assert(datagram::is_offset_ptr_v<datagram::OffsetPtr<int>>);
    static_assert(!datagram::is_offset_ptr_v<int *>);

    static_assert(datagram::is_ptr_type_v<int *>);
    static_assert(datagram::is_ptr_type_v<datagram::OffsetPtr<int>>);
    static_assert(!datagram::is_ptr_type_v<int>);

    // ptr_value_t
    static_assert(std::is_same_v<datagram::ptr_value_t<int *>, int>);
    static_assert(std::is_same_v<datagram::ptr_value_t<datagram::OffsetPtr<int>>, int>);
    static_assert(std::is_same_v<datagram::ptr_value_t<int>, int>);

    std::cout << "PASSED\n";
}

// Test Allocator
void test_allocator() {
    std::cout << "Testing Allocator... ";

    datagram::Allocator<int> alloc;

    // Allocate
    int *ptr = alloc.allocate(10);
    assert(ptr != nullptr);

    // Construct
    for (int i = 0; i < 10; ++i) {
        alloc.construct(&ptr[i], i * 10);
    }

    // Verify
    for (int i = 0; i < 10; ++i) {
        assert(ptr[i] == i * 10);
    }

    // Destroy
    for (int i = 0; i < 10; ++i) {
        alloc.destroy(&ptr[i]);
    }

    // Deallocate
    alloc.deallocate(ptr, 10);

    // Test with vector
    std::vector<int, datagram::Allocator<int>> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    assert(vec.size() == 3);
    assert(vec[0] == 1);
    assert(vec[1] == 2);
    assert(vec[2] == 3);

    // Test max_size
    assert(alloc.max_size() > 0);

    // Test rebind
    using StringAlloc = datagram::Allocator<int>::rebind<std::string>::other;
    StringAlloc string_alloc;
    auto *str_ptr = string_alloc.allocate(1);
    string_alloc.construct(str_ptr, "Hello");
    assert(*str_ptr == "Hello");
    string_alloc.destroy(str_ptr);
    string_alloc.deallocate(str_ptr, 1);

    std::cout << "PASSED\n";
}

// Test offset_ptr internal offset storage
void test_offset_ptr_offset() {
    std::cout << "Testing offset_ptr offset storage... ";

    int value = 42;
    datagram::OffsetPtr<int> p(&value);

    // Calculate expected offset
    auto expected_offset = reinterpret_cast<std::uintptr_t>(&value) - reinterpret_cast<std::uintptr_t>(&p);

    assert(p.offset() == static_cast<datagram::offset_t>(expected_offset));

    // Test set_offset
    datagram::OffsetPtr<int> p2;
    p2.set_offset(p.offset());

    // This won't point to the same location because the base address is different,
    // but the offset should be stored correctly
    assert(p2.offset() == p.offset());

    // nullptr offset
    datagram::OffsetPtr<int> p3(nullptr);
    assert(p3.offset() == datagram::NULLPTR_OFFSET);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== Datagram Pointer System Tests ===\n\n";

    test_offset_ptr_construction();
    test_offset_ptr_assignment();
    test_offset_ptr_dereferencing();
    test_offset_ptr_comparison();
    test_offset_ptr_arithmetic();
    test_offset_ptr_relocation();
    test_offset_ptr_const();
    test_ptr_mode_selection();
    test_allocator();
    test_offset_ptr_offset();

    std::cout << "\n=== All Pointer System Tests PASSED ===\n";
    return 0;
}
