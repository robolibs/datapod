#include <doctest/doctest.h>

#include <datapod/pods/adapters/error.hpp>

using namespace datapod;

TEST_SUITE("Error") {
    TEST_CASE("Default construction") {
        Error err;
        CHECK(err.code == 0);
        CHECK(err.message.empty());
        CHECK(err.is_ok());
        CHECK_FALSE(err.is_err());
    }

    TEST_CASE("Construction with code and message") {
        Error err{42, "Something went wrong"};
        CHECK(err.code == 42);
        CHECK(err.message == "Something went wrong");
        CHECK(err.is_err());
        CHECK_FALSE(err.is_ok());
    }

    TEST_CASE("Construction with const char*") {
        Error err{10, "C-string message"};
        CHECK(err.code == 10);
        CHECK(err.message == "C-string message");
    }

    TEST_CASE("Factory method - ok()") {
        Error err = Error::ok();
        CHECK(err.code == Error::OK);
        CHECK(err.message.empty());
        CHECK(err.is_ok());
    }

    TEST_CASE("Factory method - invalid_argument()") {
        Error err = Error::invalid_argument("Bad param");
        CHECK(err.code == Error::INVALID_ARGUMENT);
        CHECK(err.message == "Bad param");
        CHECK(err.is_err());
    }

    TEST_CASE("Factory method - out_of_range()") {
        Error err = Error::out_of_range("Index too large");
        CHECK(err.code == Error::OUT_OF_RANGE);
        CHECK(err.message == "Index too large");
    }

    TEST_CASE("Factory method - not_found()") {
        Error err = Error::not_found("File missing");
        CHECK(err.code == Error::NOT_FOUND);
        CHECK(err.message == "File missing");
    }

    TEST_CASE("Factory method - permission_denied()") {
        Error err = Error::permission_denied("Access denied");
        CHECK(err.code == Error::PERMISSION_DENIED);
        CHECK(err.message == "Access denied");
    }

    TEST_CASE("Factory method - already_exists()") {
        Error err = Error::already_exists("Duplicate entry");
        CHECK(err.code == Error::ALREADY_EXISTS);
        CHECK(err.message == "Duplicate entry");
    }

    TEST_CASE("Factory method - timeout()") {
        Error err = Error::timeout("Operation timed out");
        CHECK(err.code == Error::TIMEOUT);
        CHECK(err.message == "Operation timed out");
    }

    TEST_CASE("Factory method - io_error()") {
        Error err = Error::io_error("Disk failure");
        CHECK(err.code == Error::IO_ERROR);
        CHECK(err.message == "Disk failure");
    }

    TEST_CASE("Factory method - network_error()") {
        Error err = Error::network_error("Connection lost");
        CHECK(err.code == Error::NETWORK_ERROR);
        CHECK(err.message == "Connection lost");
    }

    TEST_CASE("Factory method - parse_error()") {
        Error err = Error::parse_error("Invalid JSON");
        CHECK(err.code == Error::PARSE_ERROR);
        CHECK(err.message == "Invalid JSON");
    }

    TEST_CASE("operator bool") {
        Error ok_err = Error::ok();
        Error bad_err{1, "error"};

        CHECK_FALSE(static_cast<bool>(ok_err));
        CHECK(static_cast<bool>(bad_err));
    }

    TEST_CASE("operator== equality") {
        Error err1{42, "message"};
        Error err2{42, "message"};
        CHECK(err1 == err2);
    }

    TEST_CASE("operator!= inequality - different code") {
        Error err1{42, "message"};
        Error err2{43, "message"};
        CHECK(err1 != err2);
    }

    TEST_CASE("operator!= inequality - different message") {
        Error err1{42, "message1"};
        Error err2{42, "message2"};
        CHECK(err1 != err2);
    }

    TEST_CASE("same_code - true") {
        Error err1{42, "message1"};
        Error err2{42, "message2"};
        CHECK(err1.same_code(err2));
    }

    TEST_CASE("same_code - false") {
        Error err1{42, "message"};
        Error err2{43, "message"};
        CHECK_FALSE(err1.same_code(err2));
    }

    TEST_CASE("members() reflection") {
        Error err{42, "test"};
        auto m = err.members();
        CHECK(&std::get<0>(m) == &err.code);
        CHECK(&std::get<1>(m) == &err.message);
    }

    TEST_CASE("POD properties") {
        CHECK(std::is_standard_layout_v<Error>);
        // Note: Not trivially copyable due to String, but still serializable
    }

    TEST_CASE("Error code constants") {
        CHECK(Error::OK == 0);
        CHECK(Error::INVALID_ARGUMENT == 1);
        CHECK(Error::OUT_OF_RANGE == 2);
        CHECK(Error::NOT_FOUND == 3);
        CHECK(Error::PERMISSION_DENIED == 4);
        CHECK(Error::ALREADY_EXISTS == 5);
        CHECK(Error::TIMEOUT == 6);
        CHECK(Error::IO_ERROR == 7);
        CHECK(Error::NETWORK_ERROR == 8);
        CHECK(Error::PARSE_ERROR == 9);
    }
}
