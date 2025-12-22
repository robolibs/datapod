#pragma once

#include <string>

#include "bitcon/core/exception.hpp"

namespace bitcon {

    // Verify condition with C-string message
    inline void verify(bool const condition, char const *msg) {
        if (!condition) {
            throw_exception(BitconException{msg});
        }
    }

    // Verify condition with std::string message
    inline void verify_str(bool const condition, std::string msg) {
        if (!condition) {
            throw_exception(BitconException{std::move(msg)});
        }
    }

} // namespace bitcon
