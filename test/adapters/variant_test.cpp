#include <doctest/doctest.h>

#include <datapod/pods/adapters/variant.hpp>
#include <string>

using namespace datapod;

TEST_SUITE("Variant") {
    TEST_CASE("DefaultConstruction") {
        Variant<int, double> v;
        CHECK(!v.valid());
        CHECK(!v);
        CHECK(v.index() == Variant<int, double>::NO_VALUE);
    }

    TEST_CASE("ConstructionWithValue") {
        SUBCASE("IntValue") {
            Variant<int, double, std::string> v(42);
            CHECK(v.valid());
            CHECK(v);
            CHECK(v.index() == 0);
            CHECK(v.as<int>() == 42);
        }

        SUBCASE("DoubleValue") {
            Variant<int, double, std::string> v(3.14);
            CHECK(v.valid());
            CHECK(v.index() == 1);
            CHECK(v.as<double>() == doctest::Approx(3.14));
        }

        SUBCASE("StringValue") {
            Variant<int, double, std::string> v(std::string("hello"));
            CHECK(v.valid());
            CHECK(v.index() == 2);
            CHECK(v.as<std::string>() == "hello");
        }
    }

    TEST_CASE("CopyConstruction") {
        Variant<int, std::string> v1(42);
        Variant<int, std::string> v2(v1);

        CHECK(v2.valid());
        CHECK(v2.index() == 0);
        CHECK(v2.as<int>() == 42);

        // v1 should still be valid
        CHECK(v1.valid());
        CHECK(v1.as<int>() == 42);
    }

    TEST_CASE("MoveConstruction") {
        Variant<int, std::string> v1(std::string("move me"));
        Variant<int, std::string> v2(std::move(v1));

        CHECK(v2.valid());
        CHECK(v2.index() == 1);
        CHECK(v2.as<std::string>() == "move me");
    }

    TEST_CASE("CopyAssignment") {
        SUBCASE("SameType") {
            Variant<int, double> v1(42);
            Variant<int, double> v2(v1); // Use copy constructor

            CHECK(v2.as<int>() == 42);
            CHECK(v1.as<int>() == 42); // v1 should be unchanged
        }

        SUBCASE("DifferentType") {
            Variant<int, double> v1(42);
            Variant<int, double> v2(3.14);

            v2 = v1;
            CHECK(v2.index() == 0);
            CHECK(v2.as<int>() == 42);
        }

        SUBCASE("FromEmpty") {
            Variant<int, double> v1(42);
            Variant<int, double> v2;

            v2 = v1;
            CHECK(v2.valid());
            CHECK(v2.as<int>() == 42);
        }
    }

    TEST_CASE("MoveAssignment") {
        Variant<int, std::string> v1(std::string("move"));
        Variant<int, std::string> v2;

        v2 = std::move(v1);
        CHECK(v2.valid());
        CHECK(v2.as<std::string>() == "move");
    }

    TEST_CASE("ValueAssignment") {
        SUBCASE("SameType") {
            Variant<int, double> v(42);
            v = 100;
            CHECK(v.as<int>() == 100);
            CHECK(v.index() == 0);
        }

        SUBCASE("DifferentType") {
            Variant<int, double> v(42);
            v = 3.14;
            CHECK(v.index() == 1);
            CHECK(v.as<double>() == doctest::Approx(3.14));
        }

        SUBCASE("ToEmpty") {
            Variant<int, double> v;
            v = 42;
            CHECK(v.valid());
            CHECK(v.as<int>() == 42);
        }
    }

    TEST_CASE("AsMethod") {
        Variant<int, double, std::string> v(std::string("test"));

        CHECK(v.as<std::string>() == "test");

        // Modify through as()
        v.as<std::string>() = "modified";
        CHECK(v.as<std::string>() == "modified");
    }

    TEST_CASE("GetIfChecks") {
        Variant<int, double, std::string> v(42);

        CHECK(get_if<int>(v) != nullptr);
        CHECK(get_if<double>(v) == nullptr);
        CHECK(get_if<std::string>(v) == nullptr);

        v = 3.14;
        CHECK(get_if<int>(v) == nullptr);
        CHECK(get_if<double>(v) != nullptr);
    }

    TEST_CASE("EmplaceByType") {
        Variant<int, double, std::string> v;

        // Emplace int
        int &ref = v.emplace<int>(42);
        CHECK(v.index() == 0);
        CHECK(v.as<int>() == 42);
        CHECK(&ref == &v.as<int>());

        // Emplace string
        v.emplace<std::string>("hello");
        CHECK(v.index() == 2);
        CHECK(v.as<std::string>() == "hello");

        // Emplace with constructor args
        v.emplace<std::string>("world");
        CHECK(v.as<std::string>() == "world");
    }

    TEST_CASE("EmplaceByIndex") {
        Variant<int, double, std::string> v;

        // Emplace at index 0 (int)
        v.emplace<0>(42);
        CHECK(v.index() == 0);
        CHECK(v.as<int>() == 42);

        // Emplace at index 2 (std::string)
        v.emplace<2>("world");
        CHECK(v.index() == 2);
        CHECK(v.as<std::string>() == "world");
    }

    TEST_CASE("GetByIndex") {
        Variant<int, double, std::string> v(42);

        CHECK(get<0>(v) == 42);

        v = 3.14;
        CHECK(get<1>(v) == doctest::Approx(3.14));

        v = std::string("test");
        CHECK(get<2>(v) == "test");
    }

    TEST_CASE("GetByType") {
        Variant<int, double, std::string> v(42);

        CHECK(get<int>(v) == 42);

        v = 3.14;
        CHECK(get<double>(v) == doctest::Approx(3.14));
    }

    TEST_CASE("GetIfByIndex") {
        Variant<int, double, std::string> v(42);

        auto *p_int = get_if<0>(v);
        CHECK(p_int != nullptr);
        CHECK(*p_int == 42);

        auto *p_double = get_if<1>(v);
        CHECK(p_double == nullptr);

        // Const version
        const auto &cv = v;
        auto *cp_int = get_if<0>(cv);
        CHECK(cp_int != nullptr);
        CHECK(*cp_int == 42);
    }

    TEST_CASE("GetIfByType") {
        Variant<int, double, std::string> v(std::string("hello"));

        auto *p_str = get_if<std::string>(v);
        CHECK(p_str != nullptr);
        CHECK(*p_str == "hello");

        auto *p_int = get_if<int>(v);
        CHECK(p_int == nullptr);

        // Modify through get_if
        if (auto *p = get_if<std::string>(v)) {
            *p = "modified";
        }
        CHECK(v.as<std::string>() == "modified");
    }

    TEST_CASE("Swap") {
        SUBCASE("SameType") {
            Variant<int, double> v1(42);
            Variant<int, double> v2(100);

            v1.swap(v2);
            CHECK(v1.as<int>() == 100);
            CHECK(v2.as<int>() == 42);
        }

        SUBCASE("DifferentTypes") {
            Variant<int, double> v1(42);
            Variant<int, double> v2(3.14);

            v1.swap(v2);
            CHECK(v1.index() == 1);
            CHECK(v1.as<double>() == doctest::Approx(3.14));
            CHECK(v2.index() == 0);
            CHECK(v2.as<int>() == 42);
        }
    }

    TEST_CASE("Apply") {
        Variant<int, double, std::string> v(42);

        // Apply with lambda
        auto result = v.apply([](auto &&val) -> int {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, int>) {
                return val * 2;
            } else {
                return 0;
            }
        });

        CHECK(result == 84);

        // Apply that modifies
        v.apply([](auto &&val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, int>) {
                val = 100;
            }
        });

        CHECK(v.as<int>() == 100);
    }

    TEST_CASE("ApplyConst") {
        const Variant<int, double> v(42);

        auto result = v.apply([](auto &&val) -> int {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, int>) {
                return val;
            }
            return 0;
        });

        CHECK(result == 42);
    }

    TEST_CASE("StdVisit") {
        Variant<int, double, std::string> v(42);

        auto result = std::visit(
            [](auto &&val) -> std::string {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, int>) {
                    return "int: " + std::to_string(val);
                } else if constexpr (std::is_same_v<T, double>) {
                    return "double";
                } else {
                    return "string";
                }
            },
            v);

        CHECK(result == "int: 42");
    }

    TEST_CASE("Comparisons") {
        SUBCASE("EqualitySameType") {
            Variant<int, double> v1(42);
            Variant<int, double> v2(42);
            Variant<int, double> v3(100);

            CHECK(v1 == v2);
            CHECK(v1 != v3);
        }

        SUBCASE("EqualityDifferentTypes") {
            Variant<int, double> v1(42);
            Variant<int, double> v2(3.14);

            CHECK(v1 != v2);
        }

        SUBCASE("LessThanSameType") {
            Variant<int, double> v1(42);
            Variant<int, double> v2(100);

            CHECK(v1 < v2);
            CHECK(!(v2 < v1));
        }

        SUBCASE("LessThanDifferentTypes") {
            Variant<int, double> v1(42);
            Variant<int, double> v2(3.14);

            // Compares indices when types differ
            CHECK(v1 < v2); // int (index 0) < double (index 1)
        }

        SUBCASE("OtherComparisons") {
            Variant<int, double> v1(42);
            Variant<int, double> v2(100);

            CHECK(v1 <= v2);
            CHECK(v2 > v1);
            CHECK(v2 >= v1);
            CHECK(v1 <= v1);
            CHECK(v1 >= v1);
        }
    }

    TEST_CASE("Index") {
        Variant<int, double, std::string> v;
        CHECK(v.index() == Variant<int, double, std::string>::NO_VALUE);

        v = 42;
        CHECK(v.index() == 0);

        v = 3.14;
        CHECK(v.index() == 1);

        v = std::string("test");
        CHECK(v.index() == 2);
    }

    TEST_CASE("Valid") {
        Variant<int, double> v;
        CHECK(!v.valid());
        CHECK(!static_cast<bool>(v));

        v = 42;
        CHECK(v.valid());
        CHECK(static_cast<bool>(v));
    }

    TEST_CASE("VariantSize") {
        CHECK(variant_size_v<Variant<int>> == 1);
        CHECK(variant_size_v<Variant<int, double>> == 2);
        CHECK(variant_size_v<Variant<int, double, std::string>> == 3);
    }

    TEST_CASE("MultipleTypeChanges") {
        Variant<int, double, std::string> v;

        v = 42;
        CHECK(v.index() == 0);

        v = 3.14;
        CHECK(v.index() == 1);

        v = std::string("test");
        CHECK(v.index() == 2);

        v = 100;
        CHECK(v.index() == 0);
    }

    TEST_CASE("ComplexType") {
        struct Point {
            int x, y;
            Point(int x_, int y_) : x(x_), y(y_) {}
            bool operator==(Point const &o) const { return x == o.x && y == o.y; }
        };

        Variant<int, Point, std::string> v;

        v.emplace<Point>(10, 20);
        CHECK(get_if<Point>(v) != nullptr);
        CHECK(v.as<Point>().x == 10);
        CHECK(v.as<Point>().y == 20);

        // Modify
        v.as<Point>().x = 30;
        CHECK(v.as<Point>().x == 30);
    }

    TEST_CASE("StringOperations") {
        Variant<int, std::string> v(std::string("hello"));

        CHECK(v.as<std::string>() == "hello");

        v.as<std::string>() += " world";
        CHECK(v.as<std::string>() == "hello world");

        v = std::string("replaced");
        CHECK(v.as<std::string>() == "replaced");
    }

    TEST_CASE("IndexAccess") {
        Variant<int, double, std::string> v(42);

        // Test index-based access
        CHECK(v.index() == 0);
        CHECK(get<0>(v) == 42);
    }

    TEST_CASE("ApplyOnEmpty") {
        Variant<int, double> v;

        // Apply on empty variant should throw
        CHECK_THROWS_AS(v.apply([](auto &&) {}), std::runtime_error);
    }

    TEST_CASE("EdgeCases") {
        SUBCASE("SelfAssignment") {
            Variant<int, double> v(42);
            v = v;
            CHECK(v.as<int>() == 42);
        }

        SUBCASE("EmptyComparison") {
            Variant<int, double> v1;
            Variant<int, double> v2;

            // Both empty
            CHECK(v1 == v2);
        }

        SUBCASE("EmptyAndValue") {
            Variant<int, double> v1;
            Variant<int, double> v2(42);

            CHECK(v1 != v2);
        }
    }
}
