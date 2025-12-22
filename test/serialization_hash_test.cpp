#include <doctest/doctest.h>

#include "datapod/datapod.hpp"

using namespace datapod;

// Test Map serialization
TEST_CASE("serialize - hashmap int to string") {
    Map<int, String> map;
    map.insert({1, String("one")});
    map.insert({2, String("two")});
    map.insert({3, String("three")});

    auto buf = serialize(map);
    auto result = deserialize<Mode::NONE, Map<int, String>>(buf);

    REQUIRE(result.size() == 3);
    CHECK(result[1] == "one");
    CHECK(result[2] == "two");
    CHECK(result[3] == "three");
}

TEST_CASE("serialize - hashmap string to int") {
    Map<String, int> map;
    map.insert({String("alpha"), 100});
    map.insert({String("beta"), 200});
    map.insert({String("gamma"), 300});

    auto buf = serialize(map);
    auto result = deserialize<Mode::NONE, Map<String, int>>(buf);

    REQUIRE(result.size() == 3);
    CHECK(result[String("alpha")] == 100);
    CHECK(result[String("beta")] == 200);
    CHECK(result[String("gamma")] == 300);
}

TEST_CASE("serialize - empty hashmap") {
    Map<int, int> map;
    auto buf = serialize(map);
    auto result = deserialize<Mode::NONE, Map<int, int>>(buf);

    CHECK(result.size() == 0);
}

TEST_CASE("serialize - hashmap with many entries") {
    Map<int, int> map;
    for (int i = 0; i < 100; ++i) {
        map.insert({i, i * 2});
    }

    auto buf = serialize(map);
    auto result = deserialize<Mode::NONE, Map<int, int>>(buf);

    REQUIRE(result.size() == 100);
    for (int i = 0; i < 100; ++i) {
        CHECK(result[i] == i * 2);
    }
}

// Test Set serialization
TEST_CASE("serialize - hashset int") {
    Set<int> set;
    set.insert(10);
    set.insert(20);
    set.insert(30);
    set.insert(40);

    auto buf = serialize(set);
    auto result = deserialize<Mode::NONE, Set<int>>(buf);

    REQUIRE(result.size() == 4);
    CHECK(result.find(10) != result.end());
    CHECK(result.find(20) != result.end());
    CHECK(result.find(30) != result.end());
    CHECK(result.find(40) != result.end());
}

TEST_CASE("serialize - hashset string") {
    Set<String> set;
    set.insert(String("apple"));
    set.insert(String("banana"));
    set.insert(String("cherry"));

    auto buf = serialize(set);
    auto result = deserialize<Mode::NONE, Set<String>>(buf);

    REQUIRE(result.size() == 3);
    CHECK(result.find(String("apple")) != result.end());
    CHECK(result.find(String("banana")) != result.end());
    CHECK(result.find(String("cherry")) != result.end());
}

TEST_CASE("serialize - empty hashset") {
    Set<int> set;
    auto buf = serialize(set);
    auto result = deserialize<Mode::NONE, Set<int>>(buf);

    CHECK(result.size() == 0);
}

TEST_CASE("serialize - hashset with many entries") {
    Set<int> set;
    for (int i = 0; i < 100; ++i) {
        set.insert(i);
    }

    auto buf = serialize(set);
    auto result = deserialize<Mode::NONE, Set<int>>(buf);

    REQUIRE(result.size() == 100);
    for (int i = 0; i < 100; ++i) {
        CHECK(result.find(i) != result.end());
    }
}

// Test nested containers
TEST_CASE("serialize - vector of hashmaps") {
    Vector<Map<int, String>> vec;

    Map<int, String> map1;
    map1.insert({1, String("one")});
    map1.insert({2, String("two")});
    vec.push_back(map1);

    Map<int, String> map2;
    map2.insert({3, String("three")});
    map2.insert({4, String("four")});
    vec.push_back(map2);

    auto buf = serialize(vec);
    auto result = deserialize<Mode::NONE, Vector<Map<int, String>>>(buf);

    REQUIRE(result.size() == 2);
    REQUIRE(result[0].size() == 2);
    CHECK(result[0][1] == "one");
    CHECK(result[0][2] == "two");
    REQUIRE(result[1].size() == 2);
    CHECK(result[1][3] == "three");
    CHECK(result[1][4] == "four");
}

// Test endian mode
TEST_CASE("serialize - hashmap with big endian") {
    Map<int, int> map;
    map.insert({0x12345678, 0x87654321});
    map.insert({0xAABBCCDD, 0xDDCCBBAA});

    auto buf = serialize<Mode::SERIALIZE_BIG_ENDIAN>(map);
    auto result = deserialize<Mode::SERIALIZE_BIG_ENDIAN, Map<int, int>>(buf);

    REQUIRE(result.size() == 2);
    CHECK(result[0x12345678] == 0x87654321);
    CHECK(result[0xAABBCCDD] == 0xDDCCBBAA);
}

TEST_CASE("serialize - hashset with big endian") {
    Set<int> set;
    set.insert(0x12345678);
    set.insert(0xAABBCCDD);

    auto buf = serialize<Mode::SERIALIZE_BIG_ENDIAN>(set);
    auto result = deserialize<Mode::SERIALIZE_BIG_ENDIAN, Set<int>>(buf);

    REQUIRE(result.size() == 2);
    CHECK(result.find(0x12345678) != result.end());
    CHECK(result.find(0xAABBCCDD) != result.end());
}
