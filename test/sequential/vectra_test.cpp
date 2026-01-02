#include <doctest/doctest.h>

#include <tuple>

#include "datapod/datapod.hpp"

using namespace datapod;

TEST_SUITE("Vectra") {
    TEST_CASE("Inline storage handles small sizes") {
        Vectra<int, 4> vectra;
        CHECK(vectra.using_inline_storage());
        CHECK(vectra.capacity() == Vectra<int, 4>::inline_capacity);

        vectra.push_back(1);
        vectra.push_back(2);
        vectra.push_back(3);
        vectra.push_back(4);

        CHECK(vectra.size() == 4);
        CHECK(vectra.using_inline_storage());
        CHECK(vectra.capacity() == Vectra<int, 4>::inline_capacity);
    }

    TEST_CASE("Heap spillover occurs beyond inline capacity") {
        Vectra<int, 4> vectra;
        for (int i = 0; i < 6; ++i) {
            vectra.push_back(i);
        }

        CHECK(vectra.size() == 6);
        CHECK_FALSE(vectra.using_inline_storage());
        CHECK(vectra.capacity() >= 6);
    }

    TEST_CASE("shrink_to_fit moves data back to inline storage when possible") {
        Vectra<int, 4> vectra;
        for (int i = 0; i < 6; ++i) {
            vectra.push_back(i);
        }

        CHECK_FALSE(vectra.using_inline_storage());

        // Reduce size so inline storage is sufficient again
        for (int i = 0; i < 3; ++i) {
            vectra.pop_back();
        }
        CHECK(vectra.size() == 3);

        vectra.shrink_to_fit();
        CHECK(vectra.using_inline_storage());
        CHECK(vectra.capacity() == Vectra<int, 4>::inline_capacity);
    }

    TEST_CASE("Serialization round trip preserves contents and ordering") {
        Vectra<int, 3> original;
        original.push_back(10);
        original.push_back(20);
        original.push_back(30);
        original.push_back(40); // forces heap spillover

        auto buffer = serialize(original);
        auto restored = deserialize<Mode::NONE, Vectra<int, 3>>(buffer);

        CHECK(restored.size() == original.size());
        CHECK(restored[0] == 10);
        CHECK(restored[1] == 20);
        CHECK(restored[2] == 30);
        CHECK(restored[3] == 40);
        CHECK_FALSE(restored.using_inline_storage());
    }

    TEST_CASE("members() snapshot can rebuild live storage") {
        Vectra<int, 4> vectra;
        vectra.push_back(1);
        vectra.push_back(2);

        auto members = vectra.members();
        auto &snapshot = std::get<0>(members);
        snapshot.push_back(3);

        vectra.rebuild_from_snapshot();
        CHECK(vectra.size() == 3);
        CHECK(vectra[2] == 3);
        CHECK(vectra.using_inline_storage());

        Vectra<int, 4> const &const_vectra = vectra;
        auto const_members = const_vectra.members();
        auto const &const_snapshot = std::get<0>(const_members);
        CHECK(const_snapshot.size() == vectra.size());
    }
}
