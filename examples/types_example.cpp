/// @file types_example.cpp
/// @brief Example demonstrating datapod's Rust-inspired type system
///
/// This example shows how to use dp:: types instead of std:: types

#include <datapod/types/types.hpp>

// Example 1: Basic type usage
void basic_types_example() {
    // Integer types - Rust style!
    dp::i8 tiny_int = -128;
    dp::i16 small_int = -32000;
    dp::i32 normal_int = -2000000000;
    dp::i64 big_int = -9000000000000000000LL;

    dp::u8 tiny_uint = 255;
    dp::u16 small_uint = 65535;
    dp::u32 normal_uint = 4000000000u;
    dp::u64 big_uint = 18000000000000000000ULL;

    // Floating point types
    dp::f32 single_precision = 3.14159f;
    dp::f64 double_precision = 2.718281828459045;

    // Size types (platform dependent)
    dp::usize array_size = 1024;
    dp::isize pointer_diff = -42;

    // Other types
    dp::boolean is_awesome = true;
    dp::byte raw_data = 0xFF;
}

// Example 2: Function parameters and return types
dp::u32 calculate_checksum(const dp::byte *data, dp::usize length) {
    dp::u32 checksum = 0;
    for (dp::usize i = 0; i < length; ++i) {
        checksum += data[i];
    }
    return checksum;
}

// Example 3: Struct with dp:: types
struct Packet {
    dp::u32 id;
    dp::u16 length;
    dp::u8 type;
    dp::u8 flags;
    dp::byte data[256];
};

// Example 4: Template with dp:: types
template <typename T> struct Buffer {
    T *data;
    dp::usize capacity;
    dp::usize size;

    Buffer(dp::usize cap) : capacity(cap), size(0) { data = new T[capacity]; }

    ~Buffer() { delete[] data; }

    dp::boolean push(const T &value) {
        if (size >= capacity)
            return false;
        data[size++] = value;
        return true;
    }

    dp::usize get_size() const { return size; }
};

// Example 5: Bit manipulation with dp:: types
dp::u32 set_bit(dp::u32 value, dp::u8 bit_position) { return value | (dp::u32(1) << bit_position); }

dp::u32 clear_bit(dp::u32 value, dp::u8 bit_position) { return value & ~(dp::u32(1) << bit_position); }

dp::boolean test_bit(dp::u32 value, dp::u8 bit_position) { return (value & (dp::u32(1) << bit_position)) != 0; }

// Example 6: Fixed-size array with dp:: types
template <typename T, dp::usize N> struct Array {
    T data[N];

    constexpr dp::usize size() const { return N; }

    T &operator[](dp::usize index) { return data[index]; }
    const T &operator[](dp::usize index) const { return data[index]; }
};

// Example 7: Color representation
struct Color {
    dp::u8 r;
    dp::u8 g;
    dp::u8 b;
    dp::u8 a;

    static Color from_rgba(dp::u8 red, dp::u8 green, dp::u8 blue, dp::u8 alpha = 255) {
        return Color{red, green, blue, alpha};
    }

    dp::u32 to_u32() const { return (dp::u32(r) << 24) | (dp::u32(g) << 16) | (dp::u32(b) << 8) | dp::u32(a); }
};

// Example 8: Range type
struct Range {
    dp::i32 start;
    dp::i32 end;

    dp::boolean contains(dp::i32 value) const { return value >= start && value < end; }

    dp::usize length() const { return static_cast<dp::usize>(end - start); }
};

int main() {
    // Test basic types
    basic_types_example();

    // Test checksum calculation
    dp::byte data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    dp::u32 checksum = calculate_checksum(data, sizeof(data));
    if (checksum != 15)
        return 1;

    // Test packet
    Packet packet;
    packet.id = 12345;
    packet.length = 256;
    packet.type = 1;
    packet.flags = 0x80;

    // Test buffer
    Buffer<dp::i32> buffer(10);
    buffer.push(42);
    buffer.push(100);
    buffer.push(-50);
    if (buffer.get_size() != 3)
        return 1;

    // Test bit manipulation
    dp::u32 flags = 0;
    flags = set_bit(flags, 5);
    if (!test_bit(flags, 5))
        return 1;
    flags = clear_bit(flags, 5);
    if (test_bit(flags, 5))
        return 1;

    // Test fixed-size array
    Array<dp::f64, 5> numbers;
    numbers[0] = 1.1;
    numbers[1] = 2.2;
    numbers[2] = 3.3;
    if (numbers.size() != 5)
        return 1;

    // Test color
    Color red = Color::from_rgba(255, 0, 0);
    dp::u32 red_value = red.to_u32();
    if ((red_value >> 24) != 255)
        return 1;

    // Test range
    Range r{0, 10};
    if (!r.contains(5))
        return 1;
    if (r.contains(10))
        return 1;
    if (r.length() != 10)
        return 1;

    return 0;
}
