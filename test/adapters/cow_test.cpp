#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "datapod/adapters/cow.hpp"
#include <string>

using namespace datapod;

TEST_SUITE("Cow") {
    TEST_CASE("Cow - borrowed construction") {
        int value = 42;
        auto cow = Cow<int>::borrowed(value);

        CHECK(cow.is_borrowed());
        CHECK(!cow.is_owned());
        CHECK(!cow.is_empty());
        CHECK(*cow == 42);
    }

    TEST_CASE("Cow - owned construction (move)") {
        auto cow = Cow<std::string>::owned(std::string("hello"));

        CHECK(!cow.is_borrowed());
        CHECK(cow.is_owned());
        CHECK(!cow.is_empty());
        CHECK(*cow == "hello");
    }

    TEST_CASE("Cow - owned construction (copy)") {
        std::string value = "hello";
        auto cow = Cow<std::string>::owned(value);

        CHECK(cow.is_owned());
        CHECK(*cow == "hello");
        CHECK(value == "hello"); // Original unchanged
    }

    TEST_CASE("Cow - default construction") {
        Cow<int> cow;

        CHECK(!cow.is_borrowed());
        CHECK(!cow.is_owned());
        CHECK(cow.is_empty());
    }

    TEST_CASE("Cow - dereference borrowed") {
        int value = 42;
        auto cow = Cow<int>::borrowed(value);

        CHECK(*cow == 42);
    }

    TEST_CASE("Cow - dereference owned") {
        auto cow = Cow<int>::owned(42);

        CHECK(*cow == 42);
    }

    TEST_CASE("Cow - arrow operator") {
        struct Data {
            int x;
            int y;
        };

        Data data{10, 20};
        auto cow = Cow<Data>::borrowed(data);

        CHECK(cow->x == 10);
        CHECK(cow->y == 20);
    }

    TEST_CASE("Cow - get method") {
        int value = 42;
        auto cow = Cow<int>::borrowed(value);

        CHECK(cow.get() == 42);
    }

    TEST_CASE("Cow - to_mut on borrowed clones") {
        int value = 42;
        auto cow = Cow<int>::borrowed(value);

        CHECK(cow.is_borrowed());

        int &mut_ref = cow.to_mut();

        CHECK(cow.is_owned());
        CHECK(!cow.is_borrowed());
        CHECK(mut_ref == 42);

        mut_ref = 100;
        CHECK(*cow == 100);
        CHECK(value == 42); // Original unchanged
    }

    TEST_CASE("Cow - to_mut on owned returns reference") {
        auto cow = Cow<int>::owned(42);

        int &mut_ref = cow.to_mut();
        mut_ref = 100;

        CHECK(*cow == 100);
        CHECK(cow.is_owned());
    }

    TEST_CASE("Cow - make_owned on borrowed") {
        int value = 42;
        auto cow = Cow<int>::borrowed(value);

        cow.make_owned();

        CHECK(cow.is_owned());
        CHECK(!cow.is_borrowed());
        CHECK(*cow == 42);
    }

    TEST_CASE("Cow - make_owned on owned is no-op") {
        auto cow = Cow<int>::owned(42);

        cow.make_owned();

        CHECK(cow.is_owned());
        CHECK(*cow == 42);
    }

    TEST_CASE("Cow - into_owned from borrowed") {
        int value = 42;
        auto cow = Cow<int>::borrowed(value);

        int owned = std::move(cow).into_owned();

        CHECK(owned == 42);
        CHECK(value == 42); // Original unchanged
    }

    TEST_CASE("Cow - into_owned from owned") {
        auto cow = Cow<std::string>::owned("hello");

        std::string owned = std::move(cow).into_owned();

        CHECK(owned == "hello");
    }

    TEST_CASE("Cow - clone borrowed") {
        int value = 42;
        auto cow1 = Cow<int>::borrowed(value);
        auto cow2 = cow1.clone();

        CHECK(cow2.is_owned());
        CHECK(*cow2 == 42);
    }

    TEST_CASE("Cow - clone owned") {
        auto cow1 = Cow<int>::owned(42);
        auto cow2 = cow1.clone();

        CHECK(cow2.is_owned());
        CHECK(*cow2 == 42);
    }

    TEST_CASE("Cow - copy constructor borrowed") {
        int value = 42;
        auto cow1 = Cow<int>::borrowed(value);
        auto cow2 = cow1;

        CHECK(cow2.is_borrowed());
        CHECK(*cow2 == 42);
    }

    TEST_CASE("Cow - copy constructor owned") {
        auto cow1 = Cow<int>::owned(42);
        auto cow2 = cow1;

        CHECK(cow2.is_owned());
        CHECK(*cow2 == 42);

        // Verify independence
        cow2.to_mut() = 100;
        CHECK(*cow1 == 42);
        CHECK(*cow2 == 100);
    }

    TEST_CASE("Cow - move constructor") {
        auto cow1 = Cow<int>::owned(42);
        auto cow2 = std::move(cow1);

        CHECK(cow2.is_owned());
        CHECK(*cow2 == 42);
    }

    TEST_CASE("Cow - copy assignment borrowed") {
        int value = 42;
        auto cow1 = Cow<int>::borrowed(value);
        auto cow2 = Cow<int>::owned(100);

        cow2 = cow1;

        CHECK(cow2.is_borrowed());
        CHECK(*cow2 == 42);
    }

    TEST_CASE("Cow - copy assignment owned") {
        auto cow1 = Cow<int>::owned(42);
        auto cow2 = Cow<int>::owned(100);

        cow2 = cow1;

        CHECK(cow2.is_owned());
        CHECK(*cow2 == 42);
    }

    TEST_CASE("Cow - move assignment") {
        auto cow1 = Cow<int>::owned(42);
        auto cow2 = Cow<int>::owned(100);

        cow2 = std::move(cow1);

        CHECK(cow2.is_owned());
        CHECK(*cow2 == 42);
    }

    TEST_CASE("Cow - equality borrowed") {
        int value1 = 42;
        int value2 = 42;
        auto cow1 = Cow<int>::borrowed(value1);
        auto cow2 = Cow<int>::borrowed(value2);

        CHECK(cow1 == cow2);
    }

    TEST_CASE("Cow - equality owned") {
        auto cow1 = Cow<int>::owned(42);
        auto cow2 = Cow<int>::owned(42);

        CHECK(cow1 == cow2);
    }

    TEST_CASE("Cow - equality mixed") {
        int value = 42;
        auto cow1 = Cow<int>::borrowed(value);
        auto cow2 = Cow<int>::owned(42);

        CHECK(cow1 == cow2);
    }

    TEST_CASE("Cow - inequality") {
        auto cow1 = Cow<int>::owned(42);
        auto cow2 = Cow<int>::owned(100);

        CHECK(cow1 != cow2);
    }

    TEST_CASE("Cow - ordering") {
        auto cow1 = Cow<int>::owned(10);
        auto cow2 = Cow<int>::owned(20);

        CHECK(cow1 < cow2);
        CHECK(cow1 <= cow2);
        CHECK(cow2 > cow1);
        CHECK(cow2 >= cow1);
    }

    TEST_CASE("Cow - with string") {
        std::string value = "hello";
        auto cow = Cow<std::string>::borrowed(value);

        CHECK(*cow == "hello");

        cow.to_mut() = "world";
        CHECK(*cow == "world");
        CHECK(value == "hello"); // Original unchanged
    }

    TEST_CASE("Cow - read-heavy scenario") {
        std::string value = "expensive to clone";
        auto cow = Cow<std::string>::borrowed(value);

        // Multiple reads - no cloning
        CHECK(*cow == "expensive to clone");
        CHECK(cow->length() == 18);
        CHECK(cow.get() == "expensive to clone");
        CHECK(cow.is_borrowed()); // Still borrowed

        // First mutation triggers clone
        cow.to_mut() += " - modified";
        CHECK(cow.is_owned());
        CHECK(*cow == "expensive to clone - modified");
        CHECK(value == "expensive to clone"); // Original unchanged
    }

    TEST_CASE("Cow - empty comparison") {
        Cow<int> empty1;
        Cow<int> empty2;
        Cow<int> filled = Cow<int>::owned(42);

        CHECK(empty1 == empty2);
        CHECK(empty1 != filled);
    }

    TEST_CASE("Cow - complex type") {
        struct Data {
            int x;
            std::string s;
        };

        Data data{42, "test"};
        auto cow = Cow<Data>::borrowed(data);

        CHECK(cow->x == 42);
        CHECK(cow->s == "test");

        cow.to_mut().x = 100;
        CHECK(cow->x == 100);
        CHECK(data.x == 42); // Original unchanged
    }

    TEST_CASE("Cow - optimization for read-only access") {
        int value = 42;
        auto cow = Cow<int>::borrowed(value);

        // Simulate read-only access pattern
        for (int i = 0; i < 100; ++i) {
            CHECK(*cow == 42);
        }

        // Should still be borrowed (no cloning occurred)
        CHECK(cow.is_borrowed());
    }

    TEST_CASE("Cow - dereference empty throws") {
        Cow<int> empty;
        CHECK_THROWS_AS(*empty, std::runtime_error);
    }

    TEST_CASE("Cow - to_mut empty throws") {
        Cow<int> empty;
        CHECK_THROWS_AS(empty.to_mut(), std::runtime_error);
    }

    TEST_CASE("Cow - into_owned empty throws") {
        Cow<int> empty;
        CHECK_THROWS_AS(std::move(empty).into_owned(), std::runtime_error);
    }
}
