#include <datapod/pods/sequential/bytes.hpp>
#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== Bytes Usage Examples ===" << std::endl << std::endl;

    // 1. Construction
    std::cout << "1. Construction:" << std::endl;
    Bytes b1;                            // Default: empty
    Bytes b2(10);                        // Size 10, all bytes 0
    Bytes b3(5, 0xFF);                   // Size 5, all bytes 0xFF
    Bytes b4 = {0x01, 0x02, 0x03, 0x04}; // Initializer list

    std::cout << "   Default size: " << b1.size() << std::endl;
    std::cout << "   b2 size: " << b2.size() << std::endl;
    std::cout << "   b3 size: " << b3.size() << ", value at 0: 0x" << std::hex << (int)b3[0] << std::dec << std::endl;
    std::cout << "   b4 from initializer list: size=" << b4.size() << std::endl << std::endl;

    // 2. Raw Pointer Construction
    std::cout << "2. Raw Pointer Construction:" << std::endl;
    u8 raw_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    Bytes b5(raw_data, 4);
    std::cout << "   From raw pointer: ";
    for (size_t i = 0; i < b5.size(); ++i) {
        std::cout << "0x" << std::hex << (int)b5[i] << " ";
    }
    std::cout << std::dec << std::endl << std::endl;

    // 3. Element Access
    std::cout << "3. Element Access:" << std::endl;
    Bytes b(10);
    b[0] = 0x01;
    b[5] = 0x55;
    b[9] = 0xFF;

    std::cout << "   b[0]: 0x" << std::hex << (int)b[0] << std::endl;
    std::cout << "   b[5]: 0x" << (int)b[5] << std::endl;
    std::cout << "   b[9]: 0x" << (int)b[9] << std::dec << std::endl;
    std::cout << "   front(): 0x" << std::hex << (int)b.front() << std::endl;
    std::cout << "   back(): 0x" << (int)b.back() << std::dec << std::endl << std::endl;

    // 4. Raw Data Access
    std::cout << "4. Raw Data Access:" << std::endl;
    u8 *ptr = b.data();
    void *vptr = b.void_data();
    std::cout << "   data(): 0x" << std::hex << (void *)ptr << std::endl;
    std::cout << "   void_data(): 0x" << vptr << std::dec << std::endl << std::endl;

    // 5. Push and Pop
    std::cout << "5. Push and Pop:" << std::endl;
    Bytes growing;
    growing.push_back(0x11);
    growing.push_back(0x22);
    growing.push_back(0x33);

    std::cout << "   After pushes: size = " << growing.size() << ", bytes: ";
    for (size_t i = 0; i < growing.size(); ++i) {
        std::cout << "0x" << std::hex << (int)growing[i] << " ";
    }
    std::cout << std::dec << std::endl;

    growing.pop_back();
    std::cout << "   After pop: size = " << growing.size() << std::endl << std::endl;

    // 6. Append
    std::cout << "6. Append:" << std::endl;
    Bytes b6 = {0x01, 0x02};
    u8 more[] = {0x03, 0x04, 0x05};
    b6.append(more, 3);

    std::cout << "   After append: size = " << b6.size() << ", bytes: ";
    for (size_t i = 0; i < b6.size(); ++i) {
        std::cout << "0x" << std::hex << (int)b6[i] << " ";
    }
    std::cout << std::dec << std::endl;

    Bytes b7 = {0x10, 0x20};
    Bytes b8 = {0x30, 0x40, 0x50};
    b7.append(b8);
    std::cout << "   After append(Bytes): size = " << b7.size() << std::endl << std::endl;

    // 7. Resize
    std::cout << "7. Resize:" << std::endl;
    Bytes b9(5);
    b9[2] = 0xFF;
    std::cout << "   Before: size = " << b9.size() << std::endl;

    b9.resize(10);
    std::cout << "   After resize(10): size = " << b9.size() << std::endl;
    std::cout << "   b9[2] still set: " << (b9[2] == 0xFF ? "yes" : "no") << std::endl << std::endl;

    // 8. Byte Operations
    std::cout << "8. Byte Operations:" << std::endl;
    Bytes b10 = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::cout << "   Original: ";
    for (size_t i = 0; i < b10.size(); ++i) {
        std::cout << "0x" << std::hex << (int)b10[i] << " ";
    }
    std::cout << std::dec << std::endl;

    b10.zero();
    std::cout << "   After zero(): ";
    for (size_t i = 0; i < b10.size(); ++i) {
        std::cout << "0x" << std::hex << (int)b10[i] << " ";
    }
    std::cout << std::dec << std::endl;

    b10.fill(0xAA);
    std::cout << "   After fill(0xAA): ";
    for (size_t i = 0; i < b10.size(); ++i) {
        std::cout << "0x" << std::hex << (int)b10[i] << " ";
    }
    std::cout << std::dec << std::endl << std::endl;

    // 9. Capacity Operations
    std::cout << "9. Capacity:" << std::endl;
    Bytes cap;
    std::cout << "   Initial size: " << cap.size() << ", capacity: " << cap.capacity() << std::endl;

    cap.reserve(1000);
    std::cout << "   After reserve(1000) - size: " << cap.size() << ", capacity: " << cap.capacity() << std::endl;

    cap.resize(50);
    std::cout << "   After resize(50) - size: " << cap.size() << std::endl << std::endl;

    // 10. Clear
    std::cout << "10. Clear Operation:" << std::endl;
    Bytes clearable(20);
    clearable[5] = 0xFF;
    clearable[15] = 0xAA;
    std::cout << "   Before clear: size = " << clearable.size() << std::endl;

    clearable.clear();
    std::cout << "   After clear: size = " << clearable.size() << ", empty = " << (clearable.empty() ? "true" : "false")
              << std::endl
              << std::endl;

    // 11. Comparison
    std::cout << "11. Comparison:" << std::endl;
    Bytes cmp1 = {0x01, 0x02, 0x03};
    Bytes cmp2 = {0x01, 0x02, 0x03};
    Bytes cmp3 = {0x01, 0x02, 0x04};

    std::cout << "   cmp1 == cmp2: " << (cmp1 == cmp2 ? "true" : "false") << std::endl;
    std::cout << "   cmp1 != cmp3: " << (cmp1 != cmp3 ? "true" : "false") << std::endl;
    std::cout << "   cmp1 < cmp3: " << (cmp1 < cmp3 ? "true" : "false") << std::endl << std::endl;

    // 12. Search Operations
    std::cout << "12. Search Operations:" << std::endl;
    Bytes search = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::cout << "   Bytes: ";
    for (size_t i = 0; i < search.size(); ++i) {
        std::cout << "0x" << std::hex << (int)search[i] << " ";
    }
    std::cout << std::dec << std::endl;

    auto pos = search.find(0x03);
    std::cout << "   find(0x03): " << pos << std::endl;

    auto rpos = search.rfind(0x02);
    std::cout << "   rfind(0x02): " << rpos << std::endl;

    std::cout << "   contains(0x04): " << (search.contains(0x04) ? "true" : "false") << std::endl;
    std::cout << "   contains(0xFF): " << (search.contains(0xFF) ? "true" : "false") << std::endl;

    Bytes prefix = {0x01, 0x02, 0x03};
    std::cout << "   starts_with({0x01, 0x02, 0x03}): " << (search.starts_with(prefix) ? "true" : "false") << std::endl;

    Bytes suffix = {0x03, 0x04, 0x05};
    std::cout << "   ends_with({0x03, 0x04, 0x05}): " << (search.ends_with(suffix) ? "true" : "false") << std::endl
              << std::endl;

    // 13. Sub-bytes
    std::cout << "13. Sub-bytes:" << std::endl;
    Bytes sub_src = {0x01, 0x02, 0x03, 0x04, 0x05};
    Bytes sub = sub_src.substr(1, 3);
    std::cout << "   substr(1, 3): ";
    for (size_t i = 0; i < sub.size(); ++i) {
        std::cout << "0x" << std::hex << (int)sub[i] << " ";
    }
    std::cout << std::dec << std::endl << std::endl;

    // 14. Concatenation
    std::cout << "14. Concatenation:" << std::endl;
    Bytes c1 = {0x01, 0x02};
    Bytes c2 = {0x03, 0x04};
    Bytes concat = c1 + c2;

    std::cout << "   c1 + c2: ";
    for (size_t i = 0; i < concat.size(); ++i) {
        std::cout << "0x" << std::hex << (int)concat[i] << " ";
    }
    std::cout << std::dec << std::endl << std::endl;

    // 15. Iteration
    std::cout << "15. Iteration:" << std::endl;
    Bytes iter = {0x10, 0x20, 0x30, 0x40};
    std::cout << "   Bytes: ";
    for (auto byte : iter) {
        std::cout << "0x" << std::hex << (int)byte << " ";
    }
    std::cout << std::dec << std::endl << std::endl;

    // 16. Insert and Erase
    std::cout << "16. Insert and Erase:" << std::endl;
    Bytes ie = {0x01, 0x02, 0x04};
    std::cout << "   Before insert: size = " << ie.size() << std::endl;
    ie.insert(ie.begin() + 2, 0x03);
    std::cout << "   After insert(0x03 at pos 2): size = " << ie.size() << std::endl;

    ie.erase(ie.begin() + 0);
    std::cout << "   After erase(0): size = " << ie.size() << std::endl << std::endl;

    // 17. Copy and Move
    std::cout << "17. Copy and Move:" << std::endl;
    Bytes original = {0xAA, 0xBB, 0xCC};
    Bytes copy = original;
    Bytes moved = std::move(original);

    std::cout << "   Copy size: " << copy.size() << std::endl;
    std::cout << "   Moved size: " << moved.size() << std::endl;
    std::cout << "   Original size after move: " << original.size() << std::endl << std::endl;

    // 18. Swap
    std::cout << "18. Swap:" << std::endl;
    Bytes swap1 = {0x11, 0x22};
    Bytes swap2 = {0x33, 0x44};
    std::cout << "   Before swap: swap1[0]=0x" << std::hex << (int)swap1[0] << ", swap2[0]=0x" << (int)swap2[0]
              << std::dec << std::endl;

    swap1.swap(swap2);
    std::cout << "   After swap: swap1[0]=0x" << std::hex << (int)swap1[0] << ", swap2[0]=0x" << (int)swap2[0]
              << std::dec << std::endl
              << std::endl;

    // 19. Serialization (members())
    std::cout << "19. Serialization (members()):" << std::endl;
    Bytes serial(10);
    serial[0] = 0x01;
    serial[5] = 0xFF;
    serial[9] = 0x55;

    std::cout << "   Original size: " << serial.size() << std::endl;

    auto [data] = serial.members();
    std::cout << "   Extracted data size: " << data.size() << std::endl;
    std::cout << "   Serialization support verified!" << std::endl << std::endl;

    // 20. Large Bytes
    std::cout << "20. Large Bytes:" << std::endl;
    Bytes large(10000);
    large[5000] = 0xFF;
    large[9999] = 0xAA;

    std::cout << "   Size: " << large.size() << std::endl;
    std::cout << "   Byte at 5000: 0x" << std::hex << (int)large[5000] << std::endl;
    std::cout << "   Byte at 9999: 0x" << (int)large[9999] << std::dec << std::endl << std::endl;

    std::cout << "=== All Bytes Examples Complete ===" << std::endl;

    return 0;
}
