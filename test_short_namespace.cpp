#include <datapod/datapod.hpp>
#include <iostream>

int main() {
#ifdef SHORT_NAMESPACE
    // Should work with dp:: prefix
    dp::Vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    std::cout << "SHORT_NAMESPACE is enabled!\n";
    std::cout << "dp::Vector works! Size: " << vec.size() << "\n";

// Test with sequential header only
#include <datapod/sequential.hpp>
    dp::String str("Hello from dp!");
    std::cout << "dp::String: " << str.c_str() << "\n";

    return 0;
#else
    std::cout << "SHORT_NAMESPACE is NOT enabled (expected)\n";
    std::cout << "Use datapod:: prefix instead\n";

    datapod::Vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);

    std::cout << "datapod::Vector works! Size: " << vec.size() << "\n";
    return 0;
#endif
}
