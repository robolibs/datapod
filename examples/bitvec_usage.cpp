#include <datapod/sequential/bitvec.hpp>
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== Bitvec Usage Examples ===" << std::endl << std::endl;

    // 1. Construction
    std::cout << "1. Construction:" << std::endl;
    Bitvec bv1;             // Default: empty
    Bitvec bv2(10);         // Size 10, all bits 0
    Bitvec bv3("10101010"); // From string

    std::cout << "   Default size: " << bv1.size() << std::endl;
    std::cout << "   bv2 size: " << bv2.size() << std::endl;
    std::cout << "   bv3 from string: " << bv3.str() << std::endl << std::endl;

    // 2. Setting and Testing Bits
    std::cout << "2. Setting and Testing Bits:" << std::endl;
    Bitvec bv(10);
    bv.set(0, true);
    bv.set(3, true);
    bv.set(7, true);

    std::cout << "   Bit 0: " << (bv.test(0) ? "1" : "0") << std::endl;
    std::cout << "   Bit 1: " << (bv.test(1) ? "1" : "0") << std::endl;
    std::cout << "   Bit 3: " << (bv.test(3) ? "1" : "0") << std::endl;
    std::cout << "   Bit 7: " << (bv.test(7) ? "1" : "0") << std::endl;
    std::cout << "   String: " << bv.str() << std::endl << std::endl;

    // 3. Push and Pop
    std::cout << "3. Push and Pop:" << std::endl;
    Bitvec growing;
    growing.push_back(true);
    growing.push_back(false);
    growing.push_back(true);
    growing.push_back(true);

    std::cout << "   After pushes: " << growing.str() << " (size: " << growing.size() << ")" << std::endl;

    growing.pop_back();
    std::cout << "   After pop:    " << growing.str() << " (size: " << growing.size() << ")" << std::endl << std::endl;

    // 4. Flip Operations
    std::cout << "4. Flip Operations:" << std::endl;
    Bitvec flipper("10101");
    std::cout << "   Original:     " << flipper.str() << std::endl;

    flipper.flip(2); // Flip single bit
    std::cout << "   After flip(2): " << flipper.str() << std::endl;

    flipper.flip(); // Flip all bits
    std::cout << "   After flip():  " << flipper.str() << std::endl << std::endl;

    // 5. Query Operations
    std::cout << "5. Query Operations:" << std::endl;
    Bitvec query("10001000");
    std::cout << "   Bitvec: " << query.str() << std::endl;
    std::cout << "   count(): " << query.count() << std::endl;
    std::cout << "   any():   " << (query.any() ? "true" : "false") << std::endl;
    std::cout << "   none():  " << (query.none() ? "true" : "false") << std::endl;

    Bitvec empty_query(10);
    std::cout << "   Empty bitvec none(): " << (empty_query.none() ? "true" : "false") << std::endl << std::endl;

    // 6. Capacity Operations
    std::cout << "6. Capacity:" << std::endl;
    Bitvec cap;
    std::cout << "   Initial size: " << cap.size() << ", capacity: " << cap.capacity() << std::endl;

    cap.reserve(1000);
    std::cout << "   After reserve(1000) - size: " << cap.size() << ", capacity: " << cap.capacity() << std::endl;

    cap.resize(50);
    std::cout << "   After resize(50) - size: " << cap.size() << std::endl << std::endl;

    // 7. Clear
    std::cout << "7. Clear Operation:" << std::endl;
    Bitvec clearable(20);
    clearable.set(5, true);
    clearable.set(15, true);
    std::cout << "   Before clear: size = " << clearable.size() << ", count = " << clearable.count() << std::endl;

    clearable.clear();
    std::cout << "   After clear:  size = " << clearable.size()
              << ", empty = " << (clearable.empty() ? "true" : "false") << std::endl
              << std::endl;

    // 8. Bitwise Operations
    std::cout << "8. Bitwise Operations:" << std::endl;
    Bitvec a("11110000");
    Bitvec b("10101010");

    std::cout << "   a:     " << a.str() << std::endl;
    std::cout << "   b:     " << b.str() << std::endl;

    Bitvec and_result = a;
    and_result &= b;
    std::cout << "   a & b: " << and_result.str() << std::endl;

    Bitvec or_result = a;
    or_result |= b;
    std::cout << "   a | b: " << or_result.str() << std::endl;

    Bitvec xor_result = a;
    xor_result ^= b;
    std::cout << "   a ^ b: " << xor_result.str() << std::endl;

    Bitvec not_result = ~a;
    std::cout << "   ~a:    " << not_result.str() << std::endl << std::endl;

    // 9. Iteration Over Set Bits
    std::cout << "9. Iteration Over Set Bits:" << std::endl;
    Bitvec iter("100010001");
    std::cout << "   Bitvec: " << iter.str() << std::endl;
    std::cout << "   Set bits at indices: ";
    iter.for_each_set_bit([](auto idx) { std::cout << idx << " "; });
    std::cout << std::endl << std::endl;

    // 10. Next Set Bit
    std::cout << "10. Next Set Bit:" << std::endl;
    Bitvec search("100010001");
    std::cout << "   Bitvec: " << search.str() << std::endl;

    auto idx = search.next_set_bit(0);
    while (idx.has_value()) {
        std::cout << "   Found set bit at index: " << *idx << std::endl;
        idx = search.next_set_bit(*idx + 1);
    }
    std::cout << std::endl;

    // 11. Comparison
    std::cout << "11. Comparison:" << std::endl;
    Bitvec cmp1("10101");
    Bitvec cmp2("10101");
    Bitvec cmp3("01010");

    std::cout << "   cmp1: " << cmp1.str() << std::endl;
    std::cout << "   cmp2: " << cmp2.str() << std::endl;
    std::cout << "   cmp3: " << cmp3.str() << std::endl;
    std::cout << "   cmp1 == cmp2: " << (cmp1 == cmp2 ? "true" : "false") << std::endl;
    std::cout << "   cmp1 != cmp3: " << (cmp1 != cmp3 ? "true" : "false") << std::endl << std::endl;

    // 12. Serialization
    std::cout << "12. Serialization (members()):" << std::endl;
    Bitvec original(100);
    original.set(10, true);
    original.set(50, true);
    original.set(99, true);

    std::cout << "   Original count: " << original.count() << std::endl;

    auto [size, blocks] = original.members();
    std::cout << "   Extracted size: " << size << ", blocks: " << blocks.size() << std::endl;
    std::cout << "   Serialization support verified!" << std::endl << std::endl;

    // 13. Large Bitvec
    std::cout << "13. Large Bitvec:" << std::endl;
    Bitvec large(10000);
    large.set(5000, true);
    large.set(9999, true);

    std::cout << "   Size: " << large.size() << std::endl;
    std::cout << "   Count: " << large.count() << std::endl;
    std::cout << "   Bit 5000: " << (large.test(5000) ? "1" : "0") << std::endl;
    std::cout << "   Bit 9999: " << (large.test(9999) ? "1" : "0") << std::endl << std::endl;

    // 14. Reset
    std::cout << "14. Reset:" << std::endl;
    Bitvec resettable(50);
    resettable.set(25, true);
    std::cout << "   Before reset: size = " << resettable.size() << std::endl;

    resettable.reset();
    std::cout << "   After reset:  size = " << resettable.size()
              << ", empty = " << (resettable.empty() ? "true" : "false") << std::endl
              << std::endl;

    std::cout << "=== All Bitvec Examples Complete ===" << std::endl;

    return 0;
}
