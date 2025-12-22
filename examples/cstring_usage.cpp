#include <datapod/sequential/cstring.hpp>
#include <iostream>

using namespace datapod;

void example_construction() {
    std::cout << "=== Construction Examples ===" << std::endl;

    // Default construction (empty)
    Cstring s1;
    std::cout << "Empty: '" << s1 << "' (size=" << s1.size() << ")" << std::endl;

    // Owning construction from C-string
    Cstring s2("Hello, World!");
    std::cout << "Owning: '" << s2 << "' (size=" << s2.size() << ")" << std::endl;

    // Owning construction from std::string
    std::string str = "C++ Strings";
    Cstring s3(str);
    std::cout << "From std::string: '" << s3 << "'" << std::endl;

    // Non-owning view (like string_view)
    const char *literal = "Literal String";
    CstringView sv(literal);
    std::cout << "Non-owning: '" << sv << "' (is_owning=" << sv.is_owning() << ")" << std::endl;

    // Explicit owning/non-owning
    Cstring s4(literal, Cstring::owning);
    Cstring s5(literal, Cstring::non_owning);
    std::cout << "Explicit owning: is_owning=" << s4.is_owning() << std::endl;
    std::cout << "Explicit non-owning: is_owning=" << s5.is_owning() << std::endl;

    std::cout << std::endl;
}

void example_element_access() {
    std::cout << "=== Element Access ===" << std::endl;

    Cstring s("DataPod");

    // Array-style access
    std::cout << "First char: " << s[0] << std::endl;
    std::cout << "Last char: " << s[s.size() - 1] << std::endl;

    // C-string compatible
    std::cout << "C-string: " << s.c_str() << std::endl;

    // Direct data pointer
    std::cout << "Data ptr: " << static_cast<const void *>(s.data()) << std::endl;

    // Modify through index
    s[0] = 'd';
    std::cout << "After modification: " << s << std::endl;

    std::cout << std::endl;
}

void example_capacity_operations() {
    std::cout << "=== Capacity Operations ===" << std::endl;

    Cstring s;
    std::cout << "Empty size: " << s.size() << ", empty: " << s.empty() << std::endl;

    s = "short";
    std::cout << "After 'short': size=" << s.size() << ", capacity=" << s.capacity() << ", is_short=" << s.is_short()
              << std::endl;

    // Reserve doesn't transition to heap if size fits in SSO
    s.reserve(100);
    std::cout << "After reserve(100): size=" << s.size() << ", is_short=" << s.is_short() << std::endl;

    // Long string goes to heap
    s = "this is a very long string that exceeds the SSO buffer limit";
    std::cout << "Long string: size=" << s.size() << ", capacity=" << s.capacity() << ", is_short=" << s.is_short()
              << std::endl;

    std::cout << std::endl;
}

void example_modifiers() {
    std::cout << "=== Modifier Operations ===" << std::endl;

    Cstring s;

    // push_back
    s.push_back('H');
    s.push_back('i');
    s.push_back('!');
    std::cout << "After push_back: " << s << std::endl;

    // append C-string
    s.append(" World");
    std::cout << "After append: " << s << std::endl;

    // append string_view
    std::string_view sv = " from C++";
    s.append(sv);
    std::cout << "After append sv: " << s << std::endl;

    // clear
    s.clear();
    std::cout << "After clear: size=" << s.size() << ", empty=" << s.empty() << std::endl;

    // reset
    s = "test";
    s.reset();
    std::cout << "After reset: size=" << s.size() << std::endl;

    std::cout << std::endl;
}

void example_resize() {
    std::cout << "=== Resize Operations ===" << std::endl;

    Cstring s("hello");
    std::cout << "Original: '" << s << "' (size=" << s.size() << ")" << std::endl;

    // Shrink
    s.resize(3);
    std::cout << "After resize(3): '" << s << "' (size=" << s.size() << ")" << std::endl;

    // Grow (fills with null bytes)
    s.resize(7);
    std::cout << "After resize(7): size=" << s.size() << " [";
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\0') {
            std::cout << "\\0";
        } else {
            std::cout << s[i];
        }
    }
    std::cout << "]" << std::endl;

    // Resize to zero
    s.resize(0);
    std::cout << "After resize(0): size=" << s.size() << ", empty=" << s.empty() << std::endl;

    std::cout << std::endl;
}

void example_sso_demonstration() {
    std::cout << "=== Small String Optimization (SSO) ===" << std::endl;

    // 15 chars fits in SSO
    Cstring s1("123456789012345");
    std::cout << "15 chars: is_short=" << s1.is_short() << " '" << s1 << "'" << std::endl;

    // 16 chars exceeds SSO, goes to heap
    Cstring s2("1234567890123456");
    std::cout << "16 chars: is_short=" << s2.is_short() << " '" << s2 << "'" << std::endl;

    // Growing across the boundary
    Cstring s3("123456789012345");
    std::cout << "Before push_back: is_short=" << s3.is_short() << std::endl;
    s3.push_back('X');
    std::cout << "After push_back: is_short=" << s3.is_short() << " '" << s3 << "'" << std::endl;

    std::cout << std::endl;
}

void example_owning_vs_nonowning() {
    std::cout << "=== Owning vs Non-Owning Semantics ===" << std::endl;

    const char *external = "External Data";

    // Owning: copies the data
    Cstring owning(external, Cstring::owning);
    std::cout << "Owning: is_owning=" << owning.is_owning() << " '" << owning << "'" << std::endl;

    // Non-owning: just references the data
    Cstring non_owning(external, Cstring::non_owning);
    std::cout << "Non-owning: is_owning=" << non_owning.is_owning() << " '" << non_owning << "'" << std::endl;

    // CstringView is always non-owning
    CstringView view(external);
    std::cout << "CstringView: is_owning=" << view.is_owning() << " '" << view << "'" << std::endl;

    // Regular Cstring defaults to owning
    Cstring regular(external);
    std::cout << "Regular Cstring: is_owning=" << regular.is_owning() << std::endl;

    std::cout << std::endl;
}

void example_binary_data() {
    std::cout << "=== Binary Data Handling ===" << std::endl;

    // Can contain embedded null bytes
    const char data[] = {'H', 'e', 'l', '\0', 'l', 'o', '\0'};
    Cstring s(data, 7);

    std::cout << "Binary data size: " << s.size() << std::endl;
    std::cout << "Content bytes: [";
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\0') {
            std::cout << "\\0";
        } else {
            std::cout << s[i];
        }
        if (i < s.size() - 1)
            std::cout << ", ";
    }
    std::cout << "]" << std::endl;

    // Verify null bytes are preserved
    std::cout << "s[3] is null: " << (s[3] == '\0' ? "yes" : "no") << std::endl;
    std::cout << "s[6] is null: " << (s[6] == '\0' ? "yes" : "no") << std::endl;

    std::cout << std::endl;
}

void example_iterators() {
    std::cout << "=== Iterator Support ===" << std::endl;

    Cstring s("Iterator");

    // Range-based for loop
    std::cout << "Range-based for: ";
    for (char c : s) {
        std::cout << c << " ";
    }
    std::cout << std::endl;

    // Explicit iterators
    std::cout << "Explicit iterators: ";
    for (auto it = s.begin(); it != s.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    // Modify through iterator
    if (s.size() > 0) {
        *s.begin() = 'i';
        std::cout << "After modifying first char: " << s << std::endl;
    }

    std::cout << std::endl;
}

void example_comparisons() {
    std::cout << "=== Comparison Operations ===" << std::endl;

    Cstring s1("apple");
    Cstring s2("banana");
    Cstring s3("apple");

    std::cout << "s1 == s3: " << (s1 == s3 ? "true" : "false") << std::endl;
    std::cout << "s1 != s2: " << (s1 != s2 ? "true" : "false") << std::endl;
    std::cout << "s1 < s2: " << (s1 < s2 ? "true" : "false") << std::endl;
    std::cout << "s2 > s1: " << (s2 > s1 ? "true" : "false") << std::endl;

    // Compare with C-string
    std::cout << "s1 == \"apple\": " << (s1 == "apple" ? "true" : "false") << std::endl;

    // Compare with string_view
    std::string_view sv = "banana";
    std::cout << "s2 == sv: " << (s2 == sv ? "true" : "false") << std::endl;

    std::cout << std::endl;
}

void example_conversions() {
    std::cout << "=== Type Conversions ===" << std::endl;

    Cstring s("Convert Me");

    // To std::string_view (implicit)
    std::string_view sv = s;
    std::cout << "To string_view: " << sv << std::endl;

    // To std::string (explicit)
    std::string str = s.str();
    std::cout << "To std::string: " << str << std::endl;

    // Get view explicitly
    std::string_view view = s.view();
    std::cout << "Explicit view: " << view << std::endl;

    // C-string compatibility
    const char *cstr = s.c_str();
    std::cout << "C-string: " << cstr << std::endl;

    std::cout << std::endl;
}

void example_copy_and_move() {
    std::cout << "=== Copy and Move Semantics ===" << std::endl;

    Cstring original("Original");
    std::cout << "Original: " << original << std::endl;

    // Copy construction
    Cstring copy(original);
    std::cout << "Copy: " << copy << std::endl;

    // Copy assignment
    Cstring copy_assigned;
    copy_assigned = original;
    std::cout << "Copy assigned: " << copy_assigned << std::endl;

    // Move construction
    Cstring move_constructed(std::move(copy));
    std::cout << "Move constructed: " << move_constructed << std::endl;

    // Move assignment
    Cstring move_assigned;
    move_assigned = std::move(copy_assigned);
    std::cout << "Move assigned: " << move_assigned << std::endl;

    std::cout << std::endl;
}

int main() {
    std::cout << "DataPod CString Usage Examples" << std::endl;
    std::cout << "===============================" << std::endl << std::endl;

    example_construction();
    example_element_access();
    example_capacity_operations();
    example_modifiers();
    example_resize();
    example_sso_demonstration();
    example_owning_vs_nonowning();
    example_binary_data();
    example_iterators();
    example_comparisons();
    example_conversions();
    example_copy_and_move();

    std::cout << "All examples completed successfully!" << std::endl;

    return 0;
}
