#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "datagram/core/aligned_alloc.hpp"
#include "datagram/core/bit_counting.hpp"
#include "datagram/core/buffer.hpp"
#include "datagram/core/chunk.hpp"
#include "datagram/core/mmap.hpp"
#include "datagram/core/next_power_of_2.hpp"
#include "datagram/core/strong.hpp"

// Test bit counting functions
void test_bit_counting() {
    std::cout << "Testing bit counting... ";

    // Test trailing zeros
    assert(datagram::trailing_zeros(0b1000U) == 3);
    assert(datagram::trailing_zeros(0b0100U) == 2);
    assert(datagram::trailing_zeros(0b0010U) == 1);
    assert(datagram::trailing_zeros(0b0001U) == 0);
    assert(datagram::trailing_zeros(0U) == 32);

    // Test trailing zeros 64-bit
    assert(datagram::trailing_zeros(0b1000ULL) == 3);
    assert(datagram::trailing_zeros(0ULL) == 64);
    assert(datagram::trailing_zeros(1ULL << 63) == 63);

    // Test leading zeros
    assert(datagram::leading_zeros(0b0001U) == 31);
    assert(datagram::leading_zeros(0b0010U) == 30);
    assert(datagram::leading_zeros(0b0100U) == 29);
    assert(datagram::leading_zeros(0b1000U) == 28);
    assert(datagram::leading_zeros(0U) == 32);

    // Test leading zeros 64-bit
    assert(datagram::leading_zeros(0b0001ULL) == 63);
    assert(datagram::leading_zeros(0ULL) == 64);
    assert(datagram::leading_zeros(1ULL << 63) == 0);

    // Test popcount
    assert(datagram::popcount(0b0000ULL) == 0);
    assert(datagram::popcount(0b0001ULL) == 1);
    assert(datagram::popcount(0b1010ULL) == 2);
    assert(datagram::popcount(0b1111ULL) == 4);
    assert(datagram::popcount(~0ULL) == 64);

    // Test constexpr_trailing_zeros (compile-time)
    static_assert(datagram::constexpr_trailing_zeros(0b1000U) == 3);
    static_assert(datagram::constexpr_trailing_zeros(0b0001U) == 0);

    std::cout << "PASSED\n";
}

// Test next_power_of_2 functions
void test_next_power_of_2() {
    std::cout << "Testing next_power_of_2... ";

    // Test basic power of 2 rounding
    assert(datagram::next_power_of_two(1U) == 1);
    assert(datagram::next_power_of_two(2U) == 2);
    assert(datagram::next_power_of_two(3U) == 4);
    assert(datagram::next_power_of_two(4U) == 4);
    assert(datagram::next_power_of_two(5U) == 8);
    assert(datagram::next_power_of_two(7U) == 8);
    assert(datagram::next_power_of_two(8U) == 8);
    assert(datagram::next_power_of_two(9U) == 16);
    assert(datagram::next_power_of_two(15U) == 16);
    assert(datagram::next_power_of_two(16U) == 16);
    assert(datagram::next_power_of_two(100U) == 128);
    assert(datagram::next_power_of_two(1000U) == 1024);

    // Test with 64-bit values
    assert(datagram::next_power_of_two(1000000ULL) == 1048576ULL);

    // Test to_next_multiple
    assert(datagram::to_next_multiple(10, 4) == 12);
    assert(datagram::to_next_multiple(12, 4) == 12);
    assert(datagram::to_next_multiple(13, 4) == 16);
    assert(datagram::to_next_multiple(15, 8) == 16);
    assert(datagram::to_next_multiple(16, 8) == 16);
    assert(datagram::to_next_multiple(17, 8) == 24);

    // Test constexpr
    static_assert(datagram::next_power_of_two(3U) == 4);
    static_assert(datagram::to_next_multiple(10, 4) == 12);

    std::cout << "PASSED\n";
}

// Test Strong typedef
void test_strong() {
    std::cout << "Testing Strong typedef... ";

    using MyInt = datagram::Strong<int, struct MyIntTag>;
    using MySize = datagram::Strong<std::size_t, struct MySizeTag>;

    // Construction
    MyInt a{42};
    assert(a.v_ == 42);

    // Copy construction
    MyInt b = a;
    assert(b.v_ == 42);

    // Move construction
    MyInt c = std::move(a);
    assert(c.v_ == 42);

    // Increment/decrement
    MyInt d{10};
    ++d;
    assert(d.v_ == 11);
    d++;
    assert(d.v_ == 12);
    --d;
    assert(d.v_ == 11);
    d--;
    assert(d.v_ == 10);

    // Arithmetic operations
    MyInt e{5};
    MyInt f{3};
    assert((e + f).v_ == 8);
    assert((e - f).v_ == 2);
    assert((e * f).v_ == 15);
    assert((e / f).v_ == 1);

    // Arithmetic with underlying type
    assert((e + 5).v_ == 10);
    assert((e - 2).v_ == 3);

    // Compound assignment
    MyInt g{10};
    g += 5;
    assert(g.v_ == 15);
    g -= 3;
    assert(g.v_ == 12);

    // Comparison operations
    MyInt h{10};
    MyInt i{20};
    assert(h < i);
    assert(h <= i);
    assert(i > h);
    assert(i >= h);
    assert(h == MyInt{10});
    assert(h != i);

    // Comparison with underlying type
    assert(h == 10);
    assert(h < 20);

    // Bit operations
    MyInt j{0b1010};
    MyInt k{0b0110};
    assert((j << 1).v_ == 0b10100);
    assert((j >> 1).v_ == 0b0101);

    MyInt l{0b1010};
    l |= MyInt{0b0101};
    assert(l.v_ == 0b1111);

    MyInt m{0b1111};
    m &= MyInt{0b1010};
    assert(m.v_ == 0b1010);

    // Type traits
    static_assert(datagram::is_strong_v<MyInt>);
    static_assert(!datagram::is_strong_v<int>);

    // to_idx function
    assert(datagram::to_idx(MyInt{42}) == 42);
    assert(datagram::to_idx(123) == 123);

    // base_t type alias
    static_assert(std::is_same_v<datagram::base_t<MyInt>, int>);
    static_assert(std::is_same_v<datagram::base_t<int>, int>);

    // std::numeric_limits specialization
    assert(std::numeric_limits<MyInt>::min().v_ == std::numeric_limits<int>::min());
    assert(std::numeric_limits<MyInt>::max().v_ == std::numeric_limits<int>::max());

    // Invalid value
    auto invalid = MySize::invalid();
    assert(invalid.v_ == std::numeric_limits<std::size_t>::max());

    std::cout << "PASSED\n";
}

// Test Buffer
void test_buffer() {
    std::cout << "Testing Buffer... ";

    // Default construction
    datagram::Buffer buf1;
    assert(buf1.size() == 0);
    assert(buf1.data() == nullptr);

    // Construction with size
    datagram::Buffer buf2(1024);
    assert(buf2.size() == 1024);
    assert(buf2.data() != nullptr);

    // Fill buffer with data
    for (std::size_t i = 0; i < buf2.size(); ++i) {
        buf2[i] = static_cast<std::uint8_t>(i % 256);
    }

    // Verify data
    for (std::size_t i = 0; i < buf2.size(); ++i) {
        assert(buf2[i] == static_cast<std::uint8_t>(i % 256));
    }

    // Construction from C-string
    const char *test_str = "Hello, World!";
    datagram::Buffer buf3(test_str);
    assert(buf3.size() == std::strlen(test_str));
    assert(std::memcmp(buf3.data(), test_str, buf3.size()) == 0);

    // Construction from data and size
    const char *test_data = "Test Data";
    datagram::Buffer buf4(test_data, 4);
    assert(buf4.size() == 4);
    assert(std::memcmp(buf4.data(), "Test", 4) == 0);

    // Move construction
    datagram::Buffer buf5 = std::move(buf2);
    assert(buf5.size() == 1024);
    assert(buf5.data() != nullptr);
    assert(buf2.data() == nullptr);
    assert(buf2.size() == 0);

    // Move assignment
    datagram::Buffer buf6(512);
    buf6 = std::move(buf5);
    assert(buf6.size() == 1024);
    assert(buf5.data() == nullptr);

    // Iterator support
    datagram::Buffer buf7(10);
    for (auto &byte : buf7) {
        byte = 42;
    }
    for (const auto &byte : buf7) {
        assert(byte == 42);
    }

    // begin/end
    assert(buf7.end() - buf7.begin() == 10);

    std::cout << "PASSED\n";
}

// Test aligned allocation
void test_aligned_alloc() {
    std::cout << "Testing aligned allocation... ";

    // Test various alignments
    for (std::size_t alignment : {8, 16, 32, 64, 128}) {
        void *ptr = datagram::aligned_alloc(alignment, 1024);
        assert(ptr != nullptr);
        assert(reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0);
        datagram::aligned_free(alignment, ptr);
    }

    // Test with non-power-of-2 alignment (should be rounded up)
    void *ptr1 = datagram::aligned_alloc(7, 1024);
    assert(ptr1 != nullptr);
    // Should be aligned to next power of 2 (8)
    assert(reinterpret_cast<std::uintptr_t>(ptr1) % 8 == 0);
    datagram::aligned_free(7, ptr1);

    // Test macros
    void *ptr2 = DATAGRAM_ALIGNED_ALLOC(64, 512);
    assert(ptr2 != nullptr);
    DATAGRAM_ALIGNED_FREE(64, ptr2);

    std::cout << "PASSED\n";
}

// Test chunk helper
void test_chunk() {
    std::cout << "Testing chunk... ";

    // Test chunking a range
    std::vector<std::pair<std::size_t, unsigned>> chunks;
    datagram::chunk(10, 35, [&](std::size_t offset, unsigned chunk_size) { chunks.emplace_back(offset, chunk_size); });

    assert(chunks.size() == 4);
    assert(chunks[0].first == 0 && chunks[0].second == 10);
    assert(chunks[1].first == 10 && chunks[1].second == 10);
    assert(chunks[2].first == 20 && chunks[2].second == 10);
    assert(chunks[3].first == 30 && chunks[3].second == 5);

    // Test with exact multiple
    chunks.clear();
    datagram::chunk(10, 30, [&](std::size_t offset, unsigned chunk_size) { chunks.emplace_back(offset, chunk_size); });

    assert(chunks.size() == 3);
    assert(chunks[0].first == 0 && chunks[0].second == 10);
    assert(chunks[1].first == 10 && chunks[1].second == 10);
    assert(chunks[2].first == 20 && chunks[2].second == 10);

    // Test with size smaller than chunk
    chunks.clear();
    datagram::chunk(100, 50, [&](std::size_t offset, unsigned chunk_size) { chunks.emplace_back(offset, chunk_size); });

    assert(chunks.size() == 1);
    assert(chunks[0].first == 0 && chunks[0].second == 50);

    // Test with zero size
    chunks.clear();
    datagram::chunk(10, 0, [&](std::size_t offset, unsigned chunk_size) { chunks.emplace_back(offset, chunk_size); });

    assert(chunks.empty());

    std::cout << "PASSED\n";
}

// Test mmap
void test_mmap() {
    std::cout << "Testing mmap... ";

    const char *temp_file = "/tmp/datagram_mmap_test.bin";

    // Test WRITE mode (creates new file)
    {
        datagram::Mmap mmap(temp_file, datagram::Mmap::Protection::WRITE);
        mmap.resize(1024);
        assert(mmap.size() == 1024);

        // Write some data
        for (std::size_t i = 0; i < mmap.size(); ++i) {
            mmap[i] = static_cast<std::uint8_t>(i % 256);
        }

        // Verify data
        for (std::size_t i = 0; i < mmap.size(); ++i) {
            assert(mmap[i] == static_cast<std::uint8_t>(i % 256));
        }

        // Test iterator
        std::size_t count = 0;
        for (auto byte : mmap) {
            assert(byte == static_cast<std::uint8_t>(count % 256));
            ++count;
        }

        mmap.sync();
    }

    // Test READ mode
    {
        datagram::Mmap mmap(temp_file, datagram::Mmap::Protection::READ);
        assert(mmap.size() == 1024);

        // Verify data persisted
        for (std::size_t i = 0; i < mmap.size(); ++i) {
            assert(mmap[i] == static_cast<std::uint8_t>(i % 256));
        }

        // Test view
        auto view = mmap.view();
        assert(view.size() == 1024);
    }

    // Test MODIFY mode
    {
        datagram::Mmap mmap(temp_file, datagram::Mmap::Protection::MODIFY);
        assert(mmap.size() == 1024);

        // Modify data
        for (std::size_t i = 0; i < mmap.size(); ++i) {
            mmap[i] = static_cast<std::uint8_t>(255 - (i % 256));
        }

        mmap.sync();
    }

    // Verify modifications
    {
        datagram::Mmap mmap(temp_file, datagram::Mmap::Protection::READ);
        for (std::size_t i = 0; i < mmap.size(); ++i) {
            assert(mmap[i] == static_cast<std::uint8_t>(255 - (i % 256)));
        }
    }

    // Test reserve
    {
        datagram::Mmap mmap(temp_file, datagram::Mmap::Protection::MODIFY);
        mmap.reserve(2048);
        assert(mmap.size() == 1024); // used_size should still be 1024
        mmap.resize(2048);
        assert(mmap.size() == 2048);
    }

    // Test move semantics
    {
        datagram::Mmap mmap1(temp_file, datagram::Mmap::Protection::WRITE);
        mmap1.resize(512);

        datagram::Mmap mmap2 = std::move(mmap1);
        assert(mmap2.size() == 512);

        datagram::Mmap mmap3;
        mmap3 = std::move(mmap2);
        assert(mmap3.size() == 512);
    }

    // Clean up
    std::remove(temp_file);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== Datagram Core Extended Tests ===\n\n";

    test_bit_counting();
    test_next_power_of_2();
    test_strong();
    test_buffer();
    test_aligned_alloc();
    test_chunk();
    test_mmap();

    std::cout << "\n=== All Core Extended Tests PASSED ===\n";
    return 0;
}
