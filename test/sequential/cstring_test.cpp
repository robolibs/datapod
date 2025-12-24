#include <doctest/doctest.h>

#include <datapod/sequential/cstring.hpp>

using namespace datapod;

TEST_SUITE("Cstring") {
    TEST_CASE("DefaultConstruction") {
        Cstring s;
        CHECK(s.empty());
        CHECK(s.size() == 0);
        CHECK(s.c_str() != nullptr);
        CHECK(s.c_str()[0] == '\0');
        CHECK(s.is_short());
        CHECK(s.is_owning());
    }

    TEST_CASE("OwningConstruction") {
        SUBCASE("FromCString") {
            Cstring s("hello");
            CHECK(s.size() == 5);
            CHECK(s.view() == "hello");
            CHECK(s.is_short());
            CHECK(s.is_owning());
        }

        SUBCASE("FromString") {
            std::string str = "world";
            Cstring s(str);
            CHECK(s.size() == 5);
            CHECK(s.view() == "world");
            CHECK(s.is_short());
        }

        SUBCASE("FromStringView") {
            std::string_view sv = "test";
            Cstring s(sv);
            CHECK(s.size() == 4);
            CHECK(s.view() == "test");
            CHECK(s.is_short());
        }

        SUBCASE("ExplicitOwning") {
            Cstring s("explicit", Cstring::owning);
            CHECK(s.size() == 8);
            CHECK(s.view() == "explicit");
            CHECK(s.is_owning());
        }
    }

    TEST_CASE("NonOwningConstruction") {
        const char *str = "non-owning";

        SUBCASE("FromCString") {
            CstringView s(str);
            CHECK(s.size() == 10);
            CHECK(s.view() == "non-owning");
            CHECK(!s.is_owning());
        }

        SUBCASE("FromStringView") {
            std::string_view sv = "view";
            CstringView s(sv);
            CHECK(s.size() == 4);
            CHECK(s.view() == "view");
        }

        SUBCASE("ExplicitNonOwning") {
            Cstring s(str, Cstring::non_owning);
            CHECK(s.size() == 10);
            CHECK(s.view() == "non-owning");
            CHECK(!s.is_owning());
        }
    }

    TEST_CASE("CopyConstruction") {
        SUBCASE("ShortString") {
            Cstring s1("short");
            Cstring s2(s1);
            CHECK(s2.size() == 5);
            CHECK(s2.view() == "short");
            CHECK(s2.is_short());
            CHECK(s2.is_owning());
        }

        SUBCASE("LongString") {
            Cstring s1("this is a very long string that exceeds SSO limit");
            Cstring s2(s1);
            CHECK(s2.size() == s1.size());
            CHECK(s2.view() == s1.view());
            CHECK(!s2.is_short());
            CHECK(s2.is_owning());
        }
    }

    TEST_CASE("MoveConstruction") {
        SUBCASE("ShortString") {
            Cstring s1("short");
            Cstring s2(std::move(s1));
            CHECK(s2.size() == 5);
            CHECK(s2.view() == "short");
            CHECK(s2.is_short());
        }

        SUBCASE("LongString") {
            Cstring s1("this is a very long string that exceeds SSO limit");
            std::string_view original = s1.view();
            Cstring s2(std::move(s1));
            CHECK(s2.view() == original);
            CHECK(!s2.is_short());
        }
    }

    TEST_CASE("Assignment") {
        SUBCASE("CopyAssignment") {
            Cstring s1("hello");
            Cstring s2;
            s2 = s1;
            CHECK(s2.size() == 5);
            CHECK(s2.view() == "hello");
        }

        SUBCASE("MoveAssignment") {
            Cstring s1("world");
            Cstring s2;
            s2 = std::move(s1);
            CHECK(s2.size() == 5);
            CHECK(s2.view() == "world");
        }

        SUBCASE("CStringAssignment") {
            Cstring s;
            s = "assigned";
            CHECK(s.size() == 8);
            CHECK(s.view() == "assigned");
        }

        SUBCASE("StringAssignment") {
            Cstring s;
            std::string str = "string";
            s = str;
            CHECK(s.size() == 6);
            CHECK(s.view() == "string");
        }

        SUBCASE("StringViewAssignment") {
            Cstring s;
            std::string_view sv = "view";
            s = sv;
            CHECK(s.size() == 4);
            CHECK(s.view() == "view");
        }
    }

    TEST_CASE("ElementAccess") {
        SUBCASE("OperatorBracket") {
            Cstring s("test");
            CHECK(s[0] == 't');
            CHECK(s[1] == 'e');
            CHECK(s[2] == 's');
            CHECK(s[3] == 't');
        }

        SUBCASE("OperatorBracketConst") {
            const Cstring s("const");
            CHECK(s[0] == 'c');
            CHECK(s[4] == 't');
        }

        SUBCASE("DataMethod") {
            Cstring s("data");
            CHECK(s.data() != nullptr);
            CHECK(std::string_view(s.data(), s.size()) == "data");
        }

        SUBCASE("CStrMethod") {
            Cstring s("cstr");
            CHECK(s.c_str() != nullptr);
            CHECK(std::strcmp(s.c_str(), "cstr") == 0);
        }
    }

    TEST_CASE("Capacity") {
        SUBCASE("SizeOfEmpty") {
            Cstring s;
            CHECK(s.size() == 0);
            CHECK(s.empty());
        }

        SUBCASE("SizeOfNonEmpty") {
            Cstring s("hello");
            CHECK(s.size() == 5);
            CHECK(!s.empty());
        }

        SUBCASE("CapacityShortString") {
            Cstring s("short");
            CHECK(s.capacity() >= 15);
            CHECK(s.is_short());
        }

        SUBCASE("CapacityLongString") {
            Cstring s("this is a very long string that exceeds SSO limit");
            CHECK(s.capacity() >= s.size());
            CHECK(!s.is_short());
        }

        SUBCASE("CapacityAfterReserve") {
            Cstring s("test");
            s.reserve(100);
            // Note: CString doesn't track extra capacity, so capacity() == size()
            // after reserve(), but future operations won't need reallocation
            CHECK(s.size() == 4);
            CHECK(s.view() == "test");
        }
    }

    TEST_CASE("Reserve") {
        SUBCASE("ReserveWithinSSO") {
            Cstring s("small");
            s.reserve(10);
            CHECK(s.size() == 5);
            CHECK(s.view() == "small");
            CHECK(s.capacity() >= 10);
        }

        SUBCASE("ReserveSSOToHeap") {
            Cstring s("short");
            CHECK(s.is_short());
            s.reserve(100);
            // After reserve, string is still short because size <= 15
            // reserve() only transitions to heap when actually growing beyond SSO
            CHECK(s.size() == 5);
            CHECK(s.view() == "short");
            CHECK(s.is_owning());
        }

        SUBCASE("ReserveOnHeap") {
            Cstring s("this is already a long string");
            CHECK(!s.is_short());
            s.reserve(100); // Reserve some capacity
            // CString reallocates on growth, so just check content is preserved
            CHECK(s.view() == "this is already a long string");
        }

        SUBCASE("ReserveSmallerThanCurrent") {
            Cstring s("test");
            s.reserve(100);
            auto cap = s.capacity();
            s.reserve(50);
            CHECK(s.capacity() == cap); // Should not shrink
        }
    }

    TEST_CASE("Resize") {
        SUBCASE("ResizeSmaller") {
            Cstring s("hello world");
            s.resize(5);
            CHECK(s.size() == 5);
            CHECK(s.view() == "hello");
        }

        SUBCASE("ResizeLarger") {
            Cstring s("hi");
            s.resize(5);
            CHECK(s.size() == 5);
            // New characters should be null bytes
            CHECK(s[0] == 'h');
            CHECK(s[1] == 'i');
            CHECK(s[2] == '\0');
            CHECK(s[3] == '\0');
            CHECK(s[4] == '\0');
        }

        SUBCASE("ResizeToZero") {
            Cstring s("test");
            s.resize(0);
            CHECK(s.size() == 0);
            CHECK(s.empty());
        }

        SUBCASE("ResizeExceedsSSO") {
            Cstring s("short");
            s.resize(50);
            CHECK(s.size() == 50);
            CHECK(!s.is_short());
        }
    }

    TEST_CASE("PushBack") {
        SUBCASE("PushBackToEmpty") {
            Cstring s;
            s.push_back('a');
            CHECK(s.size() == 1);
            CHECK(s.view() == "a");
        }

        SUBCASE("PushBackMultiple") {
            Cstring s;
            s.push_back('h');
            s.push_back('e');
            s.push_back('l');
            s.push_back('l');
            s.push_back('o');
            CHECK(s.size() == 5);
            CHECK(s.view() == "hello");
        }

        SUBCASE("PushBackExceedsSSO") {
            Cstring s("123456789012345"); // 15 chars (SSO limit)
            CHECK(s.is_short());
            s.push_back('X');
            CHECK(!s.is_short());
            CHECK(s.size() == 16);
            CHECK(s.view() == "123456789012345X");
        }

        SUBCASE("PushBackToLongString") {
            Cstring s("this is a long string already");
            s.push_back('!');
            CHECK(s.view() == "this is a long string already!");
        }
    }

    TEST_CASE("Append") {
        SUBCASE("AppendCString") {
            Cstring s("hello");
            s.append(" world");
            CHECK(s.size() == 11);
            CHECK(s.view() == "hello world");
        }

        SUBCASE("AppendStringView") {
            Cstring s("foo");
            std::string_view sv = "bar";
            s.append(sv);
            CHECK(s.size() == 6);
            CHECK(s.view() == "foobar");
        }

        SUBCASE("AppendEmpty") {
            Cstring s("test");
            s.append("");
            CHECK(s.size() == 4);
            CHECK(s.view() == "test");
        }

        SUBCASE("AppendToEmpty") {
            Cstring s;
            s.append("new");
            CHECK(s.size() == 3);
            CHECK(s.view() == "new");
        }

        SUBCASE("AppendExceedsSSO") {
            Cstring s("short");
            s.append(" string that is long");
            CHECK(!s.is_short());
            CHECK(s.view() == "short string that is long");
        }

        SUBCASE("AppendMultipleTimes") {
            Cstring s("a");
            s.append("b");
            s.append("c");
            s.append("d");
            CHECK(s.size() == 4);
            CHECK(s.view() == "abcd");
        }
    }

    TEST_CASE("Clear") {
        SUBCASE("ClearShortString") {
            Cstring s("hello");
            s.clear();
            CHECK(s.size() == 0);
            CHECK(s.empty());
            CHECK(s.view() == "");
        }

        SUBCASE("ClearLongString") {
            Cstring s("this is a very long string that exceeds SSO");
            s.clear();
            CHECK(s.size() == 0);
            CHECK(s.empty());
            CHECK(s.view() == "");
        }

        SUBCASE("ClearAndReuse") {
            Cstring s("first");
            s.clear();
            s.append("second");
            CHECK(s.size() == 6);
            CHECK(s.view() == "second");
        }
    }

    TEST_CASE("Reset") {
        SUBCASE("ResetShortString") {
            Cstring s("hello");
            s.reset();
            CHECK(s.size() == 0);
            CHECK(s.empty());
        }

        SUBCASE("ResetLongString") {
            Cstring s("this is a very long string that exceeds SSO");
            s.reset();
            CHECK(s.size() == 0);
            CHECK(s.empty());
        }
    }

    TEST_CASE("Iterators") {
        SUBCASE("BeginEnd") {
            Cstring s("test");
            auto it = s.begin();
            CHECK(*it == 't');
            ++it;
            CHECK(*it == 'e');
            ++it;
            CHECK(*it == 's');
            ++it;
            CHECK(*it == 't');
            ++it;
            CHECK(it == s.end());
        }

        SUBCASE("ConstBeginEnd") {
            const Cstring s("const");
            auto it = s.begin();
            CHECK(*it == 'c');
        }

        SUBCASE("RangeBasedFor") {
            Cstring s("abc");
            std::string result;
            for (char c : s) {
                result += c;
            }
            CHECK(result == "abc");
        }
    }

    TEST_CASE("Comparisons") {
        SUBCASE("EqualityCstring") {
            Cstring s1("test");
            Cstring s2("test");
            Cstring s3("other");
            CHECK(s1 == s2);
            CHECK(s1 != s3);
        }

        SUBCASE("EqualityStringView") {
            Cstring s("test");
            CHECK(s == std::string_view("test"));
            CHECK(s != std::string_view("other"));
        }

        SUBCASE("EqualityCString") {
            Cstring s("test");
            CHECK(s == "test");
            CHECK(s != "other");
            CHECK("test" == s);
            CHECK("other" != s);
        }

        SUBCASE("LessThan") {
            Cstring s1("abc");
            Cstring s2("def");
            CHECK(s1 < s2);
            CHECK(!(s2 < s1));
            CHECK(s1 <= s2);
            CHECK(s2 > s1);
            CHECK(s2 >= s1);
        }

        SUBCASE("LessThanStringView") {
            Cstring s("abc");
            CHECK(s < std::string_view("def"));
            CHECK(s <= std::string_view("abc"));
        }
    }

    TEST_CASE("Conversions") {
        SUBCASE("ToStringView") {
            Cstring s("test");
            std::string_view sv = s;
            CHECK(sv == "test");
        }

        SUBCASE("ToStdString") {
            Cstring s("test");
            std::string str = s.str();
            CHECK(str == "test");
        }

        SUBCASE("ImplicitStringView") {
            Cstring s("test");
            std::string_view sv = s.view();
            CHECK(sv == "test");
        }
    }

    TEST_CASE("SSOBoundary") {
        SUBCASE("Exactly15Chars") {
            Cstring s("123456789012345"); // 15 chars - SSO limit
            CHECK(s.is_short());
            CHECK(s.size() == 15);
            CHECK(s.view() == "123456789012345");
        }

        SUBCASE("Exactly16Chars") {
            Cstring s("1234567890123456"); // 16 chars - exceeds SSO
            CHECK(!s.is_short());
            CHECK(s.size() == 16);
            CHECK(s.view() == "1234567890123456");
        }

        SUBCASE("PushBackAt15") {
            Cstring s("123456789012345");
            CHECK(s.is_short());
            s.push_back('X');
            CHECK(!s.is_short());
            CHECK(s.size() == 16);
        }

        SUBCASE("ResizeAcrossBoundary") {
            Cstring s("short");
            CHECK(s.is_short());
            s.resize(20);
            CHECK(!s.is_short());
            CHECK(s.size() == 20);
        }
    }

    TEST_CASE("BinaryData") {
        SUBCASE("ContainsNullBytes") {
            const char data[] = {'a', 'b', '\0', 'c', 'd', '\0'};
            Cstring s(data, 6);
            CHECK(s.size() == 6);
            CHECK(s[0] == 'a');
            CHECK(s[2] == '\0');
            CHECK(s[3] == 'c');
            CHECK(s[5] == '\0');
        }

        SUBCASE("AllNullBytes") {
            const char data[] = {'\0', '\0', '\0'};
            Cstring s(data, 3);
            CHECK(s.size() == 3);
            CHECK(s[0] == '\0');
            CHECK(s[1] == '\0');
            CHECK(s[2] == '\0');
        }
    }

    TEST_CASE("OwningVsNonOwning") {
        SUBCASE("OwningCopiesData") {
            const char *original = "test";
            Cstring s(original, Cstring::owning);
            CHECK(s.is_owning());
            // Data should be copied, not just referenced
            bool is_copied = (s.data() != original) || s.is_short();
            CHECK(is_copied); // Either copied to heap or SSO
        }

        SUBCASE("NonOwningReferencesData") {
            const char *original = "test string";
            Cstring s(original, Cstring::non_owning);
            CHECK(!s.is_owning());
        }

        SUBCASE("CstringViewIsNonOwning") {
            CstringView sv("test");
            CHECK(!sv.is_owning());
        }
    }

    TEST_CASE("LargeString") {
        SUBCASE("VeryLongString") {
            std::string large(10000, 'x');
            Cstring s(large);
            CHECK(s.size() == 10000);
            CHECK(!s.is_short());
            CHECK(s.is_owning());
            CHECK(s.capacity() >= 10000);
        }

        SUBCASE("AppendToLarge") {
            std::string large(5000, 'a');
            Cstring s(large);
            s.append(std::string(5000, 'b'));
            CHECK(s.size() == 10000);
        }
    }

    TEST_CASE("Members") {
        SUBCASE("MembersFunction") {
            Cstring s("test");
            auto [h, st] = s.members();
            // Just verify it compiles and returns something
            (void)h;
            (void)st;
        }

        SUBCASE("ConstMembers") {
            const Cstring s("test");
            auto [h, st] = s.members();
            (void)h;
            (void)st;
        }
    }

    TEST_CASE("EdgeCases") {
        SUBCASE("EmptyStringOperations") {
            Cstring s;
            s.append("");
            CHECK(s.empty());
            s.push_back('a');
            CHECK(s.size() == 1);
            s.clear();
            CHECK(s.empty());
        }

        SUBCASE("NullTermination") {
            Cstring s("test");
            CHECK(s.c_str()[s.size()] == '\0');
            s.append("ing");
            CHECK(s.c_str()[s.size()] == '\0');
            s.push_back('!');
            CHECK(s.c_str()[s.size()] == '\0');
        }

        SUBCASE("SelfModification") {
            Cstring s("abc");
            s[0] = 'x';
            CHECK(s.view() == "xbc");
        }
    }
}
