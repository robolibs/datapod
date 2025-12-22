#include <doctest/doctest.h>

#include "datapod/datapod.hpp"

using namespace datapod;

TEST_SUITE("String") {
    TEST_CASE("Construction - Default") {
        String s;
        CHECK(s.size() == 0);
        CHECK(s.empty() == true);
        CHECK(s.c_str()[0] == '\0');
    }

    TEST_CASE("Construction - C-string") {
        String s("Hello");
        CHECK(s.size() == 5);
        CHECK(s.c_str() == std::string_view("Hello"));
        CHECK(s[0] == 'H');
        CHECK(s[4] == 'o');
    }

    TEST_CASE("Construction - C-string with length") {
        String s("Hello World", 5);
        CHECK(s.size() == 5);
        CHECK(s.c_str() == std::string_view("Hello"));
    }

    TEST_CASE("Construction - string_view") {
        std::string_view sv = "Test String";
        String s(sv);
        CHECK(s.size() == 11);
        CHECK(s.c_str() == sv);
    }

    TEST_CASE("Construction - Copy") {
        String s1("Original");
        String s2(s1);
        CHECK(s1.size() == s2.size());
        CHECK(std::string_view(s1.c_str()) == std::string_view(s2.c_str()));

        // Modify s2, s1 should remain unchanged
        s2 = String("Modified");
        CHECK(s1.c_str() == std::string_view("Original"));
        CHECK(s2.c_str() == std::string_view("Modified"));
    }

    TEST_CASE("Construction - Move") {
        String s1("Original");
        String s2(std::move(s1));
        CHECK(s2.c_str() == std::string_view("Original"));
    }

    TEST_CASE("SSO - Small strings stay inline") {
        // Strings <= 23 chars should use SSO
        String s("Small");
        CHECK(s.size() == 5);
        CHECK(s.capacity() == 23); // SSO capacity
    }

    TEST_CASE("SSO - Large strings use heap") {
        // Strings > 23 chars should use heap
        String s("This is a very long string that exceeds SSO size");
        CHECK(s.size() > 23);
        CHECK(s.capacity() > 23);
    }

    TEST_CASE("Element Access - operator[]") {
        String s("Test");
        CHECK(s[0] == 'T');
        CHECK(s[1] == 'e');
        CHECK(s[2] == 's');
        CHECK(s[3] == 't');

        s[0] = 'B';
        CHECK(s[0] == 'B');
        CHECK(s.c_str() == std::string_view("Best"));
    }

    TEST_CASE("Element Access - front() and back()") {
        String s("Hello");
        CHECK(s.front() == 'H');
        CHECK(s.back() == 'o');

        s.front() = 'J';
        s.back() = 'y';
        CHECK(s.c_str() == std::string_view("Jelly"));
    }

    TEST_CASE("Element Access - data() and c_str()") {
        String s("Data");
        CHECK(s.data() != nullptr);
        CHECK(s.c_str() != nullptr);
        CHECK(s.data() == s.c_str());
        CHECK(std::strcmp(s.c_str(), "Data") == 0);
    }

    TEST_CASE("Capacity - empty(), size(), length()") {
        String s1;
        CHECK(s1.empty() == true);
        CHECK(s1.size() == 0);
        CHECK(s1.length() == 0);

        String s2("Hello");
        CHECK(s2.empty() == false);
        CHECK(s2.size() == 5);
        CHECK(s2.length() == 5);
    }

    TEST_CASE("Capacity - reserve()") {
        String s("Test");
        CHECK(s.size() == 4);

        s.reserve(100);
        CHECK(s.capacity() >= 100);
        CHECK(s.size() == 4);                         // Size unchanged
        CHECK(s.c_str() == std::string_view("Test")); // Content unchanged
    }

    TEST_CASE("Capacity - resize() larger") {
        String s("Hi");
        s.resize(5, 'X');
        CHECK(s.size() == 5);
        CHECK(s.c_str() == std::string_view("HiXXX"));
    }

    TEST_CASE("Capacity - resize() smaller") {
        String s("Hello World");
        s.resize(5);
        CHECK(s.size() == 5);
        CHECK(s.c_str() == std::string_view("Hello"));
    }

    TEST_CASE("Modifiers - clear()") {
        String s("Hello");
        s.clear();
        CHECK(s.empty() == true);
        CHECK(s.size() == 0);
        CHECK(s.c_str()[0] == '\0');
    }

    TEST_CASE("Modifiers - push_back()") {
        String s("Hel");
        s.push_back('l');
        s.push_back('o');
        CHECK(s.size() == 5);
        CHECK(s.c_str() == std::string_view("Hello"));
    }

    TEST_CASE("Modifiers - pop_back()") {
        String s("Hello");
        s.pop_back();
        CHECK(s.size() == 4);
        CHECK(s.c_str() == std::string_view("Hell"));

        s.pop_back();
        s.pop_back();
        CHECK(s.size() == 2);
        CHECK(s.c_str() == std::string_view("He"));
    }

    TEST_CASE("Modifiers - append() String") {
        String s1("Hello");
        String s2(" World");
        s1.append(s2);
        CHECK(s1.c_str() == std::string_view("Hello World"));
        CHECK(s1.size() == 11);
    }

    TEST_CASE("Modifiers - append() C-string") {
        String s("Hello");
        s.append(" World");
        CHECK(s.c_str() == std::string_view("Hello World"));
    }

    TEST_CASE("Modifiers - append() repeated char") {
        String s("Hi");
        s.append(3, '!');
        CHECK(s.c_str() == std::string_view("Hi!!!"));
    }

    TEST_CASE("Modifiers - operator+=") {
        String s("Hello");
        s += " ";
        s += String("World");
        s += '!';
        CHECK(s.c_str() == std::string_view("Hello World!"));
    }

    TEST_CASE("Modifiers - insert() char") {
        String s("Heo");
        s.insert(2, 1, 'l');
        s.insert(3, 1, 'l');
        CHECK(s.c_str() == std::string_view("Hello"));
    }

    TEST_CASE("Modifiers - insert() C-string") {
        String s("Hello");
        s.insert(5, " World");
        CHECK(s.c_str() == std::string_view("Hello World"));
    }

    TEST_CASE("Modifiers - insert() String") {
        String s1("Hello");
        String s2(" Beautiful");
        s1.insert(5, s2);
        s1.insert(15, String(" World"));
        CHECK(s1.c_str() == std::string_view("Hello Beautiful World"));
    }

    TEST_CASE("Modifiers - erase() to end") {
        String s("Hello World");
        s.erase(5);
        CHECK(s.c_str() == std::string_view("Hello"));
    }

    TEST_CASE("Modifiers - erase() range") {
        String s("Hello World");
        s.erase(5, 6); // Erase " World"
        CHECK(s.c_str() == std::string_view("Hello"));
    }

    TEST_CASE("Modifiers - erase() in middle") {
        String s("ABxyzCD");
        s.erase(2, 3); // Erase "xyz"
        CHECK(s.c_str() == std::string_view("ABCD"));
    }

    TEST_CASE("Modifiers - swap()") {
        String s1("Hello");
        String s2("World");
        s1.swap(s2);
        CHECK(s1.c_str() == std::string_view("World"));
        CHECK(s2.c_str() == std::string_view("Hello"));
    }

    TEST_CASE("Search - find() String") {
        String s("Hello World");
        CHECK(s.find(String("World")) == 6);
        CHECK(s.find(String("Hello")) == 0);
        CHECK(s.find(String("xyz")) == String::npos);
    }

    TEST_CASE("Search - find() C-string") {
        String s("Hello World");
        CHECK(s.find("World") == 6);
        CHECK(s.find("o") == 4);
        CHECK(s.find("xyz") == String::npos);
    }

    TEST_CASE("Search - find() char") {
        String s("Hello World");
        CHECK(s.find('H') == 0);
        CHECK(s.find('W') == 6);
        CHECK(s.find('o') == 4);    // First 'o'
        CHECK(s.find('o', 5) == 7); // Second 'o' from pos 5
        CHECK(s.find('x') == String::npos);
    }

    TEST_CASE("Search - rfind() String") {
        String s("Hello World Hello");
        CHECK(s.rfind(String("Hello")) == 12);
        CHECK(s.rfind(String("World")) == 6);
        CHECK(s.rfind(String("xyz")) == String::npos);
    }

    TEST_CASE("Search - rfind() char") {
        String s("Hello World");
        CHECK(s.rfind('o') == 7); // Last 'o'
        CHECK(s.rfind('H') == 0);
        CHECK(s.rfind('x') == String::npos);
    }

    TEST_CASE("Search - contains()") {
        String s("Hello World");
        CHECK(s.contains("World") == true);
        CHECK(s.contains("Hello") == true);
        CHECK(s.contains('o') == true);
        CHECK(s.contains("xyz") == false);
        CHECK(s.contains('x') == false);
    }

    TEST_CASE("Search - starts_with()") {
        String s("Hello World");
        CHECK(s.starts_with("Hello") == true);
        CHECK(s.starts_with("He") == true);
        CHECK(s.starts_with('H') == true);
        CHECK(s.starts_with("World") == false);
        CHECK(s.starts_with('W') == false);
    }

    TEST_CASE("Search - ends_with()") {
        String s("Hello World");
        CHECK(s.ends_with("World") == true);
        CHECK(s.ends_with("ld") == true);
        CHECK(s.ends_with('d') == true);
        CHECK(s.ends_with("Hello") == false);
        CHECK(s.ends_with('H') == false);
    }

    TEST_CASE("Substring - substr()") {
        String s("Hello World");

        String sub1 = s.substr(0, 5);
        CHECK(sub1.c_str() == std::string_view("Hello"));

        String sub2 = s.substr(6);
        CHECK(sub2.c_str() == std::string_view("World"));

        String sub3 = s.substr(6, 3);
        CHECK(sub3.c_str() == std::string_view("Wor"));
    }

    TEST_CASE("Comparison - operator==") {
        String s1("Hello");
        String s2("Hello");
        String s3("World");
        CHECK(s1 == s2);
        CHECK_FALSE(s1 == s3);
    }

    TEST_CASE("Comparison - operator!=") {
        String s1("Hello");
        String s2("World");
        CHECK(s1 != s2);
    }

    TEST_CASE("Comparison - operator<") {
        String s1("Apple");
        String s2("Banana");
        String s3("Apple");
        CHECK(s1 < s2);
        CHECK_FALSE(s2 < s1);
        CHECK_FALSE(s1 < s3);
    }

    TEST_CASE("Comparison - compare()") {
        String s1("Hello");
        String s2("Hello");
        String s3("World");
        CHECK(s1.compare(s2) == 0);
        CHECK(s1.compare(s3) < 0);
        CHECK(s3.compare(s1) > 0);
    }

    TEST_CASE("Iterators - begin() and end()") {
        String s("Hello");
        int count = 0;
        for (auto it = s.begin(); it != s.end(); ++it) {
            count++;
        }
        CHECK(count == 5);
    }

    TEST_CASE("Iterators - Range-based for") {
        String s("Test");
        std::string result;
        for (char c : s) {
            result += c;
        }
        CHECK(result == "Test");
    }

    TEST_CASE("members() Serialization") {
        String original("Hello World");

        auto buf = serialize(original);
        auto loaded = deserialize<Mode::NONE, String>(buf);

        CHECK(loaded.size() == original.size());
        CHECK(std::string_view(loaded.c_str()) == std::string_view(original.c_str()));
    }

    TEST_CASE("Serialization - SSO string") {
        String original("Short"); // Uses SSO

        auto buf = serialize(original);
        auto loaded = deserialize<Mode::NONE, String>(buf);

        CHECK(loaded.c_str() == std::string_view("Short"));
        CHECK(loaded.size() == 5);
    }

    TEST_CASE("Serialization - Heap string") {
        String original("This is a very long string that definitely exceeds SSO capacity");

        auto buf = serialize(original);
        auto loaded = deserialize<Mode::NONE, String>(buf);

        CHECK(std::string_view(loaded.c_str()) == std::string_view(original.c_str()));
        CHECK(loaded.size() == original.size());
    }

    TEST_CASE("Edge Case - Empty string") {
        String s;
        CHECK(s.empty());
        CHECK(s.size() == 0);

        s.append("");
        CHECK(s.empty());

        s += "";
        CHECK(s.empty());

        s.insert(0, "");
        CHECK(s.empty());
    }

    TEST_CASE("Edge Case - Multiple operations") {
        String s;
        s.append("Hello");
        s += " ";
        s += "Beautiful";
        s.push_back(' ');
        s.append("World");
        s.insert(6, "Very ");

        CHECK(s.c_str() == std::string_view("Hello Very Beautiful World"));
    }

    TEST_CASE("Edge Case - SSO to heap transition") {
        String s("Short"); // SSO
        CHECK(s.capacity() == 23);

        s.append(" string that becomes very long and exceeds SSO");
        CHECK(s.capacity() > 23); // Now on heap
        CHECK(s.starts_with("Short"));
        CHECK(s.ends_with("SSO"));
    }

    TEST_CASE("Search - find_first_of()") {
        String s("Hello World");

        // Find any vowel
        CHECK(s.find_first_of("aeiou") == 1);    // 'e' at position 1
        CHECK(s.find_first_of("aeiou", 2) == 4); // 'o' at position 4
        CHECK(s.find_first_of('o') == 4);
        CHECK(s.find_first_of("xyz") == String::npos);
    }

    TEST_CASE("Search - find_last_of()") {
        String s("Hello World");

        // Find last vowel
        CHECK(s.find_last_of("aeiou") == 7); // 'o' at position 7
        CHECK(s.find_last_of('o') == 7);
        CHECK(s.find_last_of("xyz") == String::npos);
    }

    TEST_CASE("Search - find_first_not_of()") {
        String s("   Hello");

        // Skip leading spaces
        CHECK(s.find_first_not_of(' ') == 3); // 'H' at position 3
        CHECK(s.find_first_not_of(" ") == 3);

        String s2("aaa");
        CHECK(s2.find_first_not_of('a') == String::npos);
    }

    TEST_CASE("Search - find_last_not_of()") {
        String s("Hello   ");

        // Skip trailing spaces
        CHECK(s.find_last_not_of(' ') == 4); // 'o' at position 4
        CHECK(s.find_last_not_of(" ") == 4);

        String s2("aaa");
        CHECK(s2.find_last_not_of('a') == String::npos);
    }

    TEST_CASE("Modifiers - replace() substring") {
        String s("Hello World");
        s.replace(6, 5, "Universe");
        CHECK(s.c_str() == std::string_view("Hello Universe"));
    }

    TEST_CASE("Modifiers - replace() with char") {
        String s("Hello");
        s.replace(1, 2, 3, 'X'); // Replace "el" with "XXX"
        CHECK(s.c_str() == std::string_view("HXXXlo"));
    }

    TEST_CASE("Modifiers - replace() same size") {
        String s("Hello");
        s.replace(0, 5, "World");
        CHECK(s.c_str() == std::string_view("World"));
    }

    TEST_CASE("Modifiers - replace() shorter") {
        String s("Hello World");
        s.replace(6, 5, "!");
        CHECK(s.c_str() == std::string_view("Hello !"));
    }

    TEST_CASE("Modifiers - replace() longer") {
        String s("Hi");
        s.replace(0, 2, "Hello World");
        CHECK(s.c_str() == std::string_view("Hello World"));
    }

    TEST_CASE("Hash - std::hash support") {
        String s1("Hello");
        String s2("Hello");
        String s3("World");

        std::hash<String> hasher;

        // Same strings should hash the same
        CHECK(hasher(s1) == hasher(s2));

        // Different strings should (likely) hash differently
        CHECK(hasher(s1) != hasher(s3));
    }

    TEST_CASE("Hash - use in unordered_map") {
        std::unordered_map<String, int> map;

        map[String("one")] = 1;
        map[String("two")] = 2;
        map[String("three")] = 3;

        CHECK(map[String("one")] == 1);
        CHECK(map[String("two")] == 2);
        CHECK(map[String("three")] == 3);
        CHECK(map.size() == 3);
    }

    TEST_CASE("Iterators - reverse") {
        String s("Hello");

        String reversed;
        for (auto it = s.rbegin(); it != s.rend(); ++it) {
            reversed.push_back(*it);
        }

        CHECK(reversed.c_str() == std::string_view("olleH"));
    }

    TEST_CASE("Iterators - const reverse") {
        String const s("World");

        std::string result;
        for (auto it = s.crbegin(); it != s.crend(); ++it) {
            result += *it;
        }

        CHECK(result == "dlroW");
    }

    // ===== Formatting Tests =====

    TEST_CASE("Formatting - operator+ with String") {
        String s1("Hello");
        String s2(" World");
        String result = s1 + s2;

        CHECK(result.c_str() == std::string_view("Hello World"));
        CHECK(result.size() == 11);
    }

    TEST_CASE("Formatting - operator+ with C-string (lhs)") {
        String s1("Hello");
        String result = s1 + " World";

        CHECK(result.c_str() == std::string_view("Hello World"));
        CHECK(result.size() == 11);
    }

    TEST_CASE("Formatting - operator+ with C-string (rhs)") {
        String s2(" World");
        String result = "Hello" + s2;

        CHECK(result.c_str() == std::string_view("Hello World"));
        CHECK(result.size() == 11);
    }

    TEST_CASE("Formatting - operator+ with char (lhs)") {
        String s("ello");
        String result = s + '!';

        CHECK(result.c_str() == std::string_view("ello!"));
        CHECK(result.size() == 5);
    }

    TEST_CASE("Formatting - operator+ with char (rhs)") {
        String s("orld");
        String result = 'W' + s;

        CHECK(result.c_str() == std::string_view("World"));
        CHECK(result.size() == 5);
    }

    TEST_CASE("Formatting - operator+ with string_view (lhs)") {
        String s("Hello");
        std::string_view sv(" World");
        String result = s + sv;

        CHECK(result.c_str() == std::string_view("Hello World"));
        CHECK(result.size() == 11);
    }

    TEST_CASE("Formatting - operator+ with string_view (rhs)") {
        std::string_view sv("Hello");
        String s(" World");
        String result = sv + s;

        CHECK(result.c_str() == std::string_view("Hello World"));
        CHECK(result.size() == 11);
    }

    TEST_CASE("Formatting - operator+ chaining") {
        String s1("A");
        String s2("B");
        String s3("C");
        String result = s1 + s2 + s3 + "D";

        CHECK(result.c_str() == std::string_view("ABCD"));
        CHECK(result.size() == 4);
    }

    TEST_CASE("Formatting - to_string() int") {
        auto s1 = to_string(42);
        CHECK(s1.c_str() == std::string_view("42"));

        auto s2 = to_string(-123);
        CHECK(s2.c_str() == std::string_view("-123"));

        auto s3 = to_string(0);
        CHECK(s3.c_str() == std::string_view("0"));
    }

    TEST_CASE("Formatting - to_string() long") {
        auto s1 = to_string(123456789L);
        CHECK(s1.c_str() == std::string_view("123456789"));

        auto s2 = to_string(-987654321L);
        CHECK(s2.c_str() == std::string_view("-987654321"));
    }

    TEST_CASE("Formatting - to_string() long long") {
        auto s1 = to_string(9223372036854775807LL);
        CHECK(s1.size() == 19);

        auto s2 = to_string(-9223372036854775807LL);
        CHECK(s2.size() == 20); // includes '-'
    }

    TEST_CASE("Formatting - to_string() unsigned int") {
        auto s1 = to_string(42U);
        CHECK(s1.c_str() == std::string_view("42"));

        auto s2 = to_string(4294967295U); // max uint
        CHECK(s2.c_str() == std::string_view("4294967295"));
    }

    TEST_CASE("Formatting - to_string() unsigned long") {
        auto s = to_string(18446744073709551615UL); // max ulong on 64-bit
        CHECK(s.size() == 20);
    }

    TEST_CASE("Formatting - to_string() float") {
        auto s1 = to_string(3.14f);
        CHECK(s1.contains("3.1"));

        auto s2 = to_string(0.0f);
        CHECK(s2.c_str() == std::string_view("0.000000"));
    }

    TEST_CASE("Formatting - to_string() double") {
        auto s1 = to_string(3.14159);
        CHECK(s1.contains("3.14"));

        auto s2 = to_string(-2.71828);
        CHECK(s2.contains("-2.7"));
    }

    TEST_CASE("Formatting - to_string() bool") {
        auto s1 = to_string(true);
        CHECK(s1.c_str() == std::string_view("true"));

        auto s2 = to_string(false);
        CHECK(s2.c_str() == std::string_view("false"));
    }

    TEST_CASE("Formatting - to_string() char") {
        auto s1 = to_string('A');
        CHECK(s1.c_str() == std::string_view("A"));

        auto s2 = to_string('z');
        CHECK(s2.c_str() == std::string_view("z"));
    }

    TEST_CASE("Formatting - operator<< with String") {
        String s("Hello");
        s << String(" World");

        CHECK(s.c_str() == std::string_view("Hello World"));
    }

    TEST_CASE("Formatting - operator<< with C-string") {
        String s("Hello");
        s << " " << "World";

        CHECK(s.c_str() == std::string_view("Hello World"));
    }

    TEST_CASE("Formatting - operator<< with char") {
        String s("Hell");
        s << 'o';

        CHECK(s.c_str() == std::string_view("Hello"));
    }

    TEST_CASE("Formatting - operator<< with int") {
        String s("Count: ");
        s << 42;

        CHECK(s.c_str() == std::string_view("Count: 42"));
    }

    TEST_CASE("Formatting - operator<< with negative int") {
        String s("Value: ");
        s << -123;

        CHECK(s.c_str() == std::string_view("Value: -123"));
    }

    TEST_CASE("Formatting - operator<< with long") {
        String s("Big: ");
        s << 123456789L;

        CHECK(s.c_str() == std::string_view("Big: 123456789"));
    }

    TEST_CASE("Formatting - operator<< with unsigned int") {
        String s("Unsigned: ");
        s << 4294967295U;

        CHECK(s.c_str() == std::string_view("Unsigned: 4294967295"));
    }

    TEST_CASE("Formatting - operator<< with float") {
        String s("Pi: ");
        s << 3.14f;

        CHECK(s.contains("3.1"));
        CHECK(s.starts_with("Pi: "));
    }

    TEST_CASE("Formatting - operator<< with double") {
        String s("E: ");
        s << 2.71828;

        CHECK(s.contains("2.7"));
        CHECK(s.starts_with("E: "));
    }

    TEST_CASE("Formatting - operator<< with bool (true)") {
        String s("Flag: ");
        s << true;

        CHECK(s.c_str() == std::string_view("Flag: true"));
    }

    TEST_CASE("Formatting - operator<< with bool (false)") {
        String s("Flag: ");
        s << false;

        CHECK(s.c_str() == std::string_view("Flag: false"));
    }

    TEST_CASE("Formatting - operator<< chaining multiple types") {
        String s;
        s << "Count: " << 42 << ", Pi: " << 3.14 << ", Flag: " << true;

        CHECK(s.starts_with("Count: 42"));
        CHECK(s.contains("Pi: 3.1"));
        CHECK(s.ends_with("Flag: true"));
    }

    TEST_CASE("Formatting - String::format() basic") {
        auto s = String::format("Hello {}!", "World");

        CHECK(s.c_str() == std::string_view("Hello World!"));
    }

    TEST_CASE("Formatting - String::format() with int") {
        auto s = String::format("Value: {}", 42);

        CHECK(s.c_str() == std::string_view("Value: 42"));
    }

    TEST_CASE("Formatting - String::format() with multiple args") {
        auto s = String::format("{} + {} = {}", 2, 3, 5);

        CHECK(s.c_str() == std::string_view("2 + 3 = 5"));
    }

    TEST_CASE("Formatting - String::format() with mixed types") {
        auto s = String::format("Name: {}, Age: {}, Active: {}", "Alice", 30, true);

        CHECK(s.c_str() == std::string_view("Name: Alice, Age: 30, Active: true"));
    }

    TEST_CASE("Formatting - String::format() with float") {
        auto s = String::format("Pi is approximately {}", 3.14159);

        CHECK(s.starts_with("Pi is approximately 3.14"));
    }

    TEST_CASE("Formatting - String::format() no placeholders") {
        auto s = String::format("No placeholders here");

        CHECK(s.c_str() == std::string_view("No placeholders here"));
    }

    TEST_CASE("Formatting - String::format() extra args ignored") {
        auto s = String::format("Only {} placeholder", "one", "two", "three");

        CHECK(s.c_str() == std::string_view("Only one placeholder"));
    }

    TEST_CASE("Formatting - String::format() with String arguments") {
        String name("Bob");
        auto s = String::format("Hello, {}!", name);

        CHECK(s.c_str() == std::string_view("Hello, Bob!"));
    }

    TEST_CASE("Formatting - String::format() empty string") {
        auto s = String::format("");

        CHECK(s.empty());
        CHECK(s.size() == 0);
    }

    TEST_CASE("Formatting - String::format() complex example") {
        auto s = String::format("User {} ({} years old) has {} points and status: {}", "Charlie", 25, 1337, true);

        CHECK(s.contains("Charlie"));
        CHECK(s.contains("25 years old"));
        CHECK(s.contains("1337 points"));
        CHECK(s.ends_with("status: true"));
    }

    TEST_CASE("Formatting - Combined operator+ and operator<<") {
        String s1("Hello");
        String s2 = s1 + " ";
        s2 << "World" << " " << 2024;

        CHECK(s2.c_str() == std::string_view("Hello World 2024"));
    }

    TEST_CASE("Formatting - Combined format() and operator+") {
        auto s1 = String::format("Hello {}", "World");
        auto s2 = s1 + "!";

        CHECK(s2.c_str() == std::string_view("Hello World!"));
    }

    TEST_CASE("Formatting - Large number formatting") {
        String s;
        s << "Max int: " << 2147483647;

        CHECK(s.c_str() == std::string_view("Max int: 2147483647"));
    }

    TEST_CASE("Formatting - Zero values") {
        auto s1 = to_string(0);
        auto s2 = to_string(0U);
        auto s3 = to_string(0L);
        auto s4 = to_string(0LL);
        auto s5 = to_string(0.0);

        CHECK(s1.c_str() == std::string_view("0"));
        CHECK(s2.c_str() == std::string_view("0"));
        CHECK(s3.c_str() == std::string_view("0"));
        CHECK(s4.c_str() == std::string_view("0"));
        CHECK(s5.c_str() == std::string_view("0.000000"));
    }
}
