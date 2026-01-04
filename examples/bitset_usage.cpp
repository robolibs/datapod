#include <datapod/pods/adapters/bitset.hpp>
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== Bitset Usage Examples ===" << std::endl << std::endl;

    // Construction
    std::cout << "1. Construction:" << std::endl;
    Bitset<8> bs1;               // Default: all zeros
    Bitset<8> bs2("10101010");   // From string
    auto bs3 = Bitset<8>::max(); // All ones

    std::cout << "   Default:     " << bs1.to_string() << std::endl;
    std::cout << "   From string: " << bs2.to_string() << std::endl;
    std::cout << "   Max:         " << bs3.to_string() << std::endl << std::endl;

    // Setting bits
    std::cout << "2. Setting bits:" << std::endl;
    Bitset<8> bs;
    bs.set(0);       // Set bit 0
    bs.set(3, true); // Set bit 3
    bs.set(5);       // Set bit 5
    std::cout << "   Individual:  " << bs.to_string() << std::endl;

    bs.set(); // Set all bits
    std::cout << "   All set:     " << bs.to_string() << std::endl << std::endl;

    // Resetting bits
    std::cout << "3. Resetting bits:" << std::endl;
    bs.reset(0); // Reset bit 0
    bs.reset(3); // Reset bit 3
    std::cout << "   Individual:  " << bs.to_string() << std::endl;

    bs.reset(); // Reset all bits
    std::cout << "   All reset:   " << bs.to_string() << std::endl << std::endl;

    // Flipping bits
    std::cout << "4. Flipping bits:" << std::endl;
    bs.flip(1); // Flip bit 1
    bs.flip(4); // Flip bit 4
    std::cout << "   Individual:  " << bs.to_string() << std::endl;

    bs.flip(); // Flip all bits
    std::cout << "   All flipped: " << bs.to_string() << std::endl << std::endl;

    // Query operations
    std::cout << "5. Query operations:" << std::endl;
    Bitset<8> query("10001000");
    std::cout << "   Bitset:      " << query.to_string() << std::endl;
    std::cout << "   count():     " << query.count() << std::endl;
    std::cout << "   all():       " << (query.all() ? "true" : "false") << std::endl;
    std::cout << "   any():       " << (query.any() ? "true" : "false") << std::endl;
    std::cout << "   none():      " << (query.none() ? "true" : "false") << std::endl;
    std::cout << "   test(3):     " << (query.test(3) ? "true" : "false") << std::endl << std::endl;

    // Conversions
    std::cout << "6. Conversions:" << std::endl;
    Bitset<8> conv("00001111");
    std::cout << "   Bitset:      " << conv.to_string() << std::endl;
    std::cout << "   to_ulong():  " << conv.to_ulong() << std::endl;
    std::cout << "   to_ullong(): " << conv.to_ullong() << std::endl << std::endl;

    // Bitwise operations
    std::cout << "7. Bitwise operations:" << std::endl;
    Bitset<8> a("11110000");
    Bitset<8> b("10101010");
    std::cout << "   a:           " << a.to_string() << std::endl;
    std::cout << "   b:           " << b.to_string() << std::endl;
    std::cout << "   a & b:       " << (a & b).to_string() << std::endl;
    std::cout << "   a | b:       " << (a | b).to_string() << std::endl;
    std::cout << "   a ^ b:       " << (a ^ b).to_string() << std::endl;
    std::cout << "   ~a:          " << (~a).to_string() << std::endl << std::endl;

    // Shift operations
    std::cout << "8. Shift operations:" << std::endl;
    Bitset<8> shift("00001111");
    std::cout << "   Original:    " << shift.to_string() << std::endl;
    std::cout << "   << 2:        " << (shift << 2).to_string() << std::endl;
    std::cout << "   >> 2:        " << (shift >> 2).to_string() << std::endl << std::endl;

    // Method chaining
    std::cout << "9. Method chaining:" << std::endl;
    Bitset<8> chain;
    chain.set().flip(0).flip(2).reset(7);
    std::cout << "   set().flip(0).flip(2).reset(7): " << chain.to_string() << std::endl << std::endl;

    // Iteration over set bits
    std::cout << "10. Iterating over set bits:" << std::endl;
    Bitset<16> iter("1010000000001010");
    std::cout << "    Bitset: " << iter.to_string() << std::endl;
    std::cout << "    Set bits at indices: ";
    iter.for_each_set_bit([](std::size_t i) { std::cout << i << " "; });
    std::cout << std::endl << std::endl;

    // Large bitsets
    std::cout << "11. Large bitsets:" << std::endl;
    Bitset<128> large;
    large.set(0);
    large.set(64);
    large.set(127);
    std::cout << "    128-bit bitset with bits 0, 64, 127 set" << std::endl;
    std::cout << "    count(): " << large.count() << std::endl;
    std::cout << "    any():   " << (large.any() ? "true" : "false") << std::endl << std::endl;

    // Comparison
    std::cout << "12. Comparison:" << std::endl;
    Bitset<8> cmp1("00001111");
    Bitset<8> cmp2("00001111");
    Bitset<8> cmp3("11110000");
    std::cout << "    cmp1:        " << cmp1.to_string() << std::endl;
    std::cout << "    cmp2:        " << cmp2.to_string() << std::endl;
    std::cout << "    cmp3:        " << cmp3.to_string() << std::endl;
    std::cout << "    cmp1 == cmp2: " << (cmp1 == cmp2 ? "true" : "false") << std::endl;
    std::cout << "    cmp1 != cmp3: " << (cmp1 != cmp3 ? "true" : "false") << std::endl;
    std::cout << "    cmp1 < cmp3:  " << (cmp1 < cmp3 ? "true" : "false") << std::endl << std::endl;

    return 0;
}
