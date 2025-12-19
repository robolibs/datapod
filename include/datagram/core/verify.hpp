#pragma once

#include <string>

#include "datagram/core/exception.hpp"

namespace datagram {

    // Verify condition with C-string message
    inline void verify(bool const condition, char const *msg) {
        if (!condition) {
            throw_exception(DatagramException{msg});
        }
    }

    // Verify condition with std::string message
    inline void verify_str(bool const condition, std::string msg) {
        if (!condition) {
            throw_exception(DatagramException{std::move(msg)});
        }
    }

} // namespace datagram
