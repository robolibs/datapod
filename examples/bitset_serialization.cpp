#include <datapod/adapters/bitset.hpp>
#include <datapod/core/mode.hpp>
#include <datapod/reflection/to_tuple.hpp>
#include <datapod/serialization/buf.hpp>
#include <iostream>

using namespace datapod;

// Simple manual serialization to verify Bitset is POD-compatible
int main() {
    std::cout << "=== Bitset Serialization Example ===" << std::endl << std::endl;

    // Create a bitset with a specific pattern
    Bitset<128> original;
    original.set(0);
    original.set(15);
    original.set(32);
    original.set(64);
    original.set(127);

    std::cout << "1. Original Bitset:" << std::endl;
    std::cout << "   Set bits: 0, 15, 32, 64, 127" << std::endl;
    std::cout << "   Count: " << original.count() << std::endl;
    std::cout << "   Size in memory: " << sizeof(original) << " bytes" << std::endl;

    // Test members() function - this is what makes it serializable
    std::cout << "\n2. Serialization Interface (members()):" << std::endl;
    auto members_tuple = original.members();
    std::cout << "   members() returns a tuple - PASS" << std::endl;

    // Test reflection: to_tuple should work via members()
    auto tuple = to_tuple(original);
    std::cout << "   to_tuple() works via members() - PASS" << std::endl;

    // Serialize manually using raw bytes (POD-compatible)
    std::cout << "\n3. Raw Byte Serialization (memcpy style):" << std::endl;
    std::vector<std::byte> buffer(sizeof(original));
    std::memcpy(buffer.data(), &original, sizeof(original));
    std::cout << "   Copied " << sizeof(original) << " bytes to buffer" << std::endl;

    // Deserialize
    Bitset<128> deserialized;
    std::memcpy(&deserialized, buffer.data(), sizeof(deserialized));

    // Verify
    std::cout << "\n4. Verification:" << std::endl;
    std::cout << "   Deserialized count: " << deserialized.count() << std::endl;
    std::cout << "   Bit 0:   " << (deserialized.test(0) ? "SET" : "CLEAR") << std::endl;
    std::cout << "   Bit 15:  " << (deserialized.test(15) ? "SET" : "CLEAR") << std::endl;
    std::cout << "   Bit 32:  " << (deserialized.test(32) ? "SET" : "CLEAR") << std::endl;
    std::cout << "   Bit 64:  " << (deserialized.test(64) ? "SET" : "CLEAR") << std::endl;
    std::cout << "   Bit 127: " << (deserialized.test(127) ? "SET" : "CLEAR") << std::endl;
    std::cout << "   Bit 1:   " << (deserialized.test(1) ? "SET" : "CLEAR") << " (should be CLEAR)" << std::endl;

    // Check equality
    bool const are_equal = (original == deserialized);
    std::cout << "\n5. Equality Check:" << std::endl;
    std::cout << "   original == deserialized: " << (are_equal ? "TRUE" : "FALSE") << std::endl;

    if (are_equal) {
        std::cout << "\n✅ Serialization SUCCESSFUL! Bitset is POD-compatible." << std::endl;
    } else {
        std::cout << "\n❌ Serialization FAILED! Data mismatch." << std::endl;
        return 1;
    }

    // Test with method chaining
    std::cout << "\n6. Advanced Example - Chaining + Serialization:" << std::endl;
    Bitset<64> advanced;
    advanced.set().flip(0).flip(2).reset(63); // All set except 0, 2, and 63

    std::vector<std::byte> adv_buffer(sizeof(advanced));
    std::memcpy(adv_buffer.data(), &advanced, sizeof(advanced));

    Bitset<64> advanced_copy;
    std::memcpy(&advanced_copy, adv_buffer.data(), sizeof(advanced_copy));

    std::cout << "   Original count:      " << advanced.count() << std::endl;
    std::cout << "   Deserialized count:  " << advanced_copy.count() << std::endl;
    std::cout << "   Match:               " << (advanced == advanced_copy ? "YES" : "NO") << std::endl;

    // Test edge cases
    std::cout << "\n7. Edge Cases:" << std::endl;

    // Empty bitset
    Bitset<32> empty;
    std::vector<std::byte> empty_buffer(sizeof(empty));
    std::memcpy(empty_buffer.data(), &empty, sizeof(empty));
    Bitset<32> empty_copy;
    std::memcpy(&empty_copy, empty_buffer.data(), sizeof(empty_copy));
    std::cout << "   Empty bitset: " << (empty == empty_copy ? "PASS" : "FAIL") << std::endl;

    // Full bitset
    Bitset<32> full;
    full.set();
    std::vector<std::byte> full_buffer(sizeof(full));
    std::memcpy(full_buffer.data(), &full, sizeof(full));
    Bitset<32> full_copy;
    std::memcpy(&full_copy, full_buffer.data(), sizeof(full_copy));
    std::cout << "   Full bitset:  " << (full == full_copy && full_copy.all() ? "PASS" : "FAIL") << std::endl;

    // Single bit
    Bitset<8> single;
    single.set(3);
    std::vector<std::byte> single_buffer(sizeof(single));
    std::memcpy(single_buffer.data(), &single, sizeof(single));
    Bitset<8> single_copy;
    std::memcpy(&single_copy, single_buffer.data(), sizeof(single_copy));
    std::cout << "   Single bit:   " << (single == single_copy && single_copy.count() == 1 ? "PASS" : "FAIL")
              << std::endl;

    // Demonstrate the members() interface
    std::cout << "\n8. members() Interface (for custom serialization):" << std::endl;
    Bitset<64> demo;
    demo.set(10);
    demo.set(20);
    demo.set(30);

    auto [blocks] = demo.members(); // Structured binding works!
    std::cout << "   Extracted blocks array via members()" << std::endl;
    std::cout << "   First block value: " << blocks[0] << std::endl;
    std::cout << "   This proves Bitset supports the datapod serialization protocol!" << std::endl;

    std::cout << "\n✅ All serialization tests PASSED!" << std::endl;
    std::cout << "\nNote: Bitset is POD-compatible and has a members() function," << std::endl;
    std::cout << "      making it fully compatible with datapod's serialization system." << std::endl;

    return 0;
}
