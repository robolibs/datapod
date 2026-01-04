#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <datapod/pods/adapters/ref_cell.hpp>
#include <string>
#include <vector>

using namespace datapod;

TEST_CASE("RefCell - Basic construction") {
    RefCell<int> cell(42);
    CHECK(cell.get() == 42);

    RefCell<std::string> str_cell(std::string("hello"));
    CHECK(str_cell.get() == "hello");
}

TEST_CASE("RefCell - Immutable borrow") {
    RefCell<int> cell(42);

    auto ref = cell.borrow();
    CHECK(*ref == 42);
    CHECK(ref.get() != nullptr);
}

TEST_CASE("RefCell - Multiple immutable borrows") {
    RefCell<int> cell(42);

    auto ref1 = cell.borrow();
    auto ref2 = cell.borrow();
    auto ref3 = cell.borrow();

    CHECK(*ref1 == 42);
    CHECK(*ref2 == 42);
    CHECK(*ref3 == 42);
    CHECK(cell.borrow_count() == 3);
}

TEST_CASE("RefCell - Mutable borrow") {
    RefCell<int> cell(42);

    auto ref_mut = cell.borrow_mut();
    CHECK(*ref_mut == 42);

    *ref_mut = 100;
    CHECK(*ref_mut == 100);
}

TEST_CASE("RefCell - Mutable borrow updates value") {
    RefCell<int> cell(42);

    {
        auto ref_mut = cell.borrow_mut();
        *ref_mut = 100;
    }

    CHECK(cell.get() == 100);
}

TEST_CASE("RefCell - Cannot borrow mutably while immutably borrowed") {
    RefCell<int> cell(42);

    auto ref = cell.borrow();
    CHECK_THROWS_AS(cell.borrow_mut(), BorrowError);
}

TEST_CASE("RefCell - Cannot borrow immutably while mutably borrowed") {
    RefCell<int> cell(42);

    auto ref_mut = cell.borrow_mut();
    CHECK_THROWS_AS(cell.borrow(), BorrowError);
}

TEST_CASE("RefCell - Cannot borrow mutably twice") {
    RefCell<int> cell(42);

    auto ref_mut1 = cell.borrow_mut();
    CHECK_THROWS_AS(cell.borrow_mut(), BorrowError);
}

TEST_CASE("RefCell - Borrow released on scope exit") {
    RefCell<int> cell(42);

    {
        auto ref = cell.borrow();
        CHECK(cell.borrow_count() == 1);
    }

    CHECK(cell.borrow_count() == 0);
    CHECK_NOTHROW(cell.borrow_mut());
}

TEST_CASE("RefCell - Mutable borrow released on scope exit") {
    RefCell<int> cell(42);

    {
        auto ref_mut = cell.borrow_mut();
        CHECK(cell.is_borrowed_mut());
    }

    CHECK_FALSE(cell.is_borrowed_mut());
    CHECK_NOTHROW(cell.borrow());
}

TEST_CASE("RefCell - Replace value") {
    RefCell<int> cell(42);

    int old = cell.replace(100);
    CHECK(old == 42);
    CHECK(cell.get() == 100);
}

TEST_CASE("RefCell - Cannot replace while borrowed") {
    RefCell<int> cell(42);

    auto ref = cell.borrow();
    CHECK_THROWS_AS(cell.replace(100), BorrowError);
}

TEST_CASE("RefCell - Set value") {
    RefCell<int> cell(42);

    cell.set(100);
    CHECK(cell.get() == 100);
}

TEST_CASE("RefCell - Cannot set while borrowed") {
    RefCell<int> cell(42);

    auto ref = cell.borrow();
    CHECK_THROWS_AS(cell.set(100), BorrowError);
}

TEST_CASE("RefCell - Swap values") {
    RefCell<int> cell1(42);
    RefCell<int> cell2(100);

    cell1.swap(cell2);

    CHECK(cell1.get() == 100);
    CHECK(cell2.get() == 42);
}

TEST_CASE("RefCell - Cannot swap while borrowed") {
    RefCell<int> cell1(42);
    RefCell<int> cell2(100);

    auto ref = cell1.borrow();
    CHECK_THROWS_AS(cell1.swap(cell2), BorrowError);
}

TEST_CASE("RefCell - Take value") {
    RefCell<std::string> cell(std::string("hello"));

    std::string value = cell.take();
    CHECK(value == "hello");
}

TEST_CASE("RefCell - Cannot take while borrowed") {
    RefCell<int> cell(42);

    auto ref = cell.borrow();
    CHECK_THROWS_AS(cell.take(), BorrowError);
}

TEST_CASE("RefCell - is_borrowed") {
    RefCell<int> cell(42);

    CHECK_FALSE(cell.is_borrowed());

    {
        auto ref = cell.borrow();
        CHECK(cell.is_borrowed());
    }

    CHECK_FALSE(cell.is_borrowed());

    {
        auto ref_mut = cell.borrow_mut();
        CHECK(cell.is_borrowed());
    }

    CHECK_FALSE(cell.is_borrowed());
}

TEST_CASE("RefCell - Ref move semantics") {
    RefCell<int> cell(42);

    auto ref1 = cell.borrow();
    CHECK(cell.borrow_count() == 1);

    auto ref2 = std::move(ref1);
    CHECK(cell.borrow_count() == 1);
    CHECK(*ref2 == 42);
}

TEST_CASE("RefCell - RefMut move semantics") {
    RefCell<int> cell(42);

    auto ref_mut1 = cell.borrow_mut();
    CHECK(cell.is_borrowed_mut());

    auto ref_mut2 = std::move(ref_mut1);
    CHECK(cell.is_borrowed_mut());
    CHECK(*ref_mut2 == 42);
}

TEST_CASE("RefCell - Ref arrow operator") {
    struct Point {
        int x, y;
    };

    RefCell<Point> cell(Point{10, 20});

    auto ref = cell.borrow();
    CHECK(ref->x == 10);
    CHECK(ref->y == 20);
}

TEST_CASE("RefCell - RefMut arrow operator") {
    struct Point {
        int x, y;
    };

    RefCell<Point> cell(Point{10, 20});

    auto ref_mut = cell.borrow_mut();
    CHECK(ref_mut->x == 10);
    CHECK(ref_mut->y == 20);

    ref_mut->x = 30;
    ref_mut->y = 40;

    CHECK(ref_mut->x == 30);
    CHECK(ref_mut->y == 40);
}

TEST_CASE("RefCell - Complex type") {
    RefCell<std::vector<int>> cell(std::vector<int>{1, 2, 3});

    {
        auto ref_mut = cell.borrow_mut();
        ref_mut->push_back(4);
        ref_mut->push_back(5);
    }

    auto ref = cell.borrow();
    CHECK(ref->size() == 5);
    CHECK((*ref)[0] == 1);
    CHECK((*ref)[4] == 5);
}

TEST_CASE("RefCell - Move construction") {
    RefCell<int> cell1(42);
    RefCell<int> cell2(std::move(cell1));

    CHECK(cell2.get() == 42);
}

TEST_CASE("RefCell - Move assignment") {
    RefCell<int> cell1(42);
    RefCell<int> cell2(100);

    cell2 = std::move(cell1);
    CHECK(cell2.get() == 42);
}

TEST_CASE("RefCell - try_borrow") {
    RefCell<int> cell(42);

    auto ref = cell.try_borrow();
    CHECK(*ref == 42);

    CHECK_THROWS_AS(cell.try_borrow_mut(), BorrowError);
}

TEST_CASE("RefCell - try_borrow_mut") {
    RefCell<int> cell(42);

    auto ref_mut = cell.try_borrow_mut();
    CHECK(*ref_mut == 42);

    CHECK_THROWS_AS(cell.try_borrow(), BorrowError);
}

TEST_CASE("RefCell - Sequential borrows") {
    RefCell<int> cell(42);

    {
        auto ref = cell.borrow();
        CHECK(*ref == 42);
    }

    {
        auto ref_mut = cell.borrow_mut();
        *ref_mut = 100;
    }

    {
        auto ref = cell.borrow();
        CHECK(*ref == 100);
    }
}

TEST_CASE("RefCell - Ref move assignment") {
    RefCell<int> cell(42);

    auto ref1 = cell.borrow();
    auto ref2 = cell.borrow();

    CHECK(cell.borrow_count() == 2);

    ref1 = std::move(ref2);
    CHECK(cell.borrow_count() == 1);
}

TEST_CASE("RefCell - RefMut move assignment") {
    RefCell<int> cell1(42);
    RefCell<int> cell2(100);

    auto ref_mut1 = cell1.borrow_mut();
    CHECK(cell1.is_borrowed_mut());

    {
        auto ref_mut2 = cell2.borrow_mut();
        CHECK(cell2.is_borrowed_mut());

        ref_mut1 = std::move(ref_mut2);
        // After move assignment, ref_mut1 now points to cell2's data
        // cell1 is no longer borrowed, cell2 is still borrowed
        CHECK_FALSE(cell1.is_borrowed_mut());
        CHECK(cell2.is_borrowed_mut());
        CHECK(*ref_mut1 == 100);
    }
}

TEST_CASE("RefCell - In-place construction") {
    struct Point {
        int x, y;
        Point(int x_, int y_) : x(x_), y(y_) {}
    };

    RefCell<Point> cell(10, 20);
    auto ref = cell.borrow();
    CHECK(ref->x == 10);
    CHECK(ref->y == 20);
}

TEST_CASE("RefCell - Const correctness") {
    const RefCell<int> cell(42);

    auto ref = cell.borrow();
    CHECK(*ref == 42);

    // Can still get mutable borrow from const RefCell (interior mutability)
    // But we can't call borrow_mut on const RefCell in this implementation
    CHECK(cell.get() == 42);
}
