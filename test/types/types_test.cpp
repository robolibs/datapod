#include <datapod/types/types.hpp>

// Test that all types are defined and have correct sizes
static_assert(sizeof(datapod::i8) == 1, "i8 should be 1 byte");
static_assert(sizeof(datapod::i16) == 2, "i16 should be 2 bytes");
static_assert(sizeof(datapod::i32) == 4, "i32 should be 4 bytes");
static_assert(sizeof(datapod::i64) == 8, "i64 should be 8 bytes");

static_assert(sizeof(datapod::u8) == 1, "u8 should be 1 byte");
static_assert(sizeof(datapod::u16) == 2, "u16 should be 2 bytes");
static_assert(sizeof(datapod::u32) == 4, "u32 should be 4 bytes");
static_assert(sizeof(datapod::u64) == 8, "u64 should be 8 bytes");

static_assert(sizeof(datapod::f32) == 4, "f32 should be 4 bytes");
static_assert(sizeof(datapod::f64) == 8, "f64 should be 8 bytes");

// Test signedness using built-in type traits
static_assert((datapod::i8(-1) < datapod::i8(0)), "i8 should be signed");
static_assert((datapod::i16(-1) < datapod::i16(0)), "i16 should be signed");
static_assert((datapod::i32(-1) < datapod::i32(0)), "i32 should be signed");
static_assert((datapod::i64(-1) < datapod::i64(0)), "i64 should be signed");

static_assert((datapod::u8(-1) > datapod::u8(0)), "u8 should be unsigned");
static_assert((datapod::u16(-1) > datapod::u16(0)), "u16 should be unsigned");
static_assert((datapod::u32(-1) > datapod::u32(0)), "u32 should be unsigned");
static_assert((datapod::u64(-1) > datapod::u64(0)), "u64 should be unsigned");

// Test that dp:: namespace works (same underlying type)
static_assert(sizeof(dp::i32) == sizeof(datapod::i32), "dp::i32 should match datapod::i32");
static_assert(sizeof(dp::u64) == sizeof(datapod::u64), "dp::u64 should match datapod::u64");
static_assert(sizeof(dp::f32) == sizeof(datapod::f32), "dp::f32 should match datapod::f32");
static_assert(sizeof(dp::f64) == sizeof(datapod::f64), "dp::f64 should match datapod::f64");
static_assert(sizeof(dp::usize) == sizeof(datapod::usize), "dp::usize should match datapod::usize");

int main() {
    // Test basic usage with datapod:: namespace
    datapod::i8 signed_byte = -42;
    datapod::u8 unsigned_byte = 255;
    datapod::i32 signed_int = -1000000;
    datapod::u32 unsigned_int = 4000000000u;
    datapod::i64 signed_long = -9000000000000000000LL;
    datapod::u64 unsigned_long = 18000000000000000000ULL;
    datapod::f32 float_val = 3.14159f;
    datapod::f64 double_val = 2.718281828459045;
    datapod::usize size = sizeof(int);
    datapod::isize diff = -100;
    datapod::boolean flag = true;
    datapod::byte raw_byte = 0xFF;

    // Test basic usage with dp:: namespace (short form)
    dp::i8 sb = -42;
    dp::u8 ub = 255;
    dp::i32 si = -1000000;
    dp::u32 ui = 4000000000u;
    dp::i64 sl = -9000000000000000000LL;
    dp::u64 ul = 18000000000000000000ULL;
    dp::f32 fv = 3.14159f;
    dp::f64 dv = 2.718281828459045;
    dp::usize sz = sizeof(int);
    dp::isize df = -100;
    dp::boolean fg = true;
    dp::byte rb = 0xFF;

    // Test arithmetic operations
    dp::u32 sum = ui + 100;
    dp::f64 product = dv * 2.0;
    dp::i64 difference = sl - 1000;

    // Test that values are correct
    if (signed_byte != -42)
        return 1;
    if (unsigned_byte != 255)
        return 1;
    if (signed_int != -1000000)
        return 1;
    if (unsigned_int != 4000000000u)
        return 1;
    if (float_val < 3.14f || float_val > 3.15f)
        return 1;
    if (double_val < 2.71 || double_val > 2.72)
        return 1;
    if (!flag)
        return 1;
    if (raw_byte != 0xFF)
        return 1;

    // Test dp:: namespace values
    if (sb != -42)
        return 1;
    if (ub != 255)
        return 1;
    if (si != -1000000)
        return 1;
    if (ui != 4000000000u)
        return 1;
    if (fv < 3.14f || fv > 3.15f)
        return 1;
    if (dv < 2.71 || dv > 2.72)
        return 1;
    if (!fg)
        return 1;
    if (rb != 0xFF)
        return 1;

    // Test arithmetic results
    if (sum != ui + 100)
        return 1;
    if (product != dv * 2.0)
        return 1;
    if (difference != sl - 1000)
        return 1;

    // Test min/max values
    dp::u8 max_u8 = 255;
    dp::i8 min_i8 = -128;
    dp::i8 max_i8 = 127;

    if (max_u8 != 255)
        return 1;
    if (min_i8 != -128)
        return 1;
    if (max_i8 != 127)
        return 1;

    return 0;
}
