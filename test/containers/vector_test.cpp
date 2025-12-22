#include <doctest/doctest.h>

#include "datapod/datapod.hpp"

using namespace datapod;

TEST_SUITE("Vector") {
    TEST_CASE("Construction - Default") {
        Vector<int> v;
        CHECK(v.size() == 0);
        CHECK(v.empty() == true);
        CHECK(v.capacity() == 0);
    }

    TEST_CASE("Construction - Count") {
        Vector<int> v(5);
        CHECK(v.size() == 5);
        CHECK(v.empty() == false);
        for (int i = 0; i < 5; ++i) {
            CHECK(v[i] == 0);
        }
    }

    TEST_CASE("Construction - Count with Value") {
        Vector<int> v(5, 42);
        CHECK(v.size() == 5);
        for (int i = 0; i < 5; ++i) {
            CHECK(v[i] == 42);
        }
    }

    TEST_CASE("Construction - Initializer List") {
        Vector<int> v = {10, 20, 30};
        CHECK(v.size() == 3);
        CHECK(v[0] == 10);
        CHECK(v[1] == 20);
        CHECK(v[2] == 30);
    }

    TEST_CASE("Element Access - operator[]") {
        Vector<int> v = {10, 20, 30};
        CHECK(v[0] == 10);
        CHECK(v[1] == 20);
        CHECK(v[2] == 30);

        v[1] = 99;
        CHECK(v[1] == 99);
    }

    TEST_CASE("Element Access - front() and back()") {
        Vector<int> v = {1, 2, 3, 4, 5};
        CHECK(v.front() == 1);
        CHECK(v.back() == 5);

        v.front() = 100;
        v.back() = 500;
        CHECK(v.front() == 100);
        CHECK(v.back() == 500);
    }

    TEST_CASE("Element Access - data()") {
        Vector<int> v = {10, 20, 30};
        int *ptr = v.data();
        CHECK(ptr[0] == 10);
        CHECK(ptr[1] == 20);
        CHECK(ptr[2] == 30);
    }

    TEST_CASE("Iterators - begin() and end()") {
        Vector<int> v = {1, 2, 3, 4, 5};

        int sum = 0;
        for (auto it = v.begin(); it != v.end(); ++it) {
            sum += *it;
        }
        CHECK(sum == 15);
    }

    TEST_CASE("Iterators - Range-based for") {
        Vector<int> v = {10, 20, 30};
        int sum = 0;
        for (int val : v) {
            sum += val;
        }
        CHECK(sum == 60);
    }

    TEST_CASE("Capacity - reserve()") {
        Vector<int> v;
        v.reserve(100);
        CHECK(v.capacity() >= 100);
        CHECK(v.size() == 0);
    }

    TEST_CASE("Capacity - shrink_to_fit()") {
        Vector<int> v;
        v.reserve(100);
        v.push_back(1);
        v.push_back(2);

        CHECK(v.capacity() >= 100);
        v.shrink_to_fit();
        CHECK(v.capacity() == 2);
        CHECK(v.size() == 2);
    }

    TEST_CASE("Modifiers - push_back()") {
        Vector<int> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);

        CHECK(v.size() == 3);
        CHECK(v[0] == 1);
        CHECK(v[1] == 2);
        CHECK(v[2] == 3);
    }

    TEST_CASE("Modifiers - emplace_back()") {
        struct Point {
            int x, y;
            Point(int a, int b) : x(a), y(b) {}
        };

        Vector<Point> v;
        v.emplace_back(10, 20);
        v.emplace_back(30, 40);

        CHECK(v.size() == 2);
        CHECK(v[0].x == 10);
        CHECK(v[0].y == 20);
        CHECK(v[1].x == 30);
        CHECK(v[1].y == 40);
    }

    TEST_CASE("Modifiers - pop_back()") {
        Vector<int> v = {1, 2, 3, 4, 5};
        v.pop_back();

        CHECK(v.size() == 4);
        CHECK(v.back() == 4);
    }

    TEST_CASE("Modifiers - insert() single element") {
        Vector<int> v = {1, 2, 4, 5};
        auto it = v.insert(v.begin() + 2, 3);

        CHECK(v.size() == 5);
        CHECK(v[0] == 1);
        CHECK(v[1] == 2);
        CHECK(v[2] == 3);
        CHECK(v[3] == 4);
        CHECK(v[4] == 5);
        CHECK(*it == 3);
    }

    TEST_CASE("Modifiers - insert() count copies") {
        Vector<int> v = {1, 5};
        v.insert(v.begin() + 1, 3, 99);

        CHECK(v.size() == 5);
        CHECK(v[0] == 1);
        CHECK(v[1] == 99);
        CHECK(v[2] == 99);
        CHECK(v[3] == 99);
        CHECK(v[4] == 5);
    }

    TEST_CASE("Modifiers - insert() range") {
        Vector<int> v1 = {1, 5};
        Vector<int> v2 = {2, 3, 4};

        v1.insert(v1.begin() + 1, v2.begin(), v2.end());

        CHECK(v1.size() == 5);
        CHECK(v1[0] == 1);
        CHECK(v1[1] == 2);
        CHECK(v1[2] == 3);
        CHECK(v1[3] == 4);
        CHECK(v1[4] == 5);
    }

    TEST_CASE("Modifiers - emplace()") {
        struct Point {
            int x, y;
            Point(int a, int b) : x(a), y(b) {}
        };

        Vector<Point> v;
        v.emplace_back(1, 1);
        v.emplace_back(3, 3);

        auto it = v.emplace(v.begin() + 1, 2, 2);

        CHECK(v.size() == 3);
        CHECK(v[0].x == 1);
        CHECK(v[1].x == 2);
        CHECK(v[2].x == 3);
        CHECK(it->x == 2);
    }

    TEST_CASE("Modifiers - erase() single element") {
        Vector<int> v = {1, 2, 3, 4, 5};
        auto it = v.erase(v.begin() + 2);

        CHECK(v.size() == 4);
        CHECK(v[0] == 1);
        CHECK(v[1] == 2);
        CHECK(v[2] == 4);
        CHECK(v[3] == 5);
    }

    TEST_CASE("Modifiers - erase() range") {
        Vector<int> v = {1, 2, 3, 4, 5};
        v.erase(v.begin() + 1, v.begin() + 4);

        CHECK(v.size() == 2);
        CHECK(v[0] == 1);
        CHECK(v[1] == 5);
    }

    TEST_CASE("Modifiers - clear()") {
        Vector<int> v = {1, 2, 3, 4, 5};
        v.clear();

        CHECK(v.size() == 0);
        CHECK(v.empty() == true);
    }

    TEST_CASE("Modifiers - assign() count") {
        Vector<int> v = {1, 2, 3};
        v.assign(5, 99);

        CHECK(v.size() == 5);
        for (int i = 0; i < 5; ++i) {
            CHECK(v[i] == 99);
        }
    }

    TEST_CASE("Modifiers - assign() range") {
        Vector<int> v1 = {1, 2, 3};
        Vector<int> v2 = {10, 20, 30, 40};

        v1.assign(v2.begin(), v2.end());

        CHECK(v1.size() == 4);
        CHECK(v1[0] == 10);
        CHECK(v1[3] == 40);
    }

    TEST_CASE("Modifiers - resize() smaller") {
        Vector<int> v = {1, 2, 3, 4, 5};
        v.resize(3);

        CHECK(v.size() == 3);
        CHECK(v[2] == 3);
    }

    TEST_CASE("Modifiers - resize() larger") {
        Vector<int> v = {1, 2, 3};
        v.resize(5);

        CHECK(v.size() == 5);
        CHECK(v[0] == 1);
        CHECK(v[3] == 0);
        CHECK(v[4] == 0);
    }

    TEST_CASE("Modifiers - resize() with value") {
        Vector<int> v = {1, 2, 3};
        v.resize(5, 99);

        CHECK(v.size() == 5);
        CHECK(v[0] == 1);
        CHECK(v[3] == 99);
        CHECK(v[4] == 99);
    }

    TEST_CASE("Modifiers - swap()") {
        Vector<int> v1 = {1, 2, 3};
        Vector<int> v2 = {4, 5, 6, 7};

        v1.swap(v2);

        CHECK(v1.size() == 4);
        CHECK(v1[0] == 4);
        CHECK(v2.size() == 3);
        CHECK(v2[0] == 1);
    }

    TEST_CASE("Comparison - operator==") {
        Vector<int> v1 = {1, 2, 3};
        Vector<int> v2 = {1, 2, 3};
        Vector<int> v3 = {1, 2, 4};

        CHECK(v1 == v2);
        CHECK_FALSE(v1 == v3);
    }

    TEST_CASE("Comparison - operator!=") {
        Vector<int> v1 = {1, 2, 3};
        Vector<int> v2 = {1, 2, 4};

        CHECK(v1 != v2);
    }

    TEST_CASE("Comparison - operator<") {
        Vector<int> v1 = {1, 2, 3};
        Vector<int> v2 = {1, 2, 4};
        Vector<int> v3 = {1, 2, 3, 4};

        CHECK(v1 < v2);
        CHECK(v1 < v3);
        CHECK_FALSE(v2 < v1);
    }

    TEST_CASE("Comparison - operator<=") {
        Vector<int> v1 = {1, 2, 3};
        Vector<int> v2 = {1, 2, 3};
        Vector<int> v3 = {1, 2, 4};

        CHECK(v1 <= v2);
        CHECK(v1 <= v3);
    }

    TEST_CASE("Comparison - operator>") {
        Vector<int> v1 = {1, 2, 4};
        Vector<int> v2 = {1, 2, 3};

        CHECK(v1 > v2);
        CHECK_FALSE(v2 > v1);
    }

    TEST_CASE("Comparison - operator>=") {
        Vector<int> v1 = {1, 2, 4};
        Vector<int> v2 = {1, 2, 3};
        Vector<int> v3 = {1, 2, 4};

        CHECK(v1 >= v2);
        CHECK(v1 >= v3);
    }

    TEST_CASE("members() Serialization") {
        Vector<int> original = {10, 20, 30, 40};

        auto buf = serialize(original);
        auto loaded = deserialize<Mode::NONE, Vector<int>>(buf);

        CHECK(loaded.size() == 4);
        CHECK(loaded[0] == 10);
        CHECK(loaded[1] == 20);
        CHECK(loaded[2] == 30);
        CHECK(loaded[3] == 40);
    }

    TEST_CASE("Edge Case - Empty Vector Operations") {
        Vector<int> v;

        CHECK(v.empty());
        CHECK(v.size() == 0);
        v.clear(); // Should not crash
        CHECK(v.empty());
    }

    TEST_CASE("Edge Case - Single Element") {
        Vector<int> v;
        v.push_back(42);

        CHECK(v.size() == 1);
        CHECK(v[0] == 42);
        CHECK(v.front() == 42);
        CHECK(v.back() == 42);

        v.pop_back();
        CHECK(v.empty());
    }

    TEST_CASE("Edge Case - Large Vector") {
        Vector<int> v;
        for (int i = 0; i < 1000; ++i) {
            v.push_back(i);
        }

        CHECK(v.size() == 1000);
        CHECK(v[0] == 0);
        CHECK(v[999] == 999);
    }
}
