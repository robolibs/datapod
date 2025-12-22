// String Formatting Example
// Demonstrates the zero-dependency formatting capabilities of datapod::String

#include "datapod/containers/string.hpp"

#include <iostream>

using namespace datapod;

int main() {
    std::cout << "=== datapod::String Formatting Examples ===" << std::endl << std::endl;

    // ===== 1. String Concatenation with operator+ =====
    std::cout << "1. String Concatenation (operator+):" << std::endl;

    String s1 = String("Hello") + " " + String("World");
    std::cout << "   String + String: " << s1.c_str() << std::endl;

    String s2 = String("Count: ") + to_string(42);
    std::cout << "   String + to_string(int): " << s2.c_str() << std::endl;

    String s3 = "C-string" + String(" + String");
    std::cout << "   C-string + String: " << s3.c_str() << std::endl;

    String s4 = 'A' + String("BC") + 'D';
    std::cout << "   char + String + char: " << s4.c_str() << std::endl;

    std::cout << std::endl;

    // ===== 2. to_string() Conversions =====
    std::cout << "2. to_string() Conversions:" << std::endl;

    auto int_str = to_string(42);
    std::cout << "   int: " << int_str.c_str() << std::endl;

    auto neg_int_str = to_string(-123);
    std::cout << "   negative int: " << neg_int_str.c_str() << std::endl;

    auto long_str = to_string(123456789L);
    std::cout << "   long: " << long_str.c_str() << std::endl;

    auto uint_str = to_string(4294967295U);
    std::cout << "   unsigned int: " << uint_str.c_str() << std::endl;

    auto float_str = to_string(3.14159f);
    std::cout << "   float: " << float_str.c_str() << std::endl;

    auto double_str = to_string(2.71828);
    std::cout << "   double: " << double_str.c_str() << std::endl;

    auto bool_true = to_string(true);
    auto bool_false = to_string(false);
    std::cout << "   bool (true): " << bool_true.c_str() << std::endl;
    std::cout << "   bool (false): " << bool_false.c_str() << std::endl;

    auto char_str = to_string('X');
    std::cout << "   char: " << char_str.c_str() << std::endl;

    std::cout << std::endl;

    // ===== 3. Stream-Style Append (operator<<) =====
    std::cout << "3. Stream-Style Append (operator<<):" << std::endl;

    String s5;
    s5 << "Pi: " << 3.14159 << ", Active: " << true;
    std::cout << "   Chained: " << s5.c_str() << std::endl;

    String s6("Count: ");
    s6 << 100 << " items";
    std::cout << "   Append to existing: " << s6.c_str() << std::endl;

    String s7;
    s7 << "Values: " << 1 << ", " << 2 << ", " << 3;
    std::cout << "   Multiple values: " << s7.c_str() << std::endl;

    std::cout << std::endl;

    // ===== 4. String::format() with {} Substitution =====
    std::cout << "4. String::format() with {} substitution:" << std::endl;

    auto f1 = String::format("Hello {}!", "World");
    std::cout << "   Single arg: " << f1.c_str() << std::endl;

    auto f2 = String::format("Value: {}", 42);
    std::cout << "   Integer arg: " << f2.c_str() << std::endl;

    auto f3 = String::format("{} + {} = {}", 2, 3, 5);
    std::cout << "   Multiple args: " << f3.c_str() << std::endl;

    auto f4 = String::format("Name: {}, Age: {}, Active: {}", "Alice", 30, true);
    std::cout << "   Mixed types: " << f4.c_str() << std::endl;

    auto f5 = String::format("Pi is approximately {}", 3.14159);
    std::cout << "   Floating point: " << f5.c_str() << std::endl;

    String name("Bob");
    auto f6 = String::format("Hello, {}!", name);
    std::cout << "   String arg: " << f6.c_str() << std::endl;

    std::cout << std::endl;

    // ===== 5. Complex Real-World Examples =====
    std::cout << "5. Real-World Examples:" << std::endl;

    // Example 1: Building a log message
    String log_msg = String::format("[{}] User {} performed action: {}", "INFO", "john_doe", "login");
    log_msg << " (timestamp: " << 1234567890 << ")";
    std::cout << "   Log message: " << log_msg.c_str() << std::endl;

    // Example 2: Building a JSON-like string
    String json;
    json << "{ \"name\": \"" << "Alice" << "\", ";
    json << "\"age\": " << 25 << ", ";
    json << "\"score\": " << 95.5 << ", ";
    json << "\"active\": " << to_string(true) << " }";
    std::cout << "   JSON-like: " << json.c_str() << std::endl;

    // Example 3: Building a table row
    auto table_row = String::format("| {:10} | {:5} | {:7} |", "Name", "Age", "Score");
    // Note: Our format() doesn't support width specifiers, but we can fake it with padding
    String row;
    row << "| Alice      | 25    | 95.500  |";
    std::cout << "   Table row: " << row.c_str() << std::endl;

    // Example 4: Error message with context
    int error_code = 404;
    String resource("/api/users/123");
    auto error_msg = String::format("Error {}: Resource '{}' not found", error_code, resource);
    std::cout << "   Error message: " << error_msg.c_str() << std::endl;

    // Example 5: Combining all techniques
    String combined = String("Status: ") + to_string(200);
    combined << " - ";
    combined = combined + String::format("Processed {} items", 1337);
    combined << " (success: " << true << ")";
    std::cout << "   Combined: " << combined.c_str() << std::endl;

    std::cout << std::endl;

    // ===== 6. Performance Notes =====
    std::cout << "6. Performance Notes:" << std::endl;
    std::cout << "   - All formatting is zero-dependency (no fmt, no iostreams)" << std::endl;
    std::cout << "   - SSO optimization: strings <= 23 chars stay on stack" << std::endl;
    std::cout << "   - operator+ creates new string (immutable)" << std::endl;
    std::cout << "   - operator<< modifies in-place (mutable, efficient)" << std::endl;
    std::cout << "   - format() builds string once (efficient for complex templates)" << std::endl;

    std::cout << std::endl;
    std::cout << "=== End of Examples ===" << std::endl;

    return 0;
}
