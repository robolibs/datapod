#include "datapod/datapod.hpp"
#include <algorithm>
#include <doctest/doctest.h>
#include <string>
#include <vector>

using namespace datapod;

TEST_SUITE("Trie") {

    TEST_CASE("Default construction") {
        Trie<int> trie;
        CHECK(trie.empty());
        CHECK(trie.size() == 0);
    }

    TEST_CASE("Insert single key") {
        Trie<int> trie;
        trie.insert("hello", 42);

        CHECK(trie.size() == 1);
        CHECK_FALSE(trie.empty());
        CHECK(trie.contains("hello"));
    }

    TEST_CASE("Insert multiple keys") {
        Trie<int> trie;
        trie.insert("apple", 1);
        trie.insert("app", 2);
        trie.insert("application", 3);

        CHECK(trie.size() == 3);
        CHECK(trie.contains("apple"));
        CHECK(trie.contains("app"));
        CHECK(trie.contains("application"));
    }

    TEST_CASE("Insert duplicate key updates value") {
        Trie<int> trie;
        trie.insert("key", 1);
        trie.insert("key", 2);

        CHECK(trie.size() == 1);
        auto val = trie.find("key");
        CHECK(val.has_value());
        CHECK(val.value() == 2);
    }

    TEST_CASE("Find existing key") {
        Trie<int> trie;
        trie.insert("hello", 42);

        auto result = trie.find("hello");
        CHECK(result.has_value());
        CHECK(result.value() == 42);
    }

    TEST_CASE("Find non-existing key") {
        Trie<int> trie;
        trie.insert("hello", 42);

        auto result = trie.find("world");
        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("Find prefix that is not a key") {
        Trie<int> trie;
        trie.insert("hello", 42);

        auto result = trie.find("hel");
        CHECK_FALSE(result.has_value());
    }

    TEST_CASE("Contains") {
        Trie<int> trie;
        trie.insert("apple", 1);
        trie.insert("app", 2);

        CHECK(trie.contains("apple"));
        CHECK(trie.contains("app"));
        CHECK_FALSE(trie.contains("ap"));
        CHECK_FALSE(trie.contains("application"));
    }

    TEST_CASE("At accessor") {
        Trie<int> trie;
        trie.insert("key", 42);

        CHECK(trie.at("key") == 42);
        CHECK_THROWS_AS(trie.at("nonexistent"), std::out_of_range);
    }

    TEST_CASE("Erase existing key") {
        Trie<int> trie;
        trie.insert("apple", 1);
        trie.insert("app", 2);

        bool erased = trie.erase("apple");
        CHECK(erased);
        CHECK(trie.size() == 1);
        CHECK_FALSE(trie.contains("apple"));
        CHECK(trie.contains("app"));
    }

    TEST_CASE("Erase non-existing key") {
        Trie<int> trie;
        trie.insert("apple", 1);

        bool erased = trie.erase("app");
        CHECK_FALSE(erased);
        CHECK(trie.size() == 1);
    }

    TEST_CASE("Erase prefix of another key") {
        Trie<int> trie;
        trie.insert("app", 1);
        trie.insert("apple", 2);

        bool erased = trie.erase("app");
        CHECK(erased);
        CHECK(trie.size() == 1);
        CHECK_FALSE(trie.contains("app"));
        CHECK(trie.contains("apple"));
    }

    TEST_CASE("Clear") {
        Trie<int> trie;
        trie.insert("a", 1);
        trie.insert("b", 2);
        trie.insert("c", 3);

        trie.clear();
        CHECK(trie.empty());
        CHECK(trie.size() == 0);

        // Can insert after clear
        trie.insert("new", 10);
        CHECK(trie.size() == 1);
    }

    TEST_CASE("Starts with") {
        Trie<int> trie;
        trie.insert("apple", 1);
        trie.insert("application", 2);
        trie.insert("banana", 3);

        CHECK(trie.starts_with("app"));
        CHECK(trie.starts_with("apple"));
        CHECK(trie.starts_with("appl"));
        CHECK(trie.starts_with("ban"));
        CHECK_FALSE(trie.starts_with("cat"));
        CHECK_FALSE(trie.starts_with("applications"));
    }

    TEST_CASE("Autocomplete") {
        Trie<int> trie;
        trie.insert("apple", 1);
        trie.insert("app", 2);
        trie.insert("application", 3);
        trie.insert("banana", 4);

        auto results = trie.autocomplete("app");
        CHECK(results.size() == 3);

        // Convert to std::vector for easier checking
        std::vector<std::string> strs;
        for (size_t i = 0; i < results.size(); ++i) {
            strs.push_back(std::string(results[i].view()));
        }
        std::sort(strs.begin(), strs.end());

        CHECK(strs[0] == "app");
        CHECK(strs[1] == "apple");
        CHECK(strs[2] == "application");
    }

    TEST_CASE("Autocomplete empty prefix returns all keys") {
        Trie<int> trie;
        trie.insert("a", 1);
        trie.insert("b", 2);
        trie.insert("c", 3);

        auto results = trie.autocomplete("");
        CHECK(results.size() == 3);
    }

    TEST_CASE("Autocomplete no matches") {
        Trie<int> trie;
        trie.insert("apple", 1);

        auto results = trie.autocomplete("xyz");
        CHECK(results.empty());
    }

    TEST_CASE("Keys method") {
        Trie<int> trie;
        trie.insert("c", 3);
        trie.insert("a", 1);
        trie.insert("b", 2);

        auto keys = trie.keys();
        CHECK(keys.size() == 3);
    }

    TEST_CASE("Empty string key") {
        Trie<int> trie;
        trie.insert("", 42);

        CHECK(trie.size() == 1);
        CHECK(trie.contains(""));
        CHECK(trie.find("").value() == 42);
    }

    TEST_CASE("Single character keys") {
        Trie<int> trie;
        trie.insert("a", 1);
        trie.insert("b", 2);
        trie.insert("c", 3);

        CHECK(trie.size() == 3);
        CHECK(trie.contains("a"));
        CHECK(trie.contains("b"));
        CHECK(trie.contains("c"));
    }

    TEST_CASE("Long keys") {
        Trie<int> trie;
        std::string long_key(100, 'x');
        trie.insert(long_key, 42);

        CHECK(trie.contains(long_key));
        CHECK(trie.find(long_key).value() == 42);
    }

    TEST_CASE("TrieSet (set-like behavior)") {
        TrieSet trie;
        trie.insert("apple");
        trie.insert("banana");
        trie.insert("cherry");

        CHECK(trie.size() == 3);
        CHECK(trie.contains("apple"));
        CHECK(trie.contains("banana"));
        CHECK(trie.contains("cherry"));
        CHECK_FALSE(trie.contains("date"));
    }

    TEST_CASE("String values") {
        Trie<String> trie;
        trie.insert("key1", String("value1"));
        trie.insert("key2", String("value2"));

        CHECK(trie.find("key1").value().view() == "value1");
        CHECK(trie.find("key2").value().view() == "value2");
    }

    TEST_CASE("Copy construction") {
        Trie<int> original;
        original.insert("a", 1);
        original.insert("b", 2);

        Trie<int> copy(original);

        CHECK(copy.size() == 2);
        CHECK(copy.contains("a"));
        CHECK(copy.contains("b"));

        // Modify original
        original.insert("c", 3);
        CHECK_FALSE(copy.contains("c"));
    }

    TEST_CASE("Move construction") {
        Trie<int> original;
        original.insert("a", 1);
        original.insert("b", 2);

        Trie<int> moved(std::move(original));

        CHECK(moved.size() == 2);
        CHECK(moved.contains("a"));
        CHECK(moved.contains("b"));
    }

    TEST_CASE("Serialization roundtrip") {
        Trie<int> original;
        original.insert("apple", 1);
        original.insert("app", 2);
        original.insert("application", 3);
        original.insert("banana", 4);

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, Trie<int>>(buffer);

        CHECK(restored.size() == original.size());
        CHECK(restored.contains("apple"));
        CHECK(restored.contains("app"));
        CHECK(restored.contains("application"));
        CHECK(restored.contains("banana"));
        CHECK(restored.find("apple").value() == 1);
        CHECK(restored.find("app").value() == 2);
    }

    TEST_CASE("Serialization with string values") {
        Trie<String> original;
        original.insert("key1", String("value1"));
        original.insert("key2", String("value2"));

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, Trie<String>>(buffer);

        CHECK(restored.size() == 2);
        CHECK(restored.find("key1").value().view() == "value1");
        CHECK(restored.find("key2").value().view() == "value2");
    }

    TEST_CASE("Autocomplete use case - dictionary") {
        Trie<int> dictionary;
        dictionary.insert("car", 1);
        dictionary.insert("card", 2);
        dictionary.insert("care", 3);
        dictionary.insert("careful", 4);
        dictionary.insert("careless", 5);
        dictionary.insert("cat", 6);

        // User types "car"
        auto suggestions = dictionary.autocomplete("car");
        CHECK(suggestions.size() == 5); // car, card, care, careful, careless

        // User types "care"
        suggestions = dictionary.autocomplete("care");
        CHECK(suggestions.size() == 3); // care, careful, careless
    }

    TEST_CASE("Special characters in keys") {
        Trie<int> trie;
        trie.insert("hello world", 1);
        trie.insert("hello-world", 2);
        trie.insert("hello_world", 3);
        trie.insert("hello.world", 4);

        CHECK(trie.size() == 4);
        CHECK(trie.contains("hello world"));
        CHECK(trie.contains("hello-world"));
        CHECK(trie.contains("hello_world"));
        CHECK(trie.contains("hello.world"));
    }

    TEST_CASE("Case sensitivity") {
        Trie<int> trie;
        trie.insert("Hello", 1);
        trie.insert("hello", 2);
        trie.insert("HELLO", 3);

        CHECK(trie.size() == 3);
        CHECK(trie.find("Hello").value() == 1);
        CHECK(trie.find("hello").value() == 2);
        CHECK(trie.find("HELLO").value() == 3);
    }
}
